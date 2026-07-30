/*
 * wav_test.c - Phase 5 WAV player
 *
 * === Phase 5 Part 1 (commit 6716272) ===
 *   - pff_mount() + pff_open() + 64-byte RIFF header read + hex dump
 *
 * === Phase 5 Part 2 (v8, commit 1fd8863) ===
 *   - Walk RIFF chunks to find 'data' chunk
 *   - Validate fmt chunk: wFormatTag == 1 (PCM), wBitsPerSample == 16
 *   - Set DAC sample rate via dac_spr_set() (auto from WAV header)
 *   - Wake DAC: dac_set_dvol(DIG_N0DB) + bsp_change_volume + fade_in
 *   - Stream via direct AUBUFDATA writes: ONE u32 per stereo frame,
 *     bits[15:0]=L / bits[31:16]=R, SIGNED, pushed as-is.
 *   - v8 fixes channel imbalance + muddy vocals: earlier versions did
 *     two writes/frame with each channel in the upper 16 bits and an
 *     XOR 0x8000; that fed one physical channel constant zeros and
 *     mangled the other. See the Task 5.3 comment for the full autopsy.
 *
 * === Phase 5 Part 3 (this commit — Task 5.4) ===
 *   - flip PF_USE_DIR=1 in fat_test/pffconf.h (committed in same changeset)
 *   - enumerate *.WAV in SD card root via pff_opendir / pff_readdir
 *     (8.3 SFN only; AM_LFN / AM_DIR / AM_VOL entries skipped)
 *   - playlist scheduler: outer loop wraps last -> first and first -> last
 *   - KEY CONTROLS (via msg_dequeue() from the 5 ms ISR — see
 *     bsp_sys.c:347 + bsp_key.c:980):
 *       KU_PLAY (or KU_PLAY_PWR_USER_DEF)  -> pause / resume toggle
 *       KU_NEXT / KU_NEXT_VOL_UP / ...     -> next song (wraps last -> first)
 *       KU_PREV / KU_PREV_VOL_DOWN / ...   -> prev song (wraps first -> last)
 *       KU_VOL_UP / KH_VOL_UP / ...        -> bsp_set_volume(bsp_volume_inc())
 *       KU_VOL_DOWN / KH_VOL_DOWN / ...    -> bsp_set_volume(bsp_volume_dec())
 *   - pause semantics: skip AUBUFDATA writes while paused, but keep
 *     draining msg_dequeue() (delay_us(200) between polls so the 5 ms
 *     ISR stays fed) — matches the SDK template in test/参考.md:12
 *
 * BSS budget:
 *   - 64-byte RIFF header buffer
 *   - 512-byte PCM streaming buffer (aram .wav_buf)
 *   - playlist: 16 entries x 13 chars = 208 B + small index state
 */

#include "include.h"
#include "wav_test.h"
#include "pff.h"        /* MUST come after include.h - FRESULT comes from api_fs.h */

/* -------------------------------------------------------------------------
 * DAC hardware registers (locally defined — only declared inside
 * bsp_iis_ext.c in the SDK, not exposed in any public header)
 *
 * Sourced from app/platform/bsp/bsp_iis_ext.c:23-24
 * ------------------------------------------------------------------------- */
#define AUBUFDATA   SFR_RW (SFR1_BASE + 0x01*4)
#define AUBUFCON    SFR_RW (SFR1_BASE + 0x02*4)

/* -------------------------------------------------------------------------
 * Petit FatFs state
 * ------------------------------------------------------------------------- */
static FATFS s_fs;

/* 64-byte RIFF header buffer (BSS is fine for 64 B). */
static u8 s_header[64] __attribute__((aligned(4)));

/* 512-byte PCM streaming buffer (NOT BSS - placed in aram .wav_buf). */
static u8 s_pcm[512] __attribute__((aligned(4), section(".wav_buf")));

/* -------------------------------------------------------------------------
 * Task 5.4: playlist state
 *
 * Each entry holds the 8.3 SFN as pff_readdir() returns it
 * (uppercased body; PF_USE_LCC=0 in pffconf.h so we keep the case as-is
 *  but match .WAV case-insensitively below).
 *
 * Playlist order is FAT table order, NOT alphabetical. The user picks
 * playback order by choosing which file the FAT writer placed first.
 * ------------------------------------------------------------------------- */
#define WAV_MAX_FILES    16

typedef struct {
    u8  fname[13];          /* 8.3 + '\0', as filled by get_fileinfo() */
    u32 fsize;              /* bytes (from FILINFO) */
} wav_entry_t;

static wav_entry_t s_pl[WAV_MAX_FILES];
static u8  s_total;             /* number of WAVs discovered (0..WAV_MAX_FILES) */
static u8  s_index;             /* currently playing slot (0..s_total-1) */

/* Run-state flags consulted inside the 512-byte push loop. */
static volatile bool s_paused;
static volatile bool s_should_next;
static volatile bool s_should_prev;

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static const char *fr_str(FRESULT fr)
{
    switch (fr) {
    case FR_OK            : return "OK";
    case FR_DISK_ERR      : return "DISK_ERR";
    case FR_NOT_READY     : return "NOT_READY";
    case FR_NO_FILE       : return "NO_FILE";
    case FR_NOT_OPENED    : return "NOT_OPENED";
    case FR_NOT_ENABLED   : return "NOT_ENABLED";
    case FR_NO_FILESYSTEM : return "NO_FILESYSTEM";
    default               : return "UNKNOWN";
    }
}

static u16 rd16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 rd32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void print_fourcc(const u8 *p)
{
    for (int i = 0; i < 4; i++) {
        char c = (char)p[i];
        if (c >= 0x20 && c <= 0x7e) {
            printf("%c", c);
        } else {
            printf("\\x%02X", (u8)c);
        }
    }
}

/* Case-insensitive "WAV" extension test. p points at the dot or
 * just past the filename body (i.e. at the 4th char of the extension). */
static bool ext_is_wav(const char *p)
{
    return (p[0] == 'W' || p[0] == 'w') &&
           (p[1] == 'A' || p[1] == 'a') &&
           (p[2] == 'V' || p[2] == 'v') &&
           (p[3] == 0);
}

/* Convert a 16-bit unsigned sample rate from the WAV header to the
 * SDK's dac_spr_set() enum (api_sdadc.h:25-42). Returns 0xFFFF if
 * unsupported. */
static u16 wav_sample_rate_to_spr(u32 hz)
{
    switch (hz) {
    case 48000: return SPR_48000;
    case 44100: return SPR_44100;
    case 38000: return SPR_38000;
    case 32000: return SPR_32000;
    case 24000: return SPR_24000;
    case 22050: return SPR_22050;
    case 16000: return SPR_16000;
    case 12000: return SPR_12000;
    case 11025: return SPR_11025;
    case 8000 : return SPR_8000;
    default   : return 0xFFFF;
    }
}

/* -------------------------------------------------------------------------
 * Task 5.4.2: Build playlist by walking the SD card root directory.
 *
 * pf_opendir(path) with an empty path = the root dir
 *   (see pff.c:761-764 follow_path's null-path handling).
 *
 * FILINFO.fattrib mask bits (pff.h:177): AM_LFN=0x0F, AM_DIR=0x10,
 * AM_VOL=0x08. dir_read() (pff.c:639) already filters 0xE5, '.' and
 * AM_VOL, but does NOT filter AM_LFN — we must do it ourselves so a
 * long-named file (e.g. Windows "System Volume Information") doesn't
 * sneak into the playlist with garbage fname bytes.
 *
 * FAT ordering: files come out in whatever order the directory table
 * stores them. We don't sort — that would need a working buffer and
 * isn't worth the complexity for a 16-entry list.
 * ------------------------------------------------------------------------- */
static u8 wav_test_build_playlist(void)
{
    DIR     dj;
    FILINFO fno;
    FRESULT fr;

    s_total = 0;
    s_index = 0;

    fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        return 0;
    }

    /* Empty path = root dir (pff.c:761-764). */
    fr = pff_opendir(&dj, "");
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_opendir(\"\") -> %d (%s)\n", fr, fr_str(fr));
        return 0;
    }

    while (s_total < WAV_MAX_FILES) {
        fr = pff_readdir(&dj, &fno);
        if (fr != FR_OK) {
            printf("[WAV] FAIL: pff_readdir -> %d (%s)\n", fr, fr_str(fr));
            break;
        }
        if (fno.fname[0] == 0) {
            break;                       /* end of directory */
        }
        if (fno.fattrib & (AM_LFN | AM_DIR | AM_VOL)) {
            continue;                    /* skip LFN, subdir, volume label */
        }
        /* find the extension (4 chars including the dot) */
        const char *dot = 0;
        for (u8 i = 0; fno.fname[i]; i++) {
            if (fno.fname[i] == '.') { dot = &fno.fname[i + 1]; break; }
        }
        if (!dot || !ext_is_wav(dot)) {
            continue;                    /* not a .WAV */
        }
        /* accept */
        for (u8 i = 0; i < 13; i++) {
            s_pl[s_total].fname[i] = (u8)fno.fname[i];
        }
        s_pl[s_total].fsize = fno.fsize;
        s_total++;
    }

    /* pff_readdir clears fno.fname[0] on end-of-dir; no explicit close needed. */
    return s_total;
}

/* -------------------------------------------------------------------------
 * Task 5.4.5: dispatch one key / system event from msg_dequeue().
 *
 * The mapping mirrors two SDK templates:
 *   - app/projects/standard/message/msg_music.c:6-48  (play / next / prev)
 *   - app/platform/functions/func.c:210-296          (volume up / down)
 *
 * NOTE on combo codes: this build's config only enables PWRKEY
 * (USER_PWRKEY=1; USER_ADKEY=0 because PE5/6/7 belong to the SD card).
 * pwrkey_table[] (port_key.c:158) maps ADC thresholds to *combo* key IDs
 * — short-press of the 3.9K-resistor button, e.g., is KU_NEXT_VOL_UP
 * (short=next, long=VOL+). We accept BOTH the clean KU_NEXT/PREV and
 * the combo KU_NEXT_VOL_UP/PREV_VOL_DOWN so the same code works whether
 * keys are wired via ADKEY (clean) or PWRKEY (combo).
 *
 * No fall-through to func_message(): wav_test is a test harness, not a
 * music mode. Volume is set via the bsp_*_volume() helpers directly.
 * ------------------------------------------------------------------------- */
static void wav_test_handle_key(u16 msg)
{
    /* The msg queue mixes real key events with periodic system events:
     *   - real keys are in 0x000..0xFFF (base key IDs 0x00..0xEF plus
     *     action flag bits KEY_SHORT_UP=0x800 / KEY_LONG=0xA00 /
     *     KEY_HOLD=0xE00 from bsp_key.h:11-14)
     *   - system events live in 0x7D0..0x7FF (EVT_SD_INSERT=0x7FD,
     *     MSG_SYS_500MS=0x7FE, MSG_SYS_1S=0x7FF, EVT_SD_REMOVE=0x7FC,
     *     EVT_BT_UPDATE_STA=0x7D0, etc.)
     *
     * Without this filter the user would see "[WAV] key 0x07FE -> " 2x/s
     * for the 500 ms timer and 1x/s for the 1 s timer — and crucially,
     * they'd never see real KU_* codes because the periodic events would
     * dominate the printf log. (Confirmed via 2026-07-30 debug print.)
     *
     * Drop anything outside the key range silently. We do NOT re-enqueue
     * because we're the sole consumer of msg_dequeue() — there's no
     * other code in the wav_test module that needs these events. */
    if (msg < 0x0100 || (msg & 0xFF00) == 0x0700) {
        return;
    }
    /* Debug print ONLY for things we will actually try to dispatch — so
     * the log shows real keys, not periodic timer noise. */
    printf("[WAV] key 0x%04X -> ", msg);

    switch (msg) {
    /* ---- Play / pause ----
     * pwrkey_table[] (port_key.c:158) maps S5 (0Ω) to KEY_PLAY (0x20),
     * not KEY_PLAY_PWR_USER_DEF — using a PWR-family id would mean
     * KLH_PLAY_PWR_USER_DEF fires func.c:302's FUNC_PWROFF (1.2 s
     * long-press = power off), i.e. "press P/P then it reboots".
     * We still accept KU_PLAY_USER_DEF / KU_PLAY_PWR_USER_DEF in case
     * the table is later switched for a different board. */
    case KU_PLAY:
    case KU_PLAY_USER_DEF:
    case KU_PLAY_PWR_USER_DEF:
        s_paused = !s_paused;
        printf("%s\n", s_paused ? "pause" : "resume");
        break;

    /* ---- Volume up ----
     * Not on the user's 3-key teaching board, but accept the standard
     * VOL+ codes in case the table is later swapped to a 5-key config
     * (KU_VOL_UP_NEXT/PREV/DOWN are the "combo" short=VOL+ / long=NEXT
     * variants; KL_/KH_ are long-press and auto-repeat). */
    case KU_VOL_UP:
    case KU_VOL_UP_NEXT:
    case KU_VOL_UP_PREV:
    case KU_VOL_UP_DOWN:
    case KL_VOL_UP:
    case KH_VOL_UP:
        bsp_set_volume(bsp_volume_inc(sys_cb.vol));
        break;

    /* ---- Volume down ---- */
    case KU_VOL_DOWN:
    case KU_VOL_DOWN_PREV:
    case KU_VOL_DOWN_NEXT:
    case KL_VOL_DOWN:
    case KH_VOL_DOWN:
        bsp_set_volume(bsp_volume_dec(sys_cb.vol));
        break;

    /* ---- Next / prev ----
     * pwrkey_table[] maps S6 (12K) -> KEY_PREV (0x40) and
     * S7 (47K) -> KEY_NEXT (0x60). Plain macros, no combo meaning. */
    case KU_NEXT:
    case KU_NEXT_VOL_UP:
        s_should_next = true;
        break;

    case KU_PREV:
    case KU_PREV_VOL_DOWN:
        s_should_prev = true;
        break;

    default:
        /* Unrecognized key code in the key range (we filtered system
         * events earlier). Could be a long-press KL_NEXT_VOL_UP or a
         * KEY_NUM_x we don't bind. Just drop — the user's 3 keys are
         * all in the switch above. */
        printf("(unhandled)\n");
        break;
    }
}

/* -------------------------------------------------------------------------
 * Task 5.1 + 5.2: open the currently selected playlist entry
 *                  (s_pl[s_index]) and read its 64-byte RIFF header.
 *
 * Phase 5 Part 3 changes the contract: instead of trying a fixed list of
 * candidate names, we open s_pl[s_index].fname. The caller (wav_test_run)
 * builds the playlist once, then drives the outer index loop.
 * ------------------------------------------------------------------------- */
static bool wav_test_open_and_inspect(void)
{
    const char *name = (const char *)s_pl[s_index].fname;

    printf("\n[WAV] ====== Open + inspect [%u/%u] %s ======\n",
           (unsigned)s_index + 1, (unsigned)s_total, name);

    /* Re-mount each file — cheap, and Petit FatFs holds no persistent
     * mount state across opens. */
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }

    fr = pff_open(name);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_open(\"%s\") -> %d (%s)\n", name, fr, fr_str(fr));
        return false;
    }

    UINT br = 0;
    fr = pff_lseek(0);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(0) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    fr = pff_read(s_header, sizeof(s_header), &br);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_read -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    printf("[WAV] pff_read first %u bytes (file size=%lu):\n",
           br, (unsigned long)s_fs.fsize);

    printf("[WAV]   ");
    for (u32 i = 0; i < br; i++) {
        printf("%02X ", s_header[i]);
        if ((i + 1) % 16 == 0 && i + 1 < br) {
            printf("\n[WAV]   ");
        }
    }
    printf("\n");

    if (br < 12) {
        printf("[WAV] FAIL: header too short (%u bytes), expected >= 12\n", br);
        return false;
    }
    if (rd32(s_header + 0) != 0x46464952) {
        printf("[WAV] FAIL: 'RIFF' magic not found, got ");
        print_fourcc(s_header + 0);
        printf("\n");
        return false;
    }
    if (rd32(s_header + 8) != 0x45564157) {
        printf("[WAV] FAIL: 'WAVE' magic not found, got ");
        print_fourcc(s_header + 8);
        printf("\n");
        return false;
    }

    u32 riff_size = rd32(s_header + 4);
    printf("[WAV] PASS: file is RIFF/WAVE\n");
    printf("[WAV]   RIFF size field  = %lu (file size - 8 = %lu)\n",
           (unsigned long)riff_size, (unsigned long)(riff_size + 8));
    printf("[WAV]   fsize (pff)      = %lu\n", (unsigned long)s_fs.fsize);

    if (br >= 20) {
        printf("[WAV]   chunk @0x0C: fourcc = ");
        print_fourcc(s_header + 12);
        printf(", size = %lu\n", (unsigned long)rd32(s_header + 16));
    }

    if (br >= 36) {
        u16 wFormatTag     = rd16(s_header + 20);
        u16 nChannels      = rd16(s_header + 22);
        u32 nSamplesPerSec = rd32(s_header + 24);
        u32 nAvgBytesPerSec= rd32(s_header + 28);
        u16 nBlockAlign    = rd16(s_header + 32);
        u16 wBitsPerSample = rd16(s_header + 34);

        printf("[WAV]   fmt chunk contents:\n");
        printf("[WAV]     wFormatTag        = 0x%04X %s\n",
               wFormatTag,
               (wFormatTag == 0x0001) ? "(PCM)" : "(NOT PCM)");
        printf("[WAV]     nChannels         = %u\n", nChannels);
        printf("[WAV]     nSamplesPerSec    = %lu Hz\n",
               (unsigned long)nSamplesPerSec);
        printf("[WAV]     nAvgBytesPerSec   = %lu\n",
               (unsigned long)nAvgBytesPerSec);
        printf("[WAV]     nBlockAlign       = %u\n", nBlockAlign);
        printf("[WAV]     wBitsPerSample    = %u\n", wBitsPerSample);
    }

    printf("[WAV] Phase 5 Part 1 PASS - file ready for Part 2 (parse + play).\n");
    return true;
}

/* -------------------------------------------------------------------------
 * Task 5.3: parse + play  (v8 - one u32/frame, signed, no XOR)
 * -------------------------------------------------------------------------
 *
 * Sample format (verified against THREE SDK sources):
 *   - bsp_iis_ext.c:765  sine test pushes one u32/frame from a table
 *     oscillating around 0x0000  -> SIGNED, no offset
 *   - func_speaker.c:110 ADC->DAC does ((u32)s<<16)|s per write
 *     -> one write == one frame, low16=L / high16=R
 *   - test/参考.md:325   "cast PCM to u32 and push, stride 4"
 *
 * A stereo WAV frame on disk is [L_lo L_hi R_lo R_hi] little-endian,
 * which reinterpreted as a LE u32 is already (R<<16)|L — the exact DAC
 * layout — so stereo needs no per-sample work at all.
 *
 * Phase 5 Part 3 (Task 5.4) additions:
 *   - per-chunk msg_dequeue() drain (once every 512 bytes pushed).
 *     Key scanning runs from the 5 ms ISR (bsp_sys.c:347 + bsp_key.c:980)
 *     independently of this loop, so messages queue up while we push.
 *   - s_paused flag: when set, skip AUBUFDATA writes but keep polling
 *     msg_dequeue() with delay_us(200) between polls so the ISR stays fed.
 *   - s_should_next / s_should_prev: when set by wav_test_handle_key(),
 *     we break out of the push loop early (at the next chunk boundary —
 *     never in the middle of a chunk, to avoid mixing PCM from two files).
 *   - return value: true = played through to EOF; false = interrupted
 *     by next/prev (or a fatal error). Outer scheduler inspects this.
 */
static bool wav_test_parse_and_play(void)
{
    printf("\n[WAV] ====== Task 5.3: Parse + Play ======\n");

    /* --- 1. Walk RIFF chunks to find 'data' --- */
    u32 data_offset = 0;
    u32 data_size = 0;
    {
        UINT br;
        u32 pos = 12;
        while (pos + 8 <= s_fs.fsize) {
            FRESULT fr = pff_lseek(pos);
            if (fr != FR_OK) {
                printf("[WAV] FAIL: pff_lseek(%lu) -> %d (%s)\n",
                       (unsigned long)pos, fr, fr_str(fr));
                return false;
            }
            fr = pff_read(s_header, 8, &br);
            if (fr != FR_OK || br < 8) {
                printf("[WAV] FAIL: chunk header read @%lu -> %d (%s)\n",
                       (unsigned long)pos, fr, fr_str(fr));
                return false;
            }
            u32 chunk_size = rd32(s_header + 4);
            const char *id = (const char*)s_header;
            printf("[WAV]   chunk @0x%08lx: %c%c%c%c size=%lu\n",
                   (unsigned long)pos, id[0], id[1], id[2], id[3],
                   (unsigned long)chunk_size);
            if (id[0] == 'd' && id[1] == 'a' && id[2] == 't' && id[3] == 'a') {
                data_offset = pos + 8;
                data_size = chunk_size;
                break;
            }
            pos += 8 + chunk_size + (chunk_size & 1);
        }
    }
    if (data_offset == 0) {
        printf("[WAV] FAIL: no 'data' chunk found\n");
        return false;
    }

    /* --- 2. Re-validate fmt chunk --- */
    UINT br;
    FRESULT fr = pff_lseek(12);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(12) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    fr = pff_read(s_header, 36, &br);
    if (fr != FR_OK || br < 36) {
        printf("[WAV] FAIL: re-read fmt @12 -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    u16 wFormatTag     = rd16(s_header + 8);
    u16 nChannels      = rd16(s_header + 10);
    u32 nSamplesPerSec = rd32(s_header + 12);
    u16 wBitsPerSample = rd16(s_header + 22);
    if (wFormatTag != 0x0001) {
        printf("[WAV] FAIL: only PCM (wFormatTag=1) supported, got 0x%04X\n",
               wFormatTag);
        return false;
    }
    if (wBitsPerSample != 16) {
        printf("[WAV] FAIL: only 16-bit PCM supported, got %u\n", wBitsPerSample);
        return false;
    }
    if (nChannels != 1 && nChannels != 2) {
        printf("[WAV] FAIL: only mono / stereo supported, got %u channels\n",
               nChannels);
        return false;
    }

    {
        u32 bytes_per_frame = (u32)nChannels * 2;
        u32 duration_sec   = data_size / (nSamplesPerSec * bytes_per_frame);
        printf("[WAV] data chunk @%lu, size=%lu bytes (~%u sec at %lu Hz)\n",
               (unsigned long)data_offset, (unsigned long)data_size,
               (unsigned)duration_sec, (unsigned long)nSamplesPerSec);
    }

    /* --- 3. Configure DAC sample rate (auto from WAV header) --- */
    u16 spr = wav_sample_rate_to_spr(nSamplesPerSec);
    if (spr == 0xFFFF) {
        printf("[WAV] FAIL: sample rate %lu Hz not in SDK DAC enum\n",
               (unsigned long)nSamplesPerSec);
        return false;
    }
    dac_spr_set(spr);
    printf("[WAV] dac_spr_set(%lu Hz) = SPR enum %u\n",
           (unsigned long)nSamplesPerSec, (unsigned)spr);

    /* --- 4. Wake the DAC up + set digital volume ---
     * Reference (bsp_iis_ext.c:744-750) shows the correct sequence:
     *   1. bsp_change_volume (analog volume)
     *   2. dac_set_dvol(DIG_N0DB)  (digital volume — defaults to 0/mute!)
     *   3. dac_fade_in()
     *   4. dac_fade_wait()
     * Without dac_set_dvol, the DAC's digital volume stays at 0 and
     * the analog-out is silent — explains why v3-v5 produced no music. */
    dac_set_dvol(DIG_N0DB);
    bsp_change_volume(sys_cb.vol);
    dac_fade_in();
    dac_fade_wait();

    /* --- 5. Seek to data offset --- */
    fr = pff_lseek(data_offset);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(data_offset) -> %d (%s)\n",
               fr, fr_str(fr));
        return false;
    }

    /* --- 6. Stream PCM data via AUBUFDATA register (direct push) ---
     *
     * Direct AUBUFDATA register writes (the proven SDK pattern from
     * bsp_iis_ext.c:701-707 and func_speaker.c:110-116).
     *
     * The DAC's AUBUF (DACBUF) is roughly 2 KB — polling AUBUFCON
     * bit 8 before each push naturally throttles us to real-time
     * without explicit delay_us() pacing.  Bit 8 = 1 means "FIFO full";
     * we wait until it clears.
     *
     * Sample format — ONE u32 write == ONE stereo frame:
     *   - bits [15:0]  = Left  (WAV's 1st channel)
     *   - bits [31:16] = Right (WAV's 2nd channel)
     *   - samples are SIGNED 16-bit, pushed AS-IS (0x0000 == silence).
     * This matches the SDK's sine table (bsp_iis_ext.c:560-607) and the
     * ADC->DAC path (func_speaker.c:110-116), both of which push raw
     * signed samples with NO offset/XOR conversion.
     *
     * v8 FIX (channel imbalance + muddy vocals): the previous version
     * pushed TWO u32 writes per frame, each carrying one channel in the
     * UPPER 16 bits with ZERO in the lower 16 bits, plus an XOR 0x8000.
     * Both were wrong: one physical channel was fed constant zeros
     * (nearly silent) while the other received an L/R interleave at 2x
     * rate — exactly the reported symptom (one side much quieter,
     * centre-panned vocals smeared). One u32 per frame, signed, fixes it.
     *
     * A WAV stereo frame on disk is [L_lo L_hi R_lo R_hi] little-endian,
     * which reinterpreted as a little-endian u32 is already (R<<16)|L —
     * the exact layout the DAC wants — so for stereo we just cast the
     * buffer to u32* and push each frame unchanged. */
    printf("[WAV] streaming %lu bytes of %lu-bit %s PCM at %lu Hz:\n",
           (unsigned long)data_size, (unsigned long)wBitsPerSample,
           (nChannels == 2) ? "stereo" : "mono",
           (unsigned long)nSamplesPerSec);
    printf("[WAV] (direct AUBUFDATA push, polling AUBUFCON bit 8)\n");

    u32 bytes_left = data_size;
    u32 total_pushed = 0;
    u32 frames_pushed = 0;
    u32 last_print_tick = tick_get();
    u32 start_tick = tick_get();
    u32 last_wdt_tick = start_tick;

    /* Per-song clean state: each new file starts playing, not paused,
     * not requesting skip. The outer scheduler may have already toggled
     * pause mid-file; we only reset here on entry. */
    s_paused        = false;
    s_should_next   = false;
    s_should_prev   = false;

    while (bytes_left > 0) {
        /* --- WDT clear (run anywhere the SDK's func_process() would
         * normally run) ---
         * The SDK's normal main loop (func_bt / func_music / ...) calls
         * WDT_CLR() in func_process() at app/platform/functions/func.c:128.
         * wav_test_run() runs OUTSIDE that framework, so the WDT would
         * fire after ~1-2 s of operation and reset the chip — symptom:
         * "press P/P, pause works, then it reboots". Clearing every 100 ms
         * here keeps the watchdog happy during both active playback and
         * paused idle (delay_us(200) below). */
        {
            u32 now = tick_get();
            if ((u32)(now - last_wdt_tick) >= 100) {
                WDT_CLR();
                last_wdt_tick = now;
            }
        }

        /* --- Drain one message per chunk ---
         * The 5 ms ISR enqueues keys independently of this loop, so
         * msg_dequeue() returns whatever is pending right now. NO_MSG
         * (= 0) means the queue is empty — fall through and push frames. */
        {
            u16 msg = msg_dequeue();
            if (msg != NO_MSG) {
                wav_test_handle_key(msg);
            }
        }

        /* --- Honour pause: skip the push, but keep polling keys ---
         * We deliberately do NOT push silence into AUBUFDATA while
         * paused: that would inject a step discontinuity into the DAC
         * when we resume. delay_us(200) keeps the 5 ms ISR fed — the
         * ISR runs once per 5 ms regardless of what we do here, so any
         * delay up to a few ms is fine. */
        if (s_paused) {
            delay_us(200);
            continue;
        }

        /* --- Next/prev requested: exit at chunk boundary ---
         * We DO NOT break in the middle of a chunk — that would mix PCM
         * from two files. Reading + pushing the current chunk first
         * keeps the audio clean. */
        if (s_should_next || s_should_prev) {
            break;
        }

        UINT n = 0;
        const u32 want = (bytes_left < sizeof(s_pcm)) ? bytes_left
                                                        : sizeof(s_pcm);
        fr = pff_read(s_pcm, want, &n);
        if (fr != FR_OK) {
            printf("[WAV] FAIL: pff_read -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        if (n == 0) {
            break;
        }
        bytes_left -= n;

        /* Push each frame as ONE u32 to AUBUFDATA:
         *   bits[15:0]=Left, bits[31:16]=Right, signed, no conversion. */
        if (nChannels == 2) {
            /* Stereo: on-disk [L_lo L_hi R_lo R_hi] LE == (R<<16)|L as a
             * u32, which is precisely the DAC frame layout — push as-is. */
            u32 frames = n / 4;
            const u32 *src = (const u32 *)(const void *)s_pcm;
            for (u32 i = 0; i < frames; i++) {
                while (AUBUFCON & BIT(8)) { /* wait: FIFO full */ }
                AUBUFDATA = src[i];
                frames_pushed++;
            }
        } else {
            /* Mono: duplicate the single channel into both L and R. */
            u32 frames = n / 2;
            for (u32 i = 0; i < frames; i++) {
                u16 s = rd16(s_pcm + 2 * i);
                u32 v = ((u32)s << 16) | s;   /* R = L = same sample */
                while (AUBUFCON & BIT(8)) { /* wait: FIFO full */ }
                AUBUFDATA = v;
                frames_pushed++;
            }
        }
        total_pushed += n;

        /* Progress every 50 ms (no floating point — SDK has no libgcc) */
        u32 now = tick_get();
        if ((u32)(now - last_print_tick) >= 50) {
            u32 kb_pushed = total_pushed / 1024;
            u32 kb_total   = data_size / 1024;
            u32 percent    = (total_pushed * 100) / data_size;
            printf("[WAV] %lu / %lu KB (%u%%)\n",
                   (unsigned long)kb_pushed, (unsigned long)kb_total,
                   (unsigned)percent);
            last_print_tick = now;
        }
    }

    u32 elapsed_ms = (u32)(tick_get() - start_tick);
    printf("[WAV] pushed %lu bytes (%lu frames) in %lu ms\n",
           (unsigned long)total_pushed, (unsigned long)frames_pushed,
           (unsigned long)elapsed_ms);

    /* Differentiate natural end-of-file from a next/prev interrupt so
     * the outer scheduler (and the serial log) can tell them apart. */
    if (s_should_next || s_should_prev) {
        printf("[WAV] interrupted (%s)\n",
               s_should_next ? "next" : "prev");
        return false;
    }
    printf("[WAV] Phase 5 Part 2 PASS - file played.\n");
    return true;
}

/* -------------------------------------------------------------------------
 * Entry point - Part 3 outer scheduler
 *
 * Two-level structure (see plan: shimmying-pondering-graham.md):
 *   OUTER (this fn):  build playlist once, then loop "play this slot,
 *                     advance s_index when done or interrupted"
 *   INNER (parse_and_play): drain msg_dequeue() once per 512-byte chunk;
 *                           honour pause / next / prev flags
 *
 * Wrap-around:
 *   next:  s_index = (s_index + 1) % s_total;   last -> first
 *   prev:  s_index = (s_index == 0) ? s_total-1 : s_index-1
 *
 * On any single-file fatal error we advance to the next track rather
 * than halting — keeps the rest of the playlist audible.
 * ------------------------------------------------------------------------- */
void wav_test_run(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] SDIO mode, SD0MAP_G3 (PE5/6/7)\n");
    printf("  [WAV] Phase 5 Part 3: playlist + key controls\n");
    printf("=========================================\n");

    /* --- Build playlist (Task 5.4.2) --- */
    printf("[WAV] msg queue ready (USER_PWRKEY=%d)\n", USER_PWRKEY);
    u8 n = wav_test_build_playlist();
    if (n == 0) {
        printf("[WAV] SKIP: no *.WAV files in SD card root.\n");
        printf("[WAV]   To run wav_test:\n");
        printf("[WAV]     1. Format the SD card as FAT32.\n");
        printf("[WAV]     2. Copy one or more 16-bit PCM .wav files to\n");
        printf("[WAV]        the root with 8.3 short names (PLAY.WAV,\n");
        printf("[WAV]        SONG01.WAV, etc. — no LFN).\n");
        printf("[WAV]     3. Re-flash and re-run.\n");
        goto halt;
    }
    printf("[WAV] playlist: %u song(s)\n", (unsigned)n);
    for (u8 i = 0; i < n; i++) {
        printf("[WAV]   [%u] %-12s  %lu bytes\n",
               (unsigned)i + 1, (const char *)s_pl[i].fname,
               (unsigned long)s_pl[i].fsize);
    }
    printf("[WAV] Controls: KU_PLAY=pause/resume, KU_NEXT/PREV=skip, KU_VOL_UP/DOWN=volume\n");

    /* --- Outer scheduler --- */
    u32 last_wdt_tick = tick_get();
    for (;;) {
        /* WDT clear — same reason as the inner push loop: this is the
         * only "main loop" the WDT sees in our test, and we want to
         * keep the chip alive between songs and during open/inspect. */
        {
            u32 now = tick_get();
            if ((u32)(now - last_wdt_tick) >= 100) {
                WDT_CLR();
                last_wdt_tick = now;
            }
        }

        printf("\n[WAV] --- playing slot %u/%u ---\n",
               (unsigned)s_index + 1, (unsigned)n);

        /* Reset per-song state. Important: parse_and_play will reset
         * s_paused/s_should_* again internally, but we set them here
         * so the outer advance logic below sees clean values. */
        s_should_next = false;
        s_should_prev = false;
        s_paused      = false;

        bool ok_inspect = wav_test_open_and_inspect();
        bool ok_play    = ok_inspect ? wav_test_parse_and_play() : false;

        /* Advance logic. Key insight: BOTH natural EOF and a NEXT press
         * advance by 1; BOTH an error and a PREV press go elsewhere. So
         * a "user-requested skip" should win over the natural fallback,
         * but the actual advance is the same in two of three cases.
         * Cleanest is to compute the target index first, then print. */
        u8 from = s_index;
        u8 to;
        if (s_should_next) {
            to = (u8)((s_index + 1) % n);
        } else if (s_should_prev) {
            to = (u8)(s_index == 0 ? n - 1 : s_index - 1);
        } else {
            /* Natural EOF or unrecoverable error: still move on so the
             * rest of the playlist keeps playing. */
            to = (u8)((s_index + 1) % n);
        }
        s_index = to;
        const char *why;
        if (s_should_next)        why = "next";
        else if (s_should_prev)   why = "prev";
        else if (ok_play)         why = "played";
        else if (ok_inspect)      why = "skipped";
        else                      why = "skipped";
        printf("[WAV] slot %u -> %u (%s)\n",
               (unsigned)from + 1, (unsigned)s_index + 1, why);

        (void)ok_inspect;
        (void)ok_play;
    }

halt:
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
/*
 * wav_test.c - Phase 5 WAV player
 *
 * === Phase 5 Part 1 (commit 6716272) ===
 *   - pff_mount() + pff_open() + 64-byte RIFF header read + hex dump
 *
 * === Phase 5 Part 2 (v8) ===
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
 * Phase 5 Part 3 (next commit) will add KU_PLAY / KU_NEXT / KU_PREV /
 * KU_VOL_UP / KU_VOL_DOWN via the SDK message queue.
 *
 * BSS budget:
 *   - 64-byte RIFF header buffer (BSS)
 *   - 512-byte PCM streaming buffer (aram .wav_buf)
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
 * Task 5.1 + 5.2: open a WAV file and read the RIFF header
 * ------------------------------------------------------------------------- */
static bool wav_test_open_and_inspect(void)
{
    static const char *candidates[] = {
        "PLAY.WAV", "TEST.WAV", "SAMPLE.WAV", "MUSIC.WAV",
    };

    printf("\n[WAV] ====== Task 5.1+5.2: Open WAV + Read RIFF Header ======\n");

    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    printf("[WAV] PASS: mounted, fs_type=%d\n", (int)s_fs.fs_type);

    const char *picked = NULL;
    for (u32 i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        fr = pff_open(candidates[i]);
        printf("[WAV] pff_open(\"%s\") -> %d (%s)\n",
               candidates[i], fr, fr_str(fr));
        if (fr == FR_OK) { picked = candidates[i]; break; }
    }
    if (!picked) {
        printf("[WAV] SKIP: no candidate WAV file found.\n");
        printf("[WAV]   To run wav_test:\n");
        printf("[WAV]     1. On a PC, format the SD card as FAT32.\n");
        printf("[WAV]     2. Copy a 16-bit PCM .wav file to the root, named\n");
        printf("[WAV]        one of: PLAY.WAV / TEST.WAV / SAMPLE.WAV / MUSIC.WAV\n");
        printf("[WAV]     3. Re-flash and re-run.\n");
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

    while (bytes_left > 0) {
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
    printf("[WAV] Phase 5 Part 2 PASS - file played.\n");
    return true;
}

/* -------------------------------------------------------------------------
 * Entry point - Part 1 + Part 2 (Part 3 will follow)
 * ------------------------------------------------------------------------- */
void wav_test_run(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] SDIO mode, SD0MAP_G3 (PE5/6/7)\n");
    printf("  [WAV] Phase 5 Part 1+2: open + parse + play\n");
    printf("=========================================\n");

    if (!wav_test_open_and_inspect()) goto halt;
    if (!wav_test_parse_and_play())  goto halt;

    printf("\n[WAV] ====== All active tests PASSED ======\n");

halt:
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
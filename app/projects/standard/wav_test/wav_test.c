/*
 * wav_test.c — Phase 5 WAV player
 *
 * === Phase 5 Part 1 (this commit) ===
 *   - pff_mount()
 *   - pff_open() on a few candidate 8.3 filenames
 *   - pff_read() the first 44 bytes (RIFF header)
 *   - Hex dump + sanity check ("RIFF" / "WAVE" magic)
 *   - Print fsize and stop
 *
 * Phase 5 Part 2 will parse the RIFF header fully and stream PCM to DAC.
 * Phase 5 Part 3 will add KU_PLAY / KU_NEXT / KU_PREV / KU_VOL_UP / KU_VOL_DOWN.
 *
 * BSS budget: 64 bytes for the 44-byte RIFF header buffer is fine; the
 * 512-byte PCM streaming buffer for Part 2 will go into aram's .wav_buf
 * section (mirrors Phase 4's fat_test/diskio.c design).
 */

#include "include.h"
#include "wav_test.h"
#include "pff.h"        /* MUST come after include.h — FRESULT comes from api_fs.h */

/* -------------------------------------------------------------------------
 * Petit FatFs state (mirrors fat_test.c)
 * ------------------------------------------------------------------------- */
static FATFS s_fs;

/* 64-byte buffer for the RIFF header. We read 44 bytes — the smallest
 * standard RIFF/WAVE header is 44 bytes (12 RIFF + 24 fmt + 8 data
 * preamble). 64 bytes gives us headroom for unusual files with extra
 * chunks before "data".  4-byte aligned for any future DMA use. */
static u8 s_header[64] __attribute__((aligned(4)));

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

/* Little-endian reader helpers (RIFF is little-endian). */
static u16 rd16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 rd32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* Pretty-print 4 bytes as ASCII (e.g. "RIFF" / "WAVE" / "fmt " / "data"). */
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

/* -------------------------------------------------------------------------
 * Task 5.1 + 5.2: open a WAV file and read the RIFF header
 * -------------------------------------------------------------------------
 *
 * Strategy:
 *   - mount the volume (Phase 4 helper logic, repeated here so wav_test
 *     is independent of fat_test_run)
 *   - try to open one of a few well-known root-level filenames
 *   - read the first 44 bytes
 *   - verify the "RIFF" / "WAVE" magic
 *   - print a hex dump and the parsed fields
 *
 * If no candidate file exists, print a friendly hint and report the
 * task as SKIPPED (do not FAIL — same pattern as fat_test).
 */
static bool wav_test_open_and_inspect(void)
{
    static const char *candidates[] = {
        "PLAY.WAV",
        "TEST.WAV",
        "SAMPLE.WAV",
        "MUSIC.WAV",
    };

    printf("\n[WAV] ====== Task 5.1+5.2: Open WAV + Read RIFF Header ======\n");

    /* --- 1. Mount --- */
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    printf("[WAV] PASS: mounted, fs_type=%d\n", (int)s_fs.fs_type);

    /* --- 2. Open --- */
    const char *picked = NULL;
    for (u32 i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        fr = pff_open(candidates[i]);
        printf("[WAV] pff_open(\"%s\") -> %d (%s)\n",
               candidates[i], fr, fr_str(fr));
        if (fr == FR_OK) {
            picked = candidates[i];
            break;
        }
    }
    if (!picked) {
        printf("[WAV] SKIP: no candidate WAV file found.\n");
        printf("[WAV]   To run wav_test_open_and_inspect:\n");
        printf("[WAV]     1. On a PC, format the SD card as FAT32.\n");
        printf("[WAV]     2. Copy a 16-bit PCM .wav file to the root, named\n");
        printf("[WAV]        one of: PLAY.WAV / TEST.WAV / SAMPLE.WAV / MUSIC.WAV\n");
        printf("[WAV]     3. Re-flash and re-run.\n");
        return false;
    }

    /* --- 3. Read the first 44 bytes (smallest legal RIFF/WAVE header) --- */
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

    /* --- 4. Hex dump of the first 44 bytes (or however many we read) --- */
    printf("[WAV]   ");
    for (u32 i = 0; i < br; i++) {
        printf("%02X ", s_header[i]);
        if ((i + 1) % 16 == 0 && i + 1 < br) {
            printf("\n[WAV]   ");
        }
    }
    printf("\n");

    /* --- 5. Verify RIFF / WAVE magic and parse a few fields --- */
    if (br < 12) {
        printf("[WAV] FAIL: header too short (%u bytes), expected >= 12\n", br);
        return false;
    }
    if (rd32(s_header + 0) != 0x46464952 /* 'R''I''F''F' LE */) {
        printf("[WAV] FAIL: 'RIFF' magic not found, got ");
        print_fourcc(s_header + 0);
        printf("\n");
        return false;
    }
    if (rd32(s_header + 8) != 0x45564157 /* 'W''A''V''E' LE */) {
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

    /* The fmt chunk starts at offset 12.  Print its fourcc + size. */
    if (br >= 20) {
        printf("[WAV]   chunk @0x0C: fourcc = ");
        print_fourcc(s_header + 12);
        printf(", size = %lu\n", (unsigned long)rd32(s_header + 16));
    }

    /* --- 6. Print the five-format-tag hex and FormatTag value if we got
     *       far enough to read the fmt chunk header. --- */
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
               (wFormatTag == 0x0001) ? "(PCM)" : "(NOT PCM — Part 2 will reject)");
        printf("[WAV]     nChannels         = %u\n", nChannels);
        printf("[WAV]     nSamplesPerSec    = %lu Hz\n",
               (unsigned long)nSamplesPerSec);
        printf("[WAV]     nAvgBytesPerSec   = %lu\n",
               (unsigned long)nAvgBytesPerSec);
        printf("[WAV]     nBlockAlign       = %u\n", nBlockAlign);
        printf("[WAV]     wBitsPerSample    = %u\n", wBitsPerSample);
    }

    printf("[WAV] Phase 5 Part 1 PASS — file ready for Part 2 (parse + play).\n");
    return true;
}

/* -------------------------------------------------------------------------
 * Entry point — Part 1 only
 * ------------------------------------------------------------------------- */
void wav_test_run(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] SDIO mode, SD0MAP_G3 (PE5/6/7)\n");
    printf("  [WAV] Phase 5 Part 1: open + read RIFF header\n");
    printf("=========================================\n");

    if (!wav_test_open_and_inspect()) goto halt;

    printf("\n[WAV] ====== All active tests PASSED ======\n");

halt:
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
/*
 * fat_test.c - Phase 4 Petit FatFs porting tests
 *
 * This file is the learning-side equivalent of tf_test.c (Phase 3).
 * Each sub-task is a separate #if-gated function so that:
 *   1. We can run a single task at a time (single-variable debug).
 *   2. The "if it ran and passed" history is preserved per task.
 *
 * Reusable Petit FatFs globals (defined here, not in diskio.c, so the
 * bridge stays clean):
 *
 *   static FATFS  s_fs;       Mounted file system object (~40 B)
 *   static DIR    s_dir;      Directory iterator (~16 B)
 *   static FILINFO s_info;    File metadata buffer (~32 B)
 *
 * Static I/O buffer (~256 B) used for file read/write tests:
 *
 *   static u8     s_io[256];
 */

#include "include.h"
#include "fat_test.h"
#include "pff.h"        /* pff_mount / pff_open / pff_read / pff_write / pff_lseek */

/* -------------------------------------------------------------------------
 * Petit FatFs state
 * ------------------------------------------------------------------------- */
static FATFS s_fs;

/* Single I/O buffer. 256 B is plenty for Phase 4 file tests.
 * 4-byte aligned to play nice with any future DMA path. */
static u8 s_io[256] __attribute__((aligned(4)));

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

static const char *fs_type_str(BYTE t)
{
    switch (t) {
    case FS_FAT12 : return "FAT12";
    case FS_FAT16 : return "FAT16";
    case FS_FAT32 : return "FAT32";
    default       : return "?";
    }
}

/* -------------------------------------------------------------------------
 * Task 4.1 + 4.2 + 4.3: mount the volume (validates the whole port)
 * -------------------------------------------------------------------------
 *
 * pff_mount() exercises:
 *   - disk_initialize()   -> sd0_init() (Phase 3 path)
 *   - check_fs()          -> disk_readp(...,510,2) reads MBR / VBR signature
 *   - BPB parsing         -> disk_readp(...,13,sizeof(buf))
 *   - FAT-type detection  -> fs->fs_type (FAT12/16/32)
 *
 * A successful mount therefore proves:
 *   - The SDK SDIO driver is alive (sd0_init OK)
 *   - Our diskio.c cache works (disk_readp returns the right bytes)
 *   - The SD card is FAT-formatted
 *   - pff.c is correctly linked
 */
#if 1   // ACTIVE: pff_mount (Task 4.1/4.2/4.3)
static bool fat_test_mount(void)
{
    printf("\n[FAT] ====== Task 4.1+4.2+4.3: pff_mount ======\n");
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[FAT] FAIL: pff_mount returned %d (%s)\n", fr, fr_str(fr));
        if (fr == FR_NOT_READY) {
            printf("[FAT]   Hint: disk_initialize() failed -> sd0_init() failed.\n");
            printf("[FAT]   Check TF card presence and PE5/PE6/PE7 wiring.\n");
        } else if (fr == FR_NO_FILESYSTEM) {
            printf("[FAT]   Hint: no FAT signature found at LBA 0.\n");
            printf("[FAT]   Format the SD card as FAT32 in Windows.\n");
        }
        return false;
    }
    printf("[FAT] PASS: mounted, fs_type=%s\n", fs_type_str(s_fs.fs_type));
    printf("[FAT]   csize=%u sectors/cluster\n", s_fs.csize);
    printf("[FAT]   n_fatent=%lu (clusters+2)\n", (unsigned long)s_fs.n_fatent);
    return true;
}
#endif

/* -------------------------------------------------------------------------
 * Task 4.4: file open / read / write
 * -------------------------------------------------------------------------
 *
 * Strategy:
 *   - Try to open one of a few well-known root-level filenames.
 *   - If a file exists:
 *       1. pff_lseek(0)  -> start from beginning
 *       2. pff_read()    -> print first ~32 bytes as a hex preview
 *       3. pff_lseek(0)  -> back to beginning for the overwrite test
 *       4. pff_write()   -> write a known pattern into the first N bytes
 *                          (only valid because Petit FatFs cannot grow files;
 *                          the file must have pre-allocated clusters)
 *       5. pff_read()    -> verify the overwrite took effect
 *   - If none of the candidate files exist, print a friendly hint and
 *     report the task as SKIPPED rather than FAIL.
 */
#if 1   // ACTIVE: file open / read / write (Task 4.4)
static bool fat_test_file_io(void)
{
    static const char *candidates[] = {
        "README.TXT",
        "TEST.TXT",
        "TEST.BIN",
    };
    static const u8 pattern[] = "FAT_TEST_OK_";   /* 13 bytes including trailing '\0' */

    printf("\n[FAT] ====== Task 4.4: File Open / Read / Write ======\n");

    /* Try each candidate. FR_OK means the file was located on disk. */
    const char *picked = NULL;
    FRESULT fr = FR_NO_FILE;
    for (u32 i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
        fr = pff_open(candidates[i]);
        printf("[FAT] pff_open(\"%s\") -> %d (%s)\n",
               candidates[i], fr, fr_str(fr));
        if (fr == FR_OK) {
            picked = candidates[i];
            break;
        }
    }
    if (!picked) {
        printf("[FAT] SKIP: no candidate file found.\n");
        printf("[FAT]   To run pff_open/pff_read/pff_write:\n");
        printf("[FAT]     1. On a PC, format the SD card as FAT32.\n");
        printf("[FAT]     2. Copy a small file (e.g. README.TXT, ~100 bytes)\n");
        printf("[FAT]        to the root of the card.\n");
        printf("[FAT]     3. Re-flash and re-run.\n");
        return false;
    }

    /* Step 1: dump the first 32 bytes as a hex preview. */
    UINT br = 0;
    u32 fsize = s_fs.fsize;
    printf("[FAT] fsize = %lu bytes\n", (unsigned long)fsize);
    if (fsize == 0) {
        printf("[FAT] SKIP: file is zero bytes, nothing to read/write.\n");
        return false;
    }
    {
        /* pff_lseek(0) - rewind before read; relies on PF_USE_LSEEK=1 */
        fr = pff_lseek(0);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_lseek(0) -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        fr = pff_read(s_io, sizeof(s_io), &br);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_read -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
    }
    printf("[FAT] pff_read first %u bytes:\n  ", br);
    for (u32 i = 0; i < br && i < 32; i++) printf("%02X ", s_io[i]);
    printf("\n");

    /* Step 2: overwrite the first byte range with our known pattern.
     * The pattern is short (< 32) so we don't exceed s_io size or risk
     * running past pre-allocated clusters (the file must have >=13 bytes,
     * which our 100-byte test files guarantee). */
    {
        UINT bw = 0;
        /* Rewind, then overwrite */
        fr = pff_lseek(0);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_lseek(0) for write -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        /* pff_write API:
         *   write a non-zero number of bytes -> stream bytes
         *   write 0 bytes (NULL ptr OK) -> finalize the current write op
         */
        fr = pff_write(pattern, sizeof(pattern) - 1, &bw);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_write -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        printf("[FAT] pff_write %u bytes OK\n", bw);
        fr = pff_write(NULL, 0, &bw);    /* finalize */
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_write finalize -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
    }

    /* Step 3: read back and verify. */
    {
        UINT br2 = 0;
        fr = pff_lseek(0);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_lseek(0) for verify -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        fr = pff_read(s_io, sizeof(pattern) - 1, &br2);
        if (fr != FR_OK) {
            printf("[FAT] FAIL: pff_read(verify) -> %d (%s)\n", fr, fr_str(fr));
            return false;
        }
        /* Compare against expected pattern */
        u32 mismatches = 0;
        for (u32 i = 0; i < sizeof(pattern) - 1; i++) {
            if (s_io[i] != pattern[i]) {
                if (mismatches < 4) {
                    printf("[FAT]   mismatch @%u: got %02X expected %02X\n",
                           i, s_io[i], pattern[i]);
                }
                mismatches++;
            }
        }
        if (mismatches) {
            printf("[FAT] FAIL: %u byte mismatches after pff_write\n", mismatches);
            return false;
        }
        printf("[FAT] PASS: pff_write bytes verified by pff_read\n");
    }

    /* Final reminder: tear down by re-reading after rewind so we don't
     * leave any partially-staged write behind. */
    {
        UINT bw = 0;
        (void)pff_write(NULL, 0, &bw);   /* idempotent finalize */
    }

    return true;
}
#endif

/* -------------------------------------------------------------------------
 * Entry point
 * -------------------------------------------------------------------------
 *
 * Each Phase 4 sub-task is a separate #if block. Today only Task 4.1 (mount)
 * and Task 4.4 (file I/O) are ACTIVE. Tasks 4.2 and 4.3 are achieved by
 * reading the source files; no runtime code is needed.
 */
void fat_test_run(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [FAT] BT892XA2 Petit FatFs Test\n");
    printf("  [FAT] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [FAT] SDIO mode, SD0MAP_G3 (PE5/6/7)\n");
    printf("  [FAT] Phase 4: Petit FatFs R0.03a port\n");
    printf("=========================================\n");

    /* Task 4.1 + 4.2 + 4.3: mount the volume. */
#if 1
    if (!fat_test_mount()) goto halt;
#endif

    /* Task 4.4: open / read / write a file. */
#if 1
    if (!fat_test_file_io()) goto halt;
#endif

    printf("\n[FAT] ====== All active tests PASSED ======\n");

halt:
    printf("[FAT] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
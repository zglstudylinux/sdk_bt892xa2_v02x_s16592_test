#ifndef _FAT_TEST_H
#define _FAT_TEST_H

/*
 * fat_test module - Phase 4 Petit FatFs porting test
 *
 * Task list (docs/任务清单.md "Phase 4"):
 *   Task 4.1  建立 fat_test 目录                (done by directory + files)
 *   Task 4.2  学习 Petit FatFs 开源程序         (pff.h/pff.c/diskio.h read)
 *   Task 4.3  完成 Petit FatFs 移植              (diskio.c bridges sd0_***)
 *   Task 4.4  文件打开 / 读写操作                (pff_open + pff_read + pff_write)
 *
 * File layout (all under fat_test/):
 *   pff.h                    upstream API (ChaN R0.03a)
 *   pff.c                    upstream core (ChaN R0.03a)
 *   pffconf.h                configuration (read/write/lseek/FAT32 enabled)
 *   diskio.h                 upstream disk I/O interface (ChaN)
 *   diskio.c                 BT892XA2 bridge (uses sd0_read/sd0_write)
 *   fat_test.h               this file
 *   fat_test.c               4 test functions + entry point
 *   PETIT_FATFS_LICENSE.txt  upstream BSD-style licence
 *
 * Hardware (carried over from Phase 3):
 *   SD0_MAPPING = SD0MAP_G3
 *   SDCMD = PE5, SDCLK = PE6, SDDAT0 = PE7
 *
 * Usage:
 *   - main.c calls fat_test_run() instead of tf_test_run()
 *   - Each sub-task is gated by its own #if so a single task can be enabled
 *   - On PASS, write a docs/fat_test_XX.md report, then move on
 *
 * Phase 4.4 expects the user to have pre-formatted the SD card with FAT32
 * in Windows and dropped a small file (~100 bytes) named one of
 * "README.TXT", "TEST.TXT" or "TEST.BIN" at the root.
 */

void fat_test_run(void);

#endif /* _FAT_TEST_H */
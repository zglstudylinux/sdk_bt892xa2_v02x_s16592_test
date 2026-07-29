#ifndef _WAV_TEST_H
#define _WAV_TEST_H

/*
 * wav_test module — Phase 5 WAV player
 *
 * Task list (docs/任务清单.md "Phase 5"):
 *   Task 5.1  建 wav_test 目录                (done)
 *   Task 5.2  从 TF 卡中打开一个 wav 文件       (in this file)
 *   Task 5.3  解析并播放该 WAV 文件             (Phase 5 Part 2)
 *   Task 5.4  增加按键上下曲、播放暂停、音量加减 (Phase 5 Part 3)
 *
 * File layout (all under wav_test/):
 *   wav_test.h              this file
 *   wav_test.c              Phase 5.1 + 5.2 + 5.3 + 5.4 test implementation
 *
 * Phase 5 inherits all the Phase 4 building blocks:
 *   - fat_test/pff.h       Petit FatFs R0.03a API (pff_mount / pff_open / pff_read / ...)
 *   - fat_test/diskio.c    bridges sd0_read / sd0_write on PE5/6/7
 *   - platform/libs/api_dac.h   obuf_put_samples / obuf_put_one_sample
 *   - platform/libs/api_msg.h   msg_enqueue / msg_dequeue
 *   - platform/bsp/bsp_key.h   KU_PLAY / KU_NEXT / KU_PREV / KU_VOL_UP / KU_VOL_DOWN
 *
 * Hardware (unchanged from Phase 3 / 4):
 *   SD0_MAPPING = SD0MAP_G3
 *   SDCMD = PE5, SDCLK = PE6, SDDAT0 = PE7
 *
 * Usage:
 *   - main.c calls wav_test_run() instead of fat_test_run()
 *   - Put a 16-bit PCM WAV file on the SD card root, named any of:
 *     PLAY.WAV / TEST.WAV / SAMPLE.WAV / MUSIC.WAV
 *   - Build, flash, watch the serial output
 *
 * Incremental delivery plan:
 *   Part 1 (this commit): pff_mount + pff_open + hex dump of RIFF header
 *   Part 2: parse RIFF header + DAC playback of PCM data
 *   Part 3: key controls (pause/next/prev/volume)
 */

void wav_test_run(void);

#endif /* _WAV_TEST_H */
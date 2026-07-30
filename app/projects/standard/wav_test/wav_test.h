#ifndef _WAV_TEST_H
#define _WAV_TEST_H

/*
 * wav_test module — Phase 5 WAV player
 *
 * Task list (docs/任务清单.md "Phase 5"):
 *   Task 5.1  建 wav_test 目录                (done)
 *   Task 5.2  从 TF 卡中打开一个 wav 文件       (done)
 *   Task 5.3  解析并播放该 WAV 文件             (done — Phase 5 Part 2)
 *   Task 5.4  增加按键上下曲、播放暂停、音量加减  (Phase 5 Part 3, this commit)
 *
 * File layout (all under wav_test/):
 *   wav_test.h              this file
 *   wav_test.c              Phase 5.1 + 5.2 + 5.3 + 5.4 test implementation
 *
 * Phase 5 inherits all the Phase 4 building blocks:
 *   - fat_test/pff.h       Petit FatFs R0.03a API (pff_mount / pff_open / pff_read / pff_opendir / pff_readdir ...)
 *   - fat_test/diskio.c    bridges sd0_read / sd0_write on PE5/6/7
 *   - platform/libs/api_dac.h   dac_set_dvol / dac_set_volume / dac_fade_*
 *   - platform/libs/api_msg.h   msg_enqueue / msg_dequeue (key events from 5ms ISR)
 *   - platform/bsp/bsp_key.h   KU_PLAY / KU_NEXT / KU_PREV / KU_VOL_UP / KU_VOL_DOWN + their combos
 *   - platform/bsp/bsp_dac.h   bsp_change_volume / bsp_volume_inc / bsp_volume_dec / bsp_set_volume
 *
 * Hardware (unchanged from Phase 3 / 4):
 *   SD0_MAPPING = SD0MAP_G3
 *   SDCMD = PE5, SDCLK = PE6, SDDAT0 = PE7
 *
 * Usage:
 *   - main.c calls wav_test_run() instead of fat_test_run()
 *   - Put one or more 16-bit PCM 8.3-short-name WAV files in the SD card root.
 *     Files are picked up in FAT order (filesystem-defined, NOT alphabetical).
 *     Up to WAV_MAX_FILES (16) are queued.
 *   - Playback controls (sent as `msg_dequeue()` events from the 5 ms ISR):
 *       KU_PLAY (or KU_PLAY_PWR_USER_DEF) : pause / resume
 *       KU_NEXT / KU_NEXT_VOL_UP / ...    : next song  (wraps last -> first)
 *       KU_PREV / KU_PREV_VOL_DOWN / ...  : prev song  (wraps first -> last)
 *       KU_VOL_UP / KH_VOL_UP / ...       : volume up  (clamped to VOL_MAX)
 *       KU_VOL_DOWN / KH_VOL_DOWN / ...   : volume down (clamped to 0)
 *   - Build, flash, watch the serial output.
 *
 * Incremental delivery plan:
 *   Part 1: pff_mount + pff_open + hex dump of RIFF header
 *   Part 2: parse RIFF header + DAC playback of PCM data
 *   Part 3: key controls + playlist enumeration (THIS commit)
 *
 * Task 5.4 sub-tasks:
 *   5.4.1  flip PF_USE_DIR=1 in fat_test/pffconf.h            (done)
 *   5.4.2  build_playlist() — pff_opendir("") + pff_readdir loop, skip AM_LFN/AM_DIR/AM_VOL, accept *.WAV (case-insensitive), max 16 files
 *   5.4.3  refactor wav_test_run() into outer (next/prev scheduler) + inner (play_current)
 *   5.4.4  insert msg_dequeue() inside the 512-byte push-frame loop
 *   5.4.5  handle KU_PLAY → pause toggle; KU_VOL_UP/DOWN → bsp_set_volume(+/-); KU_NEXT/PREV → request next/prev
 *   5.4.6  on pause, skip AUBUFDATA writes but keep draining the message queue (delay_us(200) between polls)
 */

void wav_test_run(void);

#endif /* _WAV_TEST_H */
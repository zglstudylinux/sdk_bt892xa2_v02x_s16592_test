/*
 * wav_test.c — Phase 5 WAV 音乐播放器测试实现
 *
 * 当前阶段启用：
 *   wav_test_01  工程接入（banner + 5ms tick）
 *   wav_test_02  pff_open("TEST.WAV") 打开固定名 wav
 *
 * 后续 wav_test_03 ~ 07 按需在 wav_test_run() 里逐步 uncomment，
 * 每次只跑一个新增测试，单变量调试。
 *
 * 代码风格参考：
 *   - tf_test/tf_test.c   (Phase 3: SDIO 裸块测试)
 *   - fat_test/fat_test.c (Phase 4: Petit FatFs 移植测试)
 */

#include "include.h"
#include "wav_test.h"
#include "pff.h"        /* pff_mount / pff_open / pff_read / pff_lseek (Phase 4) */

/* ---------------------------------------------------------------------------
 * Petit FatFs 状态对象（与 fat_test 共享同一份移植代码）
 * ---------------------------------------------------------------------------
 *
 * pff_mount() 内部把它填满（fs_type, csize, n_fatent 等），后续 pff_open
 * 打开文件后会更新 fsize / fptr / org_clust 等字段。我们只需要一个实例。
 *
 * 静态分配 40 字节左右，远低于 ram.ld 限定的 13KB BSS 总余量。
 */
static FATFS s_fs;

/* ---------------------------------------------------------------------------
 * FRESULT -> 字符串映射（移植自 fat_test/fat_test.c）
 * ---------------------------------------------------------------------------
 *
 * 错误码本身只是 0~9 的小整数，直接 printf("%d") 用户看不懂；
 * 映射成 "FR_NO_FILE" 之类的串口输出后排查更快。
 */
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

/* ---------------------------------------------------------------------------
 * 5ms tick 钩子
 * ---------------------------------------------------------------------------
 *
 * 由 plugin.c 的 plugin_tmr5ms_isr() 每 5ms 调用一次。
 * wav_test_01 ~ 02 阶段：空实现（只在 wav_test_run() 主循环做实际工作）。
 * wav_test_05 起：改为流式喂 PCM 到 DAC obuf。
 *
 * AT(.com_text.wav) 把函数放到 .com_text section（ram.ld:97-111），保证
 * 它能被 5ms ISR 在 RAM 中调到（XIP 闪存执行延迟会卡时序）。
 */
AT(.com_text.wav)
void wav_test_tick(void)
{
#if 0   // wav_test_05 起启用：流式喂 DAC obuf
    extern void wav_player_tick(void);
    wav_player_tick();
#endif
}

/* ---------------------------------------------------------------------------
 * 测试函数（按需 #if 1 启用）
 * --------------------------------------------------------------------------- */

#if 1   // wav_test_01: 工程接入（ACTIVE, 仅 banner + halt）
static bool wav_test_banner(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] Phase 5: WAV music player\n");
    printf("  [WAV] wav_test_01: engineering bring-up\n");
    printf("=========================================\n");
    return true;
}
#endif

#if 1   // wav_test_02: pff_open("TEST.WAV") 打开固定名 wav
/**
 * @brief Task 2: 从 TF 卡中打开一个 wav 文件
 *
 * 复用 Phase 4 已移植的 Petit FatFs 全套 API：
 *   - pff_mount(&s_fs)            — 走通 disk_initialize + BPB 解析
 *   - pff_open("TEST.WAV")        — 按 8.3 SFN 在根目录查找
 *   - pff_lseek(0) + pff_read()   — peek 前 12 字节确认 RIFF/WAVE 头
 *
 * @return true  打开 + 头 magic 校验通过
 * @return false 任一失败（详见串口 FAIL 行）
 */
static bool wav_test_open_fixed(void)
{
    printf("\n[WAV] ====== wav_test_02: open fixed TEST.WAV ======\n");
    printf("[WAV] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7  (SDIO, SD0MAP_G3)\n");

    /* Step 1: mount the volume (re-uses diskio.c from Phase 4). */
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        if (fr == FR_NOT_READY) {
            printf("[WAV]   Hint: sd0_init() failed (Phase 3 hint).\n");
            printf("[WAV]          Check TF card presence and PE5/PE6/PE7 wiring.\n");
        } else if (fr == FR_NO_FILESYSTEM) {
            printf("[WAV]   Hint: no FAT signature at LBA 0.\n");
            printf("[WAV]          Format the SD card as FAT32 in Windows.\n");
        }
        return false;
    }
    printf("[WAV] PASS: pff_mount OK, fs_type=%d\n", s_fs.fs_type);
    printf("[WAV]   csize=%u sectors/cluster\n", s_fs.csize);
    printf("[WAV]   n_fatent=%lu (clusters+2)\n", (unsigned long)s_fs.n_fatent);

    /* Step 2: open TEST.WAV. The file must exist at SD root; Petit FatFs
     *        does NOT support creating files (see fat_test docs). */
    fr = pff_open("TEST.WAV");
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_open(\"TEST.WAV\") -> %d (%s)\n", fr, fr_str(fr));
        if (fr == FR_NO_FILE) {
            printf("[WAV]   Hint: TEST.WAV not found at SD root.\n");
            printf("[WAV]          Copy any .wav file (or rename one) to\n");
            printf("[WAV]          <SD card root>\\TEST.WAV and re-flash.\n");
        }
        return false;
    }
    printf("[WAV] PASS: pff_open OK, fsize=%lu bytes\n",
           (unsigned long)s_fs.fsize);

    /* Step 3: peek first 12 bytes (RIFF<size>WAVE). */
    UINT br = 0;
    u8 hdr[12];
    fr = pff_lseek(0);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(0) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    fr = pff_read(hdr, sizeof(hdr), &br);
    if (fr != FR_OK || br != sizeof(hdr)) {
        printf("[WAV] FAIL: pff_read(hdr, 12) -> %d (%s), br=%u\n",
               fr, fr_str(fr), br);
        return false;
    }
    printf("[WAV] PASS: first 12 bytes:\n  ");
    for (u32 i = 0; i < sizeof(hdr); i++) printf("%02X ", hdr[i]);
    printf("\n");

    /* Step 4: validate RIFF / WAVE magic (only first 4 + 8..11 matter
     *        for the "is it a wav file?" check; the size field at 4..7
     *        is the file size - 8 in little-endian). */
    if (memcmp(hdr, "RIFF", 4) != 0) {
        printf("[WAV] FAIL: first 4 bytes != \"RIFF\" (got %02X %02X %02X %02X)\n",
               hdr[0], hdr[1], hdr[2], hdr[3]);
        return false;
    }
    if (memcmp(hdr + 8, "WAVE", 4) != 0) {
        printf("[WAV] FAIL: bytes 8..11 != \"WAVE\" (got %02X %02X %02X %02X)\n",
               hdr[8], hdr[9], hdr[10], hdr[11]);
        return false;
    }
    printf("[WAV] PASS: magic \"RIFF...WAVE\" detected\n");
    return true;
}
#endif

#if 0   // wav_test_03: parse 44-byte RIFF header
static bool wav_test_parse_header(void);
#endif

#if 0   // wav_test_04: obuf one-shot play (first sound)
static bool wav_test_play_one_shot(void);
#endif

#if 0   // wav_test_05: 5ms tick streaming
static bool wav_test_stream_start(void);
#endif

#if 0   // wav_test_06: PLAY/PREV/NEXT key control
static bool wav_test_keys_init(void);
#endif

#if 0   // wav_test_07: root dir scan + playlist
static bool wav_test_scan_root(void);
#endif

/* ---------------------------------------------------------------------------
 * 入口：wav_test_run
 * ---------------------------------------------------------------------------
 *
 * 每个 wav_test_NN 子任务是一个 #if 闸门控制的测试函数，按顺序串联，
 * 任一失败立即跳到 halt。当前激活 wav_test_01 + wav_test_02。
 */
void wav_test_run(void)
{
#if 1
    if (!wav_test_banner())      goto halt;   // wav_test_01 ACTIVE
#endif

#if 1
    if (!wav_test_open_fixed())  goto halt;   // wav_test_02 ACTIVE
#endif

#if 0
    if (!wav_test_parse_header())  goto halt;  // wav_test_03
    if (!wav_test_play_one_shot()) goto halt;  // wav_test_04
    if (!wav_test_stream_start())  goto halt;  // wav_test_05
    if (!wav_test_keys_init())     goto halt;  // wav_test_06
    if (!wav_test_scan_root())     goto halt;  // wav_test_07
#endif

    printf("\n[WAV] ====== All active tests PASSED ======\n");

halt:
    // WDT 由 SDK tick 自动喂（bsp_sys_init 里 sys_set_tmr_enable(1,1)）。
    // delay_5ms(200) 进入 200ms 周期短延时，不会触发 WDT 复位。
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
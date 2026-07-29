/*
 * wav_test.c — Phase 5 WAV 音乐播放器测试实现
 *
 * 当前阶段只启用 wav_test_01（工程接入 + 5ms tick 验证）。
 * 后续 wav_test_02 ~ 07 按需在 wav_test_run() 里逐步 uncomment，
 * 每次只跑一个新增测试，单变量调试。
 *
 * 代码风格参考：
 *   - tf_test/tf_test.c   (Phase 3: SDIO 裸块测试)
 *   - fat_test/fat_test.c (Phase 4: Petit FatFs 移植测试)
 */

#include "include.h"
#include "wav_test.h"

/* ---------------------------------------------------------------------------
 * 5ms tick 钩子
 * ---------------------------------------------------------------------------
 *
 * 由 plugin.c 的 plugin_tmr5ms_isr() 每 5ms 调用一次。
 * wav_test_01 阶段用它来翻转一个内部计数器，验证 tick 通路活着；
 * wav_test_05 起改为流式喂 PCM 到 DAC obuf。
 *
 * AT(.com_text.wav) 把函数放到 .com_text section（ram.ld:97-111），保证
 * 它能被 5ms ISR 在 RAM 中调到（XIP 闪存执行延迟会卡时序）。
 */
AT(.com_text.wav)
void wav_test_tick(void)
{
#if 0   // wav_test_01 验证 tick 通路（counter + print，可选）
    static u16 cnt = 0;
    if (++cnt >= 100) {        // 100 × 5ms = 500ms
        cnt = 0;
        printf("[WAV] tick alive (%u ms)\n", (u16)(cnt + 500));
    }
#endif

#if 0   // wav_test_05 起启用：流式喂 DAC obuf
    extern void wav_player_tick(void);
    wav_player_tick();
#endif
}

/* ---------------------------------------------------------------------------
 * 测试函数占位（按需 #if 1 启用）
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
    printf("[WAV] Halt. 5ms loop (tick should be running).\n");
    return true;
}
#endif

#if 0   // wav_test_02: pff_open("TEST.WAV")
static bool wav_test_open_fixed(void);
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
 * 任一失败立即跳到 halt。wav_test_01 阶段只跑 banner。
 */
void wav_test_run(void)
{
#if 1
    if (!wav_test_banner()) goto halt;   // wav_test_01 ACTIVE
#endif

#if 0
    if (!wav_test_open_fixed())     goto halt;   // wav_test_02
    if (!wav_test_parse_header())   goto halt;   // wav_test_03
    if (!wav_test_play_one_shot())  goto halt;   // wav_test_04
    if (!wav_test_stream_start())   goto halt;   // wav_test_05
    if (!wav_test_keys_init())      goto halt;   // wav_test_06
    if (!wav_test_scan_root())      goto halt;   // wav_test_07
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
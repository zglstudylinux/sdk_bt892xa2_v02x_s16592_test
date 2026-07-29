#ifndef _WAV_TEST_H
#define _WAV_TEST_H

/*
 * wav_test 模块 — Phase 5 WAV 音乐播放器测试
 *
 * 任务清单（docs/任务清单.md "Phase 5"）：
 *   Task 1  建立 wav_test 目录                 (本目录 + 本文件 + wav_test.c)
 *   Task 2  从 TF 卡中打开一个 wav 文件
 *   Task 3  解析并播放该 WAV 文件
 *   Task 4  增加按键上下曲、播放暂停、音量加减等功能
 *
 * 子任务拆分（每个 wav_test_NN 对应一个独立测试）：
 *   wav_test_01  工程接入（banner + 5ms tick）           <-- 当前激活
 *   wav_test_02  pff_open("TEST.WAV") 打开固定名 wav
 *   wav_test_03  解析 44 字节 RIFF 头
 *   wav_test_04  obuf 一次性喂整首 wav（首次出声）
 *   wav_test_05  5ms tick 流式喂数（长文件不欠喂）
 *   wav_test_06  PLAY/PREV/NEXT 按键控制 + 暂停
 *   wav_test_07  根目录扫描 *.WAV + 上/下曲循环
 *
 * 硬件配置（沿用 Phase 3 / 4）：
 *   SD0_MAPPING = SD0MAP_G3
 *   SDCMD = PE5, SDCLK = PE6, SDDAT0 = PE7
 *   模式：SDIO（内置 SD 控制器，libplatform.a）
 *   调试 UART：UART0 @ PB3 @ 115200 8N1
 *
 * 用法：
 *   - main.c 在 bsp_sys_init() 后调用 wav_test_run() 替代 fat_test_run()
 *   - 每个测试函数由 wav_test_run() 顺序调用，由 #if 1 / #if 0 闸门启用
 *   - 5ms 节拍钩子在 plugin.c 的 plugin_tmr5ms_isr() 里调用 wav_test_tick()
 *
 * 完成标志（wav_test_01 当前阶段）：
 *   串口看到 "[WAV] ====== BT892XA2 WAV Player Test ======" banner
 *   且 LED 每 ~500ms 翻转一次（证明 5ms tick 通路活着）
 */

void wav_test_run(void);
void wav_test_tick(void);

#endif // _WAV_TEST_H
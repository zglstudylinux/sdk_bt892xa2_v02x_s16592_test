/*
 * hello_world.c — Phase 2 准备任务实现
 *
 * 每 500ms 通过 SDK 的 5ms ISR (plugin_tmr5ms_isr) 累计时间并打印。
 * 调试串口配置由 config.h 的 HUART_EN 和 xcfg_cb.huart_sel 决定。
 */

#include "include.h"
#include "hello_world.h"

// 5ms tick counter，从 0 累加到 100 (= 500ms) 后清零并打印一次
#define HELLO_TICK_5MS_MAX     (100)   // 100 * 5ms = 500ms

static u16 s_tick_5ms_cnt = 0;          // 5ms 计数器
static u32 s_uptime_ms = 0;             // 累计运行时间 (ms)

/**
 * @brief 初始化：打印启动横幅
 *
 * 由 main.c 在 func_run() 之前调用一次。
 */
void hello_world_init(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  Hello BT892XA2!  SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  Build: %s %s\n", __DATE__, __TIME__);
    printf("  This is the hello_world test task.\n");
    printf("  See docs/快速入门.md section 2 for details.\n");
    printf("=========================================\n");
    printf("\n");
    s_tick_5ms_cnt = 0;
    s_uptime_ms = 0;
}

/**
 * @brief 5ms 节拍钩子
 *
 * 由 plugin/plugin.c 的 plugin_tmr5ms_isr() 调用。每 5ms 调用一次。
 * 每 500ms 打印一次 uptime。
 */
void hello_world_tick(void)
{
    s_tick_5ms_cnt++;
    s_uptime_ms += 5;

    if (s_tick_5ms_cnt >= HELLO_TICK_5MS_MAX) {
        s_tick_5ms_cnt = 0;
        printf("Hello BT892XA2! uptime=%ums\n", s_uptime_ms);
    }
}

/**
 * @brief 打印系统状态摘要
 *
 * 可在主循环任意位置手动调用，也可由按键事件触发。
 */
void hello_world_print_status(void)
{
    printf("[hello_world] uptime=%ums, tick_5ms_cnt=%u, reset_cause=0x%x\n",
           s_uptime_ms, s_tick_5ms_cnt, LVDCON);
}
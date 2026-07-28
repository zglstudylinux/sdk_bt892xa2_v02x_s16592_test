#ifndef _HELLO_WORLD_H
#define _HELLO_WORLD_H

/*
 * hello_world 模块 — Phase 2 准备任务
 *
 * 目的：
 *   1. 验证 Code::Blocks + riscv32-elf-gcc 工具链可正常编译
 *   2. 验证 .dcf 能正常生成
 *   3. 验证 Downloader 烧录后 UART 串口能看到输出
 *
 * 用法：
 *   - main.c 在调用 func_run() 之前调用 hello_world_init()
 *   - plugin_tmr5ms_isr() 每 5ms 调用一次 hello_world_tick()
 *
 * 完成标志：
 *   - 串口每 500ms 看到一行 "Hello BT892XA2! uptime=Nms SDK=V020.0"
 *   - 烧录后芯片正常启动，能继续进入蓝牙/音乐等模式
 */

void hello_world_init(void);
void hello_world_tick(void);
void hello_world_print_status(void);

#endif // _HELLO_WORLD_H
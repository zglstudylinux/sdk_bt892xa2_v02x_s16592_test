# 08_SDK 分层架构 — HAL / OSAL / Driver / Application

> **本篇是教程系列的"全景图"**——前面 7 篇分别讲了**某一层**或**某条信号**，本篇把它们**装到一张分层架构图**里。
>
> **核心洞察**：BT892XA2 SDK **不是教科书式的 4 层架构**。它是一个**闭源 SDK（5 个 .a 静态库）+ 开放 BSP glue** 的实际工程形态。你**看不到** HAL/Driver 的 `.c` 源码，只能看到 `.h` API 头文件。
>
> 学完本篇后，**任何一段代码、任何一个信号、任何一个新功能**——你都能准确说出它在**哪一层**、**应该改哪一层**。

---

## 目录

- [0. 前置与本篇定位](#0-前置与本篇定位)
- [1. 教科书式分层](#1-教科书式分层)
- [2. 真实 BT892XA2 SDK 的分层](#2-真实-bt892xa2-sdk-的分层)
- [3. HAL 层深度](#3-hal-层深度)
- [4. OSAL 层深度](#4-osal-层深度)
- [5. Driver 层深度](#5-driver-层深度)
- [6. Application 层深度](#6-application-层深度)
- [7. 完整信号追踪示例](#7-完整信号追踪示例)
- [8. 在正确的层写代码](#8-在正确的层写代码)
- [9. 跨层调用完整示例](#9-跨层调用完整示例)
- [10. 5 个最常见的分层错误](#10-5-个最常见的分层错误)
- [11. 自测题](#11-自测题)
- [12. 与现有教程的边界](#12-与现有教程的边界)
- [13. 分层 cheat sheet](#13-分层-cheat-sheet)

---

## 0. 前置与本篇定位

### 0.1 与 [03_SDK架构与运行模型](03_SDK架构与运行模型.md) 的关系

[03](03_SDK架构与运行模型.md) 讲了 **5 ms tick + 消息队列 + WDT + func_run**——这是 SDK 的**运行模型**（即 Application 层里**一条线**的运行机制）。

本篇讲**整个 SDK 的分层**——从硬件到应用，所有层、所有线。

### 0.2 与 [02_BT892XA2芯片与外设地图](02_BT892XA2芯片与外设地图.md) 的关系

[02](02_BT892XA2芯片与外设地图.md) 讲了 **SFR / GPIO / ADC / blinky**——这是**硬件层**（L6）的细节。

本篇从硬件**向上**看，看 SFR 之上的 HAL / OSAL / Driver / Application 是怎么堆起来的。

### 0.3 读者画像

- **学过 C 语言**——会指针、结构体、函数指针
- **第一次接触嵌入式分层架构**——听过 HAL/OSAL 概念但没用过
- **能开 Code::Blocks 编译 SDK**——会跑 wav_test

### 0.4 本篇学完后能做

- ✅ 看到任意 `.c` 文件能说出"它在第 L 几层"
- ✅ 看到任意函数调用链能标注"跨了 L 几到 L 几"
- ✅ 想加新功能时，能用 §8 决策树定位到正确的层
- ✅ 调试时能用 §10 自查常见错误
- ✅ 能向同事 / AI 助手清楚地说"我在第 L 几层调错了"

---

## 1. 教科书式分层

> 在写代码前，先认识教科书里的 4 层是什么。

### 1.1 HAL (Hardware Abstraction Layer) — 硬件抽象层

**定义**：把**不同芯片**的**同一类外设**统一抽象为**同一组 API**。

教科书例子：
- **Linux** `file_operations`：把"打开文件"的语义统一给所有设备（disk / UART / network）
- **STM32 HAL 库**：`HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)`——同样的调用对 STM32F0/F4/H7 都生效
- **ESP-IDF driver**：`gpio_set_level(GPIO_NUM_5, 1)`——对 ESP32/ESP32-S3 都生效

**HAL 的关键属性**：
- **API 统一**：调用方不关心具体芯片
- **可替换**：底层换芯片，上层代码不用动
- **完整封装**：通常不暴露寄存器，只暴露语义（如 `set/get/on/off`）

### 1.2 OSAL (Operating System Abstraction Layer) — 操作系统抽象层

**定义**：把**不同 RTOS** 的**同一类原语**统一抽象。

教科书例子：
- **CMSIS-OS**：`osMutexWait / osMutexRelease` 在 FreeRTOS / RTX / ThreadX 上都一样调
- **POSIX**：把 Linux / macOS / Unix 的 pthread/mutex 统一
- **Linux VFS**：把不同文件系统（ext4 / FAT / NTFS）统一为 open/read/write

**OSAL 的关键原语**（典型 RTOS 都有）：
- 线程 / Task：`thread_create / thread_start / thread_join`
- 互斥锁：`mutex_lock / mutex_unlock`
- 信号量：`sem_wait / sem_post`
- 消息队列：`queue_send / queue_receive`
- 时基：`delay_ms / tick_get`

### 1.3 Driver — 驱动层

**定义**：**直接操作硬件寄存器**的代码层。

**Driver 与 HAL 的区别**（教科书语义）：
- **Driver 实现 HAL 接口**——`gpio_driver.c` 实现 `gpio_set() / gpio_get()`，但 `gpio_set()` 是 HAL 签名
- HAL 是**抽象**（声明），Driver 是**实现**（操作 SFR）
- 通常 Driver 在 `.c`，HAL 在 `.h`

教科书的 `drv_gpio.c` 大致长这样：

```c
// HAL header: api_gpio.h
void gpio_set(u8 pin, u8 level);

// Driver implementation: drv_gpio.c (教科书版本)
void gpio_set(u8 pin, u8 level) {
    if (level) {
        GPIOA->BSRR = (1u << pin);     // 写 1
    } else {
        GPIOA->BSRR = (1u << (pin + 16));  // 写 0
    }
}
```

### 1.4 Application — 应用层

**定义**：**业务逻辑**（播放音乐、显示温度、控制 LED），调用 HAL/OSAL 接口。

教科书的 `play_music.c`：

```c
#include "api_audio.h"   // HAL
#include "api_msg.h"     // OSAL

void play_thread(void *arg) {
    while (1) {
        msg_t m = queue_receive(audio_queue);
        if (m.id == MSG_PLAY) {
            audio_open("music.wav");
            while (!audio_is_done()) {
                audio_decode_frame();
                delay_ms(10);
            }
        }
    }
}
```

### 1.5 典型 4 层关系图（教科书版）

```
┌─────────────────────────────────────────────────┐
│ Application (业务逻辑)                           │
│   - play_thread / display_thread / input_loop   │
├─────────────────────────────────────────────────┤
│ OSAL (操作系统抽象)                              │
│   - msg_queue / mutex / sem / thread            │
├─────────────────────────────────────────────────┤
│ HAL (硬件抽象)                                   │
│   - gpio_set / uart_read / audio_decode         │
├─────────────────────────────────────────────────┤
│ Driver (驱动实现)                                │
│   - GPIOA->BSRR / USART1->DR / DAC->DHR         │
├─────────────────────────────────────────────────┤
│ Hardware (物理芯片)                              │
│   - 寄存器 / RAM / Flash / 外围                  │
└─────────────────────────────────────────────────┘
```

**调用规则**：上层 → 下层（严格向下）。同层不互相调用（避免循环依赖）。

---

## 2. 真实 BT892XA2 SDK 的分层

### 2.1 真实分层与教科书的差异（**必读**）

| 维度 | 教科书 | 实际 SDK |
|---|---|---|
| **HAL 实体可见？** | ✅ 看得到 `.c` | ❌ **在 `.a` 里看不到** |
| **Driver 可见？** | ✅ 看得到 `.c` | ❌ **在 `.a` 里看不到** |
| **OSAL 完整？** | ✅ mutex/sem/queue | ⚠️ **只有 5 函数** |
| **有 RTOS？** | 通常有 | ❌ **没有，单线程 super-loop** |
| **应用代码可见？** | ✅ | ✅ |
| **BSP 层（中间件）？** | 通常没有 | ✅ **40+ 文件**（教科书的"freelance"） |

**最反直觉的一点**：

> 本 SDK 的 HAL **不是"抽象"，而是"透明"**——把 SFR 直接暴露给应用层。

下一节用 6 层模型来精准描述实际 SDK。

### 2.2 真实 6 层模型（仓库实际）

```
┌──────────────────────────────────────────────────────────────┐
│ L1 应用层 (Application)                                       │
│   路径: app/projects/standard/{hello_world,wav_test,fat_test, │
│         tf_test}/*.c                                         │
│   文件: ~10 个 (.c + .h)                                     │
│   示例: wav_test.c (~640 行)                                 │
├──────────────────────────────────────────────────────────────┤
│ L2 框架层 (Framework)                                         │
│   路径: app/projects/standard/{port,message,plugin}/*.c       │
│         + app/platform/functions/func_*.c                    │
│   文件: ~12 port_*.c + ~10 msg_*.c + ~4 plugin + ~15 func_*  │
│   示例: func.c (~700 行), port_key.c (~250 行)               │
├──────────────────────────────────────────────────────────────┤
│ L3 BSP 层 (Board Support Package / 业务中间件)                │
│   路径: app/platform/bsp/bsp_*.c                             │
│   文件: ~40+ 个 .c                                           │
│   示例: bsp_key.c (~990 行), bsp_sys.c (~520 行)             │
├──────────────────────────────────────────────────────────────┤
│ L4 API 层 (Application Programming Interface)                │
│   路径: app/platform/libs/api_*.h                            │
│   文件: 28 个 .h (api_gpio / api_uart / api_dac / ...)       │
│   示例: api_msg.h (5 函数), api_saradc.h (4 函数)            │
├──────────────────────────────────────────────────────────────┤
│ L5 闭源库层 (Pre-compiled Static Libraries)                   │
│   路径: libplatform.a / libdrivers.a / libbtstack.a          │
│         / libcodecs.a / libvoices.a                          │
│   链接: app.cbp:48-52                                        │
│   内容: 不可见 — Driver/HAL/codec/BT/voices 实体               │
├──────────────────────────────────────────────────────────────┤
│ L6 硬件层 (Hardware)                                          │
│   路径: app/platform/header/sfr.h, app/projects/standard/ram.ld│
│   内容: SFR (12 SFR 段 0x000-0xC00) + RAM 区段                  │
│         + aram (Audio RAM, 16 KB @ 0x50000)                  │
└──────────────────────────────────────────────────────────────┘
```

### 2.3 每层职责表

| 层 | 做什么 | 不做什么 | 谁调用它 |
|---|---|---|---|
| **L1 应用** | 业务逻辑（播放/按键/打印） | 不写 SFR / 不调 `delay_5ms > 1` | 用户 / 测试 |
| **L2 框架** | 板级 HAL（pin mux）+ 模式状态机 + 消息分发 | 不实现 driver / 不调 SFR | L1 |
| **L3 BSP** | 跨接 driver 与 application 中间件（bsp_key 把 SARADC + GPIO 装成"按键事件"） | 不做模式 dispatch | L2 / L1 |
| **L4 API** | SDK 公开头，ABI 契约 | 不实现（实现是 L5） | L3 / L2 |
| **L5 闭源库** | Driver/HAL/codec/BT/voices 实体 | 不暴露给用户修改 | L4（链接时） |
| **L6 硬件** | 物理 SFR / RAM / 引脚 | 不解释语义 | L5（直接读写） |

### 2.4 5 个 .a 库的对应关系

| .a 库 | 包含内容 | 对应层 | 链接顺序（cbp） |
|---|---|---|---|
| `libplatform.a` | OSAL + GPIO/UART/sys HAL | L4 + L5 上半 | 第 1（最前）|
| `libdrivers.a` | SARADC/SD/SDADC/TKEY/SPI-flash/RTC/FM | L5 下半 | 第 4 |
| `libbtstack.a` | BT profiles (HCI/L2CAP/A2DP/AVRCP/SPP/GATT) + BT-specific helpers | L5 (BT specific) | 第 2 |
| `libcodecs.a` | HW codecs (SBC/AAC/MSBC) | L5 (codec) | 第 3 |
| `libvoices.a` | voice prompt / TTS / ASR 资源 | L5 (resource) | 第 5 |

**链接顺序硬性要求**（详见 [01_工具链与构建产物](01_工具链与构建产物.md)）：
- `libplatform.a` 必须**第一个**（因为它定义了所有 OSAL 符号）
- 其他库顺序相对灵活，但**闭源库之前要把开放代码 link 完**

### 2.5 仓库里没见过的层（澄清）

**❌ 没有** `cpu/drv/drv_gpio.c` —— GPIO 的 Driver 在 `libplatform.a` 里。

**❌ 没有** `cpu/os/os_msg.c` —— msg queue 在 `libplatform.a` 里。

**❌ 没有** RTOS 源码 —— `app/platform/libs/api_msg.h` 是**仅有的 OSAL 接口**。

**❌ 没有** linker helper 暴露 —— `AT(.section_name)` 是 Bluetrum 自定义宏，**看不到**实现。

**❌ 没有** HAL 抽象层（教科书意义）—— HAL 头文件**直接暴露 SFR**，不是教科书意义的"抽象"。

---

## 3. HAL 层深度

### 3.1 本 SDK 的 HAL 是"透明的 HAL"

教科书 HAL 把 SFR 包成 `gpio_set(pin, level)`。本 SDK 的 HAL 把 SFR **直接给应用层**：

```c
// api_gpio.h 暴露给应用层（L1-L3）
typedef struct {
    psfr_t sfr;      // ← SFR 寄存器数组指针（12 个）
    u8 num;          // ← GPIO 编号
    u8 type;         // ← type=0 普通 IO, type=1 高压 IO
} gpio_t;
```

应用层拿到 `gpio_t` 后，**直接 `gpio.sfr[GPIOxSET] = 1` 写 SFR**——**没有 `gpio_set()` 函数**。

### 3.2 api_gpio.h 完整解剖

完整 `app/platform/libs/api_gpio.h`（38 行）：

```c
enum {
    GPIOxSET    =   0,    // 置位寄存器（写 1 = 拉高）
    GPIOxCLR,             // 清位寄存器（写 1 = 拉低）
    GPIOx,                // 数据寄存器
    GPIOxDIR,             // 方向寄存器（0=IN, 1=OUT）
    GPIOxDE,              // 输入使能（0=disable, 1=enable）
    GPIOxFEN,             // 滤波使能
    GPIOxDRV,             // 驱动强度
    GPIOxPU,              // 上拉使能
    GPIOxPD,              // 下拉使能
    GPIOxPU200K,          // 200K 上拉
    GPIOxPD200K,          // 200K 下拉
    GPIOxPU300,           // 300R 上拉（强）
    GPIOxPD300,           // 300R 下拉（强）
};

typedef struct {
    psfr_t sfr;
    u8 num;
    u8 type;
} gpio_t;

void gpio_cfg_init(gpio_t *g, u8 io_num);     // 初始化
void wakeup_disable(void);
u8 get_adc_gpio_num(u8 adc_ch);
void wakeup_gpio_config(u8 io_num, u8 edge, u8 pupd_sel);
void wakeup_wko_config(void);
void wakeup_wko_hlevel_config(void);
u32 wakeup_get_status(void);
void adcch_io_pu10k_enable(u8 adc_ch);        // 打开 SARADC 通道的 10K 上拉
void adcch_io_pd10k_enable(u8 adc_ch);
```

### 3.3 写 GPIO 示例（透明 HAL 的实际效果）

```c
// 教科书写法：HAL 抽象
gpio_set(IO_PD0, 1);             // 调用 HAL 函数

// 本 SDK 写法：透明 HAL，直接 SFR poke
gpio_t g;
bsp_gpio_cfg_init(&g, IO_PD0);   // 初始化
g.sfr[GPIOxDE] = 1;              // 输入使能（必须先开）
g.sfr[GPIOxDIR] = 1;             // 设输出（0=IN, 1=OUT）
g.sfr[GPIOxSET] = 1;             // 拉高
// 或：
g.sfr[GPIOxCLR] = 1;             // 拉低
```

### 3.4 HAL 边界的不一致（4 种风格）

| HAL | 接口风格 | 调用示例 |
|---|---|---|
| **GPIO** | 透明（暴露 SFR） | `gpio.sfr[GPIOxSET] = 1` |
| **UART** | 阻塞 | `uart_read(buf, len, 100)` 阻塞到收到 N 字节 |
| **I2C** | 阻塞 + DMA | `i2c_read(addr, buf, len, 0)` |
| **SARADC** | 纯轮询 | `saradc_kick_start(); while(!is_saradc_convert_finish());` |

**为什么不一致？** 因为**HAL 头只是闭源库的 ABI 表面**——它们可以暴露任何风格，取决于底层 Driver 怎么写。

### 3.5 HAL 与 SFR 的关系图

```
        Application Layer (L1)
            ↓ 调用 api_gpio.h 的 gpio_cfg_init
        HAL Header Layer (L4)
            ↓ 包含 psfr_t sfr[12] 暴露
        Driver Library (L5, libplatform.a)
            ↓ 实现 GPIO SFR 操作
        Hardware SFR (L6, sfr.h)
            ↓ 最终 poke 寄存器
        Physical Pin (PD0/PD1/...)
```

**关键洞察**：L4 → L6 是**直接穿透**的，没有教科书意义的"中间层"。

---

## 4. OSAL 层深度

### 4.1 5 函数 OSAL 完整解剖

完整 `app/platform/libs/api_msg.h`（12 行）：

```c
#ifndef _API_MSG_H
#define _API_MSG_H

#define NO_MSG  0

void msg_queue_init(void);                          // 初始化队列
void msg_queue_clear(void);                         // 清空队列
void msg_enqueue(u16 msg);                         // 入队（ISR/应用层都能调）
u16  msg_dequeue(void);                             // 出队（返回 0 = 空）
void msg_queue_detach(u16 msg, u8 flag);            // flag=0 剔除 msg; flag=1 只保留 msg

#endif
```

**就这 5 个函数——这就是 SDK 的全部 OSAL**。

### 4.2 5 函数背后的完整 OSAL

虽然只有 5 个公开函数，但 SDK 还有**时基 / 中断 / WDT** 三个补充（散落在其他 .h）：

| 类别 | 函数 / 宏 | 头 |
|---|---|---|
| **时基** | `delay_5ms(u8 n)` / `delay_ms(u16 n)` | `api_sys.h` |
| | `tick_get(void)` / `tick_check_expire(tick, ms)` | `api_sys.h` |
| **中断开关** | `IRQ_ENABLE() / IRQ_DISABLE()` | `macro.h` |
| **WDT** | `WDT_CLR() / WDT_EN() / WDT_DIS() / WDT_RST()` | `macro.h` |
| **消息** | 上面 5 个 | `api_msg.h` |

### 4.3 没有的 OSAL 原语

| 原语 | 状态 | workaround |
|---|---|---|
| `mutex_lock / unlock` | ❌ | 关闭全局中断（IRQ_DISABLE） |
| `sem_wait / post` | ❌ | 计数器 + msg 模拟 |
| `event_group` | ❌ | 16-bit msg 字段（高位） |
| `thread_create` | ❌ | 没有线程——单线程 super-loop |
| `thread_priority` | ❌ | 没有线程——所有任务优先级等价 |
| `queue`（多消息队列） | ⚠️ | 只有 1 个共享 16-bit msg 队列 |

### 4.4 消息队列协议

**16-bit 编码**（详见 [04_按键系统完全指南 §5](04_按键系统完全指南.md)）：

```c
// 短按消息
KU_PLAY        = KEY_PLAY | 0x800       // = 0x820
KU_NEXT        = KEY_NEXT | 0x800       // = 0x80D
// 长按消息
KL_PLAY        = KEY_PLAY | 0xA00       // = 0xA20
// 持续消息
KH_VOL_UP      = KEY_VOL_UP | 0xE00     // = 0xE0B
// 系统事件
MSG_SYS_SDCARD_REMOVE = 0x07FE          // 高 8 位 = 0x07 表示系统
MSG_SYS_SDCARD_INSERT = 0x07FD
```

**发送**：`msg_enqueue(KU_PLAY)` 在 ISR / 应用层都能调。

**接收**：`msg_dequeue()` 返回 0 = 队列空（非阻塞！）。应用层必须自己 polling。

### 4.5 OSAL 调用模型（super-loop）

```
ISR (5ms)                        Application Loop
─────────                        ────────────────
tmr1_isr (HW)                    while (1) {
  ↓                                msg = msg_dequeue();
usr_tmr5ms_thread (bsp_sys.c)      if (msg) {
  ↓                                  switch (msg & 0xFF00) {
bsp_key_scan()                       case KU: handle_key(msg); break;
  ↓                                  case KL: handle_long(msg); break;
msg_enqueue(KU_PLAY)                 case MSG_SYS: handle_sys(msg); break;
  ↓                                }
return to ISR                      }
                                  delay_5ms(1);    // 让出 CPU
                                  WDT_CLR();       // 喂狗
                                }
```

### 4.6 对比 FreeRTOS 的 workaround

| FreeRTOS 原语 | 本 SDK workaround |
|---|---|
| `xSemaphoreCreateBinary` + `xSemaphoreTake` | 一个 `u8` 计数器 + `msg_enqueue` |
| `xMutexCreate` + `xMutexTake` | `IRQ_DISABLE / IRQ_ENABLE`（关全局中断，2-3 行临界区） |
| `xEventGroupSetBits` | `u32` 共享变量 + `msg` 携带 |
| `xQueueSendToBack` (多队列) | 全部塞进 1 个共享 msg 队列，按 msg id 分发 |
| `vTaskDelay(10)` | `tick_check_expire(start, 10)` 异步等 |

---

## 5. Driver 层深度

### 5.1 Driver 在哪？

| 层级 | 实体 | 文件 | 可见？ |
|---|---|---|---|
| **真实 Driver** | 寄存器读写实现 | libdrivers.a / libplatform.a | ❌ |
| **Driver 头** | API 签名 | api_*.h (28 个) | ✅ |
| **Driver 用户** | 调用 Driver | bsp_*.c / port_*.c | ✅ |

### 5.2 典型 Driver 接口模式（4 种）

#### 5.2.1 纯轮询（Polling）— SAR ADC

```c
// api_saradc.h
void saradc_init(u8 channel);
void saradc_kick_start(void);
bool is_saradc_convert_finish(void);
u16  saradc_get_value(void);
void saradc_exit(void);
```

调用流程：`init → kick_start → while(!is_finish) → get_value → exit`。**没有中断、没有 DMA**。

#### 5.2.2 回调（Callback）— UART

```c
// api_uart.h
typedef void (*uart_rx_cb_t)(u8 byte);

void uart_init(u8 port, u32 baud, uart_rx_cb_t cb);  // cb 在 RX ISR 中调
void uart_tx(u8 port, u8 *buf, u16 len);
u16  uart_rx_available(u8 port);
```

调用流程：注册 cb → ISR 自动调 cb → 应用层读 buffer。

#### 5.2.3 DMA（Direct Memory Access）— Audio

```c
// api_audio.h
int audio_path_push(s16 *samples, u16 n_samples);   // 推到 DMA buffer
int audio_path_pop(s16 *samples, u16 n_samples);    // 从 DMA buffer 取
```

调用流程：DMA 自动搬运，CPU 不用管。

#### 5.2.4 状态机（State Machine）— Bluetooth

```c
// api_bt.h
typedef enum { BT_IDLE, BT_SCANNING, BT_CONNECTED, BT_A2DP_PLAYING } bt_state_t;

bt_state_t bt_get_state(void);
int bt_connect(u8 *mac_addr);
int bt_send(u8 *data, u16 len);
```

调用流程：调用返回 0/-1；状态变化通过回调或轮询拿到。

### 5.3 SAR ADC Driver 完整解剖（作为例子）

#### 5.3.1 头文件（`api_saradc.h`）—— L4

```c
void saradc_init(u8 channel);
void saradc_kick_start(void);
bool is_saradc_convert_finish(void);
u16  saradc_get_value(void);
void saradc_exit(void);
```

#### 5.3.2 BSP 调用（`bsp_key.c:27-60`）—— L3

```c
// bsp_saradc_init
void bsp_saradc_init(void) {
    saradc_init(ADCCH_WKO);                // ← 调 Driver L5
    saradc_kick_start();                   // ← 启动一次
}
```

#### 5.3.3 BSP 业务（`bsp_key.c:65-72`）—— L3

```c
u16 get_adc_val(void) {
    saradc_kick_start();
    while (!is_saradc_convert_finish()) {  // ← busy-wait（轮询 Driver）
        ;  // 5ms ISR 已 enabled，CPU 在这里被打断没事
    }
    return saradc_get_value();
}
```

**没有任何 HAL 层在中间**——`bsp_*.c` 直接调 Driver API。

### 5.4 Driver vs HAL 的真正区别（本 SDK 语境）

| 教科书语义 | 本 SDK 实际语义 |
|---|---|
| Driver：原子操作 SFR | Driver：单原子 API（`saradc_init / kick / is_finish`） |
| HAL：抽象打开/关闭/配置 | HAL：几乎不存在；GPIO HAL 把 SFR 暴露给应用 |
| **所以 HAL 层被合并**——Driver 直接服务 BSP，应用层直跳到 HAL header | |

**结果**：本 SDK 没有教科书的"HAL 抽象层"概念。**HAL header 只是闭源库的 ABI 表面**。

---

## 6. Application 层深度

### 6.1 应用层三部分

```
A. 主应用（L1）
   └── app/projects/standard/{hello_world,wav_test,fat_test,tf_test}/*.c
       (业务逻辑: 播放/按键响应/打印)

B. 框架（L2）
   └── app/platform/functions/func_*.c (15 个 func_ 模式)
       (每个 func_*.c 是一个 "mode" — BT / Music / Aux / FM / Clock / ...)

C. 模式 dispatch（L2）
   └── app/platform/functions/func.c
       (用 func_cb.sta enum 切 mode)
```

### 6.2 func_run() 完整剖析（`func.c:672-775`）

```c
// app/platform/functions/func.c:672
AT(.text.func)
void func_run(void)
{
    printf("%s\n", __func__);

    func_bt_chk_off();
    while (1) {
        func_enter();
        switch (func_cb.sta) {
        #if FUNC_MUSIC_EN
            case FUNC_MUSIC:
                func_music();
                break;
        #endif

        #if FUNC_BT_EN
            case FUNC_BT:
                func_bt();
                break;
        #endif

        // ... 十几个 case，每个调 func_xxx()

        default:
            func_idle();
            break;
        }

        msg = msg_dequeue();           // ← OSAL 出队
        if (msg) func_message(msg);    // ← 派发到当前模式
        WDT_CLR();                     // ← OSAL 喂狗
    }
}
```

**完整剖析**：

| 行 | 在哪层 | 做什么 |
|---|---|---|
| `func_run()` 入口 | L2 | 启动应用层主循环 |
| `func_enter()` | L2 | 模式切换时的钩子（可选） |
| `switch (func_cb.sta)` | L2 | 切到当前 mode |
| `func_music() / func_bt()` | L2 | 当前 mode 的主处理 |
| `msg_dequeue()` | L5 (libplatform.a) | OSAL 出队 |
| `func_message(msg)` | L2 | 派发消息到当前 mode |
| `WDT_CLR()` | L5 (libplatform.a) | OSAL 喂狗 |

### 6.3 func_message() 完整剖析（`func.c:202-630`）

```c
// app/platform/functions/func.c:202
void func_message(u16 msg)
{
    switch (msg) {
        // ... 几百行 case，处理各种消息
    }
}
```

**关键**：每个 mode 都有自己的 `func_xxx_message()` 子函数：

```c
// func_music.c 风格
void func_music_message(u16 msg) {
    switch (msg) {
        case KU_PLAY:        // 短按 P/P
            // 暂停/恢复
            break;
        case KU_NEXT:        // 短按 NEXT
            // 切下一首
            break;
        case MSG_SYS_SDCARD_REMOVE:  // SD 卡拔出
            // 切到 idle
            break;
    }
}
```

### 6.4 应用层原则（必读）

**原则 1**：永远**不直接写 SFR**——用 `bsp_gpio_cfg_init + sfr[...]` 或 driver API。

**原则 2**：永远**不调 `delay_5ms(N > 1)`**——`delay_5ms(1)` 是让出 CPU 的标准做法；`delay_5ms(10)` 会让 10×5ms 内的 ISR 全部丢失响应。

**原则 3**：永远**不阻塞 `msg_dequeue`**——它是非阻塞的，返回 0 = 队列空。

**原则 4**：永远**不在 ISR context 调 printf**——`usr_tmr5ms_thread` 是 ISR context。

**原则 5**：永远**调 `WDT_CLR()` 在主循环里**——除了 `func_run()` 内部自动清，**自写 main loop 必须自己清**。

### 6.5 自写 main loop 的正确框架

```c
// 错的写法
void my_test_run(void) {
    while (1) {
        do_something();
        delay_5ms(100);    // ← 错：100×5ms 内 ISR 堆
        do_something_else();
    }
}

// 正确的写法
void my_test_run(void) {
    u32 last_wdt = tick_get();
    while (1) {
        do_something();

        msg = msg_dequeue();
        if (msg) handle(msg);

        // 每 100ms 清一次 WDT
        if (tick_check_expire(last_wdt, 100)) {
            WDT_CLR();
            last_wdt = tick_get();
        }

        delay_5ms(1);      // ← 让出 CPU
    }
}
```

---

## 7. 完整信号追踪示例

> 这是本篇**最有价值的一章**——通过 4 个真实信号，逐层追踪，每个例子标注文件、行号、层、是否可见。

### 7.1 信号 1：按键按下 → 暂停 / 恢复（最经典）

**触发**：用户按下 S5 P/P 键。
**目标**：`wav_test.c` 的 `s_paused` 状态翻转。
**预期耗时**：5-15 ms（取决于 ISR 是否被多次打断）。

#### 完整链路（15 步）

| 步 | 文件 : 行 | 层 | 是否可见 | 做什么 |
|---|---|---|---|---|
| 1 | (硬件) PB5 ADC 电压变化 | L6 | — | 物理信号 |
| 2 | `libdrivers.a` SAR ADC | L5 | ❌ | ADC 转换完成，硬件 ISR |
| 3 | (CPU) TIMER1 overflow | L6 | — | 5 ms 到 |
| 4 | `libplatform.a` `timer1_isr` | L5 | ❌ | 硬件 ISR 入口 |
| 5 | `bsp_sys.c:341` `usr_tmr5ms_thread` | L3 | ✅ | 5 ms ISR 主线程 |
| 6 | `bsp_sys.c:347` `bsp_key_scan()` | L3 | ✅ | 调按键扫描 |
| 7 | `bsp_key.c:531` `bsp_key_scan_do()` | L3 | ✅ | 读 ADC + 防抖 |
| 8 | `libdrivers.a` SARADC API | L5 | ❌ | `saradc_kick_start + is_finish` busy-wait |
| 9 | `bsp_key.c:902` `bsp_key_scan_do()` 内部 | L3 | ✅ | adkey_get_key_num() 查表 |
| 10 | `bsp_key.c` 内部 | L3 | ✅ | key_num_to_msg() 组装 KU_PLAY |
| 11 | `libplatform.a` `msg_enqueue` | L5 | ❌ | 入队 |
| 12 | (CPU) 5 ms ISR 退出 | — | — | — |
| 13 | `app.c:46` `wav_test_run()` 循环 | L1 | ✅ | msg_dequeue() |
| 14 | `libplatform.a` `msg_dequeue` | L5 | ❌ | 出队，返回 KU_PLAY |
| 15 | `wav_test.c:200` `wav_test_handle_key(msg)` | L1 | ✅ | switch KU_PLAY → `s_paused = !s_paused` |

#### 链路图（垂直）

```
Hardware: PB5 ADC change
   ↓ (物理层)
[libdrivers.a SARADC ISR]                          L5 ❌
   ↓
[CPU timer1_isr (HW ISR)]                          L5 ❌
   ↓
bsp_sys.c:341 usr_tmr5ms_thread()                  L3 ✅
   ↓
bsp_sys.c:347 bsp_key_scan()                       L3 ✅
   ↓
bsp_key.c:531 bsp_key_scan_do()                    L3 ✅
   ↓
[libdrivers.a saradc_kick/is_finish busy-wait]     L5 ❌
   ↓
bsp_key.c:836 adkey_get_key_num()                  L3 ✅
   ↓
bsp_key.c:797 key_num_to_msg()                     L3 ✅
   ↓
[libplatform.a msg_enqueue(KU_PLAY)]               L5 ❌
   ↓
(5ms ISR 退出)
   ↓
wav_test.c main loop: msg_dequeue()                L1 ✅
   ↓
[libplatform.a msg_dequeue()]                      L5 ❌
   ↓
wav_test.c:200 wav_test_handle_key(KU_PLAY)        L1 ✅
   ↓
wav_test.c:208 s_paused = !s_paused                L1 ✅
```

### 7.2 信号 2：wav_test 读 SD 卡扇区

**触发**：`wav_test.c` 调用 `pff_read`。
**目标**：从 SD 卡读一个 512 字节扇区到 RAM。

#### 完整链路（10 步）

| 步 | 文件 : 行 | 层 | 是否可见 | 做什么 |
|---|---|---|---|---|
| 1 | `wav_test.c:196` `pff_read(buf, 512, &br)` | L1 | ✅ | 应用层发起读 |
| 2 | `pff.c::pf_read()` | L5 | ✅ (ChaN 开源) | Petit FatFs 读 |
| 3 | `pff.c::disk_readp()` | L5 | ✅ (ChaN) | 调 disk 桥接 |
| 4 | `pff_compat.h` 宏重命名 | L5 | ✅ | `disk_readp` → `pff_disk_readp` |
| 5 | `diskio.c:38` `pff_disk_readp(buf, lba, ofs, n)` | L1/L3 桥 | ✅ | partial-sector cache 处理 |
| 6 | `app.c` → `api_sd.h` 声明 | L4 | ✅ | `sd0_read(buf, lba, 1)` |
| 7 | `libdrivers.a` SDIO API | L5 | ❌ | 实际扇区读 |
| 8 | (硬件) SDIO 控制器 | L6 | — | SD 卡物理读 |
| 9 | (回) `diskio.c` cache 合并 | L1/L3 桥 | ✅ | 拼装 partial sector |
| 10 | (回) `wav_test.c` 用 `br` 字节 | L1 | ✅ | 完成读 |

#### 链路图（垂直）

```
wav_test.c:196 pff_read(buf, 512, &br)             L1
   ↓
pff.c::pf_read() (ChaN open source)                L5
   ↓
pff.c::disk_readp() (ChaN)                         L5
   ↓
pff_compat.h macro (disk_readp → pff_disk_readp)   L4-L5 桥
   ↓
diskio.c:38 pff_disk_readp(buf, lba, ofs, n)       L1 (应用层) / L3 (BSP)
   ↓ (partial-sector cache 处理)
api_sd.h: sd0_read(buf, lba, 1)                    L4 声明
   ↓
[libdrivers.a sd0_read actual]                     L5
   ↓
SDIO Controller                                     L6
   ↓
SD card physical read                               L6
   ↓ (data back)
[cache merge if partial]                           L3
   ↓
wav_test.c: br=512 bytes used                      L1
```

### 7.3 信号 3：串口 printf 出来

**触发**：`app.c:31` `printf("Hello BT892XA2: %x\n", lvdcon)`。
**目标**：字符串出现在 PB3 串口。

#### 完整链路（6 步）

| 步 | 文件 : 行 | 层 | 是否可见 | 做什么 |
|---|---|---|---|---|
| 1 | `app.c:31` `printf(...)` | L1 | ✅ | C 标准库 printf |
| 2 | `libplatform.a` `__wrap_printf` | L5 | ❌ | 重定向到 UART（不是 stdout） |
| 3 | `libplatform.a` `vfprintf` 内部 | L5 | ❌ | 格式化 |
| 4 | `api_uart.h` `uart_tx()` | L4 | ✅ | 串口发送 API |
| 5 | `libplatform.a` UART0 SFR | L5 | ❌ | 写 UART0->DR |
| 6 | (硬件) PB3 引脚 | L6 | — | 物理信号 |

#### 链路图（垂直）

```
app.c:31 printf("Hello BT892XA2: %x\n", lvdcon)   L1
   ↓
[libplatform.a __wrap_printf / vfprintf]          L5
   ↓
api_uart.h: uart_tx(buf, len)                     L4
   ↓
[libplatform.a UART0 SFR poke]                    L5
   ↓
PB3 引脚（取决于 UART0_PRINTF_SEL config）        L6
   ↓ (物理信号)
PC 串口助手显示字符串
```

### 7.4 信号 4：开机 banner 流程

**触发**：板子上电。
**目标**：从 boot 到 wav_test 跑起来。

#### 完整链路（`bsp_sys_init`）

```c
// bsp_sys.c:1080 bsp_sys_init() (L3)
void bsp_sys_init(void) {
    // 步骤 1-3: 参数 + 基础硬件
    xcfg_init();                // L3 — 读 xcfg.bin
    bsp_io_init();              // L3 — IO mux 初始化
    bsp_var_init();             // L3 — 清 BSS

    // 步骤 4-7: 时钟
    pmu_init();                 // L4-L5 — PMU 初始化
    adpll_init();               // L4-L5 — PLL
    set_sys_clk();              // L4-L5 — 系统时钟
    rtc_init();                 // L4-L5 — RTC
    param_init();               // L3 — 参数
    xosc_init();                // L4-L5 — 外部晶振

    // 步骤 8-10: 子模块（仍 poll-only）
    plugin_init();              // L3 — 插件
    led_init();                 // L3 — LED
    key_init();                 // L3 — 按键
    gui_init();                 // L3 — GUI
    gsensor_init();             // L3 — G-sensor

    // ⚠️ 步骤 11: 中断开
    sys_set_tmr_enable(1, 1);   // L4-L5 — 5ms ISR enable（之前全 poll）

    // 步骤 12-14: 通信
    bt_init();                  // L4-L5 — Bluetooth
    dac_init();                 // L4-L5 — DAC
    func_bt_init();             // L2 — BT 模式初始化

    // 步骤 15: 应用入口
    app_init_do();              // L2 — 决定 func_cb.sta
    // (本仓库) wav_test_run() 直接跑，跳过 func_run()
}
```

#### 关键节点

| 步骤 | 行 | 层 | 关键 |
|---|---|---|---|
| `xcfg_init()` | ~1080 | L3 | 读 `xcfg.bin` 配置 |
| `sys_set_tmr_enable(1,1)` | 中段 | L5 | **中断 enable**（之前全 poll） |
| `app_init_do()` | 末尾 | L2 | **决定 `func_cb.sta`** |

**步骤 11 之后**所有事都通过 msg 队列异步发生；步骤 11 之前全是同步 poll。

---

## 8. 在正确的层写代码

> 给零基础读者的"实战指引"：我想加新功能 X，应该改哪层？

### 8.1 决策树

```
想加 ...
│
├── 新 LED 灯模式
│   ├── 简单闪烁：port_led.c [L2] + 调 bsp_led_set_on() [L3]
│   └── 复杂呼吸：app/platform/bsp/bsp_led.c [L3] + api_pwm.h [L4]
│
├── 新按键动作
│   ├── 改表：port_key.c pwrkey_table[] [L2]
│   └── 改分发：func.c:202 func_message() switch case [L2]
│
├── 新外设（I2C 加个传感器）
│   ├── 调 API：调用 api_i2c.h [L4]
│   ├── BSP 封装：新建 app/platform/bsp/bsp_my_sensor.c [L3]
│   └── 应用：app/projects/standard/foo/foo.c [L1]
│
├── 新 SD 文件格式
│   ├── 不改 driver：直接用 pff_* [L4-L5]
│   ├── 应用层 parser：app/projects/standard/foo/foo.c [L1]
│
├── 新 func_ 模式（例如 FM 收音机）
│   ├── 5 步：(1) enum 加 (2) func.c switch 加 (3) 写 func_fm.c (4) 加 msg_fm.c (5) display icon
│   └── 详见 [03 §10](03_SDK架构与运行模型.md)
│
└── 新 OSAL 原语
    └── 改不了——闭源。如果你需要 mutex，考虑关 IRQ
```

### 8.2 每层改动的可见性

| 层 | 改完能看到的 | 改错的风险 | 难度 |
|---|---|---|---|
| **L1 应用** | main.c / wav_test.c 的输出 | 编译报错立刻可见 | 🟢 低 |
| **L2 框架** | func.c / port_*.c | 影响所有 mode | 🟡 中 |
| **L3 BSP** | 链接报错 / 运行时崩 | 中等 | 🟡 中 |
| **L4 API** | 看不到（闭源） | 仅能改 .h | 🟠 高 |
| **L5 闭源库** | 完全看不到 | 不要试图改 | 🔴 不可改 |
| **L6 硬件** | 烧录后表现 | 高（烧片风险） | 🔴 极高 |

### 8.3 好的代码 vs 错的代码对比（10 个例子）

| 任务 | ❌ 错的写法（在错的层） | ✅ 正确的写法 |
|---|---|---|
| 让 PD0 输出高 | `*(volatile u32*)0x50001000 = 1;` | `gpio_t g; bsp_gpio_cfg_init(&g, IO_PD0); g.sfr[GPIOxSET] = 1;` |
| 5ms 等待 | `for (volatile int i=0; i<10000; i++);` | `delay_5ms(1);` |
| 发按键消息 | `func_cb.sta = FUNC_BT;` (直接改 enum) | `msg_enqueue(KU_PLAY);` 然后 `func_message()` 处理 |
| 读 ADC | 直接读 SARADC SFR | `saradc_init(...); saradc_kick_start(...); while(!is_saradc_convert_finish());` |
| 阻塞等 msg | `while (!msg_dequeue());` | `msg = msg_dequeue(); if (msg) handle(msg);` |
| 在 ISR printf | `usr_tmr5ms_thread(){ printf(...); }` | 只设 flag，主循环读 flag 再 printf |
| 跨 mode 共享数据 | 全局 static 变量 | 用 `func_cb.xxx` 或 `sys_cb.xxx` |
| 加 RAM buffer | 栈变量 `u8 buf[8192];` | `static u8 buf[8192] AT(.aram);` |
| 改 WDT 周期 | 直接 `WDTCON = 0x55;` | `WDT_DIS(); delay_ms(...); WDT_EN();` |
| 改应用 init 顺序 | 改 `main.c` | 改 `app_init_do()` 在 `bsp_sys_init()` 末尾 |

---

## 9. 跨层调用完整示例

> 这是新手最该做的练习——从 0 写 1 个新功能。

### 9.1 例 1：加一个 LED 灯在 PB5 上常亮

**目标**：PB5 输出高，常亮。
**改动层**：L2-L3 (~30 行代码)
**前置**：已有的 `port_led.c` 模板

#### 完整步骤

**步骤 1**：打开 `app/projects/standard/port/port_led.c`，找到 `led_func_init()`。

**步骤 2**：加新的 LED 定义：

```c
// port_led.c 顶部加
gpio_t my_led_gpio AT(.buf.led);

// port_led.c 末尾的 led_func_init() 加
void led_func_init(void) {
    // ... 已有初始化
    bsp_gpio_cfg_init(&my_led_gpio, IO_PB5);  // 新增：初始化 PB5
    my_led_gpio.sfr[GPIOxDE] = 1;             // 输入使能
    my_led_gpio.sfr[GPIOxDIR] = 1;            // 设输出
    my_led_gpio.sfr[GPIOxSET] = 1;            // ← 拉高（常亮）
}
```

**步骤 3**：在 `wav_test.c` 的某个 hook（比如 `wav_test_open_and_inspect()` 末尾）调：

```c
// wav_test.c
extern gpio_t my_led_gpio;   // 引用 port_led.c 的全局

void wav_test_open_and_inspect(void) {
    // ... 已有代码
    my_led_gpio.sfr[GPIOxSET] = 1;    // ← 触发
}
```

#### 层走查

```
wav_test.c (L1) → my_led_gpio.sfr[GPIOxSET] = 1
  ↓
port_led.c (L2-L3) → gpio_t my_led_gpio
  ↓
api_gpio.h (L4) → sfr[] 数组索引
  ↓
libplatform.a (L5) → 实际 SFR 写
  ↓
PB5 引脚 (L6) → 物理高电平
```

**没有改 .a，没有改 sfr.h，没有改 ram.ld**——纯 L1-L3 改动，编译烧录即可。

### 9.2 例 2：按键触发切换 LED 状态

**目标**：按 P/P 时切换 PB5 灯的开关。
**改动层**：L1 + L2 (~50 行代码)

#### 完整步骤

**步骤 1**：接 §9.1，加 `my_led_off()`：

```c
// port_led.c
void my_led_off(void) {
    my_led_gpio.sfr[GPIOxCLR] = 1;    // 拉低
}

void my_led_toggle(void) {
    if (my_led_gpio.sfr[GPIOx] & (1 << my_led_gpio.num)) {
        my_led_off();
    } else {
        my_led_on();
    }
}
```

**步骤 2**：在 `wav_test.c` 的 `wav_test_handle_key(msg)` 加 case：

```c
// wav_test.c
extern void my_led_toggle(void);

void wav_test_handle_key(u16 msg) {
    // ... 已有 switch
    switch (msg) {
        case KU_PLAY:                              // 短按 P/P
            s_paused = !s_paused;                  // 已有：暂停
            my_led_toggle();                       // 新增：切灯
            break;
        // ... 其他 case
    }
}
```

**步骤 3**：编译烧录验证。

#### 层走查

```
按键 → bsp_key.c (L3) → msg_enqueue(KU_PLAY) → wav_test.c (L1)
  ↓
wav_test_handle_key(KU_PLAY)
  ↓
my_led_toggle() in port_led.c (L2)
  ↓
my_led_gpio.sfr[GPIOxSET/CLR] = 1 (L4-L6)
```

### 9.3 例 3：读 SAR ADC 通道 3 显示电压

**目标**：每 5ms 读一次 ADC 通道 3，串口打印电压（mV）。
**改动层**：L1 + L3 (~80 行代码)

#### 完整步骤

**步骤 1**：在 `app/projects/standard/` 下新建 `adc_test/adc_test.c`：

```c
// app/projects/standard/adc_test/adc_test.c
#include "include.h"

AT(.text.adc_test)
void adc_test_run(void) {
    u32 last_print = 0;

    saradc_init(ADCCH_IO_PB3);              // L5 调 Driver

    while (1) {
        u16 raw = get_adc_val();            // bsp_key.c 的 helper 或自己写
        u32 mv = (u32)raw * 3300 / 1023;     // 10-bit ADC → mV

        msg = msg_dequeue();                // L5 OSAL
        if (msg) {
            // 过滤系统事件
            if (msg < 0x100 || (msg & 0xFF00) == 0x0700) continue;
            // 退出条件：长按 POWER
            if (msg == KL_PWR) return;
        }

        if (tick_check_expire(last_print, 200)) {  // 每 200ms 打印
            printf("[ADC3] %d mV (raw=0x%03X)\n", mv, raw);
            last_print = tick_get();
        }

        WDT_CLR();                          // L5 OSAL 喂狗
        delay_5ms(1);                       // L5 OSAL 让出
    }
}
```

**步骤 2**：在 `app.c` 加切换：

```c
// app/projects/standard/main.c
#include "adc_test/adc_test.h"

int main(void) {
    // ...
    bsp_sys_init();
    adc_test_run();    // [ADC_TEST]
    return 0;
}
```

**步骤 3**：在 `app.cbp` 加 `<Unit filename="adc_test/adc_test.c" />`。

#### ISR context 注意事项

`usr_tmr5ms_thread` 是 ISR context（虽然它是普通函数，但被硬件 ISR `timer1_isr` 调用）。**所以不能在里面 printf 长串**——会卡死串口。

#### 层走查

```
adc_test.c (L1) → saradc_init() → libdrivers.a (L5)
  ↓
get_adc_val() → busy-wait is_finish → saradc_get_value (L5)
  ↓
printf() → libplatform.a (L5) → PB3 (L6) → PC 串口
```

### 9.4 例 4：自己写一个 GPIO ISR 唤醒源

**目标**：配置 PB2 上升沿唤醒 deep-sleep。
**改动层**：L1 + L4 (~100 行代码)

#### 完整步骤

**步骤 1**：在 app.c 加：

```c
// app/projects/standard/main.c
int main(void) {
    bsp_sys_init();

    // 配置 PB2 上升沿唤醒（不开内部上下拉 = 0）
    wakeup_gpio_config(IO_PB2, 1, 1);
    // ↑ 参数: (io_num, edge=1 上升沿, pupd=1 上拉)

    // 进入 deep sleep (SDK 内部调 PMU)
    // 注意：实际 SDK 调用 lowpwr 模式，不是裸 deep sleep
    pmu_enter_sleep();

    // 醒来后读唤醒源
    u32 wake_src = wakeup_get_status();
    printf("Wakeup source: 0x%08X\n", wake_src);

    while (1) { delay_5ms(1); }
    return 0;
}
```

**步骤 2**：编译烧录；用杜邦线接 PB2 到 3.3V 触发上升沿。

**步骤 3**：观察串口输出 `Wakeup source: 0xXXXXXXXX` —— 高 16 位标记唤醒源 IO。

#### 层走查

```
app.c (L1) → wakeup_gpio_config() → api_gpio.h (L4)
  ↓
libplatform.a (L5) → PORTINT SFR (L6) 配置上升沿
  ↓
(deep sleep)
  ↓
硬件 PB2 上升沿 → PORTINT ISR (L5)
  ↓
唤醒 → wakeup_get_status() (L4) → libplatform.a (L5)
  ↓
返回 wake_src 值给应用
```

### 9.5 例 5：加一个新 func_ 模式 FM_RADIO

**目标**：加个新的"FM 收音机"模式。
**改动层**：L1 + L2 (~150 行代码)

#### 完整步骤（5 步）

**步骤 1**：在 `app/platform/functions/func.h` 加 enum：

```c
typedef enum {
    // ... 已有 FUNC_BT / FUNC_MUSIC / FUNC_AUX
    FUNC_FMRADIO = 100,    // 新增
} func_id_t;
```

**步骤 2**：在 `func.c:202 func_message()` switch 加 case：

```c
void func_message(u16 msg) {
    switch (msg) {
        // ... 已有 case
        case FUNC_FMRADIO:           // 新增
            func_switch_to(FUNC_FMRADIO);
            break;
    }
}
```

**步骤 3**：新建 `app/platform/functions/func_fmradio.c`：

```c
// func_fmradio.c
#include "include.h"

AT(.text.func)
void func_fmradio(void) {
    // 模式主循环
    while (func_cb.sta == FUNC_FMRADIO) {
        // 收消息
        u16 msg = msg_dequeue();
        if (msg) func_fmradio_message(msg);

        // FM 扫描
        fmrx_scan_step();

        delay_5ms(1);
        WDT_CLR();
    }
}

AT(.text.func)
void func_fmradio_message(u16 msg) {
    switch (msg) {
        case KU_PLAY:  // 切台
            fmrx_next_station();
            break;
        case KU_NEXT:
            fmrx_next_freq();
            break;
        case KL_PWR:   // 长按 POWER → 退出 FM
            func_cb.sta = FUNC_BT;
            break;
    }
}
```

**步骤 4**：在 `app/projects/standard/message/` 加 `msg_fmradio.c`（如果需要外部触发 FM 模式）。

**步骤 5**：在 `display/` 加 FM 模式的 icon / 显示字符串。

#### 层走查

```
外部事件 (e.g. 长按 MODE 键) → bsp_key.c (L3) → msg_enqueue(MODE)
  ↓
func_message() (L2) → case FUNC_FMRADIO → func_switch_to
  ↓
func_fmradio() 主循环 (L2)
  ↓
fmrx_scan_step() → api_fmrx.h (L4) → libdrivers.a (L5) → FM 调谐器芯片 (L6)
```

---

## 10. 5 个最常见的分层错误

> 在错的层写代码 = 编译过、跑起来崩、debug 找不到原因。

### 10.1 错误 1：在 ISR context 调 printf

**现象**：串口丢字符 / 卡死 / 触发 hardfault。

**根因**：`usr_tmr5ms_thread` 虽是普通函数，但被 `timer1_isr`（硬件 ISR）调用 = **ISR context**。

**修法**：

```c
// ❌ 错
AT(.com_text.timer)
void usr_tmr5ms_thread(void) {
    if (some_flag) printf("tick!\n");     // ← ISR 调 printf = 卡死
}

// ✅ 对
volatile u8 s_print_flag;                // 共享 flag
AT(.com_text.timer)
void usr_tmr5ms_thread(void) {
    if (some_flag) s_print_flag = 1;     // ← 只设 flag
}

// 应用层主循环
while (1) {
    if (s_print_flag) {
        printf("tick!\n");
        s_print_flag = 0;
    }
    delay_5ms(1);
}
```

**层**：L1-L2 in ISR（**5 ms ISR 算 L3 但执行上下文是 L6 ISR**）

### 10.2 错误 2：在应用层直接写 SFR 地址

**现象**：能跑但别的 mode 行为变了；某些 IO 时序错乱。

**根因**：跳过了 BSP/Driver 的 mutex/互斥逻辑。

**修法**：

```c
// ❌ 错
*(volatile u32*)0x50001000 = 1;          // ← 跳过了 BSP 初始化

// ✅ 对
gpio_t g;
bsp_gpio_cfg_init(&g, IO_PD0);           // ← BSP 负责 mux/init
g.sfr[GPIOxSET] = 1;                     // ← HAL 暴露 SFR
```

**层**：L1-L6（**破坏分层契约**）

### 10.3 错误 3：在 func_* 框架外不清 WDT

**现象**：跑 1-2s 后重启。

**根因**：`func_run()` 每圈自动清狗（`func.c:692 WDT_CLR`），但自写 `main loop`（如 wav_test_run / tf_test_run）**必须自己清**。

**修法**：

```c
// ❌ 错：自写 main loop 不清狗
void my_test_run(void) {
    while (1) {
        do_something();
        delay_5ms(100);
    }
    // ← 1-2s 后重启
}

// ✅ 对：每 100ms 清一次
void my_test_run(void) {
    u32 last_wdt = tick_get();
    while (1) {
        do_something();

        if (tick_check_expire(last_wdt, 100)) {
            WDT_CLR();
            last_wdt = tick_get();
        }

        delay_5ms(1);
    }
}
```

**层**：L1（**自写 main loop 必须自己清**）

### 10.4 错误 4：在应用层调 `delay_5ms(N>1)`

**现象**：按键失灵 / msg queue 满 / 5 ms ISR 堆叠。

**根因**：`delay_5ms(10)` 等于让出 CPU 10×5ms = 50ms，期间 ISR 堆 10 个 msg 没机会被处理。

**修法**：

```c
// ❌ 错
void my_func(void) {
    do_step_1();
    delay_5ms(100);                      // ← 期间 ISR 堆 20 个 msg
    do_step_2();
}

// ✅ 对：用 tick_check_expire 异步等
void my_func(void) {
    static u32 wait_start;
    do_step_1();
    if (!wait_start) wait_start = tick_get();    // 第一次记录起点
    if (!tick_check_expire(wait_start, 100)) return;  // 还没到
    wait_start = 0;                                // 重置
    do_step_2();
}
```

**层**：L1（**让出 CPU 用 delay_5ms(1) + tick_check_expire**）

### 10.5 错误 5：在 BSP 层忘记调 `func_message()`

**现象**：开了 BSP 中断 / 外部事件但 `func_cb.sta` 没更新；handler 没跑。

**根因**：BSP 直接 `msg_enqueue` 但没触发 `func_message()`——msg 进了队列但没人分发。

**修法**：

```c
// ❌ 错：BSP 只入队
void bsp_my_event(void) {
    msg_enqueue(MY_EVENT);                // ← 进了队列但没人拿
}

// ✅ 对：要么调 func_message，要么让应用 loop 拿
void bsp_my_event(void) {
    msg_enqueue(MY_EVENT);                // 入队
    func_message(MY_EVENT);               // ← 立即分发（如果当前 mode 在等）
}
```

**层**：L2-L3（**msg 进了队列但不在 func_run loop 中处理**）

---

## 11. 自测题

> 学完本篇后做这 10 题，能 9/10 算入门。

1. `app/platform/libs/api_msg.h` 的 5 个函数分别叫什么？哪个返回 `u16`？
   - **答案**：`msg_queue_init / msg_queue_clear / msg_enqueue / msg_dequeue / msg_queue_detach`；`msg_dequeue` 返回 `u16`。

2. `gpio_t.sfr[GPIOxSET]` 中 `GPIOxSET` 的数值是多少？它在 `api_gpio.h` 第几行？
   - **答案**：`GPIOxSET = 0`；在 `api_gpio.h:5`。

3. `bsp_key_scan()` 在哪一层？由谁调用？调用频率？
   - **答案**：L3（BSP）；由 `usr_tmr5ms_thread` 在 5 ms ISR 调用；频率 = 5 ms 一次。

4. 完整列出 "按键按下到 handler 跑"经过的所有层（L1-L6），每层做什么。
   - **答案**：见 §7.1（15 步追踪表）。

5. `usr_tmr5ms_thread` 是 ISR 还是普通函数？调 `printf` 会怎样？
   - **答案**：普通函数，但被硬件 ISR 调用 = ISR context。调 printf 会卡死串口或触发 hardfault。

6. 我想加一个 `func_lock_screen` 模式，要改哪些文件？
   - **答案**：5 个：
     1. `func.h` 加 `FUNC_LOCK_SCREEN`
     2. `func.c:202 func_message()` switch 加 case
     3. `func_lock_screen.c` 新建（process / enter / exit / message）
     4. `message/msg_lock_screen.c`（如果需要外部触发）
     5. `display/` 加 icon

7. `libplatform.a` 和 `libdrivers.a` 在链接顺序上有什么要求？
   - **答案**：`libplatform.a` 必须在最前（它定义 OSAL 符号）。

8. `aram` 在哪一段？和 `data` 区别是？
   - **答案**：`aram` 在 `0x50000`，16 KB，是 NOLOAD Audio RAM（音频 scratchpad）；`data` 在 `0x13c00`，13 KB，是 BSS。

9. `func.c` 的 `while(1)` 循环每圈大约耗时多久？
   - **答案**：典型 < 5 ms（取决于当前 `func_xxx()` 的工作量）。一轮里：
     - `func_enter()` ≈ 0 ms
     - `switch (func_cb.sta) func_xxx()` ≈ 1-3 ms
     - `msg_dequeue()` ≈ 0 ms（队列空时）
     - `WDT_CLR()` ≈ 0 ms

10. 找一段 `bsp_sys.c` 中的代码，标注它在哪一层。
    - **答案**：`bsp_sys.c:347 bsp_key_scan()` → **L3**（BSP 调用 Driver 组合）。

---

## 12. 与现有教程的边界

| 本篇 § | 不重复内容 | 现教程 |
|---|---|---|
| §2.2-2.5 真实 6 层 | 仓库实际分层结构 | 无（**新内容**）|
| §3 HAL 透明 | 透明 HAL 概念 | [tutorial_入门/02 §2 SFR 概念](02_BT892XA2芯片与外设地图.md) 基础 |
| §4 OSAL 5 函数 | 5 函数完整解剖 | [tutorial_入门/03 §2 消息队列](03_SDK架构与运行模型.md) 简化版 |
| §5 Driver 4 种接口 | 4 种模式分类 | [tutorial_入门/05 §6 pff_compat](05_PetitFatFs源码导读.md)（一个例子）|
| §6 func_run 完整 | 完整 while 循环剖析 | [tutorial_入门/03 §4 func_run](03_SDK架构与运行模型.md) 简化版 |
| §7 信号追踪（4 例） | 4 个端到端链路 | 无（**新内容**）|
| §8 写代码决策树 | 决策树 + 风险表 | [tutorial_入门/03 §10 加 func_*](03_SDK架构与运行模型.md) |
| §9 5 个 walkthrough | 完整可编译示例 | 无（**新内容**）|
| §10 错误 | 5 个真实分层错误 | [tutorial_入门/06 §3 SDK 架构类](06_踩坑大全.md) |
| §13 cheat sheet | 1 张分层图 | 无（**新内容**）|

---

## 13. 分层 cheat sheet

> 1 张图速记。

```
┌──────────────────────────────────────────────────────────────┐
│ L1 应用层 (Application)                                       │
│   app/projects/standard/{wav_test, hello_world, fat_test}/*.c│
│   职责: 业务逻辑 (播放/按键响应/打印)                          │
│   可改: ✅ 改 .c, .h                                          │
│   风险: 🟢 低                                                  │
├──────────────────────────────────────────────────────────────┤
│ L2 框架层 (Framework)                                         │
│   app/projects/standard/port/*.c + message/*.c + plugin/*.c  │
│   app/platform/functions/func_*.c                            │
│   职责: 板级 HAL + 模式状态机 + 消息分发                       │
│   可改: ✅ 改 .c, .h                                          │
│   风险: 🟡 中（影响所有 mode）                                │
├──────────────────────────────────────────────────────────────┤
│ L3 BSP 层 (Board Support Package / 业务中间件)                │
│   app/platform/bsp/bsp_*.c (40+ files)                       │
│   职责: 跨接 driver 与 application 中间件                      │
│   可改: ✅ 改 .c, .h                                          │
│   风险: 🟡 中                                                  │
├──────────────────────────────────────────────────────────────┤
│ L4 API 层 (Application Programming Interface)                │
│   app/platform/libs/api_*.h (28 files)                       │
│   职责: SDK 公开头, ABI 契约                                  │
│   可改: ⚠️ 改 .h (但实现是 .a)                                │
│   风险: 🟠 高（ABI 改完 .a 不兼容）                           │
├──────────────────────────────────────────────────────────────┤
│ L5 闭源库层 (Pre-compiled Static Libraries)                   │
│   libplatform.a / libdrivers.a / libbtstack.a                │
│   libcodecs.a / libvoices.a                                  │
│   职责: Driver/HAL/codec/BT/voices 实体                       │
│   可改: ❌ (看不到源码)                                       │
│   风险: 🔴 不可改                                              │
├──────────────────────────────────────────────────────────────┤
│ L6 硬件层 (Hardware)                                          │
│   SFR (sfr.h) + RAM 区段 (ram.ld) + aram                     │
│   职责: 物理芯片                                              │
│   可改: ⚠️ 改 ram.ld / 不改 sfr.h                             │
│   风险: 🔴 极高（烧片风险）                                   │
└──────────────────────────────────────────────────────────────┘
```

**核心规则**：

- **调用方向**：严格向下（L1 → L2 → L3 → L4 → L5 → L6），**禁止反向**
- **改动的可见性**：L1-L3 改了能跑；L4 改了 ABI 不兼容；L5-L6 不可改
- **debug 顺序**：症状 → §8 决策树定位层 → §10 错误自查 → 看具体文件

---

## 总结：本篇的 5 个核心洞察

1. **这不是教科书的 4 层架构**——是**闭源 SDK + BSP glue** 实际工程形态。
2. **HAL 是透明的**——SFR 直接暴露，不是"抽象"。
3. **OSAL 是 5 函数**——没有 mutex/sem/event group/thread。
4. **应用层最大的坑**：5 ms ISR + WDT + msg queue 三个机制，**不掌握等于 debug 永远是猜测**。
5. **改代码的 80% 工作**是在 L1-L3；L4 偶尔改；L5-L6 不要碰。

如果 5 条都吃透——你已经**具备"在正确层写代码 + 在错层 debug"的能力**。剩下的就是实战经验。

下一步推荐：
- 跟着 §9 5 个 walkthrough 实际跑一遍
- 遇到任何症状第一件事查 [06_踩坑大全 §8 症状索引表](06_踩坑大全.md)
- 想加新功能先用 §8 决策树定位层
- 找 `bsp_sys.c:1080 bsp_sys_init()` 自己标注每行在哪一层

---

## 附：进一步阅读

| 主题 | 文档 |
|---|---|
| SFR / GPIO / ADC 硬件细节 | [02_BT892XA2芯片与外设地图](02_BT892XA2芯片与外设地图.md) |
| 5ms ISR / msg / WDT / func_run | [03_SDK架构与运行模型](03_SDK架构与运行模型.md) |
| 按键系统 / pwrkey_table | [04_按键系统完全指南](04_按键系统完全指南.md) |
| Petit FatFs / pff.c | [05_PetitFatFs源码导读](05_PetitFatFs源码导读.md) |
| 50+ 踩坑 + 症状索引 | [06_踩坑大全](06_踩坑大全.md) |
| Phase 1-5 入门 + 决策树 | [07_Phase导航与下一步](07_Phase导航与下一步.md) |
| 工具链 / 烧录 | [01_工具链与构建产物](01_工具链与构建产物.md) |
| 第 1 天 runbook | [00_新手上路清单](00_新手上路清单.md) |
| 仓库结构导览 | [docs/快速入门 §5](../快速入门.md) |
| bsp_sys_init 启动流 | [docs/快速入门 §6](../快速入门.md) |
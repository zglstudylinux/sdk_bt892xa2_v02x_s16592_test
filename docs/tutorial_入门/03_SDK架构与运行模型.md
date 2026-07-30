# 03_SDK 架构与运行模型

> **本篇填空白 #3**：追踪任意一段 SDK 代码时，**"它在第几个时间层"**。从按键被按到喇叭放声音，整条链里哪种代码在哪种上下文里跑。
>
> 读者画像：学过 C，会写 "while(1)"。**没接触过 ISR、callback、消息队列**这些嵌入式经典抽象。
>
> 这一篇看完你应当能：
> 1. 说出"**为什么我的循环读不到按键**"——主线要 `msg_dequeue()`
> 2. 区分哪些代码在 **ISR 里**，哪些在 **主循环**
> 3. 解释 **`u32 msg_dequeue()`** 返回什么、怎么过滤系统事件
> 4. 知道** WDT 是谁喂的**——为啥测试 loop 不喂会重启
> 5. 看懂 **`func_run()` 状态机**的 9 个 func_* 入口是啥
> 6. 在 28 个 **`api_*.h`** 里查到对应 API
> 7. 理解**哪些代码看不见**（闭源静态库），哪些是开

---

## 目录

- [0. 定位：本篇与已有文档关系](#0-定位本篇与已有文档关系)
- [1. 5 ms tick：所有 ISR 的"心跳"](#1-5-ms-tick所有-isr-的心跳)
- [2. 消息队列：键到主循环的唯一通道](#2-消息队列键到主循环的唯一通道)
- [3. WDT 喂狗分裂：最容易踩的位置](#3-wdt-喂狗分裂最容易踩的位置)
- [4. `func_run()` 状态机：9 个 func_* 入口](#4-func_run-状态机9-个-func_-入口)
- [5. 28 个 api_*.h 速查表](#5-28-个-api_h-速查表)
- [6. `xcfg_cb`：三配置时态的全局大结构](#6-xcfg_cb三配置时态的全局大结构)
- [7. 测试 Phase 切入点：main.c 里那一串注释](#7-测试-phase-切入点mainc-里那一串注释)
- [8. 静态库边界：能看 vs 不能看](#8-静态库边界能看-vs-不能看)
- [9. `func_cb`：往闭源库里钩子](#9-func_cb往闭源库里钩子)
- [10. 加新 func_* 模式的 5 步](#10-加新-func_-模式的-5-步)

---

## 0. 定位：本篇与已有文档关系

| 文档 | 覆盖 | 与本篇关系 |
|---|---|---|
| [`快速入门 §5`](../快速入门.md) | 工程目录树 + 重要文件清单 | 本篇不会重新列文件，但会更深入 |
| [`快速入门 §6`](../快速入门.md) | 启动流程图（main.c → 配置 → 主循环） | 本篇是那图背后的**详细讲解** |
| [`wav_test_03.md §4`](../wav_test_03.md) | 5ms ISR/msg/WDT 是 wav_test 用到的**实例** | **本篇是参考**，`wav_test_03.md` 是实例 |
| [`快速入门 §9.5`](../快速入门.md) | api_msg.h / api_dac.h 等 10 个 API 速查 | 本篇覆盖**全部 28 个** api_*.h |
| [`AGENT_HANDOFF.md`](../AGENT_HANDOFF.md) | 给 AI 助手的接力文档 | 本篇和那个独立，但都面向"理解 SDK" |

---

## 1. 5 ms tick：所有 ISR 的"心跳"

### 1.1 一切的源头

```
                ┌───────────────────────────────┐
                │       CPU System Timer         │
                │   (SDK 内部用了其中一个 TMRn)   │
                └───────────────────────────────┘
                              │
                              ▼ 每 5ms 触发一次
                ┌───────────────────────────────┐
                │       ISR: tmr5ms_isr()       │
                │       (在 bsp_sys.c:347)       │
                └───────────────────────────────┘
                              │
                              ▼ 同步调用（不开新中断）
              ┌──── usr_tmr5ms_thread(void)         ← 你的回调
              ├──── plugin_tmr5ms_isr(void)         ← 插件注册
              ├──── bsp_key_scan(void)              ← 按键扫描
              └──── 其他 20+ 个 callback
```

### 1.2 中断和 ISR 是什么（10 秒入门）

| 词 | 一句话 | 类比 |
|---|---|---|
| **中断** | 硬件自动叫 CPU"先停下手上的活，跑这个函数" | 老板突然 @你 |
| **ISR** = Interrupt Service Routine | 处理中断的函数 | 处理 @ 的回话 |
| **优先级** | 多个中断同时到，谁先跑 | 老板 vs 同事同时 @，听谁的？|
| **嵌套** | ISR 里再触发中断 | 回老板话时被同事再 @ |
| **5ms tick** | 一个固定周期的"心跳"中断 | 微信每 5 分钟提醒一次 |

**ISR 最重要的"不能"清单**：
- ❌ **不能阻塞**（不能 `while()` 等数据）
- ❌ **不能 delay_ms**（同事回话了但你在发呆等 100ms）
- ❌ **不能操作 fs / sd0_read**（文件系统要锁）
- ✅ **可以** 改全局变量（要标 `volatile`，[Doc 02 §2.3](02_BT892XA2芯片与外设地图.md)）
- ✅ **可以** 写 FIFO/队列（如 `msg_enqueue`）

### 1.3 谁挂在 tmr5ms_isr 上

`bsp_sys.c:347` 那个函数名其实是 `usr_tmr5ms_thread`（SDK 把"用户层"和"中断层"做了区隔）。里面通常长这样：

```c
// 简化版（真实代码见 bsp_sys.c:347）
void usr_tmr5ms_thread(void)
{
    bsp_key_scan();              // ← 5ms 扫一次按键，[04_按键 §1](04_按键系统完全指南.md) 详解
    if (++tmr5ms_cnt % 100 == 0) msg_enqueue(MSG_SYS_1S);     // ← 1 秒塞一个系统事件
    if (++tmr5ms_cnt % 10  == 0) msg_enqueue(MSG_SYS_500MS);
    // ... 别的 callback ...
}
```

> 注：实际 sdk 中 `tmr5ms_cnt` 是 SDK 内部全局变量，由 ISR 自增。

### 1.4 异步代码的关键规则

| 规则 | 原因 |
|---|---|
| ISR 里**只**能改全局变量、调用 `msg_enqueue` 等"轻操作" | 不能 fs_read / printf 等长 call |
| 主循环里**必须**调 `msg_dequeue` 才能读到 ISR 塞的东西 | 主循环是消费方 |
| 主循环里**不应** `delay_ms` 长于 5 ms（除非你打算阻塞整个循环） | 否则 5 ms tick 看似正常但你感受不到 |

### 1.5 实际看到的效果

如果你写

```c
void my_main(void) {
    while (1) {
        // 没有 msg_dequeue，没有 WDT_CLR，没有 delay
    }
}
```

- 系统事件正常塞进队列（每次 ISR 跑）
- 但**没人抽**——队列满了，新事件丢失
- WDT 不喂 ~ 1-2 秒后**整颗芯片复位**
- 表现：板子**看上去**像卡死其实是**自动重启**

> §3 WDT 一节详聊这个。

---

## 2. 消息队列：键到主循环的唯一通道

### 2.1 长什么样

`app/platform/libs/api_msg.h` 提供这个接口：

```c
typedef u16 MSG;
#define NO_MSG      0       // 空

void    msg_init(void);                      // 启动时一次性调用
void    msg_enqueue(MSG m);                  // ISR 调用
MSG     msg_dequeue(void);                   // 主循环调用，返回第一个 msg（空 = NO_MSG）
bool    msg_is_empty(void);                  // 查队列
// ... 还有 peek / register hook ...
```

### 2.2 MSG 是 u16 编码

```
bit[15:12]  bit[11:8]   bit[7:0]
┌───────────┬───────────┬───────────┐
│  类型      │ 标志位     │ base id   │
└───────────┴───────────┴───────────┘
   3-4 类     KU/KH/KL     0x00-0xEF
```

详见 [04_按键系统完全指南 §5](04_按键系统完全指南.md)；这里只告诉你**拿到 msg 怎么 dispatch**：

```c
while (1) {
    u16 msg = msg_dequeue();
    if (msg == NO_MSG) break;
    switch (msg) {            /* 跳到对应 handler */
    case KU_PLAY: ...
    case MSG_SYS_500MS: ...   // 不处理 → 丢弃
    default: break;
    }
}
```

### 2.3 系统事件过滤规则（避坑必备）

`bsp_sys.c:441-460` 里塞了 8 个**系统事件**进同一个队列。常见的：

| 常量 | 值 | 含义 | 来源 |
|---|---|---|---|
| `EVT_SD_INSERT`    | 0x7FD | SD 卡插入 | `bsp_sys.c:50-86` |
| `EVT_SD_REMOVE`    | 0x7FC | SD 卡拔出 | 同上 |
| `MSG_SYS_500MS`    | 0x7FE | 500ms 心跳 | 5ms ISR 计数 |
| `MSG_SYS_1S`       | 0x7FF | 1s 心跳 | 同上 |
| `EVT_BT_UPDATE_STA`| 0x7D0 | 蓝牙状态变更 | `bsp_bt.c` |

**关键问题**：这些事件的 u16 编码跟你**真按键**的 u16 编码完全混在同一个队列。如果你直接 `switch (msg)` 不过滤，**每 500ms 会看到一次 `[WAV] key 0x07FE ->` 输出，完全淹掉真正的键**。

```c
/* 错的示范，假设这是 wav_test.c 的 handle_key */
// → "为什么我按键不响应？" —— 因为被 0x07FE 占满队列，KU_PLAY 进不来

switch (msg) {  // 不限类型
case KU_PLAY: ...
}
```

正确做法（`wav_test.c:267` 原型）：

```c
if (msg < 0x0100 || (msg & 0xFF00) == 0x0700) {
    return;     /* base 0x00..0xFF 之外 / 0x07xx 系统事件 → 丢弃 */
}
```

**为什么 0x0700**：
- KU_* = base|0x800：base 0x00-0xFF, 用 bit 0x800
- KL_* = base|0xA00：用 bit 0xA00
- KH_* = base|0xE00：用 bit 0xE00
- `0x800 & 0xFF00 = 0x0800` / `0xA00 & 0xFF00 = 0x0A00` / `0xE00 & 0xFF00 = 0x0E00`
- 系统事件全部在 0x700-0x7FF：bit `0x0700`

**magic mask**：`0xFF00 & msg == 0x0700` —— 上面两种类型的并集恰好把系统事件过滤掉。

> ⚠️ 新手**几乎一定**漏这一步——到现在 wav_test 还能跑只不过是因为我们 `[wav_test_03.md §4.2]` 专门记了。

### 2.4 队列容量 + 溢出会怎样

`api_msg.h` 默认缓冲 **6 个 u16**：

```c
#define MSG_POOL_LEN     8    // 实际可能 6 或 8，看 SDK 版本
```

队列满了还 `msg_enqueue` —— **没塞进去的丢**，没崩溃，但**按键会丢**：

```
按 5 次键，只看到 3 次 key log —— 队列满溢
```

**debug**：
- 减少主循环 `delay_ms` 时间
- 或者直接加长 `MSG_POOL_LEN`（如果 RAM 有余）

### 2.5 完整"按键到主循环"时序图

```
  时间轴 ────────►

  按键按下时刻                                          喇叭放时间
       │                                                  │
       ▼                                                  ▼
   ┌─────────────────────┐                       ┌──────────────┐
   │     硬件 PB5 ADC    │    S7 NEXT 短按        │   主循环     │
   └─────────────────────┘    见 [04 §1](04_按键系  │   dispatch   │
                            统完全指南.md)         │              │
                            │                     │              │
                            ▼                     │              ▼
   ┌─────────────────────┐                       │   msg_dequeue()
   │  5ms ISR            │                       │   msg=0x0860  │
   │  bsp_key_scan()     │                       │   KU_NEXT     │
   │  → 读 ADC 值       │                       │              │
   │  → 30ms 防抖判断    │  ←—————————————ISR 和主线不同步—————————►
   │  → 达到稳定 30ms    │                         │
   │  → msg_enqueue(     │   ─── msg queue ───►   │
   │     KU_NEXT=0x860)  │                         │
   └─────────────────────┘                         │
                                                    │
                              ◄——— 主循环             │
                              ◄——— 接下来 5ms 内        │
                              ◄——— 会读到 msg = 0x860   │
                                                    │
                                                    ▼
                                            处理 next 请求
                                            wav_test.c:316
```

**关键**：
- **ISR 跑完就返回**，不阻塞主循环
- **主循环抽 msg**，可能立即抽到也可能 1-2ms 后
- **整个网络 5-50ms 内**完成（5 是 ISR 周期，30 是防抖，加上 dispatch）

---

## 3. WDT 喂狗分裂：最容易踩的位置

### 3.1 什么是 WDT（Watch Dog）

| 项 | 值 |
|---|---|
| 默认开启 | ✅ `config.h: WDT_EN=1` |
| 周期 | ~ 1 秒 ~ 4 秒（默认 4 秒；可缩短）|
| 不喂后果 | **整颗芯片硬件复位** |
| 喂法 | 写 `WDTCON = 0xA` |

### 3.2 SDK 默认谁来喂

**只有一处**——`app/platform/functions/func.c:128 func_process()`：

```c
// app/platform/functions/func.c:128
void func_process(void)        // func 框架的主循环
{
    WDT_CLR();                 // ← 唯一喂狗点
    /* ... 状态机 + call func_*() */
}
```

`func_process` 在 **正常模式下** 每帧调用一次（每帧 ~5ms），**主循环通过它**喂狗。这是 SDK 正常的"喂狗路径"。

### 3.3 测试代码会踩这个

跑自定义测试（`tf_test / fat_test / wav_test / hello_world`）时，`main.c` 写的是：

```c
// app/projects/standard/main.c 简化版
void main(void) {
    sys_init();
    if (...) {
        wav_test_run();         // ← 你现在的"主循环"
    } else if (...) {
        tf_test_run();
    }
    // ...
}
```

**问题**：`wav_test_run` 里**没有** `func_process`，所以不调 `WDT_CLR()`。后果：

- **1-2 秒内** watchdog 触发
- 板子**自动重启**
- 表现：按 P/P 暂停一两秒**整颗芯片复位**（重启后 print 一遍"playing slot 1/4"）

### 3.4 解法（手动喂狗）

`wav_test.c:621-645` + `:794-805` 已经修过这个：

```c
u32 last_wdt_tick = tick_get();
for (;;) {
    u32 now = tick_get();
    if ((u32)(now - last_wdt_tick) >= 100) {     // 100 ms = 安全频度
        WDT_CLR();
        last_wdt_tick = now;
    }
    // ...
}
```

**两条关键点**：
1. **每 100 ms 喂一次**（100 ms ≪ WDT 4 s 周期，有 ~40 倍冗余 → 即使主循环卡一两秒也不会复位）
2. **用 `tick_get()` 取时间**（不是 `delay_ms` —— delay 会阻塞，但 tick 不会）

### 3.5 关闭 WDT（极端情况下）

```c
// 调试时可以关掉 WDT —— ⚠️ 生产必开
WDT_DIS();        // macro.h:17-21
```

但**测试阶段能开就开**——WDT 是 90% 死循环的最后一道防线。

### 3.6 衍生：wav_test 现在的双 WDT 模式

为了清晰，`wav_test_run` 有**两处**喂狗（这是有意的）：

| 位置 | 喂 / 时机 |
|---|---|
| 外层 `for(;;)` 调度器 | 每 100 ms 喂一次（在 open_and_inspect、wait 期间）|
| 内层 `parse_and_play` 的 512B 循环 | 每 100 ms 喂一次（在忙碌推帧期间）|

**为什么不合成一个**：
- 外层要管"两首歌之间"的等待
- 内层要管"播放中"的等待
- 把 WDT 集中到一个函数更难调试——两个独立 100 ms tick 让你看到 log 就能知道是谁在喂

---

## 4. `func_run()` 状态机：9 个 func_* 入口

### 4.1 是干啥的

**主循环 = `func_run()`**，里面跑 9 个分支（`func_bt`, `func_music`, `func_clock`, ...）。

```
while (1) {                          ← sdk main loop
    WDT_CLR();
    func_process();
    func_run();                      ← 状态机 dispatch
    ...
}
```

切换模式 = 改 `func_cb.sta`：

```c
// 切到 func_music 模式
func_cb.sta = FUNC_MUSIC;

// 切到自定义模式（你需要定义一个 FUNC_XXX 枚举，并写在 switch 里）
func_cb.sta = FUNC_X;   // ← 必须先在 func.c 里加 case
```

### 4.2 9 个 func_* 入口（速查）

完整 `func_*` 函数体不在本节，看 `app/platform/functions/`：

| 函数 | 用途 | 内部职责 |
|---|---|---|
| `func_bt()` | Bluetooth 模式（默认上电模式）| 扫描、配对、播放音乐等 |
| `func_music()` | SD 卡 / USB 音乐 | 调度播放列表、解码、推 DAC |
| `func_clock()` | 时钟闹钟（如果有 RTC）| 显示时间 |
| `func_fm()` | FM 收音机 | 调台 |
| `func_linein()` | Line-in 外部输入 | 输入直通 |
| `func_idle()` | 待机（OLED 显示时间） | 低功耗 |
| `func_speaker()` | 录音 / 通过 SRS / 麦克风回环 | ADC→DAC 直通 |
| `func_pwroff()` | 关机 | 切电源 |
| `func_null()` | 等价 "no func"——故意返回，整个循环啥都不做 | 用于测试 |

### 4.3 测试模式怎么用 `func_null`

`tf_test / fat_test / wav_test / hello_world` 都是"自定义测试" —— 不在 9 个 func_* 里。SDK 提供两种"测试模式"走法：

**走法 1**：**直接调用测试函数，绕开 func_**：

```c
// main.c 里某行
wav_test_run();    // 自循环，里面自己处理 WDT/msg/播放 → 见 Doc 03 §3.4
```

**走法 2**：**注册一个 func_xxx 走 func_run**:

```c
// 1. 在 enum FUNC_XXX 加名字
enum {
    FUNC_NULL = 0,
    FUNC_MUSIC,
    // ...
    FUNC_WAV_TEST,        // ← 新加
};

// 2. func.c 里 switch 加 case
case FUNC_WAV_TEST: func_wav_test(); break;

// 3. main.c 里设进去
func_cb.sta = FUNC_WAV_TEST;
```

### 4.4 一个真实例子：hello_world

看 `app/projects/standard/hello_world/hello_world.c`：

```c
void hello_world_run(void) {
    init_printf(...)
    int cnt = 0;
    for (;;) {
        printf("Hello BT892XA2 %d\n", cnt++);
        delay_5ms(100);       // 100 = 0.5 s
    }
}
```

**走法 1 模式**：直接调、不进 func_run，每 500ms 一次。

### 4.5 实际流程图

```
开机 main() ───►  sys_init()   (clock, uart, GPIO 等)
              │
              ├── 找到 main_func_ptr，dispatch
              │
              ▼
   ┌── wav_test_run() ──────────────────────────────┐
   │   for (;;) {                                   │
   │      pff_mount ...                             │
   │      pff_opendir                               │
   │      playlist 循环 (外层)                       │
   │          ↓                                     │
   │      pff_open (内)                              │
   │      推 DAC (内)                                │
   │      查 msg (内)                                │
   │      WDT_CLR (内 + 外，两套)                    │
   │   }                                            │
   └────────────────────────────────────────────────┘
```

---

## 5. 28 个 api_*.h 速查表

本节是仓库里**最有用**的一节——你写代码碰到要"做某事"时，**第一件事**就是在这个表里找对应的 `api_*.h`。

### 5.1 全部 28 个 API 头

打开 `app/platform/libs/` 你能看到：

```
api_audio.h        api_dac.h        api_key.h        api_pwm.h
api_clock.h        api_dac_speaker.h api_msg.h        api_pwr.h
...
```

大约 28 个。下面是**速查表**（按职责）：

#### 外设 / 时钟

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_clock.h` | `sys_clk_init() / clk_get()` | 配置系统时钟 |
| `api_pwr.h`   | `pwr_enter_sleep() / pwr_lvd_set()` | 低功耗 / 休眠 / 低压检测 |
| `api_lvd.h`   | `lvd_set_level() / lvd_int_enable()` | 低压检测中断 |

#### 音频

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_dac.h` | `dac_spr_set() / dac_set_dvol() / dac_fade_in()` | DAC 控制 |
| `api_dac_speaker.h` | `dac_speaker_*()` | 喇叭模式（board.c 里用得多） |
| `api_audio.h` | `audio_path_set() / audio_mix_*()` | 音频通路 / 麦克风混音 |

#### 按键 / LED / 系统事件

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_key.h` | `bsp_key_scan() / key_msg_get()` | 按键扫描（[04 §1](04_按键系统完全指南.md)） |
| `api_msg.h` | `msg_enqueue / msg_dequeue` | 消息队列（§2） |
| `api_led.h` | `led_on() / led_off() / led_toggle()` | LED 控制 |

#### 存储

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_sd.h` | `sd0_init / sd0_read / sd0_write` | SD 卡裸块 |
| `api_fs.h` | `fs_xxx` | 闭源文件系统（**别用**，栈深度大，13 KB 限制）；自己有 Petit |
| `api_norflash.h` | `nor_xxx` | SPI flash |
| `api_i2c_eeprom.h` | `ee_*()` | I²C EEPROM |

#### 串口 / 通讯

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_uart.h` | `uart_init / uart_send / uart_set_baud` | HUART 调试串口 |
| `api_spi.h` | `spi_init / spi_send_byte` | SPI 总线 |
| `api_i2c.h` | `i2c_init / i2c_read / i2c_write` | I²C 总线 |

#### PWM / Timer

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_pwm.h` | `pwm_init / pwm_set_duty / pwm_start` | PWM 输出 |
| `api_timer.h` | `timer_init / timer_set_freq / timer_irq_cb` | 计数器 / 中断 |

#### ADC / 触摸 / RTC

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_saradc.h` | `adc_get_value / saradc_setup_time_set` | SAR ADC（本篇 [02 §7](02_BT892XA2芯片与外设地图.md)） |
| `api_touch.h` | `touch_read / touch_int_cb` | 触摸控制器 |
| `api_rtc.h` | `rtc_init / rtc_set_time` | 实时时钟 |

#### Misc

| 头 | 函数举例 | 用途 |
|---|---|---|
| `api_aes.h` | `aes_encrypt / aes_decrypt` | AES 加密 |
| `api_crc.h` | `crc16 / crc32` | CRC |
| `api_update.h` | `update_*` | OTA 固件更新 |
| `api_voice.h` | `voice_*` | 语音处理（音频后处理闭源） |
| `api_btstack.h` | `btstack_*` | **Bluetooth 协议栈闭源接口** |
| `api_lcd.h` | `lcd_init / lcd_write` | LCD 显示（如果有屏） |

### 5.2 怎么决定"用什么 API"

1. **看 5.1 表**——找到目标职责的 api_*.h
2. **打开对应头**——一般 30-50 个函数，注释告诉你参数
3. **找示例代码**——`grep -n "那个函数" app/projects/standard/` 找项目代码

> 💡 不要直接读 `libplatform.a / libdrivers.a` 的二进制——闭源、符号名只是分号前的"exports"。

---

## 6. `xcfg_cb`：三配置时态的全局大结构

### 6.1 三种配置

| 配置方式 | 文件 / 工具 | 时态 |
|---|---|---|
| **编译时（config.h）** | `app/projects/standard/include/config.h` | 你加 1 行 `#define` |
| **构建时（xcfg.xm）** | `app/projects/standard/xcfg.xm` | 重 build，xcfg.bin 自动变 |
| **运行时（Downloader GUI）** | 烧入 xcfg.bin | 改 .setting 后再烧 |

`xcfg_cb`（即 `xcfg_cb_t`，全局结构体）**把第三种 + 第二种的运行时配置对接到程序全局**。

### 6.2 大结构体长啥样

```c
// app/projects/standard/include/xcfg.h 简化版
typedef struct {
    u8 user_pwrkey_en;           // §6.1 编译时开 + §6.3 运行时开
    u8 user_adkey_en;
    u8 user_iokey_en;
    u8 sd_insert_detect_en;
    u8 vol_max;
    u8 vol_default;
    // ... ~50 个字段 ...
} xcfg_cb_t;

extern xcfg_cb_t xcfg_cb;        // 全局实例
```

### 6.3 何时被填值

`xcfg_init()`（在 `bsp_sys.c` 启动序列里调用）读 `xcfg.bin` → **填充 `xcfg_cb`** 全字段。

你写代码读取 `xcfg_cb.user_pwrkey_en`：

```c
if (xcfg_cb.user_pwrkey_en) {
    /* pwrkey 在用 */
    ...
}
```

——读到的是 **runtime 值**，改 GUI 立刻生效（重烧后）。

### 6.4 经典"三态不匹配"案例：USER_PWRKEY

| 步骤 | 状态 | 效果 |
|---|---|---|
| `config.h` `#define USER_PWRKEY 0` | 编译时不编 pwrkey | **代码不存在** |
| `config.h` `#define USER_PWRKEY 1` 但 `xcfg_bin: user_pwrkey_en=0` | 代码在，**xcfg_cb.user_pwrkey_en=0** | pwrkey 函数调用走 `_skip` 分支 |
| 都开 | 都开 | **键工作** |

> 详见 [04 §7](04_按键系统完全指南.md) + `wav_test_03.md §5.4`

### 6.5 不要直接写 `xcfg_cb`

**只读**。改配置要：
1. GUI 里改，重烧
2. 或改 `xcfg.xm` 重 build

不要在代码里 `xcfg_cb.user_pwrkey_en = 1` —— 那是在跑时改全局，不被持久化，下次上电就丢。

---

## 7. 测试 Phase 切入点：main.c 里那一串注释

### 7.1 注释约定

`app/projects/standard/main.c` 有这段约定：

```c
// [TF_TEST]    tf_test_run();
// [FAT_TEST]   fat_test_run();
// [WAV_TEST]   wav_test_run();
// [HELLO]      hello_world_run();
```

切换 Phase 流程：
1. **注释掉当前**（前加 `//`）
2. **取消注释目标**（删 `//`）
3. **Ctrl+F11 Rebuild**（别用 Ctrl+F9 = 普通 build，因为不动 pffconf 之类）
4. **烧 .dcf**

### 7.2 main.c 默认结构

`main()` 大约长这样：

```c
// app/projects/standard/main.c
#include "include.h"

void main(void) {
    // 1. 启动序列（不可省）
    sys_init();                       // clock, GPIO default, RAM clear
    bsp_sys_init();                   // 5ms ISR timer, 看门狗
    bsp_uart_init();                  // HUART

    // 2. 选择模式（Ph1-5 切换处）
#if (WAV_TEST_MODE)
    wav_test_run();
#elif (FAT_TEST_MODE)
    fat_test_run();
#elif (TF_TEST_MODE)
    tf_test_run();
#else
    hello_world_run();
#endif

    // 永远不会到这里
    while (1);
}
```

### 7.3 控制宏

| 宏 | 文件 | 状态 |
|---|---|---|
| `WAV_TEST_MODE` | `app/projects/standard/include/config.h` | 1 (Phase 5) |
| `FAT_TEST_MODE` | 同上 | 0 |
| `TF_TEST_MODE` | 同上 | 0 |
| `HELLO_TEST_MODE` | 同上 | 0 |

每个 == 1 时**上面 if 选择会进对应分支**（这只是代码最末段的 if，**实际** main.c 里还有别的地方区分，例如 `func_cb.sta = ...`）。

> 这是你看 main.c 时的**导航**。通常你需要先 `grep -n "Mode" main.c` 找宏定义在哪。

---

## 8. 静态库边界：能看 vs 不能看

### 8.1 仓库树看到的 5 个 `.a`

```
libplatform.a   ← 基本内核（clock、GPIO 等）
libdrivers.a    ← driver (UART、SPI、SD、IIS...)
libbtstack.a    ← Bluetooth 协议栈
libvoices.a     ← 语音处理（DRC/EQ/AEC）
libcodecs.a     ← codec（mp3 / aac / sbc / opus...）
```

它们在 SDK 安装目录，`grep` 找不到函数体，只能看 header。

### 8.2 看源码 vs 调接口

| 谁能看 | 谁能调 |
|---|---|
| `bsp_*.c` （如 `bsp_gpio.c`） | ✅ |
| `port_*.c` （项目级 GPIO/LED 配置） | ✅ |
| `app/projects/standard/foo/*.c` | ✅ |
| `plugin/*.c` 插件 | ✅ |
| `func_*.c` 状态机 | ✅ |
| `libdrivers.a` 内部 | ❌ 仅 `bsp_*.h` 头 |
| `libbtstack.a` BT 内部 | ❌ 仅 `api_btstack.h` 头 |

### 8.3 不熟领域的"探索顺序"

以"想加一个新 BLE 功能"为例：

1. **找 API**：搜 `api_btstack.h` 或者 `libbtstack.a` 的导出（**`nm` 命令**）
2. **找模板**：SDK 一般有 `app/projects/standard/ble/` 示例工程
3. **写代码**：调 API，不改 btstack 内部
4. **本地没 API**：可能要走 BT vendor 协议，要 `void` 调用 vendor 函数

### 8.4 闭库踩坑时怎么办

如果 lib 里某个函数**表现和你预期不符**：

1. 先看有没有 SDK demo 或测试用例
2. 再看 `bsp_*.c` 怎么调它的
3. **最后**才尝试 `objdump` / `nm` 看符号表

> 99% 时候你不需要 fallback 到第 3 步——SDK 文档和示例足够了。

---

## 9. `func_cb`：往闭源库里钩子

### 9.1 什么是 callback 袋

`func_cb` 是个**结构体**，里面是**函数指针**：

```c
// app/projects/standard/include/func.h
typedef struct {
    void (*mp3_res_play)(u32 addr, u32 size, ...);
    void (*set_vol_callback)(u8 vol);
    void (*led_callback)(u8 led_id);
    // ... ~40 个 function pointer
} func_cb_t;

extern func_cb_t func_cb;
```

### 9.2 怎么用

#### 注入回调（项目加东西时）

```c
// my_func.c — 注册回调
void my_set_vol(u8 vol) {
    // 你的特殊处理
}

void my_init(void) {
    func_cb.set_vol_callback = my_set_vol;   // 塞入"钩子袋"
}
```

#### 触发回调（其他模块调用）

```c
// libvol.a 内部
void libvol_set(u8 vol) {
    if (func_cb.set_vol_callback) {
        func_cb.set_vol_callback(vol);   // 调你的函数
    }
    /* libvol 自己的实现 */
}
```

### 9.3 经典用例

| 字段 | 谁会设 | 谁会调 |
|---|---|---|
| `func_cb.mp3_res_play` | `func_music.c` 进入播放模式 | `libcodecs.a` 解码完一帧 |
| `func_cb.set_vol_callback` | 你的应用初始化 | `bsp_dac.c` 改音量时 |
| `func_cb.led_callback` | LED 模式实现 | "需要 LED 时"事件 |
| `func_cb.karaoke_record_end` | 录音实现 | 录完一帧 |

### 9.4 调试 callback 的小技巧

如果你的 callback 没被调：

1. **没注册**？搜你的 init 是否真的进了
2. **顺序问题**？有时先执行逻辑、再注册回调
3. **多个 callback 互相覆盖**？后注册的赢

---

## 10. 加新 func_* 模式的 5 步

### 10.1 5 步

1. **`config.h` 开开关**：`#define FUNC_FOO_EN 1`
2. **`func.c` switch 加 case**：打开 `app/platform/functions/func.c`，找 `switch (func_cb.sta)`，加 `case FUNC_FOO: func_foo(); break;`
3. **写 `func_foo.c`**：放 `app/projects/standard/foo/`，实现 `func_foo()`、`func_foo_enter()`、`func_foo_exit()`
4. **加 `msg_foo.c`**（可选）：放 `message/msg_foo.c`，定义消息处理
5. **加 display**（可选）：如果项目有屏，加 `func_foo_display()`

### 10.2 func_foo.c 最小骨架（参考）

```c
// app/projects/standard/foo/func_foo.c
#include "include.h"

void func_foo_enter(void) {
    // 初始化：清屏、开 LED 等
}

void func_foo_exit(void) {
    // 清理：关 LED、释放 buffer 等
}

void func_foo(void) {
    // 主循环体（状态机内部步骤）
    // 包括：按键响应 + sub-state 处理
}

void func_foo_message(u16 msg) {
    switch (msg) {
    case KU_NEXT: ... break;
    ...
    }
}
```

### 10.3 在 func.c 加 case

```c
// app/platform/functions/func.c 找 switch (func_cb.sta)
switch (func_cb.sta) {
case FUNC_NULL: func_null(); break;
case FUNC_BT:   func_bt();   break;
// ... 其他 func ...
case FUNC_FOO:  func_foo();  break;     // ← 你加的
default:        func_null(); break;
}
```

### 10.4 在 enum 加名字

```c
// app/projects/standard/include/func.h 找 enum FUNC
enum {
    FUNC_NULL = 0,
    FUNC_MUSIC,
    // ...
    FUNC_FOO,        // ← 你加的
};
```

### 10.5 在 display 加显示（可选）

`func.c` 通常有 `func_event_process` 或类似的入口。**这部分**不强求，只是给以后添加便宜。

---

## 总结：核心清单

读完本篇后，**最少要保证**：

- [ ] 知道 **ISR 不能阻塞**，要做的事**只能塞进队列**
- [ ] 主循环**必须**`msg_dequeue()`，否则 ISR 塞的事件全部进队列溢出
- [ ] **过滤系统事件**用 `if (msg < 0x100 || (msg & 0xFF00) == 0x0700) return;`
- [ ] 自定义主循环**每 100 ms `WDT_CLR()` 一次**（`tick_get()` 计）
- [ ] 切换模式 = 改 `func_cb.sta`，要改 5 处（config / enum / switch / func_X / msg_X）
- [ ] 看不到的 lib*.a：**用 SDK 提供的 API**，不改二进制

如果 6 项 ✅——你已经能"加新 func_* 模式 / 改现有 mode / 加按键 handler / 修崩 / 修狗"了。

---

## 进一步阅读

| 想深入理解 | 跳到 |
|---|---|
| 整个 SDK 的分层架构（HAL/OSAL/Driver/App 6 层） | [08_SDK分层架构 §6 完整 func_run](08_SDK分层架构HAL_OSAL_Driver_App.md) |
| 完整按键按下到 handler 的 15 步逐层追踪 | [08_SDK分层架构 §7.1](08_SDK分层架构HAL_OSAL_Driver_App.md) |
| 5 ms ISR 在 6 层模型里属于哪一层 | [08_SDK分层架构 §2.2](08_SDK分层架构HAL_OSAL_Driver_App.md) |
| "透明 HAL" 怎么暴露 SFR | [08_SDK分层架构 §3](08_SDK分层架构HAL_OSAL_Driver_App.md) |

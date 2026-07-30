# 07_Phase 导航与下一步

> **本篇填综合需求 (b)**：每 Phase 一页导向 + "我想做 X 下一步" 决策树。
>
> **本质**：本篇**几乎全是交叉引用**，几乎没有新内容。它的价值是把分散的教程**整合成一张可导航的图**。

---

## 目录

- [0. 8 篇教程的地图](#0-8-篇教程的地图)
- [1. Phase 1 导向：跑通 hello_world](#1-phase-1-导向跑通-hello_world)
- [2. Phase 2 导向：Hello 周期打印](#2-phase-2-导向hello-周期打印)
- [3. Phase 3 导向：SD 卡协议（TF 7 任务）](#3-phase-3-导向sd-卡协议tf-7-任务)
- [4. Phase 4 导向：FAT 4 任务](#4-phase-4-导向fat-4-任务)
- [5. Phase 5 导向：WAV 4 任务](#5-phase-5-导向wav-4-任务)
- [6. "想加功能 X" 决策树](#6-想加功能-x-决策树)
- [7. "想调试 Y" 决策树](#7-想调试-y-决策树)
- [8. 源码 cheat sheet：5 个最有用文件](#8-源码-cheat-sheet5-个最有用文件)
- [9. 文档索引总表](#9-文档索引总表)

---

## 0. 9 篇教程的地图

### 0.1 9 篇新文档的职责

| 文档 | 一句话 | 何时读 |
|---|---|---|
| [00_新手上路清单](00_新手上路清单.md) | 7 步开箱跑通 wav_test | **第 1 天** |
| [01_工具链与构建产物](01_工具链与构建产物.md) | CB 编译、Downloader 烧录、产物溯源 | 编译失败时 / 想 shell 化时 |
| [02_BT892XA2芯片与外设地图](02_BT892XA2芯片与外设地图.md) | SFR / GPIO / ADC / blinky | 加新 IO / 想懂一个寄存器 |
| [03_SDK架构与运行模型](03_SDK架构与运行模型.md) | 5 ms ISR / msg / WDT / func_run | 想"信号从按到处理走哪儿" |
| [04_按键系统完全指南](04_按键系统完全指南.md) | pwrkey_table / 防抖 / KU/KH/KL | 按键不响应 / 加新键 |
| [05_PetitFatFs源码导读](05_PetitFatFs源码导读.md) | pff.c / diskio.c / FRESULT | 想知道 `pff_open` 内部到底干了啥 |
| [06_踩坑大全](06_踩坑大全.md) | 50+ 踩坑目录 + 症状索引表 | **第一件事查这里** |
| [07_Phase 导航与下一步](07_Phase导航与下一步.md) | 你正在读 | 想"我该做什么" |
| **[08_SDK分层架构HAL_OSAL_Driver_App](08_SDK分层架构HAL_OSAL_Driver_App.md)** | **6 层全景图 + 4 个信号追踪 + 5 个 walkthrough** | **想"系统是怎么组织的 / 想加新功能改哪层"** |

### 0.2 先决关系图

```
(1) 00 → (2) 快速入门 §1-2 §5 §6 → (3) 01 + 02 (并行)
                                              │
                                       (4) 03
                                              │
                                    ┌─────────┴─────────┐
                                    ▼                   ▼
                                   04                  05
                                    │                   │
                                    └─────────┬─────────┘
                                              ▼
                                          06（症状索引表）
                                              │
                                              ▼
                                          07（本篇）
                                              │
                              ┌───────────────┼───────────────┐
                              ▼               ▼               ▼
                         tf_test_*     fat_test_*        wav_test_*
```

### 0.3 与现有 5,500 行 docs 的边界

本系列**不重写**：

| 现有文档 | 何时读 | 不在本系列的哪里 |
|---|---|---|
| `docs/快速入门.md`（670 行）| 第一次接触项目 | Doc 0 / Doc 1 是其"扩展手册" |
| `docs/fat入门指南.md`（503 行）| FAT 文件系统入门 | Doc 5 是其"源码深读" |
| `docs/wav入门指南.md`（459 行）| WAV/DAC 入门 | wav_test_*.md 是其"实测补充" |
| `docs/tf_test_01_02.md` ~ `_07.md` | SD 协议深度 | Doc 5 §1 的 6 层图是其前置 |
| `docs/fat_test_*.md`（3 篇 ~430 行）| Petit FatFs 移植 | Doc 5 是其源码导读 |
| `docs/wav_test_*.md`（4 篇 ~880 行）| WAV 播放器 | Doc 3 / Doc 4 的"项目实例" |
| `test/参考.md`（325 行）| 11 个工程师心得原文 | Doc 6 只挑高频踩坑 |

---

## 1. Phase 1 导向：跑通 hello_world

**目标**：从拿到一台机器到第一次看到 `Hello BT892XA2` 在串口周期打印。

### 1.1 必备文档（按顺序读）

1. [00_新手上路清单](00_新手上路清单.md) §1-7（**第 1 天必做**）
2. [01_工具链与构建产物](01_工具链与构建产物.md) §1（产物映射表）、§6（Downloader GUI 操作）
3. `docs/快速入门.md` §2-3（CB build 步骤）

### 1.2 完成标志

- [ ] `Output/bin/app.dcf` 时间戳新、大小 ~700-800 KB
- [ ] Downloader 烧完，`HUART`（PB3，115200 8N1）看到周期 Hello 字符串
- [ ] Ctrl+F11 Rebuild 后新代码生效

### 1.3 失败跳转

| 症状 | 跳到 |
|---|---|
| `riscv32-elf-gcc: not found` | [01 §9.1](01_工具链与构建产物.md) + [06 坑 T5](06_踩坑大全.md) |
| `region 'ram' overflowed` | [06 坑 T2](06_踩坑大全.md) |
| `postbuild.bat failed` | [06 坑 T6](06_踩坑大全.md) |
| Downloader 烧不进 | [06 坑 T4](06_踩坑大全.md) |
| 串口无输出 | [06 §8.1 索引表](06_踩坑大全.md) |

---

## 2. Phase 2 导向：Hello 周期打印

**目标**：把 hello_world 跑成"main 循环 while + tick 计数 + 500 ms 一次 print"。

### 2.1 必备文档

1. [03_SDK架构与运行模型](03_SDK架构与运行模型.md) §1（5 ms tick）
2. `docs/快速入门.md` §6.1-6.3（启动流）

### 2.2 一行总结

> **看到 500 ms 一次 `Hello BT892XA2` 即工具链 + 5 ms tick + UART 通路全通**。

### 2.3 一行代码（写在 main.c）

```c
static u32 cnt;
if (tick_get() != cnt) {
    cnt = tick_get();
    if ((cnt % 100) == 0)        // 100 × 5 ms = 500 ms
        printf("Hello BT892XA2\n");
}
```

### 2.4 失败跳转

| 症状 | 跳到 |
|---|---|
| 周期不对（1 s 变 2 s）| [03 §1](03_SDK架构与运行模型.md) |
| 看不到 log | [01 §8](01_工具链与构建产物.md) (HUART) |
| 跑 1-2 s 后重启 | [06 坑 C3](06_踩坑大全.md) (WDT) |

---

## 3. Phase 3 导向：SD 卡协议（TF 7 任务）

**目标**：从 SPI/SDIO 1-bit 4-bit 协议 → 控制器初始化 → 多块读写 → 实测 cheat sheet。

### 3.1 必备文档

1. [05_PetitFatFs源码导读](05_PetitFatFs源码导读.md) §1（adapter 6 层图，理解"应用层到物理层"）
2. `docs/tf_test_01_02.md` ~ `tf_test_07.md`（7 任务，每任务一篇）
3. `docs/tf_test_summary.md` §3（cheat sheet：CMD0/ACMD41/CMD58 流程）

### 3.2 7 任务一览

| 任务 | 文件 | 内容 |
|---|---|---|
| 3.1 | `tf_test_01_02.md` | SPI 协议 / SD 协议基础 / cmd/resp |
| 3.2 | `tf_test_03.md` | sd0_init 控制器初始化 |
| 3.3 | `tf_test_04.md` | 单块读 / 单块写 |
| 3.4 | `tf_test_05.md` | 多块写 (CMD25 + 0xFD stop) |
| 3.5 | `tf_test_06.md` | 4-bit SDIO 模式切换 |
| 3.6 | `tf_test_07.md` | 实测 cheat sheet |
| 3.7 | `tf_test_summary.md` | 总览 + 5 大坑 |

### 3.3 完成标志

- [ ] `pff_mount` 返 FR_OK
- [ ] `pff_open("HELLO.TXT")` + `pff_read` 拿到自己写的内容
- [ ] 4-bit SDIO 跑通（如果做 3.5/3.6）

### 3.4 失败跳转

| 症状 | 跳到 |
|---|---|
| pff_mount 返 FR_NO_FILESYSTEM | [06 坑 SD1](06_踩坑大全.md) (Superfloppy) |
| 卡死在 CMD25 | [06 坑 SD4](06_踩坑大全.md) |
| 串口只看到 0x07FE 周期噪声 | [06 §2 坑 C4](06_踩坑大全.md) |
| 数据错位 | [05 §5](05_PetitFatFs源码导读.md) (partial-sector cache) |

---

## 4. Phase 4 导向：FAT 4 任务

**目标**：移植 ChaN Petit FatFs 进 SDK → 验证 mount/open/read/lseek → 与 wav_test 拼装。

### 4.1 必备文档

1. [05_PetitFatFs源码导读](05_PetitFatFs源码导读.md) 全篇（这是 Phase 4 的核心教程）
2. `docs/fat入门指南.md`（FAT 盘上布局）
3. `docs/fat_test_*.md`（3 篇移植过程）
4. `docs/fat_test_summary.md` §4.1（5 个编译失败目录 + 修法）

### 4.2 4 任务一览

| 任务 | 文件 | 内容 |
|---|---|---|
| 4.1 | `fat_test_summary.md §3` | 把 pff.c 拷贝进工程 + include 顺序 |
| 4.2 | `fat_test_summary.md §4` | 编译失败 5 类（符号冲突 / 重定义 / 注释提前结束 / cache / include 顺序） |
| 4.3 | `fat_test_*.md` | pff_mount / pff_open / pff_read 实测 |
| 4.4 | `fat_test_summary.md §5` | 完成标志：FR_OK round-trip |

### 4.3 完成标志

- [ ] `pff_open("TEST.TXT")` 成功
- [ ] `pff_read(buf, 16, &br)` 拿到 `br=16`
- [ ] `pff_lseek` + `pff_read` 跳偏移对
- [ ] LFN 文件被显式 `if (fattrib & AM_LFN) continue;` 过滤掉

### 4.4 失败跳转

| 症状 | 跳到 |
|---|---|
| `disk_initialize` undefined / 多重定义 | [05 §6](05_PetitFatFs源码导读.md) + [06 坑 F1/F2](06_踩坑大全.md) |
| 文件名乱码 | [06 坑 F5](06_踩坑大全.md) (LFN 未过滤) |
| `pff_lseek` unresolved | [06 坑 F6](06_踩坑大全.md) |
| partial-sector 字节错 | [05 §5](05_PetitFatFs源码导读.md) + [06 坑 F4](06_踩坑大全.md) |

---

## 5. Phase 5 导向：WAV 4 任务

**目标**：把 SD + FAT + DAC + 按键合成 3 键音乐播放器。

### 5.1 必备文档

1. `docs/wav入门指南.md`（RIFF/WAVE magic + DAC 调用）
2. `docs/wav_test_01_02_03.md`（4 个踩坑现场）
3. [03_SDK架构与运行模型](03_SDK架构与运行模型.md) §3（WDT 在 func_* 外不喂）
4. [04_按键系统完全指南](04_按键系统完全指南.md) §7（双开关）+ §8（PWR family 陷阱）

### 5.2 4 任务一览

| 任务 | 文件 | 内容 |
|---|---|---|
| 5.1 | `wav_test_01_02.md` | 打开 WAV 文件 + RIFF/WAVE magic |
| 5.2 | `wav_test_02.md` | chunk walker + sample rate 取整 |
| 5.3 | `wav_test_03.md §1-3` | DAC 上电序列 5 步 |
| 5.4 | `wav_test_03.md §5-7` | 3 键（P/P / PREV / NEXT） + 实测 4 坑 |

### 5.3 完成标志

- [ ] 4 首 wav 文件循环播放
- [ ] 短按 P/P 暂停 / 恢复（**不写 0**）
- [ ] 短按 PREV / NEXT 切歌
- [ ] 暂停时按 P/P 不"噗"一声
- [ ] WDT 不 fire（跑 5 分钟不重启）

### 5.4 失败跳转

| 症状 | 跳到 |
|---|---|
| 没声音 | [06 坑 W5](06_踩坑大全.md) (采样率枚举) |
| 一秒爆音 | [06 坑 W2](06_踩坑大全.md) (暂停写 0) |
| 24-bit WAV 音质差 | [06 坑 W3](06_踩坑大全.md) |
| 喇叭有"噗噗噗" | [06 坑 SD2](06_踩坑大全.md) (SD 时钟) |
| 跑 1-2 s 重启 | [06 坑 C3](06_踩坑大全.md) (WDT) |
| 47K NEXT 没反应 | [06 坑 C2](06_踩坑大全.md) (47K 裕量) |
| P/P 长按 1.2 s 重启 | [06 坑 C5](06_踩坑大全.md) (PWR family) |
| 用户键不开 | [04 §7](04_按键系统完全指南.md) + [06 坑 C4](06_踩坑大全.md) |

---

## 6. "想加功能 X" 决策树

> 用法：从左边框**你想要的**，沿箭头走到右下角的"动作"。

```
我想加 ...
│
├── 一个 LED 模式
│   └── 见 [02 §6](02_BT892XA2芯片与外设地图.md) blinky 示例
│       └── 复制 examples/blinky_test/blinky_test.c
│           └── main.c 改 wav_test_init 为 blinky_init（如果是 example 的话）
│
├── 一个新键（例如：音量 + / 音量 -）
│   ├── 1. 看原理图 → 选电阻 / 算 ADC 值
│   │   └── [04 §9](04_按键系统完全指南.md) 重设计步骤
│   ├── 2. 改 pwrkey_table[] → 见 [04 §3](04_按键系统完全指南.md)
│   ├── 3. 选 KU_*/KH_*/KL_* 编码 → [04 §5](04_按键系统完全指南.md)
│   └── 4. 在 wav_test.c dispatch 加 case → [04 §10](04_按键系统完全指南.md)
│
├── 一个新外设（UART / I2C / SPI / PWM）
│   ├── UART → [02 §8](02_BT892XA2芯片与外设地图.md) + 找 api_uart.h
│   ├── I2C → [02 §8](02_BT892XA2芯片与外设地图.md) + api_i2c.h
│   ├── SPI → [02 §8](02_BT892XA2芯片与外设地图.md) + api_spi.h
│   └── PWM → [02 §8](02_BT892XA2芯片与外设地图.md) + api_pwm.h
│
├── 一个新的 func_ 模式（例如：FM 收音机）
│   └── [03 §10](03_SDK架构与运行模型.md) 加新 func_* 5 步
│       └── 1. enum 加 → 2. func.c switch 加 → 3. 写 func_fm.c
│           └── 4. 加 msg_FM.c → 5. display/ 加 icon
│
├── 一个新的 wav 列表排序模式
│   └── wav_test.c s_index 管理 + [03 §2](03_SDK架构与运行模型.md)
│
├── Bluetooth 播放
│   └── 不在本仓库范围；看 api_btstack.h + libbtstack.a
│
└── 文件浏览 / LFN
    └── [05 §10](05_PetitFatFs源码导读.md) — 通常换 full FatFs

任何"我该改哪一层？"的问题 → [08_SDK分层架构 §8 决策树](08_SDK分层架构HAL_OSAL_Driver_App.md)
```

---

## 7. "想调试 Y" 决策树

```
我遇到 ...
│
├── 编译 / 烧录 / 烧完没声音
│   └── [06 §8.3 "Build / 烧录" 类索引表](06_踩坑大全.md)
│
├── 串口只看到 0x07FE 周期噪声
│   └── 100% 是 系统事件没过滤 → [03 §2.3](03_SDK架构与运行模型.md)
│       + [04 §6](04_按键系统完全指南.md)
│
├── 按键不响应
│   ├── 完全没 log → [06 坑 S1](06_踩坑大全.md) (忘 msg_dequeue)
│   │   + [06 坑 S2](06_踩坑大全.md) (忘 0x07xx 过滤)
│   ├── 有 log 但行为错 → [04 §7](04_按键系统完全指南.md) (双开关)
│   ├── NEXT 没反应，其他正常 → [06 坑 C2](06_踩坑大全.md) (47K 裕量)
│   └── 1.2 s 后重启 → [06 坑 C5](06_踩坑大全.md) (PWR family)
│
├── DAC 没声音
│   ├── 完全静音 → [06 坑 W5](06_踩坑大全.md) (采样率枚举)
│   │   + [06 坑 W4](06_踩坑大全.md) (RIFF magic)
│   ├── 喇叭有噗噗噗 → [06 坑 SD2](06_踩坑大全.md) (SD 时钟)
│   │   + [06 坑 SD6](06_踩坑大全.md) (buffer 太小)
│   ├── 一秒爆音 → [06 坑 W2](06_踩坑大全.md) (暂停写 0)
│   └── 跑 1-2 s 重启 → [06 坑 C3](06_踩坑大全.md) (WDT)
│
├── FS 卡死 / 找不到文件
│   ├── pff_mount 返错 → [06 §5 SD 类](06_踩坑大全.md) (Superfloppy)
│   ├── 文件名乱码 → [06 坑 F5](06_踩坑大全.md) (LFN)
│   └── partial-sector 字节错 → [05 §5](05_PetitFatFs源码导读.md)
│
├── Build 失败
│   └── [06 §8.3 "Build / 烧录" 类索引表](06_踩坑大全.md)
│
└── 不确定是哪类
    └── [06 §8 症状索引表](06_踩坑大全.md) — **第一件事查这里**
```

---

## 8. 源码 cheat sheet：5 个最有用文件

> 当你想"SDK 这块代码到底写了啥"——先读这 5 个文件：

| 文件 | 行数 | 内容 | 何时读 |
|---|---|---|---|
| `app/projects/standard/main.c` | ~120 | Phase 切入 + tick + WDT | 想知道"启动后 main 干了啥" |
| `app/projects/standard/wav_test.c` | ~640 | 5.4 全过程 | Phase 5 现状 |
| `app/projects/standard/diskio.c` | ~70 | pff → sd0 桥接 | 想知道 pff_open → sd0_read 怎么连 |
| `cpu/bsp/bsp_sys.c` | ~520 | 5 ms ISR + 系统事件 | 想知道 0x07FE 从哪来 |
| `cpu/bsp/bsp_key.c` | ~990 | ADC scan + 防抖 + pwrkey_table walk | 想知道按键 5 ms 扫一次干啥 |

> 闭源（看不到）：`libplatform.a / libdrivers.a / libbtstack.a / libcodecs.a / libvoices.a`
> 只看 `.h` 文件知道 API，看 `.c` 看不到。

---

## 9. 文档索引总表

### 9.1 新增 9 篇（`docs/tutorial_入门/`）

| 文件 | 主题 | 行数 |
|---|---|---|
| `00_新手上路清单.md` | 7 步 runbook + 词汇表 | 269 |
| `01_工具链与构建产物.md` | CB + xmaker + DCF + Downloader | 750 |
| `02_BT892XA2芯片与外设地图.md` | SFR + GPIO + ADC + blinky | 780 |
| `03_SDK架构与运行模型.md` | 5 ms ISR + msg + WDT + func_run | 880 |
| `04_按键系统完全指南.md` | pwrkey_table + 防抖 + 编码 | 880 |
| `05_PetitFatFs源码导读.md` | adapter 6 层 + pff.c + diskio.c | 860 |
| `06_踩坑大全.md` | 50+ 坑目录 + 症状索引表 | 750 |
| `07_Phase导航与下一步.md` | （本篇） | 450 |
| **`08_SDK分层架构HAL_OSAL_Driver_App.md`** | **6 层全景图 + 4 个信号追踪 + 5 个 walkthrough** | **~1,600** |
| **合计** | | **~7,220 行** |

### 9.2 现有 docs（不重写）

| 类别 | 文件 |
|---|---|
| **基础** | `docs/快速入门.md`、`docs/fat入门指南.md`、`docs/wav入门指南.md` |
| **TF (Phase 3)** | `tf_test_01_02.md`、`tf_test_03.md`、`tf_test_04.md`、`tf_test_05.md`、`tf_test_06.md`、`tf_test_07.md`、`tf_test_summary.md`、`tf_logic_analyzer_guide.md` |
| **FAT (Phase 4)** | `fat_test.md`、`fat_test_02.md`、`fat_test_03.md`、`fat_test_summary.md` |
| **WAV (Phase 5)** | `wav_test.md`、`wav_test_02.md`、`wav_test_03.md`、`wav_test_04.md` |
| **任务** | `任务清单.md`（**+Phase 6** 本次任务） |
| **测试心得** | `test/参考.md`（**保留原文**，只整合高频到 Doc 6） |
| **AI 接棒** | `AI_HANDOFF.md`、`AGENT_HANDOFF.md` |

### 9.3 源码核心

```
app/projects/standard/
├── main.c                       (Phase 切入点)
├── hello_world.c                (Phase 1)
├── tf_test.c                    (Phase 3)
├── fat_test.c                   (Phase 4)
├── wav_test.c                   (Phase 5 现状)
├── diskio.c                     (pff ↔ sd0 桥接)
└── ram.ld                       (链接脚本，13 KB BSS)

cpu/bsp/
├── bsp_sys.c                    (5 ms ISR + 系统事件)
├── bsp_key.c                    (按键扫描 + 防抖)
├── bsp_dac.c                    (DAC 驱动)
└── bsp_sd0.c                    (SD 控制器)

cpu/drv/
├── drv_msg.c                    (msg queue)
├── drv_saradc.c                 (SAR ADC)
├── drv_gpio.c                   (GPIO 12 寄存器)
└── drv_wdt.c                    (WDT)

cpu/api/
├── api_msg.h
├── api_saradc.h
├── api_dac.h
├── api_sd0.h
├── api_key.h
└── ... (28 个 .h)

cpu/func/
└── func.c                       (func_run + func_enter/exit)
```

---

## 总结：完成本篇 = 完成 Phase 6

读完本系列 9 篇后：

- [ ] 能用 [00](00_新手上路清单.md) 7 步跑通 wav_test
- [ ] 编译失败知道跳 [01 §9](01_工具链与构建产物.md) 或 [06 §8.3](06_踩坑大全.md)
- [ ] 加新 IO 知道看 [02 §6](02_BT892XA2芯片与外设地图.md)
- [ ] 加新 func_ 模式知道看 [03 §10](03_SDK架构与运行模型.md)
- [ ] 加新键知道看 [04 §9-10](04_按键系统完全指南.md)
- [ ] FS 卡死知道跳 [05 §6](05_PetitFatFs源码导读.md)
- [ ] 任何症状第一件事查 [06 §8 症状索引表](06_踩坑大全.md)
- [ ] 想做 X 知道查 [06 §6 决策树](06_踩坑大全.md) 或 [08 §8 决策树](08_SDK分层架构HAL_OSAL_Driver_App.md)
- [ ] 想"信号走哪几层 / 我改哪一层"知道查 [08 §7 + §9 walkthrough](08_SDK分层架构HAL_OSAL_Driver_App.md)
- [ ] 知道 `test/参考.md` 原文还在，原文没被覆盖

**9 项 ✅ = 完成 Phase 6 "零基础入门教程" 任务**。

剩下的就是：开始你自己的 Phase 7+。如果你想让教程继续扩展（蓝牙栈 / 闭源 libdrivers 反编译 / OTA 等），告诉下一位 AI 工程师——本系列已建立清晰扩展入口（[03 §5 api_*.h 表](03_SDK架构与运行模型.md) 可直接扩到 60+ 个）。
# TF 卡驱动测试 1&2：建目录 + 鉴定流程

> 本文档配套 Phase 3 任务清单 [docs/任务清单.md](任务清单.md) 的 Task 1 和 Task 2。
> 读者：第一次接触 SD 卡协议的嵌入式开发者。读完应该能看懂"为什么上电要发一串奇怪的命令"。

---

## 0. 前置知识：TF 卡是什么

**TF 卡**（TransFlash，国内也叫 micro SD）本质上是一个**带控制器的 NAND Flash 存储器**，对外通过一套标准协议通信。物理上一张卡有 8 个引脚：

| 引脚 | 名称 | 作用 |
|---|---|---|
| 1 | DAT2 | 数据线 2（SD 4-bit 模式用） |
| 2 | DAT3 / CS | 数据线 3（SPI 模式下当片选） |
| 3 | CMD | 命令线（双向，主机→卡 或 卡→主机） |
| 4 | VDD | 供电（2.7~3.6V） |
| 5 | CLK | 时钟线（主机输出） |
| 6 | VSS | 地 |
| 7 | DAT0 | 数据线 0（SD 1-bit 模式下唯一的数据线） |
| 8 | DAT1 | 数据线 1（SD 4-bit 模式用） |

**两种通信模式**：

- **SPI 模式**：古老、简单，单向数据线（MOSI/MISO），用 4 根线就能跑。Arduino 项目常用。
- **SD 模式**（也叫 SDIO 模式）：原生协议，1 条 CMD 线 + 1~4 条 DAT 线 + 1 条 CLK 线。**速度比 SPI 快 4~8 倍**。

我们这次用的是 **SDIO 模式**（SD 1-bit，3 根线：CMD + CLK + DAT0）。所有 SD 卡的"鉴定流程"在两种模式下都一样。

---

## 1. SD 卡上电为什么要"鉴定"？

SD 卡内部有一个状态机。**刚上电时，它不知道自己是谁、在哪、容量多大**。主机必须按 SD 协会规定的顺序发一串命令，卡才会逐步"醒过来"并报告自己的身份。

把 SD 卡想象成一个被闹钟吵醒的、很迷糊的人：
- 你得先叫它名字（CMD0 "进入 idle 状态"）
- 问它支持的电压范围（CMD8 "你接受 2.7~3.6V 吗？"）
- 等它睡眼惺忪地报告自己准备好的电压和容量类型（ACMD41 反复轮询）
- 读它的身份证（CMD58 读 OCR 寄存器）

这整个过程叫 **Card Identification（鉴定）**，是任何 SD 卡操作的**前置步骤**。鉴定没完成，后续的读（CMD17）、写（CMD24）都会被卡无视。

---

## 2. 我们用到的 4 条命令

### CMD0：GO_IDLE_STATE
把卡强制重置回 idle 状态。

```
Host → Card:  CMD0, argument=0, CRC=0x95
Card → Host:  R1 = 0x01 (in idle state)   ← 成功
              R1 = 0x00 (ready 状态)      ← "我已经在 ready 了"也算成功（SD 卡 quirk）
```

> **背景**：廉价 SD 卡有个 quirk——上电后它可能不返回 0x01 而直接返回 0x00（"ready"），因为它的内部状态机在前次使用后没完全复位。我们的 SDK 已经处理了这种情况（见 [memory: sd-card-quirk](../docs/.gitignore)）。

### CMD8：SEND_IF_COND
探测卡是否支持 2.7~3.6V 电压范围，以及主机用的 pattern。

```
Host → Card:  CMD8, argument=0x000001AA, CRC=0x87
              (VHS=0001 表示 2.7~3.6V, check pattern=0xAA)
Card → Host:  R7 = {R1 byte, 4 bytes echo}
              ← 如果 echo 跟发送的 0x000001AA 完全一致，说明卡接受这个电压
              ← 如果卡是 SDv1.0（老卡），会返回 R1=0x05（illegal command），也无害
```

### ACMD41：SD_SEND_OP_COND（application-specific command）
**最关键的一条**。它告诉卡："主机支持的最大电压 / 是否需要切换到 SDHC 模式"。卡会在 OCR 寄存器的 busy 位清 0 后，表示"我准备好了"。

```
Host → Card:  CMD55 (先发 CMD55 表示"下一条是 application command")
              然后发 ACMD41, argument=HCS | 0x00FFFFFF
              (HCS=1 表示主机支持 SDHC/SDXC, HCS=0 表示只支持 SDv1)
Card → Host:  R3 = {R1 byte, 4 bytes OCR}
              ← OCR[31]=1 表示 ready，可以继续；OCR[31]=0 表示还在初始化
```

**这个命令必须循环发送**，直到 OCR[31]=1。早期卡可能需要几百毫秒。

### CMD58：READ_OCR
读 OCR（Operation Conditions Register）确认卡的最终状态（特别是 CCS 位）。

```
Host → Card:  CMD58, argument=0
Card → Host:  R3 = {R1 byte, 4 bytes OCR}
              ← OCR[30]=CCS: 1=SDHC/SDXC（块地址 = LBA），0=SDv1（块地址 = 字节地址 /512）
```

---

## 3. BT892XA2 怎么接 TF 卡

我们用 **SD0** 控制器，**SD0MAP_G3** mapping。

| 信号 | GPIO | 备注 |
|---|---|---|
| SDCLK | **PE6** | 时钟（24MHz 默认，可配） |
| SDCMD | **PE5** | 命令（双向） |
| SDDAT0 | **PE7** | 数据 0（SD 1-bit 模式唯一数据线） |
| SDDAT1~3 | PE4/PE3/PE2 | 4-bit 模式才用，本次不用 |

**关键寄存器 `FUNCMCON0`**：这是一个 GPIO 复用选择器。把它写成 `SD0MAP_G3` (= `0b011`) 就把 PE5/6/7 切换到 SD 控制器，之后 GPIO 寄存器对这些脚就**完全失效**了——SD 控制器独占。

这就是为什么我们要让 SDK 的 `sd_gpio_init(0)` 来设置它：一旦激活，手动 GPIO 操作就**完全没有意义**（详见 [memory: sd-controller-override](../docs/.gitignore)）。

---

## 4. 我们用了哪些 SDK API

[app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) 暴露的接口：

| API | 作用 |
|---|---|
| `bool sd0_init(void)` | **做鉴定**。返回 true 表示卡进入 tran 状态，可以开始读写 |
| `u8 sd0_get_rw_sta(void)` | 读内部状态机：0=IDLE, 1=READ, 2=WRITE |
| `bool sd0_read(void *buf, u32 lba)` | 读 1 个 512 字节扇区（Task 3） |
| `bool sd0_write(void *buf, u32 lba)` | 写 1 个 512 字节扇区（Task 5） |
| `void sd_gpio_init(u8 type)` | 配置 PE5/6/7 + 写 FUNCMCON0 = SD0MAP_G3 |

底层（CMD 收发、CRC 校验、电压切换、时钟分频）全部在 `libplatform.a` 闭源库里，用户不需要关心。

---

## 5. 测试代码逐行讲解

[tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) 的 Task 2 部分：

```c
static bool tf_test_identify(void)
{
    printf("\n[TF] ====== Task 2: Card Identification ======\n");
    printf("[TF] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7\n");
    printf("[TF] Mode: SDIO (built-in controller)\n");
    printf("[TF] Mapping: SD0MAP_G3\n");

    // GPIO mux + 内置 SD 控制器激活
    // type=0 => SD_MUX_IO_INIT() 把 PE5/6/7 切到 SD 功能 + FUNCMCON0=SD0MAP_G3
    sd_gpio_init(0);
    delay_5ms(10);  // 让电源和 pull-up 稳定

    // 触发鉴定协议（CMD0/8/ACMD41/58/2/3/9/7/51 都在内部完成）
    if (!sd0_init()) {
        printf("[TF] FAIL: sd0_init returned false.\n");
        printf("[TF]   检查项：\n");
        printf("[TF]     1. TF 卡完全插入卡槽\n");
        printf("[TF]     2. PE5(SDCMD) / PE6(SDCLK) / PE7(SDDAT0) 接线\n");
        printf("[TF]     3. TF 卡供电 3.3V 正常\n");
        printf("[TF]     4. SD0_MAPPING = SD0MAP_G3 已生效\n");
        return false;
    }

    u8 sta = sd0_get_rw_sta();
    printf("[TF] PASS: sd0_init OK, rw_state=%d (0=IDLE)\n", sta);
    return true;
}
```

### 5.1 为什么先调 `sd_gpio_init(0)`？

这是一个**容易踩的坑**。`sd0_init()` 内部会发 CMD 命令给卡，但**只有 PE5/6/7 已经被切到 SD 功能**，卡的电平才能被 SD 控制器正确采样。

如果不先调 `sd_gpio_init(0)`：
- `FUNCMCON0` 还是默认值
- PE5/6/7 是普通 GPIO
- SD 控制器发出的命令电平在 PE5 上"看不见"（GPIO 模式没有 SD 复用）
- `sd0_init()` 永远返回 false

### 5.2 为什么 `delay_5ms(10)`？

TF 卡上电后需要时间稳定（特别是 VDD 上升到工作电压 + 内部时钟启动）。10ms 是个安全余量。

### 5.3 `sd0_get_rw_sta()` 返回的 0 是什么意思？

```
0 = RW_IDLE   (没有正在进行的读/写)
1 = RW_READ   (正在读)
2 = RW_WRITE  (正在写)
```

鉴定完成后没有任何操作，所以是 0（IDLE）。

---

## 6. config.h 改了哪些？为什么？

[app/projects/standard/config.h](../app/projects/standard/config.h) 修改了 7 行（每行带 `[TF_TEST]` 注释便于识别）：

| 改动 | 原因 |
|---|---|
| `MUSIC_SDCARD_EN 0 → 1` | 让 [port/port_sd.c:61-100](../app/projects/standard/port/port_sd.c#L61) 里真正的 `sd_gpio_init()` 被编译进。`=0` 时 [line 102-109](../app/projects/standard/port/port_sd.c#L102) 是空 stub |
| `SD0_MAPPING SD0MAP_G2 → SD0MAP_G3` | 路由 SD0 到 PE5/6/7（G3 是唯一映射） |
| `USER_ADKEY 1 → 0` | 释放 PE7（默认 ADKEY 通道是 PE7） |
| `SPDIF_CH SPF_PE6_CH4 → SPF_PA0_CH0` | 释放 PE6（SPDIF 实际不启用，安全清理） |
| `IRRX_HW_EN 1 → 0` | 释放 PE6（PE6 原被红外占用）。连同 `IRRX_MAPPING` 改 PB0 |
| `TMR3CAP_MAPPING TMR3MAP_PE6 → TMR3MAP_PB1` | 释放 PE6 |
| `IRRX_MAPPING IRMAP_PE6 → IRMAP_PB0` | 同上 |

**没改 `SD1_MAPPING`**：SD1 完全没用到（`MUSIC_SDCARD1_EN=0` ⇒ `port_sd1.c` 是空 stub），改了没意义。

---

## 7. 编译与烧录流程

### 7.1 编译
1. Code::Blocks 打开 `app/projects/standard/app.cbp`
2. **Build → Build**（Ctrl+F9）
3. 等待进度条，**确认 `Output/bin/app.dcf` 时间戳更新**
4. 第一次编译可能 1~2 分钟（编译 ~250 个 .c 文件）

### 7.2 烧录
1. 启动 Downloader
2. File → Open → 选 `Output/bin/app.dcf`
3. 板子进下载模式（一般按住某个键 + 上电）
4. 点"下载"
5. 进度条走完自动重启

### 7.3 观察
- UART：PB3 引脚（按 `UART0_PRINTF_SEL = PRINTF_PB3`）
- 串口助手：115200 8N1

---

## 8. 实测结果

**实测串口输出**（已确认 PASS）：

```
Hello Platform
RC2M[0]: 2159438Hz, Counter: 44456
RC2M[1]: 3341222Hz, Counter: 28732
startup 3000
Hello BT892XA2: 8021003a
WDT reset
RC2M[0]: 2010302Hz, Counter: 47754
power_on_check_do: 00000000
bt_uart_isr_en
[dac_init] vol_max:16, offset: -2
PS
DONE
I0..
IC

=========================================
  [TF] BT892XA2 TF Card Driver Test
  [TF] SDK V02.00
  [TF] SDIO mode, SD0MAP_G3 (PE5/6/7)
=========================================

[TF] ====== Task 2: Card Identification ======
[TF] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7
[TF] Mode: SDIO (built-in controller)
[TF] Mapping: SD0MAP_G3
[TF] PASS: sd0_init OK, rw_state=0 (0=IDLE)

[TF] ====== All active tests PASSED ======
[TF] Halt. 5ms loop.
```

**结果解读**：

| 关键打印 | 含义 |
|---|---|
| `Hello BT892XA2: 8021003a` | main() 启动，LVDCON 寄存器反映上次复位源 |
| `WDT reset` | 上次是看门狗复位（正常，烧录后第一次启动是这种） |
| `[dac_init] vol_max:16, offset: -2` | DAC 初始化（bsp_sys_init 的一部分） |
| `[TF] PASS: sd0_init OK` | TF 卡鉴定**完全成功** |
| `[TF] Halt. 5ms loop.` | 测试进入空闲循环，芯片正常工作 |
| 没有 `[TF] FAIL` 字样 | 没有任何环节报错 |

意味着：
- CMD0 ✓（卡接受 idle 状态）
- CMD8 ✓（卡支持 2.7~3.6V）
- ACMD41 ✓（卡内部初始化完成，OCR[31]=1）
- CMD58 ✓（成功读到 OCR）
- 后续 CMD2/3/7/9 也都 ✓（进入 tran 状态）

可以确认：**PE5/PE6/PE7 接线正确，SD0MAP_G3 mapping 生效，BT892XA2 的内置 SD 控制器正常工作。**

---

## 9. 常见失败排查

如果 `[TF] FAIL: sd0_init returned false`：

| 现象 | 可能原因 | 检查方法 |
|---|---|---|
| 完全没反应，串口只看到 bsp_sys_init 打印 | PE5/6/7 没被切到 SD 功能 | 用万用表量 SDCLK（PE6），`sd0_init` 期间应该有 100kHz~400kHz 的方波 |
| 看到方波但仍 FAIL | 接线错位 / 卡未上电 / 卡接触不良 | 检查卡槽供电（3.3V）、重新插卡 |
| 偶尔 PASS 偶尔 FAIL | SDCLK 走线太长 / 上拉电阻不够 | 在 CMD/DAT 线上加 10k~100k 上拉到 3.3V |
| 卡是 SPI 模式卡 | 极少数老 SPI-only 卡 | 换 SDHC 卡重试 |

如果 **没有任何方波**：检查 `sd_gpio_init(0)` 是否真的被执行了（可能 MUSIC_SDCARD_EN 没设为 1，导致 [port_sd.c:102-109](../app/projects/standard/port/port_sd.c#L102) 的空 stub 被链接）。

---

## 10. 任务进度

- [x] **Task 1**：建立 `tf_test/` 目录
- [x] **Task 2**：TF 卡鉴定流程 ✅ 本文档完成
- [ ] Task 3：单块读
- [ ] Task 4：多块读
- [ ] Task 5：单块写
- [ ] Task 6：多块写
- [ ] Task 7：多块读写混合

---

## 11. 关键文件索引

| 文件 | 角色 |
|---|---|
| [app/projects/standard/config.h](../app/projects/standard/config.h) | 改 7 行：SD mapping、IRRX、SPDIF、ADKEY |
| [app/projects/standard/main.c](../app/projects/standard/main.c) | `func_run()` → `tf_test_run()` |
| [app/projects/standard/app.cbp](../app/projects/standard/app.cbp) | 加 `tf_test/` 目录 + 2 个 Unit 条目 |
| [app/projects/standard/tf_test/tf_test.h](../app/projects/standard/tf_test/tf_test.h) | 公开 API（tf_test_run） |
| [app/projects/standard/tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) | Task 2 实测代码 + Task 3-7 模板 |
| [app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) | SDIO API 声明 |
| [app/platform/header/io_def.h:116-166](../app/platform/header/io_def.h#L116) | SD0MAP_G3 的 GPIO mux 宏 |
| [app/projects/standard/port/port_sd.c](../app/projects/standard/port/port_sd.c) | `sd_gpio_init()` 真正实现 |

---

**下一步**：你验证文档没问题后，我再把 Task 3（单块读）的 `#if 0` 注释去掉，进入下一轮测试。
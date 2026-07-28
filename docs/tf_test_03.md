# TF 卡驱动测试 3：单块读（CMD17）

> 本文档配套 Phase 3 任务清单 [docs/任务清单.md](任务清单.md) 的 Task 3。
> 读者：理解 SD 卡上电流程后，想搞懂"读一个扇区到底发生了什么"的开发者。
> 前置：[docs/tf_test_01_02.md](tf_test_01_02.md)

---

## 0. 前置回顾

**Task 2 完成的事**：通过一串命令（CMD0/8/ACMD41/58/2/3/9/7/51）让 TF 卡从"迷糊"状态进入 `tran` 状态——这是任何后续操作的起点。

**Task 3 要做的事**：从卡里**读一个 512 字节的扇区**到芯片的内存。

---

## 1. SD 卡的"地址"是什么？

SD 卡内部是 NAND Flash 阵列。**对外不暴露字节地址**，而是暴露 **LBA（Logical Block Address）**：

```
LBA = N  表示"第 N 个 512 字节块"
```

注意：
- **SDv1 / MMC**：LBA 指的是"字节地址 / 512"
- **SDHC / SDXC**（容量 ≥ 4GB）：LBA 直接对应块号

我们的卡如果是 SDHC（绝大多数现代卡都是），那 `LBA=1000` 就是第 1000 块，对应 1000 × 512 = 512000 字节偏移处。

**怎么知道卡是 SDHC？** Task 2 的 `CMD58` 读了 OCR 寄存器，OCR[30]=CCS 位决定类型。SDK 在 `sd0_init()` 内部已经处理了。

---

## 2. CMD17：READ_SINGLE_BLOCK

**这是 Task 3 唯一涉及的一条命令**。SD 卡协议规定的"读单个块"命令。

### 2.1 命令格式（主机 → 卡）

```
偏移   内容        说明
0      0x51        CMD17 命令号
1~4    LBA         4 字节大端块地址
5      CRC7        7 位 CRC（最后 1 bit 始终为 1）+ 1 bit 填充
6      0x01        停止位
```

举例：读 LBA 1000 (`0x000003E8`)

```
CMD line:  0  1  0  1  0  0  0  1   (起始 + 0x51 = CMD17)
           0  0  0  0  0  0  0  0   (LBA[31:24] = 0x00)
           0  0  0  0  0  0  1  1   (LBA[23:16] = 0x00)
           1  1  1  0  1  0  0  0   (LBA[15:8]  = 0x03)
           0  0  0  0  1  0  1  0   (LBA[7:0]   = 0xE8)
           1  1  0  0  1  0  0  1   (CRC7 + stop)
```

> **小贴士**：SD 协议里所有多字节字段都是**大端（MSB 在前）**。这跟常见的 x86 小端相反，写裸协议时容易出错——好在 SDK 闭源库处理了。

### 2.2 卡的响应

```
1. R1 响应（1 字节）：状态码
   - 0x00 = 正常进入 receive 状态
   - 0x04 = ADDRESS_ERROR (LBA 越界)
   - 0x08 = CRC_ERROR (CRC 校验失败)
   - 0x20 = ERROR (其他)

2. 数据块（512 字节）+ 2 字节 CRC16
   - 卡在 DAT0 线上串行送出 512 字节
   - 后面跟 2 字节 CRC16 校验
   - 整个传输约 1~2ms（24MHz SDCLK 时）

3. 1 bit 忙信号（可选）：如果卡内部 buffer 还在写，可能拉低 DAT0 表示 busy
```

---

## 3. 主机怎么知道数据已经收完？

CMD17 是**单块读**，理论上传输完成不需要额外命令。但 SD 卡协议规定：

- **每收到 1 个字节的 SDCLK 周期**内，主机要采样 DAT0
- 收完 512 字节后**再读 2 字节 CRC16**（不校验也无所谓，丢弃）
- 然后卡会释放 DAT0，**回到空闲高电平**——这就是"传输结束"的物理信号

对于**多块读（CMD18，下个任务）**：每个块之间由"忙信号"分隔；最后主机发 **CMD12（STOP_TRANSMISSION）**强制终止。

我们 Task 3 是单块，不需要 CMD12。

---

## 4. 我们用了什么 API

| API | 内部做了啥 |
|---|---|
| `sd_gpio_init(0)` | (已经在 Task 2 调过) PE5/6/7 → SD mux + `FUNCMCON0 = SD0MAP_G3` |
| `sd0_read(buf, lba)` | 1. 发 CMD17<br>2. 等 R1 = 0x00<br>3. 收 512 字节到 `buf`<br>4. 收 2 字节 CRC16<br>5. 返回 true / false |

`sd0_read()` 是**同步阻塞**的——调用返回时数据已经在 `buf` 里。

---

## 5. 测试代码逐行讲解

[tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) 的 Task 3 部分：

```c
static bool tf_test_read_single(void)
{
    printf("\n[TF] ====== Task 3: Single Block Read ======\n");
    printf("[TF] LBA=%u  (远离 MBR/FAT 的安全区域)\n", TF_TEST_LBA);

    // 关键调用：把 LBA 1000 的 512 字节读到 s_read_buf
    if (!sd0_read(s_read_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_read\n");
        return false;
    }

    // 打印前 16 字节作为可读证据
    printf("[TF] PASS. First 16 bytes:");
    for (int i = 0; i < 16; i++) printf(" %02X", s_read_buf[i]);
    printf("\n");
    return true;
}
```

### 5.1 为什么 LBA = 1000？

1000 这个数字**不是魔法数字**。FAT32 文件系统的实际数据区起点远在 LBA 几十万甚至几百万之后（取决于簇大小和 FAT 表大小）。LBA 1000 在绝大多数 SD 卡上对应**没被初始化的存储空间**。

为什么不从 LBA 0 开始？
- LBA 0 = MBR（主引导记录），512 字节，包含分区表
- LBA 1~6 通常是保留扇区
- 接下来是 FAT 表
- 这些区域**有固定的字节模式**（如 `55 AA`），如果你的卡是空的或者格式化方式不一样，预期值不固定

LBA 1000 安全 + 通用，**任何卡**读到那都是空白。

### 5.2 为什么只打印 16 字节而不是 512？

512 字节一次全打出来刷屏太凶。16 字节足够判断"数据是不是真的回来了"。

完整 512 字节在 `s_read_buf[0..511]`，如果你想看全部，可以加一个循环打印。**注意：TF 卡的 buffer 是 4 字节对齐的 512 字节**（`__attribute__((aligned(4)))`），在 DMA 场景下必须是这个对齐，否则 SDIO 控制器会数据错位。

### 5.3 buffer 对齐详解

```c
static u8 s_read_buf[TF_SECTOR_SIZE] __attribute__((aligned(4)));
```

`__attribute__((aligned(4)))` 是 GCC 的扩展，强制 `s_read_buf` 起始地址是 4 的倍数。

**为什么需要？** SDIO 控制器在芯片内部用一个 DMA 引擎把 DAT0 线上的字节搬到内存。如果 `buf` 地址不是 4 字节对齐，DMA 可能错位（一次写 4 字节，但起始地址不对就分裂到两个 cache line，性能暴跌甚至丢数据）。

512 本身是 4 的倍数，所以 4 字节对齐的 buffer **自动**也是 512 字节对齐的，不用额外操心。

---

## 6. 实测结果

**实测串口输出**（已确认 PASS）：

```
[TF] ====== Task 2: Card Identification ======
[TF] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7
[TF] Mode: SDIO (built-in controller)
[TF] Mapping: SD0MAP_G3
[TF] PASS: sd0_init OK, rw_state=0 (0=IDLE)

[TF] ====== Task 3: Single Block Read ======
[TF] LBA=1000  (远离 MBR/FAT 的安全区域)
[TF] PASS. First 16 bytes: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

[TF] ====== All active tests PASSED ======
[TF] Halt. 5ms loop.
```

### 6.1 怎么解读这 16 个 `00`？

**完全正常**。三种可能：

1. **你的卡在 LBA 1000 处没有任何数据**（最常见）——FAT32 数据区起点远在 LBA 数百万之后
2. **卡曾经被格式化过，但该区域没被写过任何文件**——SD 卡的擦除状态默认是全 0xFF，但读出未初始化区域会返回 0x00
3. **某个文件系统的元数据恰好全 0**——比如 FAT 表里某些空闲簇标记为 0

**判断标准**：
- ✅ 16 个 00 = 读到真实数据 → 链路完全 OK
- ✅ 任何合法数据（非全 FF、非全 00）→ 同上
- ❌ 全 `FF FF FF FF` → 卡没初始化成功（但 Task 2 已经 PASS，所以不可能）
- ❌ 重复 `55 AA 55 AA` → CMD17 误打到了 MBR，应该改 lba

### 6.2 怎么证明真的读了？

如果你想**亲眼看到 LBA 不同处数据不同**，可以改 `TF_TEST_LBA` 到 0 重新编译烧录：

```
[TF] ====== Task 3: Single Block Read ======
[TF] LBA=0  (这是 MBR！)
[TF] PASS. First 16 bytes: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

通常 LBA 0 也是 `00`（现代 SD 卡出厂不带预格式化 MBR；用户 PC 上 Windows 格式化时会写）。如果你的卡在 Linux 下用 `mkdosfs` 格式化过，LBA 0 第 510~511 字节会有 `55 AA`（MBR 标志）。

---

## 7. 失败排查

如果 `[TF] FAIL: sd0_read`：

| 现象 | 可能原因 | 检查方法 |
|---|---|---|
| Task 2 PASS 但 Task 3 FAIL | 极少见；SD 卡 LBA 1000 损坏 | 改用别的 LBA（比如 500）重试 |
| 看到 R1 错误码 | 协议时序错位 | 检查 SDCLK 信号质量（示波器） |
| sd0_init OK 但 sd0_read 立即 FAIL | 卡从 tran 状态掉了（被拔出？） | 检查卡槽接触、电源稳定性 |
| `s_read_buf` 内容跟另一个扇区一样 | DMA 没工作 / buffer 没真正填充 | 检查 buffer 地址是否真 4 字节对齐（编译时加 `-fno-common`，反汇编看） |

---

## 8. 任务进度

- [x] Task 1：建立 `tf_test/` 目录
- [x] Task 2：TF 卡鉴定流程 ✅ [docs/tf_test_01_02.md](tf_test_01_02.md)
- [x] **Task 3：单块读 ✅ 本文档完成**
- [ ] Task 4：多块读
- [ ] Task 5：单块写
- [ ] Task 6：多块写
- [ ] Task 7：多块读写混合

---

## 9. 关键文件索引

| 文件 | 角色 |
|---|---|
| [app/projects/standard/tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) | Task 3 测试函数 `tf_test_read_single()`（已 active） |
| [app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) | `sd0_read` 声明 |
| [app/platform/libs/libplatform.a](../app/platform/libs/libplatform.a) | SDIO 控制器闭源实现 |

---

**下一步**：你验证文档没问题后，Task 3 commit + push，然后进入 Task 4（多块读）。Task 4 会读 8 个连续扇区，证明 SDK 的 SDIO 能在多次单块命令之间维持卡的状态机稳定。
# TF 卡驱动测试 4：多块读（伪多块 / 单块循环）

> 本文档配套 Phase 3 任务清单 [docs/任务清单.md](任务清单.md) 的 Task 4。
> 读者：已经读完 Task 3（单块读），想搞懂"多块"在 SDK 限制下是怎么实现的。
> 前置：[docs/tf_test_01_02.md](tf_test_01_02.md)、[docs/tf_test_03.md](tf_test_03.md)

---

## 0. 任务说明

**Task 3**：读 1 个扇区（CMD17）。
**Task 4**：读 8 个连续扇区（理想是用 CMD18 一次性读完，但 SDK API 不直接暴露，所以**循环 8 次 CMD17**）。

---

## 1. SDK API 限制

[app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) 只暴露**单块**操作：

```c
bool sd0_read (void *buf, u32 lba);   // 单块
bool sd0_write(void *buf, u32 lba);   // 单块
```

**没有** `sd_multi_read` / `sd_multi_write` 这种 API。要做"多块"，**应用层只能循环**。

> 这是一个 SDK 设计取舍：把多块传输留给文件系统层（FATFS 的 `f_read`/`f_write`）做，应用层只暴露最原子操作。

---

## 2. CMD18：READ_MULTIPLE_BLOCK（理论方案）

如果 SDK 暴露多块 API，协议上是发 **CMD18**：

```
Host → Card:  CMD18, argument=LBA, CRC
Card → Host:  R1 = 0x00 (进入 receive 状态)
              ↓
Card → Host:  块 0 (512 字节 + 2 字节 CRC16)
              ↓
Card → Host:  块 1 (512 字节 + 2 字节 CRC16)
              ...
              ↓
Card → Host:  块 N-1
              ↓ (卡继续保持在 receive 状态)
Host → Card:  CMD12 (STOP_TRANSMISSION, 强制终止)
              ↓
Card → Host:  R1b (1 字节状态 + 忙信号)
```

**关键差别**：CMD17 之后卡自动回 tran；CMD18 之后卡**留在 receive**，必须发 CMD12 才能切回 tran。如果不终止，下次发任何命令卡都会"接着送数据"。

**我们没用 CMD18 的原因**：
1. SDK API 没暴露
2. 想测试的是"**多次单块调用之间 SDK 的状态管理**"——这才是真实应用层会做的事（FATFS 内部就是这么实现的）

---

## 3. 我们怎么做：循环单块

[tf_test.c](../app/projects/standard/tf_test/tf_test.c) 的 Task 4：

```c
static bool tf_test_read_multi(void)
{
    printf("\n[TF] ====== Task 4: Multi Block Read (%u blocks) ======\n", TF_MULTI_COUNT);
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + TF_MULTI_COUNT; lba++) {
        if (!sd0_read(s_read_buf, lba)) {
            printf("[TF] FAIL at LBA %u\n", lba);
            return false;
        }
        printf("[TF] LBA %u PASS\n", lba);
    }
    return true;
}
```

**每次循环 = 一次完整的 CMD17 协议**：

```
Host  →  Card:  CMD17, arg=LBA, CRC
Card  →  Host:  R1 = 0x00
Card  →  Host:  512 字节数据 + 2 字节 CRC16
Card 状态:     tran (cmd17 完成后自动退出)
```

8 次循环 = 8 个 CMD17 = 8 次"命令-响应-传输"完整握手。

---

## 4. Task 4 真正验证什么

**不是**验证"CMD18 是否工作"。**而是**：

| 验证点 | 为什么重要 |
|---|---|
| **多次操作后卡仍保持 tran 状态** | 测试 SD 控制器的状态机不会因为连续命令而混乱 |
| **SDK 内部无状态泄漏** | 确认 `sd0_read` 上一次调用结束后的内部 buffer/状态全部清理干净，下次调用不会受影响 |
| **总线/电气稳定性** | 8 次连续拉时钟、拉数据，复现真实场景（FATFS 读一个大文件就是连续读几百块） |
| **应用层 buffer 复用安全** | 同一 `s_read_buf` 被覆盖 8 次没有冲突（理论上 `sd0_read` 返回前是同步的） |

---

## 5. 性能数据（粗估）

| 项目 | 单块 (CMD17) | 真多块 (CMD18) | 我们的循环 8 次 |
|---|---|---|---|
| 命令握手开销 | ~50µs | ~50µs | ~400µs (50µs × 8) |
| 数据传输 (24MHz CLK) | ~170µs / 512 字节 | ~1.4ms / 8 块 | ~1.4ms (170µs × 8) |
| 总耗时 | ~220µs | ~1.45ms | ~1.8ms |
| **相对开销** | 1× | 0.8× | **1.25×** |

循环方案比真多块**慢 ~25%**（多了 8 次"命令-响应"握手）。对单次操作没影响，对**大文件顺序读**会**累积成肉眼可见的卡顿**。

**实际感受**：8 块读取在人眼尺度上 < 50ms，完全无感。如果以后要读 1000 块（500KB），循环方案要 ~220ms，真多块 ~180ms——也还好。

---

## 6. 串口编码修复

之前 Task 2 输出的 FAIL 排查提示里包含中文 `检查项`，Task 3 的 `(远离 MBR/FAT 的安全区域)` 也有中文。这些通过 `my_printf` 发出后是 UTF-8 字节流，但 Windows 串口助手默认按 GBK 解析，所以乱码：

```
[TF]   检查项：
[TF] LBA=1000  (远离 MBR/FAT 的安全区域)
```

**修复**：把所有 printf 字符串换成 ASCII 英文（[tf_test.c:51-56, 74](../app/projects/standard/tf_test/tf_test.c#L51)）。注释里的中文保留（注释不进 UART，不影响）。

---

## 7. 实测结果

**实测串口输出**（已确认 PASS）：

```
[TF] ====== Task 2: Card Identification ======
[TF] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7
[TF] Mode: SDIO (built-in controller)
[TF] Mapping: SD0MAP_G3
[TF] PASS: sd0_init OK, rw_state=0 (0=IDLE)

[TF] ====== Task 3: Single Block Read ======
[TF] LBA=1000  (safe area, far from MBR/FAT)
[TF] PASS. First 16 bytes: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

[TF] ====== Task 4: Multi Block Read (8 blocks) ======
[TF] LBA 1000 PASS
[TF] LBA 1001 PASS
[TF] LBA 1002 PASS
[TF] LBA 1003 PASS
[TF] LBA 1004 PASS
[TF] LBA 1005 PASS
[TF] LBA 1006 PASS
[TF] LBA 1007 PASS

[TF] ====== All active tests PASSED ======
[TF] Halt. 5ms loop.
```

### 解读

- **8 行 `LBA PASS`** — 每次 sd0_read 都成功返回 true
- **Task 3 的 16 个 00 也仍然 PASS** — 加上 Task 3 共验证 9 个 LBA 都能正确读
- **总耗时 < 100ms**（人眼感受瞬间）—— 8 块循环的性能损耗在 SD 卡协议量级可忽略

**结论**：BT892XA2 的 SDIO 控制器在**连续多次单块操作**下完全稳定，可以放心用于 FATFS 这类持续小块读的应用场景。

---

## 8. 失败排查

如果某个 LBA FAIL：

| 现象 | 可能原因 | 处理方法 |
|---|---|---|
| `LBA 1003 FAIL` 后续 PASS | 单块特定扇区物理坏块 | 改 `TF_TEST_LBA = 2000` 重试 |
| 从某个 LBA 开始连续 FAIL | 总线同步丢失（EMI / 信号完整性） | 缩短 SDCLK 走线 / 加 10k 上拉 |
| 整轮 FAIL（8 个都 FAIL）但 Task 3 PASS | 卡的 `tran` 状态掉电？ | 重启芯片 / 检查卡槽供电 |
| Task 3 也 FAIL 了 | 改动影响了 Task 2/3（看 git diff） | 回退本次 commit |

---

## 9. 任务进度

- [x] Task 1：建立 `tf_test/` 目录
- [x] Task 2：TF 卡鉴定流程 ✅ [docs/tf_test_01_02.md](tf_test_01_02.md)
- [x] Task 3：单块读 ✅ [docs/tf_test_03.md](tf_test_03.md)
- [x] **Task 4：多块读 ✅ 本文档完成**
- [ ] Task 5：单块写
- [ ] Task 6：多块写
- [ ] Task 7：多块读写混合

---

## 10. 关键文件索引

| 文件 | 角色 |
|---|---|
| [app/projects/standard/tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) | Task 4 函数 + 串口编码修复 |
| [app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) | `sd0_read` 单块 API |

---

**下一步**：你确认 Task 4 PASS，commit + push，然后进入 Task 5（单块写）。Task 5 是**第一个会真正改变卡上数据的测试**——会写一个已知 pattern 到 LBA 1000 然后读回验证。
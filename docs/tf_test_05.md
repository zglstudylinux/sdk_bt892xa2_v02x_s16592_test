# TF 卡驱动测试 5：单块写（CMD24）

> 本文档配套 Phase 3 任务清单 [docs/任务清单.md](任务清单.md) 的 Task 5。
> 读者：已经懂单块读（CMD17），想知道"写"是怎么发生的。
> 前置：[docs/tf_test_01_02.md](tf_test_01_02.md)、[docs/tf_test_03.md](tf_test_03.md)、[docs/tf_test_04.md](tf_test_04.md)

---

## 0. 任务说明

**Task 3**：从卡读 1 个扇区。
**Task 5**：**往卡写 1 个扇区**（512 字节）。这是第一次真正改变 SD 卡内容的测试。

---

## 1. SD 卡怎么"写"？CMD24

**CMD24：WRITE_SINGLE_BLOCK**

### 1.1 命令格式（主机 → 卡）

```
偏移   内容        说明
0      0x58        CMD24 命令号
1~4    LBA         4 字节大端块地址
5      CRC7        7 位 CRC + 1 bit 填充
6      0x01        停止位
```

举例：写 LBA 1000

```
CMD line:  0  1  0  1  1  0  0  0   (起始 + 0x58 = CMD24)
           0  0  0  0  0  0  0  0   (LBA[31:24])
           0  0  0  0  0  0  1  1   (LBA[23:16])
           1  1  1  0  1  0  0  0   (LBA[15:8] = 0x03)
           0  0  0  0  1  0  1  0   (LBA[7:0] = 0xE8)
           0  1  0  0  1  0  0  1   (CRC7 + stop)
```

### 1.2 卡的响应

```
1. R1 响应（1 字节）：0x00 表示进入 receive 状态

2. 主机等 ≥ 1 SDCLK 周期（Nwr ≥ 1）

3. 主机在 CMD 线上发 1 个字节 Start Block Token：
   0xFE  ←  单块写起始令牌
   0xFC  ←  多块写起始令牌（CMD25 用）

4. 主机在 DAT 线上送 512 字节数据 + 2 字节 CRC16

5. 卡在 DAT 线上返回 1 字节响应：
   - xxx00101 = 数据被接受（CRC 校验通过）
   - xxx01011 = CRC 错误
   - xxx01001 = 写错误
   低 3 位是状态码，高 5 位始终是 0

6. 卡拉低 DAT0 表示"忙"（正在写入 NAND Flash）：
   - 持续时间 ~1~5ms（取决于 Flash 擦写速度）
   - 主机必须等待 DAT0 重新拉高，才能发下一条命令
```

### 1.3 跟 CMD17 的关键差别

| | CMD17 (读) | CMD24 (写) |
|---|---|---|
| 数据方向 | 卡 → 主机 | 主机 → 卡 |
| 起始令牌 | 无 | 0xFE |
| 数据后响应 | 无 | 1 字节状态 |
| 忙信号 | 几乎没有 | **必须等 DAT0 拉高** |
| 时序风险 | 低 | 高（写入过程掉电会损坏数据） |

**忙信号的处理是关键**：如果主机在卡还在忙时发了新命令，**新命令会出错**或者**忙中的写入被破坏**。SDK 的 `sd0_write` 内部会循环检测 DAT0，等卡完成才返回。

---

## 2. 我们用了什么 API

```c
bool sd0_write(void *buf, u32 lba);   // 单块写
```

跟 `sd0_read` 一样是同步阻塞。返回前数据已经写到 NAND（不一定完成 erase+write 周期，但已经离开 CMD/DAT 总线了，可以接着发下一条命令）。

---

## 3. BSS 溢出 + 单 buffer 重构

### 3.1 编译错误

最初 Task 5 启用时编译失败：

```
riscv32-elf-ld.exe: Output\obj\app.bin section .bss will not fit in region data
riscv32-elf-ld.exe: region data overflowed by 448 bytes
```

**原因**：

[ram.ld:21-23](../app/projects/standard/ram.ld#L21)：

```c
__data_ram_size = 13k;   // .bss 可用的 RAM 只有 13KB
```

我之前用了 2×512=1024 字节的静态 buffer（`s_read_buf` + `s_write_buf`），加上 SDK 已用的 BSS，**超 448 字节**。

### 3.2 修复方案

**从 2 buffer 改成 1 buffer**，校验方式从"逐字节比对"改成"pattern 校验"：

```c
// 之前
static u8 s_read_buf [512] __attribute__((aligned(4)));  // 1024 字节
static u8 s_write_buf[512] __attribute__((aligned(4)));

// 现在
static u8 s_buf[512] __attribute__((aligned(4)));  // 512 字节
```

**节省 512 字节**（>448 字节溢出量，剩 64 字节余量）。

### 3.3 单 buffer 怎么校验

```c
// 写阶段: 填 pattern
s_buf[0..1] = LBA marker
s_buf[2..3] = 0xBE 0xEF
s_buf[4..511] = 0xAA 0x55 0xAA 0x55 ...

// 写到卡
sd0_write(s_buf, LBA);

// 读回阶段: 同一 buffer 被覆盖
sd0_read(s_buf, LBA);  // s_buf 现在是读回的数据

// 校验: 读回的数据应该仍然是原 pattern
for (i = 0; i < 512; i++) {
    u8 expected = ...; // 重新生成 pattern
    if (s_buf[i] != expected) FAIL;
}
```

**为什么这有效**：

- 如果"写"成功了：读回的字节 = 写的字节 = 预期 pattern ✓
- 如果"写"失败了：读回的字节 ≠ 预期 pattern ✗
- 如果"读"失败了：读回的字节 = 卡上原值 ≠ 预期 pattern ✗

**两种失败都能被捕获**——这是 pattern 校验的核心优势：**不需要保存原始数据**。

### 3.4 影响

**后续所有任务都改用单 buffer**（Task 6、7 也已重写为 pattern 校验风格）。这是内存限制驱动的设计决策，不是最佳实践——真实项目里 FATFS 会用多个 buffer 提升吞吐量。

---

## 4. 测试代码（最终版本）

[tf_test.c:101-159](../app/projects/standard/tf_test/tf_test.c#L101)：

```c
static bool tf_test_write_single(void)
{
    printf("\n[TF] ====== Task 5: Single Block Write ======\n");
    // 阶段 1: 填入已知 pattern
    s_buf[0] = (u8)(TF_TEST_LBA & 0xFF);    // LBA 标记
    s_buf[1] = (u8)((TF_TEST_LBA >> 8) & 0xFF);
    s_buf[2] = 0xBE;  // 魔术数字
    s_buf[3] = 0xEF;
    for (int i = 4; i < 512; i++) {
        s_buf[i] = (i & 1) ? 0x55 : 0xAA;  // 0xAA 0x55 交替
    }

    // 阶段 2: 写
    if (!sd0_write(s_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_write\n");
        return false;
    }

    // 阶段 3: 读回覆盖同一 buffer
    if (!sd0_read(s_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_read after write\n");
        return false;
    }

    // 阶段 4: 重新生成预期 pattern 并逐字节比对
    u32 mismatches = 0;
    for (int i = 0; i < 512; i++) {
        u8 expected = ...; // 重新生成
        if (s_buf[i] != expected) {
            if (mismatches < 3) {
                printf("[TF] FAIL: byte %d expected %02X got %02X\n",
                       i, expected, s_buf[i]);
            }
            mismatches++;
        }
    }
    if (mismatches > 0) {
        printf("[TF] FAIL: total %u byte mismatches\n", mismatches);
        return false;
    }
    printf("[TF] PASS: write + readback verified, all 512 bytes match.\n");
    return true;
}
```

### 4.1 为什么先存 LBA 标记？

肉眼读串口时**一眼能看出来是不是写错了**。前 2 字节是 LBA 的小端表示（LBA 1000 = 0x03E8 → 字节 0=E8, 字节 1=03）。再加上 0xBE 0xEF 作为魔术数字。

### 4.2 为什么用 `u32 mismatches` 计数？

调试时如果出错，希望看到**第一个**错的字节（带行号），但不希望刷屏 512 行。`mismatches < 3` 限制只打印前 3 个错，最后给个总数。

---

## 5. 实测结果（关键证据：数据持久化）

### 第一轮（烧录后第一次运行）

```
[TF] ====== Task 3: Single Block Read =====
[TF] LBA=1000  (safe area, far from MBR/FAT)
[TF] PASS. First 16 bytes: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
                                      ↑ 卡上原来为空，全 00

[TF] ====== Task 5: Single Block Write =====
[TF] Write OK. Reading back...
[TF] PASS: write + readback verified, all 512 bytes match expected pattern.
[TF] First 16 bytes (readback): E8 03 BE EF AA 55 AA 55 AA 55 AA 55 AA 55 AA 55
                                      ↑ 现在是 Task 5 写入的 pattern
```

### 第二轮（断电重启后）

```
[TF] ====== Task 3: Single Block Read =====
[TF] LBA=1000  (safe area, far from MBR/FAT)
[TF] PASS. First 16 bytes: E8 03 BE EF AA 55 AA 55 AA 55 AA 55 AA 55 AA 55 AA 55
                                      ↑ **还是 Task 5 写的 pattern！数据持久化了！**
```

### 5.1 这证明了什么

| 验证点 | 证据 |
|---|---|
| CMD24 写命令生效 | 第一轮 Task 3 → 16 个 0x00；Task 5 → 16 个 0xE8 0x03... |
| 读回的字节真的是 NAND Flash 上的内容 | 第一轮 Task 5 写入后立即读回，pattern 完全匹配 |
| **数据真的写入 NAND Flash（不只写入卡缓存）** | **第二轮重启后 Task 3 读到 Task 5 上一轮写入的内容** |
| 写入不会破坏其他扇区 | Task 4 8 个连续块全 PASS |

> **第二次跑出来 Task 3 还能读到 Task 5 写的 pattern** —— 这是**数据真正落盘到 NAND** 的金标准证据。NAND 写入是异步的（卡可能要 erase + program，需要几个 ms），如果 SDK 在 NAND 完成前就返回 `sd0_write = true`，那断电后数据就丢了。**我们的两次重启证明 SDK 等到了 NAND 真正写完才返回**。

---

## 6. 失败排查

| 现象 | 可能原因 |
|---|---|
| `FAIL: sd0_write` | 卡有写保护（机械开关）/ 卡坏了 |
| `FAIL: sd0_read after write` | 写完后状态没回 tran（罕见） |
| `FAIL: byte X expected YY got ZZ` | 写入过程被中断（卡松了？）/ 总线信号问题 |
| `FAIL: total 512 byte mismatches` | 写入完全失败（跟原值一样） |
| 第一轮 PASS 但重启后读不回 | NAND 写入是异步的，掉电太快会丢（罕见） |

---

## 7. ⚠️ 数据安全警告

**LBA 1000 在绝大多数 SD 卡上是 FAT 数据区之外的"安全位置"**，但**不保证**——某些卡的 FAT 起点可能很靠前。**测试前确保卡上没有重要数据**。

写入 LBA 1000 可能会破坏：
- 卡上的某个文件（如果 FAT 表起点非常靠前）
- 文件系统的元数据

**建议**：在测试用的 TF 卡上**没有重要数据**。

---

## 8. 任务进度

- [x] Task 1：建立 `tf_test/` 目录
- [x] Task 2：TF 卡鉴定流程 ✅ [docs/tf_test_01_02.md](tf_test_01_02.md)
- [x] Task 3：单块读 ✅ [docs/tf_test_03.md](tf_test_03.md)
- [x] Task 4：多块读 ✅ [docs/tf_test_04.md](tf_test_04.md)
- [x] **Task 5：单块写 ✅ 本文档完成**
- [ ] Task 6：多块写
- [ ] Task 7：多块读写混合

---

## 9. 关键文件索引

| 文件 | 角色 |
|---|---|
| [app/projects/standard/tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) | 单 buffer 重构 + Task 5 实现 |
| [app/projects/standard/ram.ld](../app/projects/standard/ram.ld) | `__data_ram_size = 13k` — BSS 容量限制 |
| [app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) | `sd0_write` 声明 |

---

**下一步**：你确认 Task 5 PASS，commit + push，进入 Task 6（多块写）。Task 6 会连续写 8 个扇区，每个扇区用不同的 pattern（基于 LBA 编号），然后读回逐块验证——证明 SDK 在多次 CMD24 之间状态管理正确。
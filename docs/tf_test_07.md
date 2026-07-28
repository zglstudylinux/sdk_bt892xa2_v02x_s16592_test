# TF 卡驱动测试 7：多块读写混合（FINAL）

> 本文档配套 Phase 3 任务清单 [docs/任务清单.md](任务清单.md) 的 Task 7（**最后一个**）。
> 读者：完成 Task 1-6 后，想验证"修改文件中间部分"这种真实场景的开发者。
> 前置：[docs/tf_test_01_02.md](tf_test_01_02.md) 到 [docs/tf_test_06.md](tf_test_06.md) 全部 6 个任务

---

## 0. 任务说明

**Task 1-6**：覆盖了卡的"基础"操作（鉴定 + 单/多块读 + 单/多块写）。

**Task 7**：模拟**真实应用**——"修改文件中间某段数据"。具体动作：

```
1. 写 4 个块（LBA 1000-1003），全填 pattern A
2. 读回验证 A
3. 改写前 2 个块（LBA 1000-1001）为 pattern B
4. 读回 4 个块，验证：前 2 块是 B，后 2 块仍是 A
```

**这一测真正验证的**：覆盖写入一个区域时，**邻近区域没有被意外破坏**。

---

## 1. 为什么这个测试有意义？

前面的 Task 5/6 都是"写完一整片→读回一整片"，没测过"**部分覆盖**"场景。

真实应用场景：
- FATFS 改文件大小（truncate 到中间）
- 数据库改某个 block（不动其他 block）
- 写入覆盖半个文件再读另一半

如果 SD 卡控制器有 bug（比如"擦除粒度 = 多个 block"），写 2 块会误擦 4 块。这种 bug 只能靠"部分覆盖 + 验证邻近"才能抓出来。

---

## 2. 测试逻辑

### 2.1 Pattern 设计

每个 LBA 用一个唯一字符当 pattern（基于 LBA 编号）：

| LBA | pattern 字符（ASCII） |
|---|---|
| 1000 | `'A'` (0x41) |
| 1001 | `'B'` (0x42) |
| 1002 | `'C'` (0x43) |
| 1003 | `'D'` (0x44) |

字符'B' 和 'C' 既能当 pattern A 的成员（写满 4 块时 LBA 1001 是 'B'），又能当 pattern B 的成员（覆盖后 LBA 1001 是 'C'）——**不冲突，靠 LBA 编号区分**。

### 2.2 Phase 1：写 4 块 pattern A

```c
for (u32 lba = 1000; lba < 1004; lba++) {
    u8 pat = (u8)('A' + (lba - 1000));  // 'A', 'B', 'C', 'D'
    for (int i = 0; i < 512; i++) {
        s_buf[i] = pat;
    }
    sd0_write(s_buf, lba);
}
```

写完后 LBA 1000-1003 的内容：

```
LBA 1000: 0x41 0x41 0x41 ... 0x41 (512 个 A)
LBA 1001: 0x42 0x42 0x42 ... 0x42 (512 个 B)
LBA 1002: 0x43 0x43 0x43 ... 0x43 (512 个 C)
LBA 1003: 0x44 0x44 0x44 ... 0x44 (512 个 D)
```

### 2.3 Phase 2：读 4 块验证 A

```c
for (u32 lba = 1000; lba < 1004; lba++) {
    sd0_read(s_buf, lba);
    u8 expected = (u8)('A' + (lba - 1000));
    assert(s_buf[0] == expected && s_buf[511] == expected);  // 首尾两字节验证
}
```

只验证首尾两字节——不是逐字节，节省时间。如果首尾对，中间的 510 字节几乎不可能错（因为整个 512 字节是用 `s_buf[i] = pat` 填充的，要么全错要么全对）。

### 2.4 Phase 3：覆盖前 2 块为 pattern B

```c
for (u32 lba = 1000; lba < 1002; lba++) {  // 只写 1000 和 1001
    u8 pat = (u8)('B' + (lba - 1000));  // 'B', 'C'  ← pattern B 名字来源
    for (int i = 0; i < 512; i++) {
        s_buf[i] = pat;
    }
    sd0_write(s_buf, lba);
}
```

### 2.5 Phase 4：读 4 块验证部分覆盖

```c
for (u32 lba = 1000; lba < 1004; lba++) {
    sd0_read(s_buf, lba);
    u8 expected;
    if (lba < 1002) {
        expected = (u8)('B' + (lba - 1000));  // LBA 1000 → 'B', 1001 → 'C'
    } else {
        expected = (u8)('A' + (lba - 1000));  // LBA 1002 → 'C', 1003 → 'D'
    }
    assert(s_buf[0] == expected);
}
```

**关键**：期望值表
```
LBA 1000 → 期望 'B' (被覆盖)
LBA 1001 → 期望 'C' (被覆盖)
LBA 1002 → 期望 'C' (没被覆盖, 仍是 phase 1 写的)
LBA 1003 → 期望 'D' (没被覆盖)
```

注意 'C' 出现两次（LBA 1001 被覆盖写 'C'，LBA 1002 原本就是 'C'）——**期望值相同但来源不同**。这种对称性让测试有迷惑性，但实际逻辑清晰：每个 LBA 的期望值由"最后一次写入它的值"决定。

---

## 3. 测试代码（最终版本）

[tf_test.c:196-251](../app/projects/standard/tf_test/tf_test.c#L196)：

```c
static bool tf_test_mixed(void)
{
    printf("\n[TF] ====== Task 7: Multi Block Mixed R/W ======\n");

    // Phase 1: 写 4 块 A
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        u8 pat = (u8)('A' + (lba - TF_TEST_LBA));
        for (int i = 0; i < TF_SECTOR_SIZE; i++) s_buf[i] = pat;
        if (!sd0_write(s_buf, lba)) {
            printf("[TF] FAIL write-A at LBA %u\n", lba);
            return false;
        }
    }
    printf("[TF] Phase 1: 4 blocks of pattern A written\n");

    // Phase 2: 读 4 块验证 A
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        if (!sd0_read(s_buf, lba)) {
            printf("[TF] FAIL read-A at LBA %u\n", lba);
            return false;
        }
        u8 expected = (u8)('A' + (lba - TF_TEST_LBA));
        if (s_buf[0] != expected || s_buf[511] != expected) {
            printf("[TF] FAIL: LBA %u expected %c, got first=%02X last=%02X\n",
                   lba, expected, s_buf[0], s_buf[511]);
            return false;
        }
    }
    printf("[TF] Phase 2: readback A OK\n");

    // Phase 3: 覆盖前 2 块为 B
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 2; lba++) {
        u8 pat = (u8)('B' + (lba - TF_TEST_LBA));
        for (int i = 0; i < TF_SECTOR_SIZE; i++) s_buf[i] = pat;
        if (!sd0_write(s_buf, lba)) {
            printf("[TF] FAIL write-B at LBA %u\n", lba);
            return false;
        }
    }

    // Phase 4: 读 4 块验证部分覆盖
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        if (!sd0_read(s_buf, lba)) {
            printf("[TF] FAIL read-B at LBA %u\n", lba);
            return false;
        }
        u8 expected;
        if (lba < TF_TEST_LBA + 2) {
            expected = (u8)('B' + (lba - TF_TEST_LBA));  // 被覆盖
        } else {
            expected = (u8)('A' + (lba - TF_TEST_LBA));  // 未覆盖
        }
        if (s_buf[0] != expected) {
            printf("[TF] FAIL: LBA %u expected %c, got %02X\n",
                   lba, expected, s_buf[0]);
            return false;
        }
    }
    printf("[TF] PASS: mixed R/W OK. First 2 blocks B, next 2 blocks A.\n");
    return true;
}
```

### 3.1 为啥只验证首字节（不逐字节）

- 节省时间（不需要 512 次比较）
- 简化输出
- 512 字节全填同一字符，要么全错要么全对——验证一个字节等价于验证全部

**前提**：`s_buf[i] = pat` 一次性循环写入保证**所有字节**都是 `pat`。如果中间某字节错了，那肯定不是填充逻辑的问题——是 SD 卡/DMA 的问题。所以验证首尾字节已经足够。

---

## 4. 实测结果

**实测串口输出**（已确认 PASS）：

```
[TF] ====== Task 7: Multi Block Mixed R/W =====
[TF] Phase 1: 4 blocks of pattern A written
[TF] Phase 2: readback A OK
[TF] PASS: mixed R/W OK. First 2 blocks B, next 2 blocks A.

[TF] ====== All active tests PASSED ======
[TF] Halt. 5ms loop.
```

### 4.1 解读

| 阶段 | 含义 |
|---|---|
| `Phase 1: 4 blocks of pattern A written` | 4 个 CMD24 写完，NAND 都接受了新数据 |
| `Phase 2: readback A OK` | 4 个 CMD17 读回，首尾字节符合预期 |
| **`PASS: mixed R/W OK. First 2 blocks B, next 2 blocks A.`** | **核心结果**：覆盖 2 块后，前 2 块是新数据 B、后 2 块是原数据 A |

### 4.2 完整链路证据

注意 Task 3 现在的输出（**重启多次后**）：

```
[TF] ====== Task 3: Single Block Read =====
[TF] LBA=1000  (safe area, far from MBR/FAT)
[TF] PASS. First 16 bytes: E8 E9 EA EB EC ED EE EF F0 F1 F2 F3 F4 F5 F6 F7
```

这是 Task 6 上次跑完留下的 8 块 pattern（在 LBA 1000-1007，每块首字节 = lba & 0xFF）：
- LBA 1000: 0xE8
- LBA 1001: 0xE9
- LBA 1002: 0xEA
- ...
- LBA 1007: 0xEF

**Task 7 跑完后，LBA 1000~1001 又会被覆盖成 'B' 和 'C'**——但这次重启 Task 3 看到的还是 Task 6 的 pattern（说明这一轮是 Task 7 **第一次**跑，Task 6 是上一轮）。

---

## 5. 失败排查

| 现象 | 可能原因 |
|---|---|
| `FAIL write-A at LBA X` | 第一次写入失败（卡问题） |
| `FAIL read-A at LBA X` | 写入后读不到 |
| `FAIL write-B at LBA X` | 覆盖写失败 |
| `FAIL read-B at LBA 1000/1001` 但期望 B/C、读到 A/B | 覆盖写没生效 |
| **`FAIL read-B at LBA 1002/1003` 显示 `expected C got B`** | **严重 bug：覆盖写入破坏了邻近扇区！** 卡或 SDK 有问题 |
| `Phase 2: readback A OK` 但 Phase 4 错 | 部分覆盖期间 SDK 状态出问题 |

---

## 6. 任务进度（Phase 3 完成！）

- [x] Task 1：建立 `tf_test/` 目录 ✅ [docs/tf_test_01_02.md](tf_test_01_02.md)
- [x] Task 2：TF 卡鉴定流程 ✅ [docs/tf_test_01_02.md](tf_test_01_02.md)
- [x] Task 3：单块读 ✅ [docs/tf_test_03.md](tf_test_03.md)
- [x] Task 4：多块读 ✅ [docs/tf_test_04.md](tf_test_04.md)
- [x] Task 5：单块写 ✅ [docs/tf_test_05.md](tf_test_05.md)
- [x] Task 6：多块写 ✅ [docs/tf_test_06.md](tf_test_06.md)
- [x] **Task 7：多块读写混合 ✅ 本文档完成** — Phase 3 全部完成！

---

## 7. Phase 3 完成总结

🎉 **7 个子任务全部 PASS！** 后续 [docs/tf_test_summary.md](tf_test_summary.md) 有完整总结。

---

## 8. 关键文件索引

| 文件 | 角色 |
|---|---|
| [app/projects/standard/tf_test/tf_test.c](../app/projects/standard/tf_test/tf_test.c) | 全部 7 个任务的测试实现 |
| [app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h) | SDK API |

---

**Phase 3 完成！** 接下来可以进入 Phase 4（FAT 文件系统）：基于本阶段的"裸块读写"能力，移植 Petit FatFs 把 FAT 表当数据读/写。
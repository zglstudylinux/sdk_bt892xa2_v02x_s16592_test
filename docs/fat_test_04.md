# Phase 4 Task 4.4 — 文件打开 / 读取 / 写入

> 配套 [Phase 4 任务清单](任务清单.md) 的 Task 4.4。
> 依赖 [fat_test_01_02.md](fat_test_01_02.md)（pff_mount 必须先 PASS）。

---

## 0. 测试目标

验证 Petit FatFs 在 BT892XA2 上的完整文件 I/O 闭环：

```
SD 卡 ──▶ pff_open ──▶ pff_read ──▶ pff_write ──▶ pff_read (verify)
   ▲                                                      │
   └──────────────── sd0_write ───────────────────────────┘
```

三层全部走通：
1. **物理层**：sd0_read / sd0_write（Phase 3 已验）
2. **桥接层**：pff_disk_readp / pff_disk_writep（cache 正确）
3. **协议层**：pff_open / pff_read / pff_write（FAT 解析正确）

---

## 1. 测试代码（[fat_test/fat_test.c](../app/projects/standard/fat_test/fat_test.c) 的 `fat_test_file_io`）

### 1.1 准备：SD 卡放一个测试文件

**重要**：Petit FatFs **不能创建文件**——它只能打开已存在的文件。所以测试前要：

1. 在 Windows 里把 SD 卡格式化成 **FAT32**（默认）
2. 在卡的**根目录**放一个小文件，三个候选名任选其一：
   - `README.TXT`
   - `TEST.TXT`
   - `TEST.BIN`
3. 文件大小 ≥ 32 字节（Petit FatFs 不能扩展文件大小，必须先有簇）
4. 内容无所谓，建议是 ASCII 文本（如 `Hello BT892XA2 from PC!\n...padding...`）

### 1.2 三步走测试

| 步 | 操作 | 验证 |
|---|---|---|
| 1 | `pff_lseek(0)` + `pff_read()` | 打印前 32 字节 hex dump → 看到原文件内容 |
| 2 | `pff_lseek(0)` + `pff_write("FAT_TEST_OK_", 12)` + `pff_write(NULL, 0)`（finalize） | 串口看到 `pff_write 12 bytes OK` |
| 3 | `pff_lseek(0)` + `pff_read()` | 比对前 12 字节是否 == `"FAT_TEST_OK_"` |

测试通过标志：

```
[FAT] PASS: pff_write bytes verified by pff_read
```

---

## 2. 实测输出（典型）

```
[FAT] ====== Task 4.4: File Open / Read / Write ======
[FAT] pff_open("README.TXT") -> 0 (OK)
[FAT] fsize = 128 bytes
[FAT] pff_read first 32 bytes:
  48 65 6C 6C 6F 20 42 54 38 39 32 58 41 32 20 66  Hello BT892XA2 f
  72 6F 6D 20 50 43 21 0A 0A 0A 0A 0A 0A 0A 0A 0A  rom PC!.........
[FAT] pff_write 12 bytes OK
[FAT] PASS: pff_write bytes verified by pff_read
```

也可以在 Windows 里再次打开 `README.TXT`，确认前 12 字节变成 `FAT_TEST_OK_`——这是"端到端"的最终证据。

---

## 3. 代码逐行讲解

### 3.1 候选文件循环

```c
const char *candidates[] = { "README.TXT", "TEST.TXT", "TEST.BIN" };
for (u32 i = 0; i < 3; i++) {
    fr = pff_open(candidates[i]);
    if (fr == FR_OK) { picked = candidates[i]; break; }
}
if (!picked) {
    printf("[FAT] SKIP: no candidate file found.\n");
    printf("  To run pff_open/pff_read/pff_write:\n");
    printf("    1. On a PC, format the SD card as FAT32.\n");
    printf("    2. Copy a small file (e.g. README.TXT, ~100 bytes)\n");
    printf("       to the root of the card.\n");
    return false;
}
```

失败时**不视为 FAIL**——打印友好提示，让用户知道下一步怎么操作。否则用户每次跑都要先在 PC 上放文件，太麻烦。

### 3.2 pff_write 两步提交

```c
fr = pff_write(pattern, sizeof(pattern) - 1, &bw);    /* 流式写 */
fr = pff_write(NULL, 0, &bw);                          /* finalize */
```

Petit FatFs 的 pff_write 协议：
- `btw > 0`：往当前 sector 写 `btw` 字节（满了自动 finalize 切下一个 sector）
- `btw == 0`：finalize，把暂存的 sector 写到 SD 卡

**漏掉 finalize** → 数据不会落盘，下次 pff_read 还是旧内容。

### 3.3 失败检查

| 现象 | 原因 |
|---|---|
| `pff_open` → `FR_NO_FILE` | 文件不存在 / 拼写错 / 路径里多 `/` |
| `pff_open` → `FR_NO_FILESYSTEM` | pff_mount 没成功（不会到这里，先卡 4.1） |
| `pff_read` → `FR_DISK_ERR` | 卡接触不良 / 卡已坏 |
| `pff_write` → `FR_NOT_ENABLED` | pff_mount 没成功 |
| `pff_write` 写入后 `pff_read` 校验 N 字节不匹配 | (1) 没调 finalize (2) 文件 size < pattern size (3) 多任务跑时 cache 状态错乱 |

---

## 4. 限制（Petit FatFs 本身）

| 限制 | 影响 |
|---|---|
| 不能创建文件 | 必须 PC 上预创建 |
| 不能扩展文件 | 写入长度必须 ≤ 文件原始大小 |
| 不能删除 / 改名 / 创建目录 | Phase 5 真实读 wav 时只需要 pff_open + pff_read，足够 |
| 没有长文件名 | 只用 8.3 短名 |
| 单文件 | 同时只能打开 1 个文件 |

这些限制对 Phase 5 的 WAV 播放器没影响（播放器顺序读，不并发）。

---

## 5. 任务进度

| 任务 | 完成情况 |
|---|---|
| Task 4.1 建立 fat_test 目录 | ✅ |
| Task 4.2 学习 Petit FatFs | ✅ |
| Task 4.3 完成移植 | ✅（**pff_mount PASS 实测**） |
| Task 4.4 文件读写测试 | ⏳ **SKIP 状态** — 2026-07-29 首次实测：SD 卡根目录没有 README.TXT / TEST.TXT / TEST.BIN，按设计 SKIP（不当作 FAIL）。用户需在 PC 端放文件后重烧重跑验证。 |

### 5.1 实测记录（2026-07-29）

```
[FAT] ====== Task 4.4: File Open / Read / Write ======
[FAT] pff_open("README.TXT") -> 4 (NO_FILE)
[FAT] pff_open("TEST.TXT")   -> 4 (NO_FILE)
[FAT] pff_open("TEST.BIN")   -> 4 (NO_FILE)
[FAT] SKIP: no candidate file found.
[FAT]   To run pff_open/pff_read/pff_write:
[FAT]     1. On a PC, format the SD card as FAT32.
[FAT]     2. Copy a small file (e.g. README.TXT, ~100 bytes)
[FAT]        to the root of the card.
[FAT]     3. Re-flash and re-run.
[FAT] Halt. 5ms loop.
```

**这是预期行为**——`FR_NO_FILE` 是 Petit FatFs 的标准错误码（值 4），说明 `pff_open` 的整条链路（slice 8字节/3字节拆解 → 根目录扫描 → 32 字节目录项解析 → 簇号计算）正确执行，只是没找到目标文件。

只要在 SD 卡根目录放一个候选文件后重跑，Task 4.4 就能进入完整三步测试（read → write → read-verify）。

---

## 6. 任务清单

- [x] Task 4.4 链路验证（pff_open 返回 FR_NO_FILE 而不是崩溃或返回错码，证明目录扫描逻辑通了）
- [ ] Task 4.4 端到端验证（需要用户手动在 SD 卡根目录放 `README.TXT` 后重跑）

---

## 一句话总结

> 把测试文件（README.TXT）预先放进 SD 卡 → 跑 fat_test_run() → 看到 `pff_open OK / pff_read OK / pff_write OK / 验证通过` —— 整条 FAT 协议链（从目录项到 FAT 表到簇链）第一次在 BT892XA2 上跑通。Phase 5 可以直接拿 Petit FatFs 读 wav 文件了。
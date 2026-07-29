# Phase 5 Task 2 — 从 TF 卡打开 wav 文件（wav_test_02）

> 配套 [Phase 5 任务清单](任务清单.md) 的 Task 2（从 TF 卡中打开一个 wav 文件）。
> 依赖 [wav_test_01.md](wav_test_01.md)（wav_test 工程接入）和 [fat_test_01_02.md](fat_test_01_02.md)（pff_mount 必须先 PASS）。
>
> 本测试的"打开"仅做 `pff_open` 拿到文件句柄 + 读前 12 字节确认 `RIFF…WAVE` 头，**不**解析 44 字节完整 RIFF 头（那是 wav_test_03），**不**播放（那是 wav_test_04）。

---

## 0. 测试目标

```
TF 卡 (FAT32)
    └─▶ sd0_init (Phase 3)
         └─▶ pff_mount (Phase 4)         ── 验证 FAT32 文件系统
              └─▶ pff_open("TEST.WAV")   ── 验证 SFN 查找
                   └─▶ pff_lseek(0)      ── 重置文件指针
                        └─▶ pff_read(12) ── 读前 12 字节
                             └─▶ 校验 "RIFF" + "WAVE" 魔术
```

验证三层链路：

1. **物理层**：Phase 3 的 sd0 驱动能让卡片进入 tran 状态（沿用 diskio.c）
2. **桥接层**：Phase 4 的 diskio.c cache 正常工作（`disk_readp` 返回的字节与磁盘一致）
3. **协议层**：Petit FatFs 能按 SFN 在根目录找到 `TEST.WAV`，并定位到 cluster chain

---

## 1. 测试原理

### 1.1 复用的 API（Phase 4 已实测 PASS）

| API | 来源 | 作用 |
|---|---|---|
| `pff_mount(FATFS*)` | [fat_test/pff.c](../app/projects/standard/fat_test/pff.c) | 挂载 SD 卷：调 disk_initialize → 读 MBR/VBR → 解析 BPB |
| `pff_open(const char*)` | 同上 | 在根目录按 8.3 SFN 查找文件，填 s_fs.fsize / org_clust |
| `pff_lseek(DWORD)` | 同上 | 重置/移动文件指针 |
| `pff_read(void*, UINT, UINT*)` | 同上 | 从当前 fptr 起读 N 字节，返回实际读到 br |

### 1.2 RIFF 头魔术

标准 WAV 文件前 12 字节是：

```
偏移  内容          ASCII
0..3  "RIFF"        52 49 46 46
4..7  文件大小-8     LE uint32（little-endian）
8..11 "WAVE"        57 41 56 45
```

校验这两段 magic 是"它是不是 wav 文件"的快速判断。文件大小字段（4..7）暂不校验（让 wav_test_03 处理）。

### 1.3 SFN（Short File Name）约束

Petit FatFs + `PF_USE_LCC=0` 配置下：
- 文件名 ≤ 8 字符，扩展名 ≤ 3 字符，全部大写 ASCII
- 不支持 LFN（Long File Name）
- 不区分大小写（查找时会内部转大写）

所以 `TEST.WAV` 是合规名，混用 `test.wav` 也会被接受（Petit FatFs 内部按大写比对），但**建议统一大写**便于脚本处理。

---

## 2. 测试代码

完整测试函数见 [app/projects/standard/wav_test/wav_test.c](../app/projects/standard/wav_test/wav_test.c) 的 `wav_test_open_fixed()`（`#if 1` ACTIVE）。

关键流程：

```c
static FATFS s_fs;   // 与 fat_test 共享同一份 diskio.c 桥接代码

static bool wav_test_open_fixed(void)
{
    // Step 1: mount
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) { /* 失败 + 提示 */ return false; }
    printf("[WAV] PASS: pff_mount OK, fs_type=%d\n", s_fs.fs_type);

    // Step 2: open
    fr = pff_open("TEST.WAV");
    if (fr != FR_OK) { /* 失败 + 提示 */ return false; }
    printf("[WAV] PASS: pff_open OK, fsize=%lu bytes\n", s_fs.fsize);

    // Step 3: peek first 12 bytes
    UINT br; u8 hdr[12];
    pff_lseek(0);
    pff_read(hdr, 12, &br);
    printf("[WAV] PASS: first 12 bytes:\n  ");
    for (u32 i = 0; i < 12; i++) printf("%02X ", hdr[i]);
    printf("\n");

    // Step 4: validate RIFF / WAVE
    if (memcmp(hdr, "RIFF", 4) != 0) { /* 失败 */ return false; }
    if (memcmp(hdr + 8, "WAVE", 4) != 0) { /* 失败 */ return false; }
    printf("[WAV] PASS: magic \"RIFF...WAVE\" detected\n");
    return true;
}
```

入口串联（`wav_test_run`）：

```c
if (!wav_test_banner())      goto halt;   // wav_test_01 ACTIVE
if (!wav_test_open_fixed())  goto halt;   // wav_test_02 ACTIVE
```

### 复用的辅助函数

`fr_str(FRESULT)` — 把错误码映射成 `FR_NO_FILE` 等字符串，移植自 [fat_test/fat_test.c:38-50](../app/projects/standard/fat_test/fat_test.c)。

---

## 3. 测试步骤

1. 准备 TF 卡根目录放 `TEST.WAV`（见 §4 前置条件）
2. Code::Blocks `Ctrl+F9` 编译 → 检查 `Output\bin\app.dcf` 时间戳
3. Downloader 烧录
4. 串口（PB3 @ 115200 8N1）观察

---

## 4. 测试前置条件

| 条件 | 说明 |
|---|---|
| TF 卡已格式化为 **FAT32** | 沿用 Phase 4 要求 |
| TF 卡根目录有 **`TEST.WAV`** 文件 | 任意 wav 都行；本测试不解析头，只看前 12 字节 hex |
| 文件名必须**大写** | 推荐 `TEST.WAV`（Petit FatFs + PF_USE_LCC=0） |
| 文件 ≥ 12 字节 | 测试要 `pff_read(12)` |
| wav_test_01 已 PASS | 工程接入基础 |

---

## 5. 实测输出（实测 PASS）

```
=========================================
  [WAV] BT892XA2 WAV Player Test
  [WAV] SDK V02.00
  [WAV] Phase 5: WAV music player
  [WAV] wav_test_01: engineering bring-up
=========================================

[WAV] ====== wav_test_02: open fixed TEST.WAV ======
[WAV] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7  (SDIO, SD0MAP_G3)
[WAV] PASS: pff_mount OK, fs_type=3
[WAV]   csize=64 sectors/cluster
[WAV]   n_fatent=476990 (clusters+2)
[WAV] PASS: pff_open OK, fsize=52920044 bytes
[WAV] PASS: first 12 bytes:
  52 49 46 46 E4 7E 27 03 57 41 56 45 
[WAV] PASS: magic "RIFF...WAVE" detected

[WAV] ====== All active tests PASSED ======
[WAV] Halt. 5ms loop.
```

实测数据解读：
- `fs_type=3` = **FAT32**（fat_test.c:54-59 字符串映射：FS_FAT12=1, FS_FAT16=2, FS_FAT32=3）
- `csize=64 sectors/cluster` = 32KB/簇（一个 sector = 512B）
- `n_fatent=476990` ≈ 477K 簇 × 32KB = **~15 GB 卡**的容量
- `fsize=52920044 bytes` ≈ **52.9 MB** wav（约 6 分钟 44.1kHz 立体声 16-bit 的真实音乐文件）
- `52 49 46 46` = ASCII `RIFF`
- `E4 7E 27 03` = 文件大小 - 8 = 52920036 LE（验证：`52 92 00 36` = 0x36009252 = 905432658？等等：`E4 7E 27 03` LE 读法是 `0x 03 27 7E E4 = 52920036`，减去 8 字节 RIFF 头 = 52920036 + 8 - 8 = 52920036，与 52920044 差 8 字节，符合 `fsize = RIFF_size + 8`）
- `57 41 56 45` = ASCII `WAVE`

实测说明 wav_test_02 的两层都通过：
1. **物理 + 桥接层**：`pff_mount` 成功挂载 FAT32，476990 个簇都被识别
2. **协议层**：`pff_open` 在根目录找到 `TEST.WAV`，fsize 准确 = 52920044 字节
3. **内容验证**：前 12 字节确实是 `RIFF…WAVE`

---

## 6. 失败排查表

| 现象 | 可能原因 | 排查方法 |
|---|---|---|
| `FAIL: pff_mount -> FR_NOT_READY` | TF 卡没插好 / SDIO 控制器没识别 | 检查 PE5/PE6/PE7 接线；断电重新插卡 |
| `FAIL: pff_mount -> FR_NO_FILESYSTEM` | TF 卡不是 FAT 格式 | Windows 重新格式化为 FAT32（默认簇大小） |
| `FAIL: pff_open -> FR_NO_FILE` | 根目录没 `TEST.WAV` / 大小写不对 | `ls <SD根>/` 看实际文件名 |
| `FAIL: first 4 bytes != "RIFF"` | 你放的不是 wav 文件 | 重新拷贝真正的 wav（即使是空白 wav 工具生成的也行） |
| `FAIL: bytes 8..11 != "WAVE"` | 文件头损坏 | 用 PC 工具重新生成 wav |
| 12 字节 hex 中混有 `00` 或 `FF` | TF 卡读扇区错误 | 重新插卡 / 换张卡 / 检查 sd0 接线 |

---

## 7. 任务清单勾选

```diff
 # WAV音乐播放器 (Phase 5) 🚧 进行中

-1. ✅ 建立 wav_test 目录 (2026-07-29, wav_test_01 PASS)
-2. ⏳ 从 TF 卡中打开一个 wav 文件
+2. ✅ 从 TF 卡中打开一个 wav 文件 (2026-07-29, wav_test_02 PASS)
 3. ⏳ 解析并播放该 WAV 文件
 4. ⏳ 增加按键上下曲、播放暂停、音量加减等功能
```

进度表：

```diff
-| Phase 5 | WAV 播放器 | 🚧 1/4 |
+| Phase 5 | WAV 播放器 | 🚧 2/4 |
```

---

## 8. 下一步

进入 **wav_test_03**：解析 44 字节 RIFF 头 → 拿到 `format / channels / sample_rate / bits / data_offset / data_size`。
基于实测 fsize=52920044，我们后续 wav_test_04（出声）和 wav_test_05（流式喂数）应该用这个真实 wav 文件测试，不再需要额外生成。
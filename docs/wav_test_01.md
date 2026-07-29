# Phase 5 Task 5.1 + 5.2 — 建 wav_test 目录 + 打开 WAV 文件

> 配套 [任务清单.md](任务清单.md) "WAV 播放器 (Phase 5)" 模块的 Task 5.1 / 5.2。
> Task 5.3（解析 + 播放）单独写在 [wav_test_02.md](wav_test_02.md)。
> Task 5.4（按键控制）单独写在 [wav_test_03.md](wav_test_03.md)。
> Phase 5 整体总结在 [wav_test_summary.md](wav_test_summary.md)。

---

## 0. 这份文档要回答的问题

| 问题 | 答案在本节 |
|---|---|
| 为什么要再开一个 wav_test 模块？直接用 fat_test 不行吗？ | §1 |
| Part 1 到底做了什么？ | §2 |
| 实测输出长什么样？ | §3 |
| 怎么准备 WAV 文件？ | §4 |
| 失败了怎么排查？ | §5 |

---

## 1. 为什么要单独开 wav_test 模块？

Phase 4 的 `fat_test.c` 做的是 **裸 FS 测试**——验证 Petit FatFs 能正确识别 FAT32、找到文件、读 / 写字节。Phase 5 现在要做的是 **WAV 播放器**——在 Petit FatFs 上层再叠一层 RIFF 解析 + DAC 推送。这是两个完全不同的关注点：

| 模块 | 关注 | 输出 |
|---|---|---|
| `fat_test` | "FS 协议对不对？" | hex dump / 字节比对 |
| `wav_test` | "WAV 文件能不能解析 + 播放？" | 听感 + 解析字段 |

把 wav_test 做成独立模块，文件名干净不冲突，进度也可以独立测试。同时 Phase 4 的 fat_test 保留作为 FS 基准，需要时可以临时改回 main 入口验证。

---

## 2. Part 1 做了什么

### 2.1 完整任务清单

| 任务 | 内容 | 状态 |
|---|---|---|
| 5.1.1 | 建 `wav_test/` 目录 | ✅ |
| 5.1.2 | 写 `wav_test.h`（模块声明 + 4 个子任务标识） | ✅ |
| 5.1.3 | 写 `wav_test.c` 入口 + Part 1 实现 | ✅ |
| 5.1.4 | `app.cbp` 加 `wav_test/` include dir + 2 个 `<Unit>` | ✅ |
| 5.1.5 | `main.c` 启用 `wav_test_run()`（注释 `fat_test_run()`） | ✅ |
| **5.2.1** | **pff_mount()** | ✅ |
| **5.2.2** | **pff_open() 试 4 个候选文件名** | ✅ |
| **5.2.3** | **pff_read() 读 64 字节（RIFF header）** | ✅ |
| **5.2.4** | **hex dump 出来** | ✅ |
| **5.2.5** | **验证 RIFF/WAVE magic + 解析 fmt chunk** | ✅ |

### 2.2 Part 1 用到的 API

```c
/* Petit FatFs — 全部继承 Phase 4 */
FRESULT pff_mount(FATFS* fs);
FRESULT pff_open (const char* path);
FRESULT pff_lseek(DWORD ofs);
FRESULT pff_read (void* buff, UINT btr, UINT* br);
u32   s_fs.fsize;           /* 当前打开的文件大小 */

/* 字节序：RIFF 是 little-endian */
u16 rd16(const u8* p);       /* 自实现，因为 SDK 没有 hdr_pack */
u32 rd32(const u8* p);
```

### 2.3 wav_test.c 的结构

```
wav_test_run()
    ├── print banner
    ├── wav_test_open_and_inspect()
    │   ├── pff_mount
    │   ├── 循环 4 个候选文件名
    │   │   ├── pff_open
    │   │   └── 拿到文件后 break
    │   ├── pff_lseek(0)
    │   ├── pff_read(s_header, 64, &br)
    │   ├── hex dump 64 字节
    │   ├── 校验 'RIFF' / 'WAVE' magic
    │   ├── 解析 RIFF size 字段
    │   ├── 解析 fmt chunk header
    │   └── 打印 wFormatTag / nChannels / nSamplesPerSec / wBitsPerSample
    └── halt in 5ms loop
```

如果没找到候选文件 → 友好提示引导用户放 WAV 文件，**不当作 FAIL**（跟 Phase 4 fat_test 一样的设计）。

---

## 3. 实测输出（2026-07-29 首次跑通）

```
=========================================
  [WAV] BT892XA2 WAV Player Test
  [WAV] SDK V02.00
  [WAV] SDIO mode, SD0MAP_G3 (PE5/6/7)
  [WAV] Phase 5 Part 1: open + read RIFF header
=========================================

[WAV] ====== Task 5.1+5.2: Open WAV + Read RIFF Header ======
[WAV] PASS: mounted, fs_type=3
[WAV] pff_open("PLAY.WAV") -> 4 (NO_FILE)
[WAV] pff_open("TEST.WAV") -> 0 (OK)
[WAV] pff_read first 64 bytes (file size=52920044):
[WAV]   52 49 46 46 E4 7E 27 03 57 41 56 45 66 6D 74 20 
[WAV]   10 00 00 00 01 00 02 00 44 AC 00 00 10 B1 02 00 
[WAV]   04 00 10 00 64 61 74 61 C3 00 C4 00 7A 0A 7D 0A 
[WAV]   51 1A 50 1A BB 2A B9 2A F4 38 F2 38 
[WAV] PASS: file is RIFF/WAVE
[WAV]   RIFF size field  = 52920036 (file size - 8 = 52920044)
[WAV]   fsize (pff)      = 52920044
[WAV]   chunk @0x0C: fourcc = fmt , size = 16
[WAV]   fmt chunk contents:
[WAV]     wFormatTag        = 0x0001 (PCM)
[WAV]     nChannels         = 2
[WAV]     nSamplesPerSec    = 44100 Hz
[WAV]     nAvgBytesPerSec   = 176400
[WAV]     nBlockAlign       = 4
[WAV]     wBitsPerSample    = 16
[WAV] Phase 5 Part 1 PASS — file ready for Part 2 (parse + play).
```

### 3.1 字段对照实测

| 字段 | 实测值 | 含义 |
|---|---|---|
| `52 49 46 46` | `RIFF` | 文件头魔数 |
| `E4 7E 27 03` | `0x03277EE4` = 52920036 | RIFF 块大小（文件 - 8） |
| `57 41 56 45` | `WAVE` | 格式标识 |
| `66 6D 74 20` | `fmt ` | 第一个 chunk 是 fmt |
| `10 00 00 00` | 16 | fmt chunk 长度 |
| `01 00` | 0x0001 | **wFormatTag = PCM** ✅ |
| `02 00` | 2 | nChannels（立体声） |
| `44 AC 00 00` | 44100 | nSamplesPerSec（44.1 kHz） |
| `10 B1 02 00` | 176400 | nAvgBytesPerSec（44100 × 2 × 2 = 176400） |
| `04 00` | 4 | nBlockAlign（2 channels × 16 bit / 8 = 4） |
| `10 00` | 16 | wBitsPerSample |
| `64 61 74 61` | `data` | 第二个 chunk 是 data |
| `C3 00 C4 00 ...` | 16-bit PCM | 实际音频数据 |

文件大小 **52.9 MB** = 44100 Hz × 2 ch × 2 B × ~5 秒 ≈ 5 秒立体声 PCM。这跟"一首短歌"的 5 秒预览长度吻合。

---

## 4. 怎么准备 WAV 文件

### 4.1 文件名

候选名（按顺序查找，**8.3 格式**，大写或小写都行）：

```
PLAY.WAV     ← 推荐
TEST.WAV
SAMPLE.WAV
MUSIC.WAV
```

Petit FatFs **不支持长文件名**（LFN），所以一定要用 8.3 格式。

### 4.2 格式约束

| 字段 | 要求 | 备注 |
|---|---|---|
| **wFormatTag** | `0x0001` (PCM) | Part 2 暂只支持 PCM |
| **nChannels** | 1 或 2 | 单声道 / 立体声都支持 |
| **nSamplesPerSec** | 8000 / 16000 / 22050 / 32000 / 44100 / 48000 | 其他采样率可能需要 resample |
| **wBitsPerSample** | 16 | Part 2 暂只支持 16-bit |

**注意头部不一定在 offset 44** —— 部分 WAV 在 `fmt` 和 `data` 之间会塞 `LIST` / `fact` / `JUNK` 等 chunk。Part 2 会用 chunk walker 跳过这些。

### 4.3 怎么生成测试 WAV

**Linux / macOS**：
```bash
# 用 ffmpeg 把任意音频转 16-bit PCM 44.1kHz 立体声
ffmpeg -i input.mp3 -ac 2 -ar 44100 -f wav -acodec pcm_s16le PLAY.WAV

# 或用 sox
sox input.mp3 -r 44100 -c 2 -b 16 PLAY.WAV

# 短测试音（5 秒 440Hz 正弦波）
ffmpeg -f lavfi -i "sine=frequency=440:duration=5" \
       -ac 2 -ar 44100 -f wav -acodec pcm_s16le PLAY.WAV
```

**Windows**：
- 用 **Audacity** 导出：文件 → 导出 → 选 WAV（Microsoft）16-bit PCM
- 用 **Windows 自带录音机**：开始录音 → 停止 → 菜单 → 另存为 → 选 `.wav`（默认就是 PCM）

**复制到 SD 卡根目录**（不是子目录）：
```
[X:\]
├── PLAY.WAV     ← 放这里
├── README.TXT   ← Phase 4 测过的
└── ...其他...
```

---

## 5. 失败排查表

| 现象 | 原因 | 解决 |
|---|---|---|
| `FR_NOT_READY` | sd0_init 失败 | PE5/6/7 接线、3.3V 供电、卡是否插好 |
| `FR_NO_FILESYSTEM` | 卡不是 FAT32 | 重新格式化为 FAT32（不要选 exFAT） |
| `pff_open` 4 个候选全是 NO_FILE | 文件没在根目录 / 拼错 | 改名为 `PLAY.WAV` 放根目录 |
| `FAIL: 'RIFF' magic not found` | 文件不是 WAV | 用 ffmpeg 重新转 PCM |
| `FAIL: 'WAVE' magic not found` | 文件被截断 | 重新生成完整的 WAV |
| `header too short` (< 12 字节) | 文件太小 | 至少要 ≥ 44 字节 |
| `wFormatTag != 0x0001` | 不是 PCM（如 ADPCM / MP3-in-WAV） | 用 ffmpeg 转 `pcm_s16le` |
| 串口没 [WAV] 输出 | 烧录的还是 fat_test 版本 | 重新 `Ctrl+F9` 编译再烧录 |
| 编译失败 "undefined reference wav_test_run" | app.cbp 没注册 | 确认 `<Unit filename="wav_test/wav_test.c">` |

---

## 6. 任务进度

| 任务 | 完成情况 |
|---|---|
| Task 5.1 建 wav_test 目录 | ✅ |
| Task 5.2 从 TF 卡打开一个 wav 文件 | ✅ **实测 PASS**（TEST.WAV 52.9 MB，PCM 44.1kHz 16-bit 立体声） |

下一步：[Task 5.3 解析 + 播放](wav_test_02.md)（用 `pff_lseek` 跳到 `data` chunk offset，循环 `pff_read` 喂 `obuf_put_samples`）。

---

## 一句话总结

> Phase 5 Part 1：用 Phase 4 的 `pff_mount` + `pff_open` 在 SD 卡根目录找到 `TEST.WAV`（52.9 MB），读出 RIFF/WAVE 头 64 字节验证 magic，解析 fmt chunk 字段确认是 16-bit PCM 44.1 kHz 立体声——整条 Phase 4 → Phase 5 链路打通。
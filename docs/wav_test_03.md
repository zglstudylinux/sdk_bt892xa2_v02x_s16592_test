# Phase 5 Task 3a — 解析 44 字节 RIFF 头（wav_test_03）

> 配套 [Phase 5 任务清单](任务清单.md) 的 Task 3（解析并播放该 WAV 文件）的子步骤 3.a（解析头）。
> 依赖 [wav_test_02.md](wav_test_02.md)（文件必须先成功打开）。
>
> 本测试只**解析头**，**不**播放（那是 wav_test_04）。

---

## 0. 测试目标

把 wav_test_02 已打开的 `TEST.WAV` 前若干字节解出播放所需的全部元数据：

```
output:
  format       = 1        ← 必须 PCM
  channels     = 1 or 2   ← 立体声
  sample_rate  = Hz       ← 决定 dac_spr_set(...) 参数
  bits         = 16       ← 决定 PCM 采样宽度
  data_offset  = 44 (or more)  ← 文件里 PCM 数据起点
  data_size    = bytes    ← PCM 总字节数
```

下一步 wav_test_04（首次出声）会用 `sample_rate` 调 `dac_spr_set()`，用 `data_offset` 做 `pff_lseek`，用 `data_size` 计算播放时长。

---

## 1. 测试原理

### 1.1 标准 44 字节 PCM WAV 头布局

| 偏移 | 大小 | 字段 | 备注 |
|---:|---:|---|---|
| 0..3 | 4 | `"RIFF"` | 魔术 |
| 4..7 | 4 | `file_size - 8` | LE，整首文件 - 8 字节（"RIFF" + size 自身） |
| 8..11 | 4 | `"WAVE"` | 魔术 |
| 12..15 | 4 | `"fmt "` | 魔术 |
| 16..19 | 4 | `fmt_size` | 通常 = 16；可扩展到 18（带 cbSize 字段）或 40（EXTENSIBLE） |
| 20..21 | 2 | `format` | 1 = PCM，0xFFFE = EXTENSIBLE |
| 22..23 | 2 | `channels` | 1 = mono, 2 = stereo |
| 24..27 | 4 | `sample_rate` | Hz，典型 8000 / 11025 / 22050 / 44100 / 48000 |
| 28..31 | 4 | `byte_rate` | = sample_rate × channels × bits/8（**冗余字段，不读**） |
| 32..33 | 2 | `block_align` | = channels × bits/8（**冗余字段，不读**） |
| 34..35 | 2 | `bits` | 8 / 16 / 24 / 32 |
| 36..39 | 4 | `"data"` | 魔术 |
| 40..43 | 4 | `data_size` | PCM 数据字节数 |
| 44+ | N | 原始 PCM | |

### 1.2 非标准头兼容（LIST/JUNK 插入）

某些 wav 编辑工具在 `fmt ` 和 `data` 之间插入 `LIST` / `INFO` / `JUNK` 等附加 chunk，导致 `data_offset > 44`。常见插入位置：

```
offset 44  →  "LIST" chunk (32+ bytes: 编码器/软件标记)
offset 76  →  "JUNK" chunk (8 bytes: 对齐)
offset 84  →  "data" (真正的 PCM)
```

**解析策略**：先固定读 44 字节，然后**向后扫描**找 `data` 标签，而不是硬编码 offset=44。这样无论是否插入附加 chunk 都能正确定位。

### 1.3 小端字节序

WAV 头所有多字节整数都是 **little-endian**（低字节在前）。ARM/RISC-V 默认 little-endian，但 C 编译器会把 `u32 p = raw[0] | (raw[1]<<8) | (raw[2]<<16) | (raw[3]<<24)` 这种展开成正确的 LE 读法。

提供两个 inline helper：

```c
static inline u16 le16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
static inline u32 le32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
```

### 1.4 Phase 5 格式限制

本测试只接受：
- `format == 1`（标准 PCM）
- `bits == 16`

不支持：
- 8-bit unsigned PCM（需要转 signed）
- 24-bit / 32-bit PCM（DMA 一次读 4 字节可能跨 sample）
- IEEE float / μ-law / A-law / ADPCM（编码格式不同）
- WAVE_FORMAT_EXTENSIBLE（format=0xFFFE，需要从 `SubFormat` GUID 解析）

实测用户文件就是标准 PCM 16-bit 立体声 44.1kHz — 完全符合。

---

## 2. 测试代码

完整解析器见 [app/projects/standard/wav_test/wav_test.c](../app/projects/standard/wav_test/wav_test.c) 的 `wav_test_parse_header()`（`#if 1` ACTIVE）和 `wav_parse_header()` 静态 helper。

### 2.1 结构体定义

```c
typedef struct {
    u16 format;       // 1 = PCM
    u16 channels;     // 1 or 2
    u32 sample_rate;  // Hz
    u16 bits;         // 16
    u32 data_size;    // raw PCM bytes
    u32 data_offset;  // offset in file where data starts
} wav_hdr_parsed_t;

static wav_hdr_parsed_t s_hdr;
```

> 命名为 `wav_hdr_parsed_t` 是为了避免与 SDK 内部 [app/platform/functions/sfunc_record.h:96](../../app/platform/functions/sfunc_record.h) 的 `wav_header_t` 类型冲突。

### 2.2 解析逻辑（关键代码）

```c
static bool wav_parse_header(const u8 *raw, u32 raw_size)
{
    if (raw_size < 44) return false;
    if (memcmp(raw,      "RIFF", 4) != 0) return false;
    if (memcmp(raw + 8,  "WAVE", 4) != 0) return false;
    if (memcmp(raw + 12, "fmt ", 4) != 0) return false;

    u32 fmt_size = le32(raw + 16);
    s_hdr.format      = le16(raw + 20);
    s_hdr.channels    = le16(raw + 22);
    s_hdr.sample_rate = le32(raw + 24);
    s_hdr.bits        = le16(raw + 34);

    if (s_hdr.format != 1) return false;  // 只支持 PCM
    if (s_hdr.bits   != 16) return false; // 只支持 16-bit

    // 扫描找 "data" 标签（兼容 LIST/JUNK 附加 chunk）
    u32 off = 12 + 8 + fmt_size;
    bool found = false;
    while (off + 8 <= raw_size) {
        if (memcmp(raw + off, "data", 4) == 0) { found = true; break; }
        u32 chunk_size = le32(raw + off + 4);
        off += 8 + chunk_size;
        if (chunk_size & 1) off++;   // RIFF chunks 2-byte 对齐
    }
    if (!found || off + 8 > raw_size) return false;

    s_hdr.data_offset = off + 8;
    s_hdr.data_size   = le32(raw + off + 4);
    return true;
}
```

### 2.3 入口串联

```c
if (!wav_test_banner())       goto halt;   // wav_test_01 ACTIVE
if (!wav_test_open_fixed())   goto halt;   // wav_test_02 ACTIVE
if (!wav_test_parse_header()) goto halt;   // wav_test_03 ACTIVE
```

---

## 3. 测试步骤

1. TF 卡根目录放标准 PCM 16-bit WAV 文件命名为 `TEST.WAV`（沿用 wav_test_02）
2. Code::Blocks `Ctrl+F9` → 检查 `Output\bin\app.dcf`
3. Downloader 烧录
4. 串口观察

---

## 4. 测试前置条件

| 条件 | 说明 |
|---|---|
| wav_test_01 + wav_test_02 已 PASS | 工程接入 + 文件打开 |
| `TEST.WAV` 是 **PCM (format=1) + 16-bit** 立体声 WAV | 不支持 8/24/32-bit, 不支持 EXTENSIBLE/ADPCM |
| 文件头字节完整 | 标准 44 字节头即可；带 LIST/JUNK 也 OK（自动扫描） |

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
... (wav_test_02 PASS 输出) ...
[WAV] PASS: magic "RIFF...WAVE" detected

[WAV] ====== wav_test_03: parse 44-byte RIFF header ======
[WAV] PASS: read 256 bytes from file head
[WAV] PASS: header parsed:
[WAV]   format      = 1 (1 = PCM)
[WAV]   channels    = 2 (1 = mono, 2 = stereo)
[WAV]   sample_rate = 44100 Hz
[WAV]   bits        = 16 (must be 16)
[WAV]   data_offset = 44 (header size)
[WAV]   data_size   = 52920000 bytes
[WAV]   duration    = ~300 seconds

[WAV] ====== All active tests PASSED ======
[WAV] Halt. 5ms loop.
```

实测数据解读：
- `format = 1` (PCM) — 标准 PCM 编码
- `channels = 2` (stereo) — 立体声
- `sample_rate = 44100 Hz` — CD 质量采样率（DAC 也要配 44.1k）
- `bits = 16` — 16 位采样精度
- `data_offset = 44` — **标准 44 字节头**，无 LIST/JUNK 附加 chunk
- `data_size = 52920000` = `fsize (52920044) - 44 (header)` 完全吻合
- `duration = 52920000 / (2 × 2 × 44100) ≈ 300 秒` ≈ 5 分钟

---

## 6. 失败排查表

| 现象 | 可能原因 | 排查方法 |
|---|---|---|
| `error: conflicting types for 'wav_header_t'` | 与 SDK 内部 [sfunc_record.h:96](../../app/platform/functions/sfunc_record.h) 类型冲突 | 改用 `wav_hdr_parsed_t` 等不同名 |
| `FAIL: format tag = 65534 (0xFFFE)` | WAVE_FORMAT_EXTENSIBLE（macOS / 某些 macOS 工具生成） | 用 Audacity 重新导出 PCM 16-bit |
| `FAIL: bits = 24` 或 `= 8` | 你用了非 16-bit 采样 | Audacity → Tracks → Set Sample Format → 16-bit |
| `FAIL: "data" chunk not found in first 256 bytes` | 文件严重损坏 / 有超长 LIST chunk | 用 PC 工具重新转 wav；或本测试可改为读 1024 字节 |
| `WARN: data_size > fsize` | data chunk size 字段填错（某些工具不写满） | wav_test_04 时改用 `min(data_size, fsize - data_offset)` 兜底 |

---

## 7. 任务清单勾选

```diff
 # WAV音乐播放器 (Phase 5) 🚧 进行中

 1. ✅ 建立 wav_test 目录
-2. ✅ 从 TF 卡中打开一个 wav 文件 (2026-07-29, wav_test_02 PASS)
-3. ⏳ 解析并播放该 WAV 文件
+3. 🚧 解析并播放该 WAV 文件 (wav_test_03 解析头 PASS, 还未播放)
 4. ⏳ 增加按键上下曲、播放暂停、音量加减等功能
```

进度表：

```diff
-| Phase 5 | WAV 播放器 | 🚧 2/4 |
+| Phase 5 | WAV 播放器 | 🚧 3/4 (wav_test_03 解析头 PASS, wav_test_04 出声待测) |
```

---

## 8. 下一步

进入 **wav_test_04**：首次出声。
- `dac_spr_set(s_hdr.sample_rate)` 把 DAC 配到 44.1kHz
- `dac_aubuf_clr()` 清 obuf
- `pff_lseek(s_hdr.data_offset)` 跳过头
- 循环 `pff_read(buf, 256) → obuf_put_samples(buf, br)` 一股脑喂
- 耳机立即听到完整 wav

⚠️ **风险点**（详见 wav_test_04 文档）：
1. `dac_spr_set` 入参单位是 raw Hz 还是 `SPR_xxx` 宏？首次试 raw Hz，失败查 bsp_audio.c
2. obuf 只有 ~2KB，一次性喂 52MB 会**覆盖前段**（旧数据被新数据覆盖），但出声即可（流式喂数是 wav_test_05 才解决）
3. DAC 已由 `bsp_sys_init()` 初始化，**不要重复调** `dac_power_on / dac_digital_enable`
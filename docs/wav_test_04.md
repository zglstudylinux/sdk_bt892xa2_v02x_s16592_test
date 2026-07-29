# Phase 5 Task 3b — 首次出声：obuf 喂 PCM（wav_test_04）

> 配套 [Phase 5 任务清单](任务清单.md) 的 Task 3（解析并播放该 WAV 文件）的子步骤 3.b（首次出声）。
> 依赖 [wav_test_03.md](wav_test_03.md)（44 字节头解析出 sample_rate / data_offset）。
>
> 本测试**只喂 1MB**（约 5 秒音频），验证 DAC 通路完整可达。**完整播放**交给 wav_test_05。

---

## 0. 测试目标

把 wav_test_03 解析好的 PCM 数据通过 `obuf_put_samples()` 喂给 DAC，并通过 3.5mm 耳机听到声音。**关键卡点**是 DAC 启动序列：

```
解析好的 s_hdr (sample_rate, data_offset, ...)
        ↓
Step 0: 唤醒 DAC 输出通路（dac_restart + loudspeaker_unmute + dac_fade_in）
        ↓
Step 1: dac_spr_set(hz_to_spr(sample_rate))  ← Hz → SPR 枚举转换
        ↓
Step 2: dac_aubuf_clr()  ← 清 obuf
        ↓
Step 3: pff_lseek(data_offset)
        ↓
Step 4: while (pff_read 256) → obuf_put_samples(buf, br)
        ↓
耳机听到 ~5 秒 wav 内容
```

---

## 1. 测试原理

### 1.1 DAC 启动序列（关键）

`bsp_sys_init()` 中 `dac_init()` 完成寄存器配置，但**不会主动启动音频输出**。开机默认播放的 `power_on` 音（`mp3_res_play`）走的是另一套通路（res.bin → 提示音解码），播完会回到静音。

要把 obuf 里的 PCM 真正播放出去，需要按 SDK 内部 `bsp_sys_unmute()` 流程手动激活：

```c
// 来源: bsp_sys.c:514 bsp_sys_unmute()
sys_cb.mute = 0;
bsp_loudspeaker_unmute();   // 解硬件 MUTE（PA 静音信号）
dac_fade_in();              // 启动 DAC 数字淡入（让 obuf 开始被 DMA drain）

// 如果 power_on 播完后 sys_cb.dac_sta = 0（DAC 已断电）：
if (sys_cb.dac_sta == 0) {
    sys_cb.dac_sta = 1;
    dac_restart();          // 重启 DAC
}
```

**不调这套流程，`obuf_put_samples()` 喂进去的 PCM 永远不会被 DMA 主动 drain**，所以听不到声音。这是 wav_test_04 的核心卡点。

### 1.2 DAC 采样率是 SPR 枚举，不是 raw Hz

`dac_spr_set()` 接的是 `SPR_xxx` 枚举，不是 raw Hz 值。`api_sdadc.h:25-42` 的定义：

```c
enum {
    SPR_48000,   // = 0
    SPR_44100,   // = 1
    SPR_38000,   // = 2
    ...
};
```

SDK 内部例子 [func_usbdev.c:97](app/projects/standard/functions/func_usbdev.c#L97)：
```c
dac_spr_set(0);                 // samplerate 48K
DACDIGCON0 |= BIT(6);           // Src0 Sample Rate Synchronization Enable
dac_fade_in();
```

注释说"48K"，传 0 = `SPR_48000`。所以 wav 必须用 `hz_to_spr()` 查表转：

```c
static u32 hz_to_spr(u32 hz) {
    switch (hz) {
    case 48000: return SPR_48000;
    case 44100: return SPR_44100;
    // ...
    default:    return SPR_44100;  // fallback
    }
}
dac_spr_set(hz_to_spr(s_hdr.sample_rate));   // 44100 → SPR_44100 (= 1)
```

### 1.3 为什么只喂 1MB

完整 wav 文件 **40MB / 256B = 16 万次 pff_read**，每次 ~5-10ms（SD 卡读扇区开销）。总耗时 **30-60 分钟**。而且 obuf 容量只有 ~2KB，DMA 在几毫秒内抽空，新数据到达前 obuf 是空的，根本播不出声音。

wav_test_04 只喂 **1MB（5 秒音频）**：
- 实际耗时 ~5 秒
- obuf 能在喂速 > DMA 消费的前提下保持满
- 足以验证 DAC 通路完整可达
- 完整播放交给 wav_test_05（5ms tick 流式喂数，CPU 持续喂 obuf 抵消 DMA 消费）

### 1.4 obuf_put_samples 的 PCM 字节序

`obuf_put_samples()` 接受 raw bytes，内部 DAC 按 signed 16-bit LE 解码。**标准 PCM 16-bit WAV 已经是 signed LE**，所以直接喂原字节即可，**不需要做大小端转换**。

---

## 2. 测试代码

完整测试函数见 [app/projects/standard/wav_test/wav_test.c](../app/projects/standard/wav_test/wav_test.c) 的 `wav_test_play_one_shot()`（`#if 1` ACTIVE）。

### 2.1 关键代码：DAC 启动序列

```c
/* Step 0: 唤醒 DAC 输出通路（关键！） */
if (sys_cb.dac_sta == 0) {
    sys_cb.dac_sta = 1;
    dac_restart();
}
if (sys_cb.mute) {
    bsp_sys_unmute();  /* 内部会 loudspeaker_unmute + dac_fade_in */
} else {
    bsp_loudspeaker_unmute();
    dac_fade_in();
}
/* 确保音量不为 0（防止 dac_init 后 vol=0 听不到） */
if (sys_cb.vol == 0) sys_cb.vol = 8;
dac_set_volume(sys_cb.vol);
```

### 2.2 关键代码：Hz → SPR 转换 + 喂数

```c
dac_spr_set(hz_to_spr(s_hdr.sample_rate));   // SPR_44100
dac_aubuf_clr();
pff_lseek(s_hdr.data_offset);

while (total < WAV_TEST04_FEED_BYTES) {       // 1MB cap
    UINT br;
    FRESULT fr = pff_read(s_pcm, sizeof(s_pcm), &br);
    if (fr != FR_OK || br == 0) break;
    obuf_put_samples(s_pcm, br);
    total += br;
}
```

### 2.3 入口串联

```c
if (!wav_test_banner())        goto halt;   // wav_test_01 ACTIVE
if (!wav_test_open_fixed())    goto halt;   // wav_test_02 ACTIVE
if (!wav_test_parse_header())  goto halt;   // wav_test_03 ACTIVE
if (!wav_test_play_one_shot()) goto halt;   // wav_test_04 ACTIVE
```

---

## 3. 测试步骤

1. TF 卡根目录放 `MUSIC.WAV`（标准 PCM 16-bit 立体声 44.1kHz，沿用 wav_test_02/03）
2. Code::Blocks `Ctrl+F9` → 检查 `Output\bin\app.dcf`
3. Downloader 烧录
4. **戴上耳机插入 3.5mm 插座**
5. 串口观察

---

## 4. 测试前置条件

| 条件 | 说明 |
|---|---|
| wav_test_01/02/03 已 PASS | 工程接入 + 文件打开 + 头解析 |
| `MUSIC.WAV` 是 PCM 16-bit 立体声 WAV | wav_test_03 已验证 |
| **3.5mm 耳机插入** | 关键！DAC 输出走 headphone 通路 |
| `config.h:604 EARPHONE_DETECT_EN = 1` | 默认开启，复用 PB1/PB2 检测 |

---

## 5. 实测输出（实测 PASS）

```
=========================================
  [WAV] BT892XA2 WAV Player Test
  [WAV] SDK V02.00
  ...
=========================================

[WAV] ====== wav_test_02: open fixed MUSIC.WAV ======
... (wav_test_02 PASS) ...

[WAV] ====== wav_test_03: parse 44-byte RIFF header ======
[WAV] PASS: header parsed:
[WAV]   format      = 1 (PCM)
[WAV]   channels    = 2 (stereo)
[WAV]   sample_rate = 44100 Hz
[WAV]   bits        = 16
[WAV]   data_offset = 44
[WAV]   data_size   = 42232512 bytes (~239 seconds)

[WAV] ====== wav_test_04: obuf one-shot play ======
[WAV]   wav sample_rate = 44100 Hz
[WAV]   wav channels    = 2
[WAV]   wav data_offset = 44
[WAV]   wav data_size   = 42232512 bytes (~239 seconds)
[WAV]   feed limit     = 1048576 bytes (~5 seconds)
[WAV] Step 0: wake up DAC output path...
[WAV]   explicit loudspeaker_unmute + dac_fade_in
[WAV] Step 1: dac_spr_set(SPR_xxx for 44100 Hz)...
[WAV] Step 2: dac_aubuf_clr()...
[WAV] Step 3: pff_lseek(44) jump header...
[WAV] Step 4: feeding PCM (capped at 1048576 bytes)...
[WAV]   fed 262144 / 1048576 bytes (25%)
[WAV]   fed 524288 / 1048576 bytes (50%)
[WAV]   fed 786432 / 1048576 bytes (75%)
[WAV]   fed 1048576 / 1048576 bytes (100%)
[WAV] PASS: feed done, total 1048576 bytes (~5 seconds)
[WAV]   (obuf now has data; DMA will drain in background)

[WAV] ====== All active tests PASSED ======
[WAV] Halt. 5ms loop.
```

实测现象：**耳机听到 MUSIC.WAV 开头的 ~5 秒音频**（用户实测：能听到声音）。

---

## 6. 失败排查表

| 现象 | 可能原因 | 排查 |
|---|---|---|
| 听到 power_on 音但**没有 wav 声音** | DAC 在 mute / fade_out 状态 | **本测试已修复**：加 Step 0 唤醒 DAC 通路（dac_restart + loudspeaker_unmute + dac_fade_in） |
| 串口 Step 4 跑得极慢 | pff_read 卡 SD | 检查 PE5/PE6/PE7 接线 / 换 SD 卡 |
| 听到的是**噪音**而非音乐 | DAC 采样率配错（hz_to_spr 没匹配） | 检查 `SPR_44100=1` 是否返回正确；首字节 `0xD4 0x12` 不是 `0x00` |
| 听到的是**快进/慢放** | 喂速太快 / obuf 溢出 | 正常（one-shot 模式固有问题，wav_test_05 改流式解决） |
| 听到的是**静音**但程序打印 OK | 音量 sys_cb.vol=0 | 已加保护：强制 vol >= 8 |
| DAC 启动后听到**"啪"一声** | loudspeaker_unmute 时序问题 | 正常，加 `dac_unmute_set_delay(6)` 可消除 |

---

## 7. 任务清单勾选

```diff
 # WAV音乐播放器 (Phase 5) 🚧 进行中

 1. ✅ 建立 wav_test 目录
 2. ✅ 从 TF 卡中打开一个 wav 文件
-3. 🚧 解析并播放该 WAV 文件 (wav_test_03 解析头 PASS, 还未播放)
+3. 🚧 解析并播放该 WAV 文件 (wav_test_03 头 PASS, wav_test_04 首 5 秒 PASS, 完整播放待 wav_test_05)
 4. ⏳ 增加按键上下曲、播放暂停、音量加减等功能
```

进度表：

```diff
-| Phase 5 | WAV 播放器 | 🚧 3/4 (wav_test_03 解析头 PASS, wav_test_04 出声待测) |
+| Phase 5 | WAV 播放器 | 🚧 3/4 (wav_test_04 首 5 秒 PASS, wav_test_05 完整播放待测) |
```

---

## 8. 下一步

进入 **wav_test_05**：5ms tick 流式喂数 → 完整播放整首 40MB MUSIC.WAV。
- `wav_test_tick()` 钩子每 5ms 喂 256B 到 obuf
- 状态机 IDLE/PLAYING/PAUSED/DONE
- `pff_read()` EOF (br=0) 自动转 DONE
- 持续约 5 分钟音频（40MB / 176KB/s ≈ 239s）
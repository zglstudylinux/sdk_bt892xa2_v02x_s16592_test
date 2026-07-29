# Phase 5 Task 5.3 — 解析 WAV + 直推 DAC 播放

> 配套 [任务清单.md](任务清单.md) "WAV 播放器 (Phase 5)" 模块的 Task 5.3（解析 + 播放）。
> 依赖 [wav_test_01.md](wav_test_01.md)（Task 5.1 / 5.2：打开文件 + 读 RIFF 头必须先 PASS）。
> WAV / RIFF 格式基础见 [wav入门指南.md](wav入门指南.md)。
> Task 5.4（按键控制）单独写在 [wav_test_03.md](wav_test_03.md)。
> Phase 5 整体总结在 [wav_test_summary.md](wav_test_summary.md)。

---

## 0. 这份文档要回答的问题

| 问题 | 答案在本节 |
|---|---|
| Task 5.3 到底做了什么？ | §1 |
| 解析 + 播放的代码结构长什么样？ | §2 |
| 一个 16-bit PCM 采样到底怎么喂给 DAC？ | §3 |
| 之前"左声道小 + 人声糊"是怎么回事？ | §4 |
| 串口输出长什么样、每行什么意思？ | §5 |
| 播放出问题了怎么排查？ | §6 |

---

## 1. Task 5.3 做了什么

Task 5.1 / 5.2（见 [wav_test_01.md](wav_test_01.md)）只做到"打开文件 + 读 64 字节 RIFF 头 + 解析 `fmt` 字段"。Task 5.3 把剩下的半条链路补齐：**从 `data` chunk 把 16-bit PCM 帧一帧一帧直接推给 DAC 的 `AUBUFDATA` 寄存器**，让 SD 卡里的 `TEST.WAV` 真正出声。

整条流水线是五步：

```
找 data ──▶ 校验 fmt ──▶ dac_spr_set(按头部采样率) ──▶ 唤醒 DAC ──▶ 循环 pff_read → 推 AUBUFDATA
```

| 步骤 | 内容 | 状态 |
|---|---|---|
| 5.3.1 | chunk walker 从 `pos=12` 起逐块跳，按 `data` 标签定位（**不假设** data 在 offset 44） | ✅ |
| 5.3.2 | 重读 `fmt` chunk，校验 `wFormatTag==1`（PCM）、`wBitsPerSample==16`、`nChannels ∈ {1,2}` | ✅ |
| 5.3.3 | `dac_spr_set()` 按头部采样率选 `SPR_*` 枚举（**收枚举下标，不是填 Hz**） | ✅ |
| 5.3.4 | 唤醒 DAC：`dac_set_dvol(DIG_N0DB)` + `bsp_change_volume` + `dac_fade_in/wait` | ✅ |
| 5.3.5 | `pff_lseek(data_offset)` 后循环 `pff_read` 512 字节，逐帧推 `AUBUFDATA` | ✅ |

代码在 [wav_test/wav_test.c](../app/projects/standard/wav_test/wav_test.c) 的 `wav_test_parse_and_play()`（[:238](../app/projects/standard/wav_test/wav_test.c#L238)）。

---

## 2. 实现结构

`wav_test_parse_and_play()` 的六步调用树，跟 [wav_test_01.md](wav_test_01.md) §2.3 一个画法：

```
wav_test_parse_and_play()
    ├── 1. chunk walker 找 data
    │   ├── pos = 12
    │   ├── while (pos + 8 <= fsize)
    │   │   ├── pff_lseek(pos) + pff_read(8)     读 8 字节 chunk 头
    │   │   ├── chunk_size = rd32(+4)
    │   │   ├── 命中 'data' → data_offset=pos+8, data_size=size, break
    │   │   └── 否则 pos += 8 + chunk_size + (chunk_size & 1)   // &1 = RIFF 字对齐填充
    │   └── data_offset == 0 → FAIL: no 'data' chunk
    ├── 2. 重读 fmt @12 校验
    │   ├── wFormatTag == 1 (PCM)
    │   ├── wBitsPerSample == 16
    │   └── nChannels == 1 或 2
    ├── 3. dac_spr_set(wav_sample_rate_to_spr(hz))     // 收 SPR 枚举，不是 Hz
    ├── 4. 唤醒 DAC
    │   ├── dac_set_dvol(DIG_N0DB)         // 数字音量缺省 = 0 = 静音，必调
    │   ├── bsp_change_volume(sys_cb.vol)  // 模拟音量
    │   ├── dac_fade_in()
    │   └── dac_fade_wait()
    ├── 5. pff_lseek(data_offset)
    └── 6. 循环 pff_read(s_pcm, 512) → 逐帧推 AUBUFDATA
        ├── 立体声：src=(const u32*)s_pcm; while(AUBUFCON&BIT(8)){} AUBUFDATA=src[i]
        ├── 单声道：s=rd16(...); v=((u32)s<<16)|s; 推 v
        └── 每 50 ms 打印一次进度 %
```

两个和 Phase 4 不同的关键点：

- **chunk walker（步骤 1）**：`data` 不一定在 offset 44 —— `fmt` 和 `data` 之间可能塞 `LIST` / `fact` / `JUNK`。walker 从 `pos=12` 读 8 字节头、`pos += 8 + chunk_size + (chunk_size & 1)` 逐块跳，命中 `data` 才记下偏移。`(chunk_size & 1)` 是 RIFF 的字对齐填充字节。见 [wav_test.c:247-271](../app/projects/standard/wav_test/wav_test.c#L247)。
- **采样率收 SPR 枚举（步骤 3）**：`dac_spr_set()` 的形参是 `SPR_*` 枚举**下标**，不是赫兹。`wav_sample_rate_to_spr()` 做映射，未命中返回 `0xFFFF`。

| WAV 头 `nSamplesPerSec` | 传给 `dac_spr_set()` | 枚举下标 |
|---|---|---|
| 48000 | `SPR_48000` | 0 |
| 44100 | `SPR_44100` | 1 |
| 38000 / 32000 / 24000 / 22050 | `SPR_38000` … `SPR_22050` | 2 / 3 / 4 / 5 |
| 16000 / 12000 / 11025 / 8000 | `SPR_16000` … `SPR_8000` | 6 / 7 / 8 / 9 |
| 其他（如 96000） | —— | 返回 `0xFFFF`（不支持） |

枚举定义见 [api_sdadc.h:25-42](../app/platform/libs/api_sdadc.h#L25)，映射表见 [wav_test.c:96-108](../app/projects/standard/wav_test/wav_test.c#L96)。**填 `44100` 而不是 `SPR_44100` 会选错档，播放速度全乱** —— 这是新手最常踩的坑。

---

## 3. 关键：一个采样怎么推给 DAC

这是整个 Task 5.3 的核心事实，务必吃透。

`AUBUFDATA`（`SFR_RW(SFR1_BASE + 0x01*4)`）每次接收 **一个 `u32`**，而 **一次 `u32` 写入 == 一个立体声帧**：

```
磁盘上一个立体声帧（4 字节，小端）：
  +-------+-------+-------+-------+
  | L_lo  | L_hi  | R_lo  | R_hi  |   偏移  +0  +1  +2  +3
  +-------+-------+-------+-------+
      └──── L ────┘   └──── R ────┘

原地重解释为一个小端 u32：
  bits[31:16] = R   （高 16 位）
  bits[15:0]  = L   （低 16 位）
              == (R << 16) | L   ← 恰好就是 DAC 要的帧布局
```

硬事实四条：

- **一个 `u32` = 一帧**，两个 16-bit 半字：低 16 位 = 左（WAV 第 1 声道），高 16 位 = 右（WAV 第 2 声道）。
- 样本是 **有符号 16-bit**，**原样推**（`0x0000` == 静音），**无偏移、无异或**。
- 推之前轮询 `AUBUFCON` 的 `BIT(8)`：**`BIT(8)==1` 表示 DACBUF FIFO 已满**，`while (AUBUFCON & BIT(8)) {}` 等它清零再写。
- DACBUF 内部约 **2 KB**；满位轮询本身就把推送节奏自然节流到实时，不需要显式 `delay_us()` 配速。

三处 SDK 源交叉印证"一 `u32` = 一帧、有符号、原样推"：

| 源 | 位置 | 佐证 |
|---|---|---|
| 正弦测试 | [bsp_iis_ext.c:765](../app/platform/bsp/bsp_iis_ext.c#L765) | 每帧一个 `u32` 从正弦表取值，表数据绕 `0x0000` 振荡、含负值 → **有符号、无偏移** |
| ADC→DAC | [func_speaker.c:110](../app/platform/functions/func_speaker.c#L110) | `AUBUFDATA = ((u32)s<<16)｜s` → 一次写 = 一帧，同一样本复制进高 / 低 16 位 |
| 现场心得 | 项目内部 `test/参考.md`（金国强；该笔记暂未随仓库发布） | "推 PCM 数据可以直接强制转换为 32 位……偏移量为 4，缓冲区长度要除 4" → 强转 `u32*`、步长 4 字节直推 |

> 注：`bsp_iis_ext.c:765` 与 `func_speaker.c:110` 两段都在 `#if 0` 演示 / 关闭块内，作为**寄存器用法范式**依然成立；真正的"活代码"直推范式是 [bsp_iis_ext.c:700-708](../app/platform/bsp/bsp_iis_ext.c#L700) 的 `iis_rx_process_test`（`u32 *ptr=(u32*)buf; … while((AUBUFCON&BIT(8))==0) AUBUFDATA=*ptr++;`），写法完全一致。

**关键推论——立体声零逐样本处理**：磁盘上的立体声帧 `[L_lo L_hi R_lo R_hi]` 小端重解释为 `u32` 就已经是 `(R<<16)|L`，恰是 DAC 要的布局。所以立体声只需把缓冲区 `(const u32*)` 强转、每 4 字节一帧原样推；单声道则把同一样本复制进高低两半：

```c
/* 立体声：磁盘 [L_lo L_hi R_lo R_hi] LE 重解释为 u32 == (R<<16)|L，直接推 */
const u32 *src = (const u32 *)(const void *)s_pcm;
while (AUBUFCON & BIT(8)) { }   /* BIT(8)=1 满，等它清零 */
AUBUFDATA = src[i];             /* 一次写入 = 一整帧，有符号原样，无偏移无异或 */

/* 单声道：同一样本复制进 L(低16) 与 R(高16) */
u16 s = rd16(s_pcm + 2 * i);
u32 v = ((u32)s << 16) | s;     /* R = L = s，与 func_speaker.c:110 同型 */
while (AUBUFCON & BIT(8)) { }
AUBUFDATA = v;
```

现行实现见 [wav_test.c:408-428](../app/projects/standard/wav_test/wav_test.c#L408)。

---

## 4. 踩坑实录：左声道小 + 人声糊（完整 autopsy）

Task 5.3 第一次能出声后，反复出现一个诡异现象，前后改了 v3–v7 好几版才定位。这一节把它完整剖开——它正是"没吃透 §3 那条帧格式"会掉进的坑。

### 4.1 现象

> 音乐能放，旋律听得出来；但**左声道明显比右声道轻**，而且**人声发糊、听不清**。

### 4.2 根因（两点错叠加）

旧的立体声推送代码同时干了两件错事：

```c
/* 错误（旧版，据本次会话中的修复叙述重建；buggy 提交未留在 git 历史里） */
u32 ul = ((u32)l ^ 0x8000) << 16;   /* 错1：^0x8000 把有符号→无符号；错2：<<16 使低16=0 */
u32 ur = ((u32)r ^ 0x8000) << 16;
AUBUFDATA = ur;   /* 开发者以为这是"右声道" */
AUBUFDATA = ul;   /* 开发者以为这是"左声道" */
```

- **错 1 —— `^0x8000`**：把有符号样本异或成"无符号"。但 DAC 要的是**有符号**，这一步凭空注入了直流偏置 / 回绕失真。
- **错 2 —— 每帧两写、且低 16 位恒为 0**：`<<16` 把声道塞进高 16 位，低 16 位恒为 `0`；又对每帧写了两次。

### 4.3 把每个症状对回根因

既然 **一个 `u32` = 一整帧（低 16 = L / 高 16 = R）**，上面两次写其实**各是"一整帧"**，而每帧的**左半（低 16 位）恒为 0**：

| 症状 | 机理 |
|---|---|
| 左声道明显更轻 | 每帧左半恒 `0` → 物理左声道被喂常数 `0` → 近乎静音 |
| 人声不清但旋律可辨 | 另一半（高 16 位）以 **2× 帧率**收到 `R,L,R,L` 交织流 → 退化成一路含混的"伪单声道"混音；需要 L / R 相干等值才能居中的**人声被抹糊**，而硬声像（左偏 / 右偏）的**旋律乐器还能辨认** |
| 整体发浑 | 叠加 `^0x8000` 的直流 / 回绕失真，进一步加重浑浊感 |

> 说明："人声糊 / 旋律可辨"是对上述机理的**合理听感描述**，不是示波器级实测结论。

早期还有一版（v7）以为是"L / R 声道接反"，把推送顺序改成 R 先——但**纯粹的左右对调既不会造成失衡、也不会让人声变糊**，所以那是对**同一个打包 bug 的误诊**，改了等于没改。

### 4.4 修好了：新旧对照

```c
/* 正确（现行 wav_test.c:408-428）—— 一 u32 = 一帧，有符号原样推 */
if (nChannels == 2) {                 /* 立体声：磁盘 [L_lo L_hi R_lo R_hi] LE == (R<<16)|L */
    u32 frames = n / 4;
    const u32 *src = (const u32 *)(const void *)s_pcm;
    for (u32 i = 0; i < frames; i++) {
        while (AUBUFCON & BIT(8)) { }  /* BIT(8)=1 满，等它清零 */
        AUBUFDATA = src[i];            /* 一次写入 = 一整帧，无偏移无异或 */
    }
} else {                              /* 单声道：同一样本复制进 L(低16) 与 R(高16) */
    u32 frames = n / 2;
    for (u32 i = 0; i < frames; i++) {
        u16 s = rd16(s_pcm + 2 * i);
        u32 v = ((u32)s << 16) | s;
        while (AUBUFCON & BIT(8)) { }
        AUBUFDATA = v;
    }
}
```

为什么这样就对了：新代码**一个 `u32` 写一整帧**，低 16 位（左）、高 16 位（右）各就各位，有符号原样推，**不再异或、不再一帧两写**；左右声道恢复独立且等响、居中人声重新相干 → 2026-07-29 一次通过，左右均衡、人声清晰。

---

## 5. 串口输出解读

> ⚠️ **本节的串口文本是"重建示意"，不是逐字节抓包。** 它按 [wav_test.c](../app/projects/standard/wav_test/wav_test.c) 里**实际的 `printf` 格式串** + [wav_test_01.md](wav_test_01.md) §3 **实测头部字节**重建，用来讲解每一行的含义与来历。凡是**能从已提交证据确定推算**的数字（下面标 🅳），都给出算式；凡属**运行时才定**的（耗时 `ms`、每 50 ms 一行的中间进度），标 🅡 表示示意值，以你自己烧录后的串口为准。本次"通过"的最终依据是 §5.2 的**实际听感确认**。

### 5.1 Part 2 串口输出（重建）

接在 [wav_test_01.md](wav_test_01.md) §3 的 Part 1 输出之后：

```
[WAV] ====== Task 5.3: Parse + Play ======
[WAV]   chunk @0x0000000c: fmt  size=16                         🅳
[WAV]   chunk @0x00000024: data size=12845251                   🅳
[WAV] data chunk @44, size=12845251 bytes (~72 sec at 44100 Hz) 🅳
[WAV] dac_spr_set(44100 Hz) = SPR enum 1                        🅳
[WAV] streaming 12845251 bytes of 16-bit stereo PCM at 44100 Hz:🅳
[WAV] (direct AUBUFDATA push, polling AUBUFCON bit 8)
[WAV] 0 / 12544 KB (0%)                                         🅡
[WAV] 258 / 12544 KB (2%)                                       🅡
        ......（每 50 ms 一行，中间值随运行而变）......             🅡
[WAV] 6272 / 12544 KB (50%)                                     🅡
        ......
[WAV] 12544 / 12544 KB (100%)                                   🅳(KB) 🅡(时机)
[WAV] pushed 12845251 bytes (3211312 frames) in <elapsed> ms    🅳(前两数) 🅡(ms≈72000)
[WAV] Phase 5 Part 2 PASS - file played.

[WAV] ====== All active tests PASSED ======
[WAV] Halt. 5ms loop.
```

> 注：串口里印的是 `Phase 5 Part 2 PASS`（[wav_test.c:448](../app/projects/standard/wav_test/wav_test.c#L448) 把"解析 + 播放"记作 Part 2），对应任务清单的 **Task 5.3**。

### 5.2 每个数字的来历（🅳 都能自己验算）

| 输出行 | 含义 | 怎么来的 |
|---|---|---|
| `chunk @0x0000000c: fmt  size=16` | walker 第一块命中 `fmt `，长度 16 | Part-1 dump：`fmt ` @0x0C，长度字段 `10 00 00 00`=16 🅳 |
| `chunk @0x00000024: data size=12845251` | 第二块是 `data`，**在 0x24**（不在别处假设） | Part-1 dump：`data` @0x24（`64 61 74 61`），其后长度字段 `C3 00 C4 00` 小端 = `0x00C400C3` = **12,845,251** 🅳 |
| `data chunk @44, ... (~72 sec ...)` | `data_offset = 0x24 + 8 = 44`；时长 = `data_size / (nSamplesPerSec × 声道 × 2)` | 代码 [wav_test.c:311-315](../app/projects/standard/wav_test/wav_test.c#L311) 整数除：`12845251 / (44100×4)` = `12845251/176400` = **72** 🅳 |
| `dac_spr_set(44100 Hz) = SPR enum 1` | 44100 → `SPR_44100` | `SPR_44100` 是枚举第 2 项，下标 **1**（`SPR_48000`=0） 🅳 |
| `streaming 12845251 bytes ... stereo ...` | 走立体声零转换直推路径 | 同 `data_size`、`nChannels==2` 🅳 |
| `12544 KB` (进度分母) | `data_size / 1024` | `12845251 / 1024` = **12544** KB 🅳 |
| `pushed 12845251 bytes (3211312 frames)` | 推完的字节 / 帧数 | 帧 = `⌊12845251 / 4⌋` = **3,211,312** 🅳 |
| `in <elapsed> ms` | 耗时 | 被 DACBUF 满位轮询节流到实时，≈ 72,000 ms 🅡（硬件计时，运行时定） |
| 中间 `xxx / 12544 KB (n%)` | 每 50 ms 一行的进度 | 具体行数 / 数值随运行节奏变 🅡 |

**一个值得注意的点**：这个 `TEST.WAV` 头里声明的 `data` 长度（12,845,251 B ≈ 72 s）**小于文件物理大小**（[wav_test_01.md](wav_test_01.md) 实测 `file size=52920044` ≈ 52.9 MB）。播放器**以头部声明的 `data` 长度为准**收尾，所以按 ~72 秒停——这刚好印证了 §2 的设计：**信任解析出来的长度字段，而不是拿文件大小硬算**。若想让整段都被识别播放，用 `ffmpeg` 重新生成，让 `data` 长度字段与实际 PCM 一致。

### 5.3 听感确认（2026-07-29，本次"通过"的真正依据）

- **左右声道均衡**：不再一边轻一边响 → §4 的低 16 位恒 0 问题已解。
- **居中人声清晰**：L / R 恢复相干等值，人声不再被抹糊。
- **旋律与之前一致**：说明修的是打包 / 声道，不是数据本身。

三条同时成立、且**一次跑通** → v8 的"一帧一 `u32`、有符号原样推"修复生效。这是用户实测听感，不是推断。

---

## 6. 失败排查表

| 现象 | 原因 | 解决 |
|---|---|---|
| "噗噗噗"杂音 / 时断时续 | SD 波特率太低，DACBUF 欠载（underrun） | 拉高 SD 波特率（杜工：2M 以上才正常；李宗霖：8M 下限、实跑 12M；李坪臻：16M 可、17M 出问题） |
| 播放变快 / 变慢 | `dac_spr_set` 的 `SPR` 档与头部采样率不匹配 | 用 `wav_sample_rate_to_spr()` 按头部 `nSamplesPerSec` 选，别写死档位 |
| 一个声道几乎无声 / 人声糊 | 声道打包错误（低 16 位恒 0、每帧两写、`^0x8000`） | 见 §4：一个 `u32` 写一整帧，有符号原样推 |
| 完全无声（有进度输出但没声） | 忘了 `dac_set_dvol(DIG_N0DB)`，数字音量缺省 0 = 静音 | 唤醒序列补上 `dac_set_dvol`（[wav_test.c:337](../app/projects/standard/wav_test/wav_test.c#L337)） |
| 播放莫名提前结束 | 头里 `data` 长度字段短于实际 PCM（见 §5.2） | PC 上用 `ffmpeg` 重新生成，让长度字段与数据一致 |
| `sample rate ... not in SDK DAC enum` | 采样率不在 SPR 表内（如 96000 / 176400） | PC 上重采样到 44100 / 48000 等表内值 |
| `only 16-bit PCM supported` | 源是 24-bit / 32-bit | PC 上转成 16-bit（姚嘉隆 / 李坪臻：24/32→16 音质差，直接用 16-bit 源） |
| `only PCM (wFormatTag=1) supported` | 不是 PCM（ADPCM / MP3-in-WAV） | `ffmpeg -acodec pcm_s16le` 重转 |
| 读缓冲太小 / 有杂波 | `s_pcm` 太小，读推节奏跟不上 | 512 字节是甜点（杜工 / 李宗霖）；别在读路径放 ms 级 `delay` 或大数组 `memset` |
| `no 'data' chunk found` | 文件被截断，或不是标准 RIFF | 重新生成完整 WAV |

补充：本平台**无硬件浮点 / libgcc**，进度百分比用整数算（`total_pushed * 100 / data_size`，见 [wav_test.c:436](../app/projects/standard/wav_test/wav_test.c#L436)），别引入浮点。

---

## 7. 任务进度

| 任务 | 完成情况 |
|---|---|
| Task 5.3 解析 + 播放 | ✅ **2026-07-29 实测 PASS**（`TEST.WAV` 44.1 kHz 16-bit 立体声；按头部声明的 `data` 长度推完；**左右均衡、人声清晰、一次跑通**——见 §5.3 听感确认） |

下一步：[Task 5.4 按键控制](wav_test_03.md)（`KU_PLAY` / `KU_NEXT` / `KU_PREV` / `KU_VOL_UP` / `KU_VOL_DOWN`，接 SDK 消息队列）。Phase 5 整体收尾见 [wav_test_summary.md](wav_test_summary.md)。

---

## 一句话总结

> Phase 5 Task 5.3：chunk walker 定位 `data` → 校验 16-bit PCM → `dac_spr_set` 按头部采样率选 `SPR_*` 枚举 → `dac_set_dvol(DIG_N0DB)` 唤醒 DAC → 把每个立体声帧当**一个有符号 `u32`（低 16 = L / 高 16 = R）原样直推 `AUBUFDATA`**、轮询 `AUBUFCON` 的 `BIT(8)` 自然节流——一举修掉"左声道小 + 人声糊"的声道打包 bug，`TEST.WAV` 2026-07-29 第一次就在 BT892XA2 上左右均衡、人声清晰地放了出来（§5 的串口数字为按代码格式串 + 实测头部字节重建的示意值，PASS 以 §5.3 实际听感为准）。

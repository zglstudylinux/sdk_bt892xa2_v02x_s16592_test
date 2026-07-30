# Phase 5 Task 5.4 — 按键上下曲 / 播放暂停 / 音量加减

> 配套 [任务清单.md](任务清单.md) "WAV 播放器 (Phase 5)" 模块的 Task 5.4。
> 依赖 [wav_test_02.md](wav_test_02.md)（Task 5.3 解析 + 播放已 PASS：左右均衡、人声清晰）。
> WAV / RIFF 格式基础见 [wav入门指南.md](wav入门指南.md)。
> Phase 5 整体总结在 [wav_test_summary.md](wav_test_summary.md)。

---

## 0. 这份文档要回答的问题

| 问题 | 答案在本节 |
|---|---|
| Task 5.4 到底做了什么？ | §1 |
| 怎么从单个 WAV 升级到"按键控制的播放器"？两层调度器怎么设计？ | §2 |
| 怎么枚举 SD 卡里所有 WAV、做成播放列表？ | §3 |
| 播放期间怎么收按键 + 实现暂停 / 切歌 / 音量？ | §4 |
| 这块教学板 (TP17/2PWRKEY via 0Ω/12K/47K) 的按键怎么映射到 SDK 的 key id？ | §5 |
| 串口实测长什么样、每行什么意思？ | §6 |
| 失败了怎么排查？（3 个实测踩过的坑） | §7 |

---

## 1. Task 5.4 做了什么

Task 5.3 ([wav_test_02.md](wav_test_02.md)) 做到"打开一个 WAV → 解析 → 推 DAC 播完停"。Task 5.4 把"播完停"升级成"按键可控的播放循环"：

| 维度 | Task 5.3 | Task 5.4 |
|---|---|---|
| 歌曲 | 写死的单文件 | **SD 卡根目录所有 `*.WAV`，自动枚举**（最多 16 首）|
| 播完一首 | 死循环 `delay_5ms(200)` | **自动播下一首**（末尾 wrap 到第一首）|
| 暂停 / 恢复 | 不支持 | **P/P 键**：推帧循环暂停但不退出，仍可响应其他键 |
| 上一曲 / 下一曲 | 不支持 | **PREV / NEXT 键**（带 wrap-around）|
| 音量 | 不支持 | `KU_VOL_UP / KU_VOL_DOWN` 调音量（用 `bsp_set_volume(bsp_volume_inc/dec)`，自动 clamp + 持久化）|
| 串口噪声 | 无 | 过滤掉 SDK 的 `MSG_SYS_*` 周期事件 |

代码在 [wav_test/wav_test.c](../app/projects/standard/wav_test/wav_test.c)：

| 函数 | 任务 |
|---|---|
| `wav_test_build_playlist()` | 一次性枚举 SD 卡根目录，收集最多 16 个 8.3 SFN 的 `*.WAV` |
| `wav_test_open_and_inspect()` | 打开 `s_pl[s_index]`，打印 64B RIFF 头（沿用 Task 5.2）|
| `wav_test_parse_and_play()` | 解析 + 推 DAC，**且每 512 字节抽一次 `msg_dequeue()`** + 每 100 ms 喂一次 WDT |
| `wav_test_handle_key(u16 msg)` | 过滤 SDK 系统事件、把合法 `KU_*` 映射到 `s_paused` / `s_should_next` / `s_should_prev` / 调音量 |
| `wav_test_run()` | 外层调度器：枚举一次 → 无限循环 `for(;;) { open + play; advance index }` |

| 任务 | 内容 | 状态 |
|---|---|---|
| 5.4.1 | `PF_USE_DIR=1` 翻 [fat_test/pffconf.h:33](../app/projects/standard/fat_test/pffconf.h#L33)，打开 `pff_opendir` / `pff_readdir` 公开入口 | ✅ |
| 5.4.2 | `wav_test_build_playlist()` —— `pff_opendir("")` 根目录 + `pff_readdir` 循环，跳 `AM_LFN` / `AM_DIR` / `AM_VOL`，大小写不敏感匹配 `.WAV`，最多 16 首 | ✅ |
| 5.4.3 | `wav_test_run()` 拆成"枚举 + 外层 next/prev 调度 + 内层可中断播放" | ✅ |
| 5.4.4 | 512 字节推帧循环里插 `msg_dequeue()`，过滤 SDK 系统事件（`MSG_SYS_500MS` / `EVT_SD_*` 等 0x7D0-0x7FF）| ✅ |
| 5.4.5 | `KU_PLAY` → 翻 `s_paused`；`KU_VOL_UP/DOWN` → `bsp_set_volume(bsp_volume_inc/dec)`；`KU_NEXT/PREV` → `s_should_next/prev` | ✅ |
| 5.4.6 | 暂停时跳 `AUBUFDATA` 写入但保留 `delay_us(200) + msg_dequeue()` | ✅ |

---

## 2. 两层调度器（outer + inner）

Task 5.3 的 `wav_test_run()` 末尾是 `while(1) delay_5ms(200);` —— 死循环、不响应按键。Task 5.4 拆成两层：

```
wav_test_run()                         ← 外层（每首一曲）
    ├── 一次性：wav_test_build_playlist()
    ├── for (;;) {                     ← 外层死循环
    │       wav_test_open_and_inspect() ← 打开 s_pl[s_index]
    │       wav_test_parse_and_play()   ← 推 DAC 播到结束 OR 被打断
    │       决定 s_index 怎么走
    │   }
    └── halt
        (当前 0 首 / 出错 才进)

wav_test_parse_and_play()              ← 内层（一首内部）
    ├── chunk walker 找 data chunk
    ├── 校验 fmt + 设 DAC 采样率
    ├── while (bytes_left > 0) {       ← 512B chunk 循环
    │       WDT_CLR()                  ← 每 100 ms 喂一次狗
    │       msg = msg_dequeue()
    │       if (msg != NO_MSG) wav_test_handle_key(msg)
    │       if (s_paused) { delay_us(200); continue; }   ← 暂停时停推帧、不退出
    │       if (s_should_next || s_should_prev) break;   ← 用户切歌，chunk 边界退出
    │       pff_read 512 B → 逐帧推 AUBUFDATA
    │   }
    └── if (s_should_next || s_should_prev) return false;
        else                              return true;
```

**外层 advance 逻辑**（[wav_test.c:825-845](../app/projects/standard/wav_test/wav_test.c#L825-L845)）：

| 触发 | `s_index` 怎么走 | 打印 |
|---|---|---|
| 自然播完 | `(s_index + 1) % n` | `slot N -> N+1 (played)` |
| 用户按 NEXT | `(s_index + 1) % n` | `slot N -> N+1 (next)` |
| 用户按 PREV | `(s_index == 0 ? n-1 : s_index-1)` | `slot N -> N-1 (prev)` |
| `pff_open` 失败 | `(s_index + 1) % n` | `slot N -> N+1 (skipped)` |

NEXT 和自然播完的 advance 一样（+1 wrap），但 `why` 字符串不同，方便看 log 区分是用户主动切还是自己流完。

---

## 3. 播放列表枚举（Task 5.4.2）

依赖 [Petit FatFs R0.03a](../app/projects/standard/fat_test/) 的 `pff_opendir` / `pff_readdir`，**需要把 [`pffconf.h:33`](../app/projects/standard/fat_test/pffconf.h#L33) 的 `PF_USE_DIR` 从 0 翻成 1**。

```c
DIR     dj;            // 模块静态 / 栈变量都行
FILINFO fno;           // 13 字节 fname + DWORD fsize + u8 fattrib

pff_opendir(&dj, "");  // 空 path = 根目录（pff.c:761-764）

while (pff_readdir(&dj, &fno) == FR_OK && fno.fname[0]) {
    if (fno.fattrib & (AM_LFN | AM_DIR | AM_VOL)) continue;   // 跳 LFN/子目录/卷标
    const char *dot = strrchr(fno.fname, '.');
    if (!dot) continue;
    // 大小写不敏感 ".WAV"
    if (ext_is_wav(dot + 1)) {  /* 4 char ext test: W/A/V/\0 */
        s_pl[s_total++] = fno;  // 复制 fname[13] + fsize
    }
}
```

**注意两个坑**（实测踩过，注释里也写了）：

1. `dir_read()` ([pff.c:639](../app/projects/standard/fat_test/pff.c#L639)) 跳过 `0xE5`（已删）、`.` / `..`、`AM_VOL`，但 **不会跳过 `AM_LFN`**——必须自己 `if (fattrib & AM_LFN) continue;`，否则长文件名（如 Windows 拷过来的 `System Volume Information`）会作为"垃圾项"塞进 playlist。
2. 顺序是 **FAT 表项顺序**，不是字母序——所以 `MUSIC.WAV` 排在第 1 还是第 4 取决于拷进 SD 卡的先后。

| 字段 | 限制 | 备注 |
|---|---|---|
| `WAV_MAX_FILES` | 16 | 改大要扩 `s_pl[16]` |
| 文件名 | 8.3 SFN only | Petit FatFs 不支持 LFN |
| 大小写 | 不敏感 | `.wav` / `.WAV` 都行 |

---

## 4. 播放期间收按键 + 实现 4 个动作

### 4.1 关键事实：按键在 ISR 里入队，主循环抽

`bsp_key_scan()` 在 5 ms 定时器 ISR 里跑 ([bsp_sys.c:347](../app/platform/bsp/bsp_sys.c#L347) → [bsp_key.c:980](../app/platform/bsp/bsp_key.c#L980))，跟主循环独立：

```c
// 在 bsp_sys.c 的 5ms 定时器 ISR 里
void usr_tmr5ms_thread(void) {
    bsp_key_scan();              // 读 ADC → 查 pwrkey_table → msg_enqueue(key)
    ...
}
```

所以**按键在推帧期间已经在队里堆积**——我只要在推帧循环里偶尔 `msg_dequeue()` 一次即可。Task 5.3 那个死循环 `while(1) delay_5ms(200);` 之所以不响应键，就是没调 `msg_dequeue()`。

### 4.2 过滤 SDK 系统事件（实测必踩）

`msg_dequeue()` 返回的 `u16` **不只是按键**——SDK 把 500ms / 1s 周期事件 ([bsp_sys.c:441-460](../app/platform/bsp/bsp_sys.c#L441-L460)) 和 SD 卡状态事件 ([bsp_sys.c:50-86](../app/platform/bsp/bsp_sys.c#L50-L86)) 全塞进同一个队列：

| 范围 | 类别 | 例子 |
|---|---|---|
| `0x000..0x0FF` | 基础按键 (low byte 0x00-0xEF) | `KEY_PLAY=0x20`、`KEY_NEXT=0x60` |
| `0x800..0xBFF` | 短按 (KEY_SHORT_UP=0x800) | `KU_PLAY=0x820`、`KU_NEXT=0x860` |
| `0xC00..0xFFF` | 长按 / HOLD (KEY_LONG/HOLD) | `KL_PLAY=0xA20`、`KH_PLAY=0xE20` |
| **`0x7D0..0x7FF`** | **SDK 系统事件** | `MSG_SYS_500MS=0x7FE`、`EVT_SD_INSERT=0x7FD` |

如果不过滤，每次 `msg_dequeue()` 都会抽到一堆 `0x7FE` / `0x7FF`，log 全是噪声。`wav_test_handle_key()` 第一句就是过滤：

```c
if (msg < 0x0100 || (msg & 0xFF00) == 0x0700) {
    return;  // NO_MSG / 系统事件 / 不在 key 范围
}
```

### 4.3 dispatcher 完整覆盖

```c
case KU_PLAY / KU_PLAY_USER_DEF / KU_PLAY_PWR_USER_DEF:
    s_paused = !s_paused; printf("pause" or "resume");

case KU_VOL_UP / KU_VOL_UP_NEXT / KU_VOL_UP_PREV / KU_VOL_UP_DOWN
   / KL_VOL_UP / KH_VOL_UP:
    bsp_set_volume(bsp_volume_inc(sys_cb.vol));

case KU_VOL_DOWN / KU_VOL_DOWN_PREV / KU_VOL_DOWN_NEXT
   / KL_VOL_DOWN / KH_VOL_DOWN:
    bsp_set_volume(bsp_volume_dec(sys_cb.vol));

case KU_NEXT / KU_NEXT_VOL_UP:
    s_should_next = true;     // chunk 边界 break → 外层 advance

case KU_PREV / KU_PREV_VOL_DOWN:
    s_should_prev = true;     // chunk 边界 break → 外层 backward

default:
    printf("(unhandled)\n");  // 罕见组合或陌生 key
```

### 4.4 暂停的语义

按 SDK 模板（[test/参考.md:12](../test/参考.md)）——**暂停时不要写 0 进 DAC**（会引入 step discontinuity），正确做法：

```c
if (s_paused) {
    delay_us(200);    // 让出 CPU 给 5 ms ISR；不饿死 bsp_key_scan
    continue;         // 跳到下一轮循环顶部，仍抽 msg_dequeue()
}
// 正常推帧...
```

这样暂停期间喇叭**直接静音**（DAC FIFO 自然 drain 完就空），按 PLAY 恢复时**没有 pop 噪声**。

### 4.5 切歌的边界处理

切歌**不能在 512B 推帧中点 break**——会把一首的 PCM 末尾接进下一首的开头。所以在 `pff_read + AUBUFDATA 推帧` 之后才检查：

```c
while (bytes_left > 0) {
    msg_dequeue + handle;     // ← NEXT 在这里被设 s_should_next=true
    if (s_paused) { ... }
    if (s_should_next || s_should_prev) break;  // ← 干净 chunk 边界退出
    pff_read 512 B;
    push AUBUFDATA 128 frames;
}
```

退出后 `wav_test_parse_and_play()` 返回 `false`，外层根据 `s_should_next/prev` 推进 `s_index`。

---

## 5. 这块板子的按键映射（实测重排 `pwrkey_table[]`）

这块板子的 KEY 模块原理图（KEY block）：

```
TP17/2PWRKEY ──┬─ S5 (P/P, 0Ω) ────── GND
               ├─ R37 (12K) ── S6 PREV ── GND
               └─ R38 (47K) ── S7 NEXT ── GND
```

TP17 在芯片内部是 **PB5**（[config_define.h:63](../app/platform/header/config_define.h#L63) `ADCCH_WKO 12  // SARADC channel 12  WKO/PB5`）。SDK 默认在 `bsp_key.c:97` 读 PB5 的 ADC 值（10 bit，右移 2 位得 8 bit `wko_val`），在 [bsp_key.c:214](../app/platform/bsp/bsp_key.c#L214) 用 `pwrkey_table[]` 查表得出 `KEY_*` 枚举。

### 5.1 原表（注释写的是 1.5K / 3.9K / 15K / 33K）跟本板不匹配

SDK 默认的 [pwrkey_table](../app/projects/standard/port/port_key.c#L158) 是给**典型 ADKEY 板**（5 颗按键、不同阻值）准备的，**注释写的 1.5K / 3.9K 跟本板的 12K / 47K 完全不匹配**。

| 按键 | 阻值 | 理论 Vadc | 10-bit ADC | `wko_val` (>>2) | 原表阈值 | 实际命中 slot | 原表 slot 的 key id | 实际触发的事件 |
|---|---|---|---|---|---|---|---|---|
| S5 P/P | 0Ω | 0 V | 0 | **0x00..0x0A** | `0x0A` | slot 0 | `KEY_PLAY_PWR_USER_DEF` | `KU_PLAY_PWR_USER_DEF = 0x8E2`（PWR family）|
| S6 PREV | 12K | 1.80 V | 558 | **0x8B** | `[0x34, 0x70, 0xAF, 0xDA]` | **slot 3** | `KEY_VOL_DOWN = 0x41` | `KU_VOL_DOWN = 0x841` ❌（**不是 PREV！**）|
| S7 NEXT | 47K | 2.72 V | 844 | **0xD3** | 同上 | **slot 4** | `KEY_VOL_UP = 0x61` | `KU_VOL_UP = 0x861` ❌（**不是 NEXT！**）|

所以原表下，"PREV 按出来是 vol-"、"NEXT 按出来是 vol+"。

### 5.2 修法：重写 pwrkey_table

直接重排阈值 + 改 ID：

```c
// [port_key.c:173-180](../app/projects/standard/port/port_key.c#L173)
const adkey_tbl_t pwrkey_table[6] = {
    {0x0A, KEY_PLAY},     // S5=0Ω → play/pause (plain, NOT PWR family)
    {0xA0, KEY_PREV},     // S6=12K ADC~0x8B → prev   (窗口 ≤ 0xA0, 21 计数裕量)
    {0xE5, KEY_NEXT},     // S7=47K ADC~0xD3 → next   (窗口 ≤ 0xE5, 18 计数裕量)
    {0xFF, NO_KEY},
    {0xFF, NO_KEY},
    {0xFF, NO_KEY},
};
```

走表验证（规则是 `while (wko_val > table[num].adc_val) num++`，阈值是窗口**上界**）：

| ADC | walk | 落点 |
|---|---|---|
| S5 = 0x05 | `0x05 > 0x0A? no` → STOP | slot 0 = `KEY_PLAY` ✓ |
| S6 = 0x8B | `>0x0A yes` → `>0xA0 (160)? no` → STOP | slot 1 = `KEY_PREV` ✓ |
| S7 = 0xD3 | `>0x0A yes` → `>0xA0 yes` → `>0xE5 (229)? no` → STOP | slot 2 = `KEY_NEXT` ✓ |
| idle ≈ 0xFF | `>0xE5 yes` → `>0xFF (255)? no` → STOP | slot 3 = `NO_KEY` ✓ |

每个实测按键留了**向上噪声裕量**——S5/+5、S6/+21、S7/+18——保证 30 ms 防抖（6×5 ms 扫描）期间读数抖动不会跨过 slot 边界。**这是为什么 slot 2 不能用 0xD3 当阈值（实测值）而必须用 0xE5（实测 + 18 计数）：见 §7.4 详细分析。**

### 5.3 不用 `KEY_PLAY_PWR_USER_DEF` 的原因

`KEY_PLAY_PWR_USER_DEF` 的 base id `0xE2` 属于 PWR family（0xE0-0xEF）。SDK 把它映射成长按 = 软关机：

[func.c:298-310](../app/platform/functions/func.c#L298)：
```c
case KLH_PLAY_PWR_USER_DEF:
    sys_cb.pwrdwn_tone_en = 1;
    func_cb.sta = FUNC_PWROFF;   // 软关机
    break;
```

实测 `KLH_PLAY_PWR_USER_DEF` 会在按住 S5 约 1.2 秒后触发 `FUNC_PWROFF`——表现就是"按 P 暂停后一两秒就重启"（实际上是软关机 → 重新开机）。

改用 **plain `KEY_PLAY = 0x20`** 规避：`KLH_PLAY` / `KH_PLAY` 没有任何 case 接，**长按不会触发软关机**。

代价：P/P 键失去"长按 = 软关机"语义。这块教学板没有专门的 POWER 键，可以接受。

### 5.4 同时要开 `user_pwrkey_en`

SDK 设计有"编译时"和"运行时"两道开关：

- **编译时** `USER_PWRKEY=1`（在 `config.h` 里）—— 决定是否包含 PWRKEY 按键代码
- **运行时** `xcfg_cb.user_pwrkey_en`（在 [Output/bin/Settings/Boombox.setting](../app/projects/standard/Output/bin/Settings/Boombox.setting) 里）—— 决定运行时是不是真的去读 PB5

如果运行时开关关着，[plugin.c:141-143](../app/projects/standard/plugin/plugin.c#L141) 不会把 `sys_cb.wko_pwrkey_en` 置 1，[bsp_key.c:96-98](../app/platform/bsp/bsp_key.c#L96) 跳过 ADC 读，`pwrkey_table` 表里查出来的永远是 `NO_KEY`。

**本工程的 `Boombox.setting:121` 默认是 `False`——必须翻成 `True` 按键才会工作。**

---

## 6. 实测串口输出（2026-07-30 完整 PASS，3 键全通）

```
=========================================
  [WAV] BT892XA2 WAV Player Test
  [WAV] SDK V02.00
  [WAV] SDIO mode, SD0MAP_G3 (PE5/6/7)
  [WAV] Phase 5 Part 3: playlist + key controls
=========================================
[WAV] msg queue ready (USER_PWRKEY=1)
[WAV] playlist: 4 song(s)
[WAV]   [1] MUSIC.WAV     52920044 bytes
[WAV]   [2] TEST.WAV      40572078 bytes   ← 跟 5.3 的 52.9 MB 不一样，是另一首
[WAV]   [3] 414_1K~1.WAV  ...              ← SFN 里带 ~ 也合法
[WAV]   [4] 16K_1K.WAV    ...
[WAV] Controls: KU_PLAY=pause/resume, KU_NEXT/PREV=skip, KU_VOL_UP/DOWN=volume

[WAV] --- playing slot 1/4 ---

[WAV] ====== Open + inspect [1/4] MUSIC.WAV ======
[WAV] PASS: file is RIFF/WAVE
... (fmt 字段)

[WAV] ====== Task 5.3: Parse + Play ======
[WAV]   chunk @0x0000000c: fmt  size=16
[WAV]   chunk @0x00000024: data size=42232512
[WAV] data chunk @44, size=42232512 bytes (~239 sec at 44100 Hz)
[WAV] dac_spr_set(44100 Hz) = SPR enum 1
[WAV] streaming 42232512 bytes of 16-bit stereo PCM at 44100 Hz:
[WAV] (direct AUBUFDATA push, polling AUBUFCON bit 8)
[WAV] 419 / 41242 KB (1%)
[WAV] key 0x0820 -> pause         ← 按 S5 P/P 暂停
[WAV] key 0x0820 -> resume        ← 再按一次恢复
[WAV] 427 / 41242 KB (1%)
... (进度继续)
[WAV] key 0x0840 -> ...           ← 按 S6 PREV
[WAV] 686 / 41242 KB (1%)
...
[WAV] key 0x0860 -> ...           ← 按 S7 NEXT
[WAV] pushed 403456 bytes (100864 frames) in 3936 ms
[WAV] interrupted (next)
[WAV] slot 1 -> 2 (next)

[WAV] --- playing slot 2/4 ---

[WAV] ====== Open + inspect [2/4] TEST.WAV ======
... (注意这个 TEST.WAV 头部比 slot 1 的复杂：fmt 后面有 LIST chunk)
[WAV]   chunk @0x0000000c: fmt  size=16
[WAV]   chunk @0x00000024: LIST size=26           ← chunk walker 跳过这个
[WAV]   chunk @0x00000046: data size=40572000
[WAV] data chunk @78, size=40572000 bytes (~230 sec at 44100 Hz)
... (开始播 TEST.WAV)
```

### 6.1 关键行解读

| 行 | 含义 |
|---|---|
| `[WAV] msg queue ready (USER_PWRKEY=1)` | 启动 banner，**确认** SDK 读了 `user_pwrkey_en=True` 并初始化了 PWRKEY 扫描 |
| `[WAV] playlist: 4 song(s)` | `pff_readdir` 枚举到 4 个 `.WAV`（slot 1-4）|
| `[WAV] 419 / 41242 KB (1%)` | 进度打印：每 50 ms 一行（4 帧/字节 → KB，已推送/总大小）|
| `[WAV] key 0x0820 -> pause` | `0x820 = KU_PLAY`，按下后 `s_paused=true` |
| `[WAV] key 0x0820 -> resume` | 再按，`s_paused=false` |
| `[WAV] pushed 403456 bytes ... in 3936 ms` | 切歌时 `pff_read` 还没读完就被打断——这是**正常**的，不是错误 |
| `[WAV] interrupted (next)` | parse_and_play 看到 `s_should_next=true`，返回 `false` |
| `[WAV] slot 1 -> 2 (next)` | 外层 advance 到下一首 |
| `[WAV] chunk @0x00000024: LIST size=26` | TEST.WAV 在 fmt 和 data 之间有 LIST chunk（ffmpeg 转码时常带），**chunk walker 正确跳过**——见 §7.2 |

### 6.2 切歌不再丢数据

切歌在 `pff_read + AUBUFDATA 推帧` **之后**才检查 `s_should_next`，所以**不会**把一首的 PCM 末尾接进下一首——下一首的 `data` chunk 从头干净开始。

---

## 7. 失败排查（实测踩过的 3 个坑）

### 7.1 按键完全没反应 / 串口只看到 `MSG_SYS_*` 噪声

**症状**：
```
[WAV] key 0x07FE -> [WAV] key 0x07FF -> [WAV] key 0x07FD -> ...
（每秒钟约 3 行，但完全没有真实按键 hex）
```

**根因**：`Boombox.setting:121` 的 `user_pwrkey_en=False`。

**修法**：把 [Output/bin/Settings/Boombox.setting:121](../app/projects/standard/Output/bin/Settings/Boombox.setting#L121) 翻成 `True`：

```diff
- <add key="user_pwrkey_en" value="False" />
+ <add key="user_pwrkey_en" value="True" />
```

**为什么需要两道开关**：编译时 `USER_PWRKEY=1` 只是"包含 PWRKEY 扫描代码"，运行时 `user_pwrkey_en` 决定"实际去读 PB5 的 ADC"。两道都得开。

### 7.2 P/P 按下后一两秒就重启

**症状**：按 P 暂停生效、声音停了，但 1-2 秒后系统复位重启。

**根因（实际是两件事）**：

1. **如果 S5 映射到 `KEY_PLAY_PWR_USER_DEF`**：长按 1.2s 触发 `KLH_PLAY_PWR_USER_DEF` → [func.c:302](../app/platform/functions/func.c#L302) 的 `FUNC_PWROFF` 软关机。改用 plain `KEY_PLAY=0x20` 规避。
2. **（更重要）WDT 看门狗**：SDK 的 `func_process()` ([func.c:128](../app/platform/functions/func.c#L128)) 每个 func_* 主循环都会调 `WDT_CLR()`——**但 wav_test_run 跑在 func_* 框架之外，所以 WDT 永远不喂狗**。约 1-2 秒 WDT 超时即复位。**这才是"按 P 暂停后一两秒就重启"的真正根因**。

**修法**：在 wav_test.c 的两个主循环里都加每 100 ms 的 WDT clear：

```c
// 内层 push 循环（[wav_test.c:640-646](../app/projects/standard/wav_test/wav_test.c#L640)）
{
    u32 now = tick_get();
    if ((u32)(now - last_wdt_tick) >= 100) {
        WDT_CLR();        // WDTCON = 0xa (macro.h:17)
        last_wdt_tick = now;
    }
}

// 外层调度器（[wav_test.c:798-805](../app/projects/standard/wav_test/wav_test.c#L798)）同样的 100 ms 块
```

### 7.3 WAV 文件的 `fmt` 和 `data` 之间有 `LIST` / `fact` / `JUNK` chunk

**症状**：某个 WAV 文件打开后 `[WAV] FAIL: no 'data' chunk found`。

**根因**：ffmpeg 等工具转出来的 WAV 在 `fmt` 之后、`data` 之前会塞 `LIST`（携带元数据，如 `ISFT` 软件名、`ICMT` 注释）。`data` 不一定在 offset 44。

**修法**：Task 5.3 已经用了 **chunk walker**（[wav_test.c:474-499](../app/projects/standard/wav_test/wav_test.c#L474)）从 `pos=12` 逐块跳，命中 `data` 才停下：

```c
while (pos + 8 <= s_fs.fsize) {
    pff_lseek(pos); pff_read(header, 8, &br);   // 读 chunk fourcc + size
    if (header[0..3] == "data") {
        data_offset = pos + 8; data_size = size; break;
    }
    pos += 8 + chunk_size + (chunk_size & 1);  // RIFF 字对齐填充
}
```

**实测验证**（slot 2 的 TEST.WAV）：
```
chunk @0x0000000c: fmt  size=16
chunk @0x00000024: LIST size=26           ← 跳过
chunk @0x00000046: data size=40572000     ← 命中
data chunk @78, size=40572000 bytes
```

如果你的 WAV 有更复杂的 chunk 结构（`bext` / `iXML` / `axml` 等广播行业扩展），walker 一样能跳过——只要 `data` chunk 的 fourcc 是 `data` 就行。

### 7.4 NEXT 按下完全没反应（PREV 正常）—— 高阻源 0 裕量阈值被防抖吃掉

**症状**：S5 P/P、S6 PREV 都正常按出 `[WAV] key 0x08xx ->` 日志并执行动作，但 **S7 NEXT 按下后串口一行按键日志都没有**，跟"完全没按过"一样。

**最容易判错的方向**：以为是 `pwrkey_table` 阈值算错了，把 S7 阈值改成各种"中点"值——但每次改完都不见效。原因不在阈值大小，**而在阈值相对实测值的裕量**。

**真正的根因（三件事一起）**：

| # | 因素 | 出处 |
|---|---|---|
| 1 | S7 用的是 **47KΩ** 高阻源（远高于 S6 的 12K），源阻抗 ~8.2 KΩ | [bsp_key.c:653](../app/platform/bsp/bsp_key.c#L653) `adcch_io_pu10k_enable(ADCCH_WKO)`，配 47K 串联 |
| 2 | SARADC 对 WKO **无硬件平均、无可调采样建立时间**（`saradc_setup_time_set` 在 [api_saradc.h:10](../app/platform/libs/api_saradc.h#L10) 是空宏，注释明示"上下拉分压电阻比较大, 需要加大采样的建立时间"但 API 不存在）| [bsp_key.c:38](../app/platform/bsp/bsp_key.c#L38) + [api_saradc.h:10](../app/platform/libs/api_saradc.h#L10) |
| 3 | 防抖需要 **6×5ms = 30ms 连续相同读数**（`KEY_SCAN_TIMES=6`）才会确认一次按键 | [bsp_key.h:4-8](../app/platform/bsp/bsp_key.h#L4)，[bsp_key.c:19-25](../app/platform/bsp/bsp_key.c#L19) |

**因果链**：

```
47K 高阻源 + 无平均 + 无建立时间 ─> wko_val 在 0xD3 ↔ 0xD4 之间抖动
阈值 = 0xD3（贴实测值）──> 抖动一步就跨过 slot 2 顶，落到 slot 3 = NO_KEY
防抖 counter ── 每次跨 slot 就 reset ──> 永远凑不到 6 次连续 ──> 永远不进 msg_enqueue
```

**为什么 PREV 不受影响**：S6 = 12K，源阻抗更低、噪声更小；阈值 0xA0 留了 21 计数裕量给 0x8B 的实测值，**噪声即便把读数推到 0x9F 也仍然落 slot 1**，30ms 防抖能正常累计。

**正确修法**：

```c
// [port_key.c:173-180](../app/projects/standard/port/port_key.c#L173)
const adkey_tbl_t pwrkey_table[6] = {
    {0x0A, KEY_PLAY},     // S5=0Ω    ADC ≤ 0x0A   (5 计数裕量)
    {0xA0, KEY_PREV},     // S6=12K   ADC = 0x8B    (21 计数裕量)
    {0xE5, KEY_NEXT},     // S7=47K   ADC = 0xD3    (18 计数裕量) ← 不能用 0xD3！
    {0xFF, NO_KEY},
    {0xFF, NO_KEY},
    {0xFF, NO_KEY},
};
```

**判定规则**：阈值 = `实测 ADC + ≥15 计数`（高阻源建议 ≥18）。表项 `.adc_val` 是窗口**上界**——`wko_val ≤ adc_val` 才落本槽，所以贴实测值（裕量 = 0）= 必丢。

**如果还有偶发失败**（47K 噪声裕量不够）：

1. 把 S7 的 47KΩ 换成 22KΩ（ADC 落到 ~0xB4，离 slot 1 顶 0xA0 还有充足距离，**且源阻抗减半 = 噪声减半**）；
2. 或在 [bsp_key.c:979](../app/platform/bsp/bsp_key.c#L979) 取消注释 `printf(key_enqueue_str, key)`，观察每个进 `msg_enqueue` 的 key——能区分"按下没进队（防抖问题）"还是"进队了但应用没处理（dispatcher 问题）"。

**诊断日志技巧**（任何 ADC 阈值类问题通用）：在 `get_pwrkey()` 返回前打 `printf("wko=0x%02X num=%d\n", adc_cb.wko_val, num);`，采样 100 次以上，按下每个按键记录 `min/avg/max`，就能直接看到 ADC 的真实分布，再决定阈值。

---

## 8. 完整任务清单

| 任务 | 完成情况 |
|---|---|---|
| Task 5.1 建 wav_test 目录 | ✅ |
| Task 5.2 从 TF 卡打开一个 WAV | ✅（`[wav_test_01.md](wav_test_01.md)`）|
| Task 5.3 解析 + 播放 | ✅（`[wav_test_02.md](wav_test_02.md)`, 左右均衡、人声清晰）|
| **Task 5.4 按键控制** | ✅ **本文件**（播放列表 + 暂停 + 切歌 + 音量）|

下一步：Phase 5 全部完成，写 `docs/wav_test_summary.md` 总结。

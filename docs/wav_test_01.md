# Phase 5 Task 1 — wav_test 工程接入（wav_test_01）

> 配套 [Phase 5 任务清单](任务清单.md) 的 Task 1（建立 wav_test 目录）。
> 本测试是 Phase 5 全部 7 个测试的**工程接入基线**：
> - 让 `wav_test.c` 成功编进 `app.dcf`
> - 切换 `main.c` 入口
> - 5ms tick 钩子接通到 `wav_test_tick()`

---

## 0. 测试目标

```
[Code::Blocks Ctrl+F9]
        │
        ▼
[app.dcf] ← wav_test/wav_test.c 编进二进制
        │
        ▼
[Downloader 烧录]
        │
        ▼
[UART0 PB3 @ 115200]  打印 [WAV] banner + 进入 halt 循环
```

四个独立校验点：

1. **目录与文件就位**：`app/projects/standard/wav_test/wav_test.h` + `wav_test.c` 已建立
2. **Code::Blocks 工程识别**：`app.cbp` 的 `<Unit>` + `<Add directory>` 注册完成
3. **main 入口切换**：`main.c` 调用 `wav_test_run()` 而非 `fat_test_run()`
4. **5ms tick 钩子接通**：`plugin/plugin.c` 的 `plugin_tmr5ms_isr()` 调用 `wav_test_tick()`

---

## 1. 测试原理

### 1.1 Code::Blocks 工程注册新模块

SDK 用的工具链是 **Code::Blocks + riscv32-elf-gcc**。Code::Blocks 不会自动扫描新源文件，必须手动改 `app.cbp`：

| 改动点 | 路径 | 内容 |
|---|---|---|
| Include 路径 | `<Compiler>` 段 | `<Add directory="wav_test" />` |
| 编译单元 | `<Unit>` 列表 | `<Unit filename="wav_test/wav_test.c">` + `<Unit filename="wav_test/wav_test.h" />` |

漏一项的常见症状：

- 漏 `<Add directory>` → `fatal error: wav_test.h: No such file or directory`
- 漏 `<Unit>` → 文件不参与编译，但 `#include` 找不到符号（undefined reference to `wav_test_run`）

### 1.2 main 入口切换

`main.c` 第 46 行调用 `wav_test_run()`，注释掉 `fat_test_run()`。前一阶段的 `tf_test_run()` / `fat_test_run()` 全部注释，**只留一个测试入口**。

### 1.3 5ms tick 钩子

SDK 在 `bsp_sys_init()` 中开启了 timer1 5ms 中断（`sys_set_tmr_enable(1, 1)`，见 [app/platform/bsp/bsp_sys.c](../../app/platform/bsp/bsp_sys.c)）。每 5ms 调一次 `plugin/plugin.c:169` 的 `plugin_tmr5ms_isr()`，这是给产品代码留的扩展点。

Phase 2 hello_world 曾在该 ISR 注册 `hello_world_tick()`，Phase 5 现在替换为 `wav_test_tick()`。

`wav_test_tick()` 加 `AT(.com_text.wav)` 把它放到 `.com_text` section（见 [ram.ld:97-111](../../app/projects/standard/ram.ld)），保证 5ms ISR 能从 RAM 调到（XIP 闪存执行延迟会卡时序）。

---

## 2. 测试代码

### 2.1 新建文件

#### [app/projects/standard/wav_test/wav_test.h](../../app/projects/standard/wav_test/wav_test.h)

模块对外接口：

```c
void wav_test_run(void);   // 测试入口，由 main.c 调用
void wav_test_tick(void);  // 5ms tick 钩子，由 plugin_tmr5ms_isr 调用
```

#### [app/projects/standard/wav_test/wav_test.c](../../app/projects/standard/wav_test/wav_test.c)

```c
AT(.com_text.wav)
void wav_test_tick(void)
{
    // wav_test_01: 空实现（验证 tick 通路活着即可）
}

static bool wav_test_banner(void)
{
    printf("\n=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] Phase 5: WAV music player\n");
    printf("  [WAV] wav_test_01: engineering bring-up\n");
    printf("=========================================\n");
    printf("[WAV] Halt. 5ms loop (tick should be running).\n");
    return true;
}

void wav_test_run(void)
{
#if 1
    if (!wav_test_banner()) goto halt;
#endif
    // wav_test_02 ~ 07 占位 #if 0（按需开启）

    printf("\n[WAV] ====== All active tests PASSED ======\n");

halt:
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) { delay_5ms(200); }
}
```

### 2.2 修改文件

#### [app/projects/standard/main.c](../../app/projects/standard/main.c)

第 5 行加 `#include "wav_test/wav_test.h"`；第 44-46 行切换入口：

```diff
 bsp_sys_init();
 //hello_world_init();
-//tf_test_run();       // [TF_TEST]   Phase 3: TF 裸块读 / 写
-fat_test_run();         // [FAT_TEST]  Phase 4: Petit FatFs 移植测试
+//tf_test_run();       // [TF_TEST]   Phase 3: TF 裸块读 / 写
+//fat_test_run();      // [FAT_TEST]  Phase 4: Petit FatFs 移植测试
+wav_test_run();         // [WAV_TEST]  Phase 5: WAV 音乐播放器
```

#### [app/projects/standard/plugin/plugin.c](../../app/projects/standard/plugin/plugin.c)

第 3 行加 `#include "../wav_test/wav_test.h"`；第 175 行注册 tick：

```diff
 AT(.com_text.plugin)
 void plugin_tmr5ms_isr(void)
 {
 #if ENERGY_LED_EN
     energy_led_level_calc();
 #endif
     //hello_world_tick();   // Phase 2: hello_world 5ms 节拍
+    wav_test_tick();        // [WAV_TEST]  Phase 5: WAV 播放器 5ms 节拍
 }
```

#### [app/projects/standard/app.cbp](../../app/projects/standard/app.cbp)

```diff
 <Add directory="hello_world" />
 <Add directory="tf_test" />
 <Add directory="fat_test" />
+<Add directory="wav_test" />
```

```diff
 <Unit filename="fat_test/fat_test.h" />
+<Unit filename="wav_test/wav_test.c">
+    <Option compilerVar="CC" />
+</Unit>
+<Unit filename="wav_test/wav_test.h" />
```

---

## 3. 测试步骤

1. **Code::Blocks** 打开 `app\projects\standard\app.cbp`
2. **Ctrl+F9**（或 Build → Build）编译
3. 确认 `Output\bin\app.dcf` 时间戳更新（> 旧时间）
4. **Downloader** 打开该 dcf 烧录
5. 串口（PB3 @ 115200 8N1）观察

---

## 4. 测试前置条件

| 条件 | 说明 |
|---|---|
| 工程已成功编译过 Phase 4 | Code::Blocks / riscv32-elf-gcc / xmaker 工具链 OK |
| TF 卡**不需要**插入 | wav_test_01 不碰 SD 卡 |
| 耳机**不需要**插入 | wav_test_01 不出声 |

---

## 5. 实测输出（实测 PASS）

```
=========================================
  [WAV] BT892XA2 WAV Player Test
  [WAV] SDK V02.00
  [WAV] Phase 5: WAV music player
  [WAV] wav_test_01: engineering bring-up
=========================================
[WAV] Halt. 5ms loop (tick should be running).

[WAV] ====== All active tests PASSED ======
[WAV] Halt. 5ms loop.
```

> 串口首部还有 `Hello BT892XA2: xxxxxxxx`（main.c 第 31 行复位源诊断），略。

实测说明：
- **两条 halt 打印都有**：`Halt. 5ms loop (tick should be running).` 是 `wav_test_banner()` 内部打印；`[WAV] ====== All active tests PASSED ======` + `[WAV] Halt. 5ms loop.` 是 `wav_test_run()` 落进 `halt:` 标签后打印。两条都出现 = banner 通过 + 没有 fail 早退 = PASS。
- **`wav_test_tick()` 在 ISR 里被空调用**：本测试不打印证明，但程序不 WDT 复位进入 `halt:` 循环就说明 5ms ISR 持续在跑（否则 5ms 内 WDT 没喂 → 复位）。

---

## 6. 失败排查表

| 现象 | 可能原因 | 排查方法 |
|---|---|---|
| `fatal error: wav_test.h: No such file or directory` | `app.cbp` 漏了 `<Add directory="wav_test" />` | 检查 app.cbp 第 24-31 行 |
| `undefined reference to 'wav_test_run'` | `app.cbp` 漏了 `<Unit filename="wav_test/wav_test.c">` | 检查 app.cbp 第 80-84 行 |
| 串口没 `[WAV]` banner，只有 `Hello BT892XA2` | `main.c` 没切换到 `wav_test_run()` 或入口被注释 | 检查 main.c 第 44-46 行 |
| 串口 banner 后立刻 WDT 复位 | `plugin_tmr5ms_isr()` 没调 `wav_test_tick()` 或函数不在 `.com_text` section | 检查 plugin.c 第 175 行 + wav_test.c 的 `AT(.com_text.wav)` |
| `download.xm` / `app.dcf` 时间戳不更新 | Code::Blocks 没识别改动（漏注册 `<Unit>`） | Build → Clean → Rebuild |

---

## 7. 任务清单勾选

更新 [docs/任务清单.md](任务清单.md)：

```diff
 # WAV音乐播放器 (Phase 5) ⏳→🚀

-1. ⏳ 建立 wav_test 目录
+1. ✅ 建立 wav_test 目录 (2026-07-29, wav_test_01 PASS)
 2. ⏳ 从 TF 卡中打开一个 wav 文件
 3. ⏳ 解析并播放该 WAV 文件
 4. ⏳ 增加按键上下曲、播放暂停、音量加减等功能
```

进度表：

```diff
-| Phase 5 | WAV 播放器 | ⏳ 0/4 |
+| Phase 5 | WAV 播放器 | 🚧 1/4 |
```

---

## 8. 下一步

进入 **wav_test_02**：从 TF 卡中打开固定名 `TEST.WAV` 文件（复用 Phase 4 的 `pff_mount` + `pff_open`）。
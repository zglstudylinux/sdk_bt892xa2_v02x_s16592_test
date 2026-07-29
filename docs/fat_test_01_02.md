# Phase 4 Task 4.1 + 4.2 + 4.3 — 建目录 / 学习 / 移植 Petit FatFs

> 配套 [Phase 4 任务清单](任务清单.md) "文件系统" 模块的 Task 4.1 / 4.2 / 4.3。
> Task 4.4（文件读写）单独写在 [fat_test_04.md](fat_test_04.md)。
> Phase 4 整体总结在 [fat_test_summary.md](fat_test_summary.md)。

---

## 0. 这份文档要回答的问题

| 问题 | 答案在本节 |
|---|---|
| 为什么要自己移植 Petit FatFs？SDK 已经有 FS API 了 | §1 |
| Petit FatFs 长什么样？哪些文件？API 是什么？ | §2 |
| 我们怎么把它接到 BT892XA2 SDK 上？ | §3 |
| 移植要踩哪些坑？ | §4 |
| 跑通 pff_mount 应该看到什么？ | §5 |
| 失败了怎么排查？ | §6 |

---

## 1. 为什么要自己移植？

[app/platform/libs/api_fs.h](../app/platform/libs/api_fs.h) 已经有 SDK 自带的 FS 接口：

```c
FRESULT fs_open(const char *path, u8 mode);
FRESULT fs_read (void* buff, UINT btr, UINT* br);
FRESULT fs_write(void* buff, UINT btw);
...
```

但这些都是**闭源**的（实现藏在 [libplatform.a](../app/platform/libs/) 里），我们看不到 `fs_open` 怎么定位目录项、`fs_read` 怎么走 FAT 链。这违背了"看清每一层"的学习目标。

所以 Phase 4 用 **Petit FatFs**（ChaN 写的极简 FAT 实现，[BSD-style license](https://elm-chan.org/fsw/ff/00index_p.html)）来代替闭源 FS 层。整套源码只有 ~2K 行 C，我们能逐行读完每一句 `pff_mount` / `pff_read` / `pff_write`。

---

## 2. Petit FatFs 长什么样

### 2.1 文件清单

| 文件 | 来源 | 用途 |
|---|---|---|
| `pff.h` | 上游 ChaN R0.03a | 类型 + API 声明 |
| `pff.c` | 上游 ChaN R0.03a | FAT 协议实现 |
| `pffconf.h` | 我们写 | 功能开关（开 write / 开 FAT32） |
| `diskio.h` | 上游 ChaN | disk I/O 接口 |
| `diskio.c` | **我们写** | 桥接到 BT892XA2 SDK |
| `fat_test.h/c` | 我们写 | 测试入口 |
| `PETIT_FATFS_LICENSE.txt` | 上游 ChaN | 许可证 |

下载源：[elm-chan.org/fsw/ff/arc/pff3a.zip](https://elm-chan.org/fsw/ff/00index_p.html)

### 2.2 API 速查

```c
/* 装载卷（必须最先调用） */
FRESULT pff_mount(FATFS* fs);

/* 打开文件（短文件名，8.3 格式） */
FRESULT pff_open(const char* path);

/* 读 / 写 / seek */
FRESULT pff_read (void* buff, UINT btr, UINT* br);
FRESULT pff_write(const void* buff, UINT btw, UINT* bw);
FRESULT pff_lseek(DWORD ofs);

/* 目录遍历（默认关，我们没用到） */
FRESULT pff_opendir (DIR* dj, const char* path);
FRESULT pff_readdir (DIR* dj, FILINFO* fno);
```

### 2.3 关键限制

- **只能追加写到已分配簇**：pff_write 不能扩展文件大小——文件必须先在 PC 上创建并分配好簇（哪怕只是 1 个 cluster）
- **短文件名**：只支持 8.3 格式（`README.TXT`，最长 8+3=11 字节）
- **单文件、单卷**：同时只能打开 1 个文件
- **没有 mkdir / delete / rename**
- **没有长文件名**

这些都是"为 8-bit MCU 设计的极简 FS"的代价。Phase 4 只用前 3 个 API 就够了。

### 2.4 内存占用

- FATFS 结构体：~40 B
- DIR：~16 B
- FILINFO：~32 B
- 代码段：~2-4 KB（开 write 后接近 4 KB）

我们的 `.bss`（13 KB）还很富裕。

---

## 3. 怎么把它接到 BT892XA2

### 3.1 SDK 暴露的 SD 卡 API

[app/platform/libs/api_sd.h](../app/platform/libs/api_sd.h)：

```c
void  sd_gpio_init(u8 type);    // GPIO mux + FUNCMCON0=SD0MAP_G3
bool  sd0_init   (void);        // 完整鉴定（CMD0/8/ACMD41/...）
bool  sd0_read   (void* buf, u32 lba);   // 读 1 个 512 字节扇区
bool  sd0_write  (void* buf, u32 lba);   // 写 1 个 512 字节扇区
```

**关键限制**：SDK **只能整扇区**读写，没有"partial sector"。

Petit FatFs 通过 `disk_readp(buff, sector, offset, count)` 和 `disk_writep(buff, sc)` 走 partial sector 协议——所以我们要么改 pff.c，要么在 diskio.c 里加 cache。

### 3.2 我们的 diskio.c 做了什么

[fat_test/diskio.c](../app/projects/standard/fat_test/diskio.c) 维护两个 512 字节静态 buffer：

| Buffer | 作用 |
|---|---|
| `s_rd_cache[512]` | 读缓存。disk_readp 调用时整扇区读到 cache，再 memcpy 给调用方 |
| `s_wr_cache[512]` | 写暂存。disk_writep 三阶段协议：init(选 sector, 拷旧内容到 cache) → data(往 cache 写) → finalize(sd0_write cache 到 disk) |

代码里 `read_full_sector(lba)` 检查命中——同 sector 第二次读直接走 cache，省一次 SD 总线往返。

### 3.3 **重要**：重命名避开 SDK 符号

`libplatform.a` / `libdrivers.a` 里**已经定义了** `disk_initialize` / `disk_readp` / `disk_writep`（看 [Output/bin/map.txt](../app/projects/standard/Output/bin/map.txt) 第 3499/9766/9769 行，`.text.disk_*`）。如果我们也定义同名符号会**链接冲突**。

解法：把所有"我们的 disk I/O 实现"重命名为 `pff_disk_*`，定义在 [fat_test/diskio.c](../app/projects/standard/fat_test/diskio.c)。在 [fat_test/diskio.h](../app/projects/standard/fat_test/diskio.h) 里有这些 `pff_disk_*` 的原型声明。

然后为了不改 pff.c 里的每一处 `disk_initialize()` / `disk_readp()` / `disk_writep()` 调用，加了一个**只在 pff.c 里 include 的小头** [fat_test/pff_compat.h](../app/projects/standard/fat_test/pff_compat.h)：

```c
#define disk_initialize pff_disk_initialize
#define disk_readp      pff_disk_readp
#define disk_writep     pff_disk_writep
```

`pff.c` 在 include diskio.h **之前** include pff_compat.h——之后所有 `disk_*` 调用都被宏重写为 `pff_disk_*`。我们的 diskio.c 定义后者，链接到它。SDK 的 `disk_*` 符号在 lib*.a 里"休眠"（没人 reference 它们，所以不被 pull in）。

> **为什么宏不直接放 diskio.h 里**？因为 `#define` 在整个 TU 里都 active——diskio.c、fat_test.c 都会 include diskio.h，然后间接 include `api_fs.h`（也声明 `disk_readp`/`disk_writep`，但签名不同）。宏会污染 API 的声明，导致 "conflicting types"。把宏隔离到 pff_compat.h 里，**只有 pff.c 受影响**，其他 TU 看到的是干净的 diskio.h。

### 3.4 类型兼容

`pff.h` 想 typedef `BYTE/UINT/WORD/DWORD`，但 [app/platform/header/typedef.h](../app/platform/header/typedef.h) 已经 typedef 了同样的名字。我们把 pff.h 的 typedef 块改成 `#ifndef BYTE` / `#ifndef UINT` 等单 type 守卫——已有则跳过，未有则按 ChaN 规则定义。

`FRESULT` 和 `DRESULT` 不能用同样的 `#ifndef` 守卫（见 §4.1），所以直接删掉这俩 enum 定义，统一从 SDK 的 `api_fs.h` 拿。

---

## 4. 移植踩到的坑

| 坑 | 解决 |
|---|---|
| 1. SDK 已有 disk_* 符号 → 链接冲突 | 重命名为 `pff_disk_*` + `pff_compat.h` 宏重写 |
| 2. 宏 `#define disk_* pff_disk_*` 写在 diskio.h 里 → 所有 TU 都被宏污染 → 与 SDK api_fs.h 签名冲突 | 宏搬到独立的 `pff_compat.h`，**只有 pff.c include 它** |
| 3. **`#ifndef FRESULT` / `#ifndef DRESULT` 守卫无效** —— `#ifndef` 只检测**预处理器宏**，不检测 typedef 名。api_fs.h 的 `typedef enum { ... } FRESULT` 不定义 `FRESULT` 宏，所以守卫照样进 block 重定义 → "redeclaration of enumerator" | **直接删掉 pff.h 和 diskio.h 里的 FRESULT/DRESULT enum 定义**，统一从 api_fs.h 拿 |
| 4. pff.h typedef 与 SDK typedef.h 冲突 | 用 `#ifndef BYTE/UINT/...` 单 type guard（typedef.h 也是用 typedef 不创建宏——但这里 typedef.h 没有重定义冲突问题，所以不需要 guard） |
| 5. diskio.c 的 `#include "diskio.h"` 排在 `#include "include.h"` 之前 → diskio.h 先定义 enums → api_fs.h 重新定义 → "redeclaration of enumerator" 错误 | diskio.c 改成 **include.h 先 include**，diskio.h 后 include |
| 6. **diskio.c 第 15 行 `RES_*/BYTE` 中的 `*/` 提前结束注释** → 后续内容被当作 C 代码解析 | 把 `RES_*` 改成 `RES_OK`，避免字面 `*/` 出现在 `/* */` 注释里 |
| 7. **SDK 的 libdrivers.a(pff.o) 已经定义了 pf_mount/pf_open/pf_read/pf_write/pf_lseek** → 与我们的 pff.c 链接冲突 | 我们全部改名为 `pff_*`（`pff_mount` / `pff_open` 等），SDK 的 `pf_*` 没人 reference 自然淘汰 |
| 8. **`.bss` 13KB 装不下 1KB cache + 1KB write staging** → "region `data' overflowed by 752 bytes" | 把 `s_rd_cache` 和 `s_wr_cache` 用 `__attribute__((section(".diskio_buf")))` 放进 SDK 在 `ram.ld` 预留的 `.aram_music` 段（0x3800~0x4000 共 2KB） |
| 9. **`.aram_music` 0x4000 硬上限推不回去** → "cannot move location counter backwards (from 0x54194 to 0x54000)" | SDK 的 `.pff.buf` 占 404 B 在同一个槽里。合并为单个 512 B `s_cache`（read-modify-write 协议），总占用 916 B << 0x800 槽大小 |
| 9. disk_writep 没有 partial-sector 支持（SDK 只有整扇区） | 自维护 512B 写暂存 + 三阶段协议（init/data/finalize） |
| 10. disk_readp 每次都 sd0_read 太慢 | 加 s_rd_cache 命中短路 |
| 11. pff_write 不增长文件大小 | 测试用预创建文件，不依赖运行时新建 |

### 4.1 关键教训：typedef 不等于宏

这是这次踩的最大、最隐蔽的坑：

```c
/* api_fs.h */
typedef enum {
    FR_OK = 0,
    FR_NOT_READY = 3,    // ← 注意值是 3，不是 2
    ...
} FRESULT;
```

```c
/* pff.h 第一版 —— 以为这能防重复定义 */
#ifndef FRESULT
typedef enum {
    FR_OK = 0,
    FR_NOT_READY = 2,    // ← 这里值是 2
    ...
} FRESULT;
#endif
```

直觉上 `#ifndef FRESULT` 应该看到 api_fs.h 已经定义了 `FRESULT` 而跳过——但**它不会**。`#ifndef` 是**预处理器指令**，只检测**宏定义**（`#define FRESULT ...`）。`typedef` 是 C 编译期的语法，不是预处理宏。所以：

1. api_fs.h 被 include → typedef FRESULT → **没有创建宏**
2. pff.h 被 include → `#ifndef FRESULT` → FRESULT **不是宏** → 检查为 TRUE → 进 block 重定义 → **"redeclaration of enumerator"**

而且就算 typedef 重定义允许，**值还会冲突**（api_fs.h 的 FR_NOT_READY=3 vs 我们的 FR_NOT_READY=2）——任何比较 `fr == FR_NOT_READY` 的代码逻辑会错乱。

**最终修法**：

- 删掉 pff.h 里的 FRESULT enum 定义
- 删掉 diskio.h 里的 DRESULT enum 定义
- 所有 TU 必须在 include 我们头文件**之前**先 include api_fs.h（通过 `include.h`）

> 经验法则：**typedef 名永远不能靠 `#ifndef` 守卫重复定义**。要么 include 顺序约定好，要么每个 typedef 只在一个头文件里定义一次。

### 4.2 关键教训：注释里的 `*/`

`RES_*/BYTE` 里的 `*/` 让 C 词法分析器误以为注释提前结束。后续的 `/BYTE/UINT etc.` 被当成代码 → 语法错误。

> **C 注释里不要出现字面 `*/`**。最稳是 `RES_OK` / `RES_xxx` / `RES_enum` 等。

### 4.3 关键教训：include 顺序和宏作用域

C 预处理器的 `#define` 在**整个翻译单元**里都是活动的（直到 `#undef` 或文件结束）。如果宏定义在 `diskio.h`，而 `diskio.h` 被 diskio.c、fat_test.c 等多个 .c include，那么所有这些 .c 文件**同时**也间接 include 了那个宏。

后果：SDK 的 `api_fs.h` 里也声明了 `disk_readp`/`disk_writep`（不同签名）。如果 diskio.h 的宏 active 之后再 include `api_fs.h`，宏会把 api_fs.h 的声明也改写成我们的 `pff_disk_*`，但签名对不上 → "conflicting types"。

修法：**宏只能放被它影响的 .c 文件独享的 .h 文件里**，不能放在公共头里。`pff_compat.h` 就是这种"只给 pff.c 用"的隔离头。

> "**Include what you use, but don't leak your macros.**" —— 一个好用的原则是：宏只放在"只有 1 个 .c include"的小头里。

### 4.4 关键教训：SDK 已经实现了完整的 Petit FatFs

第一版改完 typedef 之后，链接报：

```
pff.c:(.text.pf_lseek+0x0): multiple definition of `pf_lseek'
Output\obj\projects\standard\fat_test\pff.o:pff.c:(.text.pf_lseek+0x0): first defined here
```

`app/platform/libs/libdrivers.a(pff.o)` 里**已经定义了** `pf_mount / pf_open / pf_read / pf_write / pf_lseek`（看 [Output/bin/map.txt:3715-3726](../app/projects/standard/Output/bin/map.txt)）——SDK 自己的闭源 Petit FatFs 实现。我们又定义一遍 → 链接冲突。

**修法**：

- 我们的 `pf_*` 全部改名为 `pff_*`（`pff_mount` / `pff_open` / `pff_read` / `pff_write` / `pff_lseek` / `pff_opendir` / `pff_readdir`）
- `pff.h` 里的 prototype 同步改名
- `fat_test.c` 里的调用点同步改名

SDK 的闭源 pf_* 仍然在 libdrivers.a 里，但没人 reference 它们（fat_test.c 改叫 pff_* 了），所以不被 pull in。

### 4.5 关键教训：BSS 13KB 装不下 1KB cache

链接报：

```
region `data' overflowed by 752 bytes
```

`ram.ld` 里 `__data_ram_size = 13k`。我们的改动是 1024 字节的 `s_rd_cache` + 1024 字节的 `s_wr_cache` 直接进 BSS，超出 752 字节。

**修法 v1**：把这两个 512 字节 buffer 用 `__attribute__((section(".diskio_buf")))` 放进 SDK 在 `app/projects/standard/ram.ld:502` 预留的 `.aram_music` 段里的 `*(.diskio_buf)` 槽（0x3800~0x4000 共 2KB）。这样 cache 还是 512 字节，但**不进 BSS**，释放 1KB 给 SDK。

```c
static u8 s_rd_cache[512] __attribute__((aligned(4), section(".diskio_buf")));
static u8 s_wr_cache[512] __attribute__((aligned(4), section(".diskio_buf")));
```

**链接失败 v2**：

```
ram.o:412 cannot move location counter backwards (from 0x54194 to 0x54000)
```

SDK 在同一个 `.aram_music` 槽里还有 `*(.pff.buf)`（占 404 B / 0x194）和 `*(.fname.buf)`。2 × 512 B + 404 B = 1428 B 已经超过 0x3800~0x4000 的 2 KB 槽边界，链接器推到 0x4194 之后 `. = 0x4000` 试图 "reset" 位置计数器反向 → 报错。

**修法 v2（最终）**：合并为 **单个 512 字节 buffer `s_cache`**，用 read-modify-write 协议：

- 读阶段：`read_full_sector(lba)` 整 sector 读入 `s_cache`，命中短路
- 写 INIT：选 sector → 把目标扇区读到 `s_cache`（覆盖原内容）
- 写 DATA：把新字节拷到 `s_cache + offset`
- 写 FINALIZE：`sd0_write(s_cache, lba)` 整 sector 写回

```c
static u8 s_cache[512] __attribute__((aligned(4), section(".diskio_buf")));
```

512 B cache + 404 B SDK `.pff.buf` = 916 B << 0x800（2048 B）槽大小 ✓。

> 这种"占 SDK 预留内存区"的策略在嵌入式项目里很常见——SDK 在 `ram.ld` 里给各种功能预分配了 buffer 槽（`.pff_buf`、`*.diskio_buf`、`*.fnav_buf` 等），我们用 `__attribute__((section(...)))` 直接复用，避免占用紧缺的 BSS。**但要先 map.txt 看清 SDK 槽里已经塞了什么**，算好总大小别越过 0x4000 之类的硬上限。

---

## 5. 跑通 pff_mount 应该看到什么

实测输出（2026-07-29 在 branch_01 验证）：

```c
[FAT] ====== Task 4.1+4.2+4.3: pff_mount ======
[FAT] PASS: mounted, fs_type=FAT32         ← 关键证据
[FAT]   csize=64 sectors/cluster           ← 簇大小 = 64 × 512 = 32 KB
[FAT]   n_fatent=476990 (clusters+2)        ← 476988 个可用簇
```

`fs_type` 是 FAT32 / FAT16 / FAT12 都行（取决于 SD 卡格式）。**我们的 8 GB SD 卡是 FAT32**——看到 `fs_type=FAT32` 即视为整条移植链路打通。

**任何一个 FAIL 都要看 §6**。

**注意排查表里的常量值会因 SDK 库升级变化**——例如 api_fs.h 的 `FR_NOT_READY` 在我们这套 SDK 里是 3（不是 ChaN 原版的 2），但 `pff.c` 不会自己用这个值，只返回 FR_NOT_READY 给我们，然后我们用 SDK 的 `FR_NOT_READY` 宏比对。匹配成功。

---

## 6. 失败排查表

| 现象 | 最可能原因 | 检查 |
|---|---|---|
| `FR_NOT_READY` (2) | sd0_init 失败 | PE5/PE6/PE7 接线；3.3V 供电；卡是否插好 |
| `FR_NO_FILESYSTEM` (6) | 卡不是 FAT 格式 | 在 Windows 里格式化 → FAT32（默认选项），别选 exFAT |
| `FR_DISK_ERR` (1) | 卡接触不好 / 卡已坏 | 换一张卡试试 |
| 看不到任何 [FAT] 输出 | 卡识别都没成功 | 串口确认 `Hello BT892XA2` + `[FAT] ====== Task 4.1` 头打印了 |
| `Hello BT892XA2` 没打印 | 编译或烧录问题 | 跟 Phase 2 一样的排查：CB build log + Downloader 通信 |
| 卡是 16MB 以下 | 太小，可能没有 FAT16/32 | 换 32MB+ 的卡 |

---

## 7. 任务进度

| 任务 | 完成情况 |
|---|---|
| Task 4.1 建立 fat_test 目录 | ✅ |
| Task 4.2 学习 Petit FatFs | ✅ |
| Task 4.3 完成移植 | ✅ **实测 pff_mount PASS, fs_type=FAT32** |

下一步：[Task 4.4 文件读写](fat_test_04.md)（需要先在 SD 卡根目录放一个 `README.TXT` 或 `TEST.TXT`）。

---

## 一句话总结

> 把 ChaN 的 Petit FatFs R0.03a 装进 BT892XA2 SDK：重命名 `disk_*` 避开闭源符号冲突、维护 1 个 512 字节 cache 处理 partial-sector（read-modify-write）、typedef 逐项守卫避免与 SDK 重复——`pff_mount()` 返回 FAT32 即视为整条链路打通。
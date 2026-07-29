# 🤖 AI Agent Handoff 文档

> **目的**：这是给"未来的 Claude Code / Codex / 其他 AI 助手"的接棒说明。
> 当你看到这份文档时，说明之前的 AI 已经完成了一段工作，你需要 **接着干**。
>
> **读者**：AI coding agent（不是人类用户）。人类用户看 [docs/任务清单.md](任务清单.md) 即可。

---

## 0. 🚨 必读：项目核心信息

```
仓库:    sdk_bt892xa2_v02x_s16592_test
平台:    BT892XA2 (Bluetrum AB565X 家族, RISC-V 32-bit 自定义 ISA)
SDK:     V020.0 (libplatform.a / libbtstack.a / libdrivers.a / libvoices.a / libcodecs.a 闭源)
构建:    Code::Blocks + riscv32-elf-gcc + riscv32-elf-xmaker
烧录:    Windows GUI 工具 "Downloader", 烧 Output/bin/app.dcf 到芯片
当前分支: branch_02 (默认工作分支)
主分支:   main (已同步 branch_02 的所有内容)
工作目录: D:\Code\work\sdk_bt892xa2_v02x_s16592_20251212_branch_02\
```

**重要约束**：
- 用户**不会**频繁测试，**每次只测一个子任务** → 你必须**分阶段实施 + 等用户确认**
- **不要**自作主张 force push 改 main 分支
- **不要**未经用户确认就跑多个新任务
- 用户可能不在场 → 你写完代码 → 等待 → 用户测试 → 给反馈 → 写文档 → push

---

## 1. 当前进度

**已完成**：
- ✅ Phase 1: docs/快速入门.md 入门文档
- ✅ Phase 2: hello_world 串口打印验证（工具链 OK）
- ✅ Phase 3: TF 驱动 7 个子任务全部 PASS
- ✅ LA 抓取验证（Kingst VIS LA1010）

**进行中**：等待用户进入 Phase 4

**下一步**：见 [docs/任务清单.md](任务清单.md) "Phase 4 - 文件系统" 部分

---

## 2. 关键文档地图

```
docs/
├── 任务清单.md                  ← 用户看的进度表（用 ✓ ⏳ 标记任务状态）
├── AGENT_HANDOFF.md             ← 你正在读的这份（AI 接棒）
├── 快速入门.md                  ← 项目整体入门（构建/烧录/API 速查）
├── tf_test_01_02.md             ← Phase 3 Task 1+2: 建目录 + 鉴定
├── tf_test_03.md                ← Phase 3 Task 3: 单块读
├── tf_test_04.md                ← Phase 3 Task 4: 多块读
├── tf_test_05.md                ← Phase 3 Task 5: 单块写
├── tf_test_06.md                ← Phase 3 Task 6: 多块写
├── tf_test_07.md                ← Phase 3 Task 7: 混合读写
├── tf_test_summary.md            ← Phase 3 完整总结（必读）
└── tf_logic_analyzer_guide.md   ← 用 LA 抓 SD 卡数据（零基础）
```

**优先级**：
1. [docs/任务清单.md](任务清单.md) - 看当前该做啥
2. [docs/AGENT_HANDOFF.md](AGENT_HANDOFF.md) - 看怎么跟用户协作
3. [docs/快速入门.md](快速入门.md) - 看 SDK 怎么用
4. [docs/tf_test_summary.md](tf_test_summary.md) - Phase 3 学到了啥（Phase 4 必读 §9）

---

## 3. 关键文件地图（代码）

```
app/
├── projects/
│   └── standard/                ← 我们用的工程 (earphone/ 也存在但不用)
│       ├── main.c               ← 入口：bsp_sys_init → hello_world_init → tf_test_run
│       ├── config.h             ← SDK 配置（SD mapping 等已改）
│       ├── app.cbp              ← Code::Blocks 工程（每个 .c 必须显式 Unit 注册）
│       ├── ram.ld               ← 链接脚本（data region 只有 13KB！）
│       └── tf_test/
│           ├── tf_test.h         ← 单个函数声明 void tf_test_run(void);
│           └── tf_test.c        ← 7 个测试函数实现 + 渐进启用机制
├── platform/
│   ├── libs/
│   │   ├── api.h               ← API 总入口
│   │   ├── api_sd.h             ← sd0_init/read/write 等（Phase 3 用）
│   │   └── lib*.a               ← 闭源静态库（别尝试读源码）
│   ├── header/
│   │   ├── include.h           ← 几乎所有 .c 都 include 它
│   │   ├── io_def.h             ← GPIO mux 宏（含 SD_MUX_IO_INIT 等）
│   │   ├── config_define.h      ← SD0MAP_G3 = (3 << 0) 在这里
│   │   └── config_extra.h        ← 自动联动宏（关 FUNC_MUSIC_EN → 关 MUSIC_SDCARD_EN）
│   ├── bsp/                     ← Board Support Package
│   ├── functions/               ← 应用模式入口
│   └── modules/                 ← 可选功能模块
```

**特别关注**：
- `app/projects/standard/ram.ld` 第 21-23 行：`__data_ram_size = 13k`（BSS 容量上限）
- `app/projects/standard/config.h` 第 55, 441, 473, 529, 534, 547, 548 行：TF 测试相关的 7 处 `[TF_TEST]` 改动
- `app/projects/standard/tf_test/tf_test.c`：7 个测试函数模板，全部按需 `#if 1`/`#if 0` 启用

---

## 4. 与用户协作的标准流程

**每个新任务的标准循环**：

```
1. 用户说："开始 Phase X.Y" 或类似
2. AI: 写代码 + 测试代码（保持旧的测试不动，只 uncomment 新任务）
3. AI: 改代码（如改 config.h、tf_test.c）
4. 用户: 编译烧录测试
5. 用户: 给反馈（PASS/FAIL + 串口输出）
6. 如果 PASS: AI 写 docs/tf_test_XX.md（含原理 + 测试步骤 + 实测结果）
7. AI: git commit + 询问用户 push 权限
8. 用户: push 确认 → AI push
9. 回到步骤 1，开始下一个任务
```

**关键规则**：
- ❌ 不要一次启用多个任务（保持单变量调试）
- ❌ 不要 push 未经用户确认
- ❌ 不要改不在当前任务范围内的代码
- ✅ 写代码时保留前一个任务的代码（已验证的基线）
- ✅ 失败时详细分析，给出排查表
- ✅ 完成后提供"下一步"建议

---

## 5. TF 驱动测试实施模式（参考模板）

Phase 3 成功的模式，后续 Phase 4/5 可以直接套用：

### 5.1 测试代码模板

```c
// tf_test.h
#ifndef _TF_TEST_H
#define _TF_TEST_H
void tf_test_run(void);
#endif

// tf_test.c
#include "include.h"
#include "tf_test.h"

// 单 buffer 复用（避免 BSS 溢出）
#define BUF_SIZE 512
static u8 s_buf[BUF_SIZE] __attribute__((aligned(4)));

static bool tf_test_task_n(void) {
    printf("\n[TF] ====== Task N: Description ======\n");
    // ... 测试逻辑 ...
    return true;  // 或 false on fail
}

void tf_test_run(void) {
    if (!tf_test_task_1()) goto halt;  // 已验证的基线
    if (!tf_test_task_2()) goto halt;
    // ... 一个一个加 ...
halt:
    while (1) delay_5ms(200);
}
```

### 5.2 文档模板

每个新任务的 `docs/tf_test_XX.md` 应该包含：
1. **前置**（指向已有文档）
2. **任务说明**（用户想测什么）
3. **协议/技术细节**（涉及什么 SD 命令/API）
4. **测试代码逐行讲解**
5. **实测结果**（用户提供的串口输出 + 解读）
6. **失败排查表**
7. **任务进度**（整体 Phase 进度）

---

## 6. 已知坑（不要重蹈覆辙）

### 6.1 BSS 容量限制
- `ram.ld` data region = **13KB**
- 2×512 字节静态 buffer = 1024 字节就溢出
- **解决**：用单 buffer + pattern 校验（不要保存原始数据）

### 6.2 串口中文乱码
- BT892XA2 `my_printf` 发 raw 字节
- Windows 串口助手按 GBK 解析 UTF-8 中文会乱码
- **解决**：所有 printf 字符串用 ASCII 英文

### 6.3 Code::Blocks 不自动 glob
- 新建的 `.c` 文件**不会**自动被编译
- **必须**编辑 `.cbp` 加 `<Unit filename="..."/>`
- 也要加 `<Add directory="..."/>` 才能 include 路径下的 `.h`

### 6.4 SD 引脚冲突
- 我们用 SD0MAP_G3（PE5/PE6/PE7）
- 这些引脚被很多功能占用，要关：IRRX_HW_EN、USER_ADKEY、SPDIF_CH=SPF_PE6_CH4
- 见 `config.h` 中带 `[TF_TEST]` 标记的 7 行

### 6.5 远程分支可能存在冲突
- 旧 bit-bang 实现可能在 `origin/branch_02` 上
- 用 `git push --force-with-lease` 覆盖前**询问用户**
- 远程也可能有 `origin/branch_01`，谨慎处理

### 6.6 逻辑分析仪调试
- 接线不够时**两段式**：先断卡抓 LA，再断 LA 接卡
- Kingst VIS SDIO Analyzer 有 bug：把 ACMD41 误识别为 CMD1 "Unknown"
- 抓不到波形先检查 GND 是否共地

---

## 7. 当前分支与 main 的关系

```
branch_02 (默认)   ─→  Phase 3 + LA guide 全在这
main              ─→  已合并 branch_02（用 git merge）
```

**所有改动流程**：
```bash
# 改代码 + 写文档
git add -A
git commit -m "..."

# 询问用户是否 push
# 用户确认后:
git push origin branch_02

# 询问用户是否 merge 到 main
# 用户确认后:
git checkout main
git merge branch_02
git push origin main
```

---

## 8. 当用户重新打开终端说"继续 Phase X.Y"时

**你的第一步**：

1. `git log --oneline -10` — 看最近 commit
2. `cat docs/任务清单.md` — 看哪个阶段/任务在进行中
3. `cat docs/AGENT_HANDOFF.md`（本文件）— 确认自己知道状态
4. 跟用户确认："现在做 Phase X.Y 吗？"
5. 用户确认后，按上面 §4 标准流程走

**不要**：
- 自作主张继续未完成的任务
- 不读文档就开始改代码
- 假设用户跟你上一个 agent 谈过话

---

## 9. 紧急情况：用户反馈 FAIL

如果用户说"FAIL"或贴了失败的串口输出：

1. **不要慌** —— 多数 FAIL 是接线/卡问题，不是代码问题
2. **分析串口输出** —— 找具体 FAIL 行
3. **跑排查表** —— 看 `docs/tf_test_05.md` §6 这种格式
4. **不要立刻改代码** —— 先问用户排查情况
5. **必要时回退 commit**（`git revert`）

---

## 10. 完成后的状态交接

如果一个 Phase 全部完成：

1. 更新 `docs/任务清单.md` —— 把对应阶段标记为 ✅
2. 更新本文件 §1 — 反映新进度
3. 写"总结"文档（参考 [docs/tf_test_summary.md](tf_test_summary.md)）
4. 询问用户是否 merge 到 main

---

## 11. 速查命令

```bash
# 看当前状态
git status
git log --oneline -10
cat docs/任务清单.md | head -30

# 测试编译流程
cd app/projects/standard
# 在 Code::Blocks 里 Ctrl+F9 (Build)
# 烧 Output/bin/app.dcf 到芯片

# 串口观察（115200 8N1，PB3 TX）
# Windows: 用 putty 或 XCOM
```

---

## 最后一句

**用户负责测试，AI 负责写代码 + 写文档 + 提交。**

每个 Phase / Task 完成 → 写一份 markdown 文档 → 用户确认 → push → merge → 继续下一个。

有疑问先问用户，不要猜。祝顺利！🤖
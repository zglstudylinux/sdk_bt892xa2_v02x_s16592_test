#ifndef _TF_TEST_H
#define _TF_TEST_H

/*
 * tf_test 模块 — Phase 3 TF 卡驱动测试
 *
 * 任务清单（docs/任务清单.md）：
 *   Task 1  建 tf_test 目录               （已建立）
 *   Task 2  TF 卡鉴定流程（CMD0/8/ACMD41/58）
 *   Task 3  单块读 (CMD17)
 *   Task 4  多块读 (CMD18)
 *   Task 5  单块写 (CMD24)
 *   Task 6  多块写 (CMD25)
 *   Task 7  多块读写混合
 *
 * 硬件配置：
 *   SD0_MAPPING = SD0MAP_G3
 *   SDCMD = PE5, SDCLK = PE6, SDDAT0 = PE7
 *   模式：SDIO（内置 SD 控制器，libplatform.a）
 *
 * 用法：
 *   - main.c 在 bsp_sys_init() 后调用 tf_test_run() 替代 func_run()
 *   - 每次只 uncomment 一个测试函数，配合代码块的注释完成渐进式验证
 *   - 每个测试通过后写一份 docs/tf_test_XX.md 报告，再开下一个
 *
 * 完成标志（Task 2 当前阶段）：
 *   串口看到 "[TF] PASS: sd0_init OK, rw_state=0"，TF 卡正确识别
 */

void tf_test_run(void);

#endif // _TF_TEST_H
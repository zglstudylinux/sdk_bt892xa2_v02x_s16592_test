/*
 * tf_test.c — Phase 3 TF 卡驱动测试实现
 *
 * 当前阶段只启用 Task 2（鉴定流程）。后续任务按需在 tf_test_run() 里
 * 逐步 uncomment 对应的测试调用，每次只跑一个新增任务。
 */

#include "include.h"
#include "tf_test.h"
#include "api_sd.h"     // sd0_init / sd0_read / sd0_write / sd0_get_rw_sta
#include "port_sd.h"    // sd_gpio_init (板级 GPIO mux 配置 + FUNCMCON0 = SD0_MAPPING)

#define TF_SECTOR_SIZE      512u
#define TF_TEST_LBA         1000u   // 安全 LBA，远离 MBR/FAT 区域
#define TF_MULTI_COUNT      8u      // 多块测试块数

// 4 字节对齐：SDIO DMA 基线要求；512 本身是 4 倍数，自动对齐 512 字节边界
static u8 s_read_buf [TF_SECTOR_SIZE] __attribute__((aligned(4)));
static u8 s_write_buf[TF_SECTOR_SIZE] __attribute__((aligned(4)));

/**
 * @brief Task 2: TF 卡鉴定流程
 *
 * 调用 SDK 的 sd0_init()，内部完成：
 *   - CMD0  (GO_IDLE_STATE)
 *   - CMD8  (SEND_IF_COND, 检查 2.7~3.6V 范围)
 *   - ACMD41 (SD_SEND_OP_COND, 反复轮询直到 OCR[31]=1 完成)
 *   - CMD58 (READ_OCR, 确认 CCS 位 = SDHC/SDXC)
 *   - CMD2 / CMD3 (读 CID / 分配 RCA)
 *   - CMD9  (读 CSD)
 *   - CMD7  (SELECT_CARD, 进入 tran 状态)
 *   - ACMD51 (读 SCR)
 *
 * @return true  鉴定成功，TF 卡进入 tran 状态
 * @return false 鉴定失败，详情查串口输出
 */
static bool tf_test_identify(void)
{
    printf("\n[TF] ====== Task 2: Card Identification ======\n");
    printf("[TF] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7\n");
    printf("[TF] Mode: SDIO (built-in controller)\n");
    printf("[TF] Mapping: SD0MAP_G3\n");

    // GPIO mux + 内置 SD 控制器激活
    // type=0 => SD_MUX_IO_INIT() 把 PE5/6/7 切到 SD 功能 + FUNCMCON0=SD0MAP_G3
    sd_gpio_init(0);
    delay_5ms(10);  // 让电源和 pull-up 稳定

    // 触发鉴定协议
    if (!sd0_init()) {
        printf("[TF] FAIL: sd0_init returned false.\n");
        printf("[TF]   检查项：\n");
        printf("[TF]     1. TF 卡完全插入卡槽\n");
        printf("[TF]     2. PE5(SDCMD) / PE6(SDCLK) / PE7(SDDAT0) 接线\n");
        printf("[TF]     3. TF 卡供电 3.3V 正常\n");
        printf("[TF]     4. SD0_MAPPING = SD0MAP_G3 已生效\n");
        return false;
    }

    // rw_state: 0=IDLE, 1=READ, 2=WRITE（api_sd.h:4-8）
    u8 sta = sd0_get_rw_sta();
    printf("[TF] PASS: sd0_init OK, rw_state=%d (0=IDLE)\n", sta);
    return true;
}

// ============================================================================
// 后续任务的测试函数暂留位置（按需 uncomment）
// ============================================================================

#if 0   // Task 3: 单块读
static bool tf_test_read_single(void)
{
    printf("\n[TF] ====== Task 3: Single Block Read ======\n");
    printf("[TF] LBA=%u  (远离 MBR/FAT 的安全区域)\n", TF_TEST_LBA);
    if (!sd0_read(s_read_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_read\n");
        return false;
    }
    printf("[TF] PASS. First 16 bytes:");
    for (int i = 0; i < 16; i++) printf(" %02X", s_read_buf[i]);
    printf("\n");
    return true;
}
#endif

#if 0   // Task 4: 多块读（API 只有单块，循环调用）
static bool tf_test_read_multi(void)
{
    printf("\n[TF] ====== Task 4: Multi Block Read (%u blocks) ======\n", TF_MULTI_COUNT);
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + TF_MULTI_COUNT; lba++) {
        if (!sd0_read(s_read_buf, lba)) {
            printf("[TF] FAIL at LBA %u\n", lba);
            return false;
        }
        printf("[TF] LBA %u PASS\n", lba);
    }
    return true;
}
#endif

#if 0   // Task 5: 单块写（写后再读回校验）
static bool tf_test_write_single(void)
{
    printf("\n[TF] ====== Task 5: Single Block Write ======\n");
    // 填充已知 pattern：第一扇区标记 + LBA + 0xAA..0x55.. 交替
    for (int i = 0; i < TF_SECTOR_SIZE; i++) {
        s_write_buf[i] = (u8)((i & 1) ? 0x55 : 0xAA);
    }
    // 在前 4 字节写入 LBA 标记便于辨识
    s_write_buf[0] = (u8)(TF_TEST_LBA & 0xFF);
    s_write_buf[1] = (u8)((TF_TEST_LBA >> 8) & 0xFF);
    s_write_buf[2] = 0xBE;
    s_write_buf[3] = 0xEF;

    if (!sd0_write(s_write_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_write\n");
        return false;
    }
    // 读回校验
    if (!sd0_read(s_read_buf, TF_TEST_LBA)) {
        printf("[TF] FAIL: sd0_read after write\n");
        return false;
    }
    // 比对
    for (int i = 0; i < TF_SECTOR_SIZE; i++) {
        if (s_read_buf[i] != s_write_buf[i]) {
            printf("[TF] FAIL: data mismatch at byte %d (wrote %02X, read %02X)\n",
                   i, s_write_buf[i], s_read_buf[i]);
            return false;
        }
    }
    printf("[TF] PASS: write + readback verified, all 512 bytes match.\n");
    return true;
}
#endif

#if 0   // Task 6: 多块写（循环写 + 循环读回校验）
static bool tf_test_write_multi(void)
{
    printf("\n[TF] ====== Task 6: Multi Block Write (%u blocks) ======\n", TF_MULTI_COUNT);
    // 阶段 1: 连续写
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + TF_MULTI_COUNT; lba++) {
        // 用 LBA 作为 pattern 第一字节，后面是 LBA+0xAA 的循环
        for (int i = 0; i < TF_SECTOR_SIZE; i++) {
            s_write_buf[i] = (u8)((lba + i) & 0xFF);
        }
        s_write_buf[0] = (u8)(lba & 0xFF);
        if (!sd0_write(s_write_buf, lba)) {
            printf("[TF] FAIL write at LBA %u\n", lba);
            return false;
        }
    }
    printf("[TF] Phase 1 (write) OK. Starting phase 2 (readback)...\n");
    // 阶段 2: 连续读回校验
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + TF_MULTI_COUNT; lba++) {
        if (!sd0_read(s_read_buf, lba)) {
            printf("[TF] FAIL readback at LBA %u\n", lba);
            return false;
        }
        for (int i = 0; i < TF_SECTOR_SIZE; i++) {
            u8 expected = (u8)((lba + i) & 0xFF);
            if (s_read_buf[i] != expected) {
                printf("[TF] FAIL: LBA %u byte %d mismatch (wrote %02X, read %02X)\n",
                       lba, i, expected, s_read_buf[i]);
                return false;
            }
        }
        printf("[TF] LBA %u readback PASS\n", lba);
    }
    return true;
}
#endif

#if 0   // Task 7: 多块读写混合（边读边写验证 DMA 缓冲不冲突）
static bool tf_test_mixed(void)
{
    printf("\n[TF] ====== Task 7: Multi Block Mixed R/W ======\n");
    // 用两块独立 buffer，避免读写别名
    // 1. 写 4 块 pattern A
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        for (int i = 0; i < TF_SECTOR_SIZE; i++) {
            s_write_buf[i] = (u8)('A' + (lba - TF_TEST_LBA));
        }
        if (!sd0_write(s_write_buf, lba)) {
            printf("[TF] FAIL write-A at LBA %u\n", lba);
            return false;
        }
    }
    printf("[TF] Phase 1: 4 blocks of pattern A written\n");
    // 2. 立即读 4 块到 s_read_buf
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        if (!sd0_read(s_read_buf, lba)) {
            printf("[TF] FAIL read-A at LBA %u\n", lba);
            return false;
        }
        u8 expected = (u8)('A' + (lba - TF_TEST_LBA));
        if (s_read_buf[0] != expected) {
            printf("[TF] FAIL: LBA %u expected pattern %c, got %02X\n",
                   lba, expected, s_read_buf[0]);
            return false;
        }
    }
    printf("[TF] Phase 2: readback A OK\n");
    // 3. 改写前 2 块为 pattern B，再读回
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 2; lba++) {
        for (int i = 0; i < TF_SECTOR_SIZE; i++) {
            s_write_buf[i] = (u8)('B' + (lba - TF_TEST_LBA));
        }
        if (!sd0_write(s_write_buf, lba)) {
            printf("[TF] FAIL write-B at LBA %u\n", lba);
            return false;
        }
    }
    for (u32 lba = TF_TEST_LBA; lba < TF_TEST_LBA + 4; lba++) {
        if (!sd0_read(s_read_buf, lba)) {
            printf("[TF] FAIL read-B at LBA %u\n", lba);
            return false;
        }
        u8 expected;
        if (lba < TF_TEST_LBA + 2) {
            expected = (u8)('B' + (lba - TF_TEST_LBA));
        } else {
            expected = (u8)('A' + (lba - TF_TEST_LBA));
        }
        if (s_read_buf[0] != expected) {
            printf("[TF] FAIL: LBA %u expected %c, got %02X\n",
                   lba, expected, s_read_buf[0]);
            return false;
        }
    }
    printf("[TF] PASS: mixed R/W OK. First 2 blocks B, next 2 blocks A.\n");
    return true;
}
#endif

// ============================================================================
// 入口：tf_test_run
// ============================================================================

void tf_test_run(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [TF] BT892XA2 TF Card Driver Test\n");
    printf("  [TF] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [TF] SDIO mode, SD0MAP_G3 (PE5/6/7)\n");
    printf("=========================================\n");

    if (!tf_test_identify())      goto halt;

    // 后续任务按顺序 uncomment，一次只加一个
    // if (!tf_test_read_single())  goto halt;   // Task 3
    // if (!tf_test_read_multi())   goto halt;   // Task 4
    // if (!tf_test_write_single()) goto halt;   // Task 5
    // if (!tf_test_write_multi())  goto halt;   // Task 6
    // if (!tf_test_mixed())        goto halt;   // Task 7

    printf("\n[TF] ====== All active tests PASSED ======\n");

halt:
    // WDT 由 SDK tick 自动喂（bsp_sys_init 里 sys_set_tmr_enable(1,1)）
    // delay_5ms(200) 进入 200ms 周期短延时，不会触发 WDT 复位
    printf("[TF] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
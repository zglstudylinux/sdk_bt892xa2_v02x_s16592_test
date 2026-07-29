/*
 * wav_test.c — Phase 5 WAV 音乐播放器测试实现
 *
 * 当前阶段启用：
 *   wav_test_01  工程接入（banner + 5ms tick）
 *   wav_test_02  pff_open("MUSIC.WAV") 打开固定名 wav
 *
 * 后续 wav_test_03 ~ 07 按需在 wav_test_run() 里逐步 uncomment，
 * 每次只跑一个新增测试，单变量调试。
 *
 * 代码风格参考：
 *   - tf_test/tf_test.c   (Phase 3: SDIO 裸块测试)
 *   - fat_test/fat_test.c (Phase 4: Petit FatFs 移植测试)
 */

#include "include.h"
#include "wav_test.h"
#include "pff.h"            /* pff_mount / pff_open / pff_read / pff_lseek (Phase 4) */
#include "api_dac.h"        /* dac_spr_set / dac_aubuf_clr / obuf_put_samples (api_dac.h) */
#include "api_sdadc.h"      /* SPR_48000 / SPR_44100 / ... enum (Hz -> SPR 转换) */
#include "bsp_sys.h"        /* bsp_sys_unmute() — 唤醒 mute 后的 DAC 输出通路 */

/* ---------------------------------------------------------------------------
 * Petit FatFs 状态对象（与 fat_test 共享同一份移植代码）
 * ---------------------------------------------------------------------------
 *
 * pff_mount() 内部把它填满（fs_type, csize, n_fatent 等），后续 pff_open
 * 打开文件后会更新 fsize / fptr / org_clust 等字段。我们只需要一个实例。
 *
 * 静态分配 40 字节左右，远低于 ram.ld 限定的 13KB BSS 总余量。
 */
static FATFS s_fs;

/* ---------------------------------------------------------------------------
 * FRESULT -> 字符串映射（移植自 fat_test/fat_test.c）
 * ---------------------------------------------------------------------------
 *
 * 错误码本身只是 0~9 的小整数，直接 printf("%d") 用户看不懂；
 * 映射成 "FR_NO_FILE" 之类的串口输出后排查更快。
 */
static const char *fr_str(FRESULT fr)
{
    switch (fr) {
    case FR_OK            : return "OK";
    case FR_DISK_ERR      : return "DISK_ERR";
    case FR_NOT_READY     : return "NOT_READY";
    case FR_NO_FILE       : return "NO_FILE";
    case FR_NOT_OPENED    : return "NOT_OPENED";
    case FR_NOT_ENABLED   : return "NOT_ENABLED";
    case FR_NO_FILESYSTEM : return "NO_FILESYSTEM";
    default               : return "UNKNOWN";
    }
}

/* ---------------------------------------------------------------------------
 * 5ms tick 钩子
 * ---------------------------------------------------------------------------
 *
 * 由 plugin.c 的 plugin_tmr5ms_isr() 每 5ms 调用一次。
 * wav_test_01 ~ 02 阶段：空实现（只在 wav_test_run() 主循环做实际工作）。
 * wav_test_05 起：改为流式喂 PCM 到 DAC obuf。
 *
 * AT(.com_text.wav) 把函数放到 .com_text section（ram.ld:97-111），保证
 * 它能被 5ms ISR 在 RAM 中调到（XIP 闪存执行延迟会卡时序）。
 */
AT(.com_text.wav)
void wav_test_tick(void)
{
#if 0   // wav_test_05 起启用：流式喂 DAC obuf
    extern void wav_player_tick(void);
    wav_player_tick();
#endif
}

/* ---------------------------------------------------------------------------
 * 测试函数（按需 #if 1 启用）
 * --------------------------------------------------------------------------- */

#if 1   // wav_test_01: 工程接入（ACTIVE, 仅 banner + halt）
static bool wav_test_banner(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("  [WAV] BT892XA2 WAV Player Test\n");
    printf("  [WAV] SDK V%02x.%02x\n",
           (SDK_VERSION >> 8) & 0xff, SDK_VERSION & 0xff);
    printf("  [WAV] Phase 5: WAV music player\n");
    printf("  [WAV] wav_test_01: engineering bring-up\n");
    printf("=========================================\n");
    return true;
}
#endif

#if 1   // wav_test_02: pff_open("MUSIC.WAV") 打开固定名 wav
/**
 * @brief Task 2: 从 TF 卡中打开一个 wav 文件
 *
 * 复用 Phase 4 已移植的 Petit FatFs 全套 API：
 *   - pff_mount(&s_fs)            — 走通 disk_initialize + BPB 解析
 *   - pff_open("MUSIC.WAV")       — 按 8.3 SFN 在根目录查找
 *   - pff_lseek(0) + pff_read()   — peek 前 12 字节确认 RIFF/WAVE 头
 *
 * @return true  打开 + 头 magic 校验通过
 * @return false 任一失败（详见串口 FAIL 行）
 */
static bool wav_test_open_fixed(void)
{
    printf("\n[WAV] ====== wav_test_02: open fixed MUSIC.WAV ======\n");
    printf("[WAV] Pins: SDCMD=PE5  SDCLK=PE6  SDDAT0=PE7  (SDIO, SD0MAP_G3)\n");

    /* Step 1: mount the volume (re-uses diskio.c from Phase 4). */
    FRESULT fr = pff_mount(&s_fs);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_mount -> %d (%s)\n", fr, fr_str(fr));
        if (fr == FR_NOT_READY) {
            printf("[WAV]   Hint: sd0_init() failed (Phase 3 hint).\n");
            printf("[WAV]          Check TF card presence and PE5/PE6/PE7 wiring.\n");
        } else if (fr == FR_NO_FILESYSTEM) {
            printf("[WAV]   Hint: no FAT signature at LBA 0.\n");
            printf("[WAV]          Format the SD card as FAT32 in Windows.\n");
        }
        return false;
    }
    printf("[WAV] PASS: pff_mount OK, fs_type=%d\n", s_fs.fs_type);
    printf("[WAV]   csize=%u sectors/cluster\n", s_fs.csize);
    printf("[WAV]   n_fatent=%lu (clusters+2)\n", (unsigned long)s_fs.n_fatent);

    /* Step 2: open MUSIC.WAV. The file must exist at SD root; Petit FatFs
     *        does NOT support creating files (see fat_test docs). */
    fr = pff_open("MUSIC.WAV");
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_open(\"MUSIC.WAV\") -> %d (%s)\n", fr, fr_str(fr));
        if (fr == FR_NO_FILE) {
            printf("[WAV]   Hint: MUSIC.WAV not found at SD root.\n");
            printf("[WAV]          Copy a real .wav file to\n");
            printf("[WAV]          <SD card root>\\MUSIC.WAV and re-flash.\n");
        }
        return false;
    }
    printf("[WAV] PASS: pff_open OK, fsize=%lu bytes\n",
           (unsigned long)s_fs.fsize);

    /* Step 3: peek first 12 bytes (RIFF<size>WAVE). */
    UINT br = 0;
    u8 hdr[12];
    fr = pff_lseek(0);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(0) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    fr = pff_read(hdr, sizeof(hdr), &br);
    if (fr != FR_OK || br != sizeof(hdr)) {
        printf("[WAV] FAIL: pff_read(hdr, 12) -> %d (%s), br=%u\n",
               fr, fr_str(fr), br);
        return false;
    }
    printf("[WAV] PASS: first 12 bytes:\n  ");
    for (u32 i = 0; i < sizeof(hdr); i++) printf("%02X ", hdr[i]);
    printf("\n");

    /* Step 4: validate RIFF / WAVE magic (only first 4 + 8..11 matter
     *        for the "is it a wav file?" check; the size field at 4..7
     *        is the file size - 8 in little-endian). */
    if (memcmp(hdr, "RIFF", 4) != 0) {
        printf("[WAV] FAIL: first 4 bytes != \"RIFF\" (got %02X %02X %02X %02X)\n",
               hdr[0], hdr[1], hdr[2], hdr[3]);
        return false;
    }
    if (memcmp(hdr + 8, "WAVE", 4) != 0) {
        printf("[WAV] FAIL: bytes 8..11 != \"WAVE\" (got %02X %02X %02X %02X)\n",
               hdr[8], hdr[9], hdr[10], hdr[11]);
        return false;
    }
    printf("[WAV] PASS: magic \"RIFF...WAVE\" detected\n");
    return true;
}
#endif

#if 1   // wav_test_03: parse 44-byte RIFF header
/* ---------------------------------------------------------------------------
 * RIFF/WAV 头结构
 * ---------------------------------------------------------------------------
 *
 * 标准 PCM WAV 的 44 字节布局（little-endian）：
 *   0..3   "RIFF"
 *   4..7   file_size - 8 (uint32)
 *   8..11  "WAVE"
 *  12..15  "fmt "
 *  16..19  fmt chunk size (通常 = 16, 可扩展到 18/40)
 *  20..21  format tag (1 = PCM, 0xFFFE = WAVE_FORMAT_EXTENSIBLE)
 *  22..23  channels (1 = mono, 2 = stereo)
 *  24..27  sample rate (Hz)
 *  28..31  byte rate (sample_rate * channels * bits/8)
 *  32..33  block align (channels * bits/8)
 *  34..35  bits per sample
 *  36..39  "data"
 *  40..43  data chunk size (bytes)
 *  44..    raw PCM data
 *
 * Phase 5 只支持 PCM (format=1) + 16-bit。
 */
typedef struct {
    u16 format;       // 1 = PCM
    u16 channels;     // 1 or 2
    u32 sample_rate;  // Hz
    u16 bits;         // 16
    u32 data_size;    // raw PCM bytes
    u32 data_offset;  // offset in file where data starts
} wav_hdr_parsed_t;

static wav_hdr_parsed_t s_hdr;

/* 小端 16-bit 读取 */
static inline u16 le16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
/* 小端 32-bit 读取 */
static inline u32 le32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* ---------------------------------------------------------------------------
 * Hz → SPR 枚举转换（dac_spr_set 接的是 SPR enum，不是 raw Hz）
 * ---------------------------------------------------------------------------
 *
 * api_sdadc.h 的 SPR enum:
 *   SPR_48000 = 0, SPR_44100 = 1, SPR_38000 = 2, ..., SPR_8000 = 9
 *
 * 任何 wav 采样率都要先查表转成 SPR 索引。不在表内的 → 用 SPR_44100 兜底。
 */
static u32 hz_to_spr(u32 hz)
{
    switch (hz) {
    case 48000: return SPR_48000;
    case 44100: return SPR_44100;
    case 38000: return SPR_38000;
    case 32000: return SPR_32000;
    case 24000: return SPR_24000;
    case 22050: return SPR_22050;
    case 16000: return SPR_16000;
    case 12000: return SPR_12000;
    case 11025: return SPR_11025;
    case 8000:  return SPR_8000;
    case 96000: return SPR_96000;
    case 88200: return SPR_88200;
    default:    return SPR_44100;  // fallback
    }
}

/* ---------------------------------------------------------------------------
 * obuf 喂数 PCM buffer（4 字节对齐，DMA 基线要求）
 * ---------------------------------------------------------------------------
 *
 * 256B 是 Petit FatFs 单次 pff_read 的实际上限（看 pff.c 内部 sect size）。
 * 改大也没意义，会被拆成多次。256B 够用。
 */
static u8 s_pcm[256] __attribute__((aligned(4)));

/**
 * @brief 从 raw 字节解析 RIFF/WAV 头
 *
 * @param raw       指向 wav 文件前 raw_size 字节
 * @param raw_size  raw 缓冲区大小（建议 ≥ 256 字节以覆盖 LIST/JUNK 等附加 chunk）
 * @return          true = 解析成功且 format=1, bits=16
 *
 * 算法：
 *   1. 验证 "RIFF" + "WAVE" + "fmt "
 *   2. 读取 fmt chunk fields (channels/sample_rate/bits 等)
 *   3. **扫描** 找 "data" 标签位置（兼容带 LIST/INFO/JUNK 附加 chunk 的 wav）
 *   4. 校验 format=1 (PCM) 和 bits=16
 *
 * 注：fmt_size 可变（16/18/40），不在本测试限制范围内。
 */
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

    if (s_hdr.format != 1) {
        printf("[WAV]   FAIL: format tag = %u (need 1 = PCM)\n", s_hdr.format);
        return false;
    }
    if (s_hdr.bits != 16) {
        printf("[WAV]   FAIL: bits = %u (Phase 5 only supports 16-bit)\n",
               s_hdr.bits);
        return false;
    }

    /* Scan for "data" tag (some wav tools insert LIST/INFO/JUNK between fmt
     * and data). Most PC-generated wavs have data at offset 44 (no extra
     * chunks). We accept either. */
    u32 off = 12 + 8 + fmt_size;   // start of next chunk after fmt
    bool found = false;
    while (off + 8 <= raw_size) {
        if (memcmp(raw + off, "data", 4) == 0) {
            found = true;
            break;
        }
        if (off + 8 > raw_size) break;
        u32 chunk_size = le32(raw + off + 4);
        off += 8 + chunk_size;
        if (chunk_size & 1) off++;   // RIFF chunks are 2-byte aligned
    }
    if (!found || off + 8 > raw_size) {
        printf("[WAV]   FAIL: \"data\" chunk not found in first %u bytes\n",
               raw_size);
        return false;
    }
    s_hdr.data_offset = off + 8;
    s_hdr.data_size   = le32(raw + off + 4);
    return true;
}

/**
 * @brief wav_test_03: 解析 44 字节 RIFF 头
 *
 * 复用 wav_test_02 已打开的 s_fs（Petit FatFs 单文件系统对象）。
 * 读首 256 字节足够覆盖标准 44 头 + 常见 LIST/JUNK 附加 chunk。
 *
 * @return true  解析成功且格式合规 (PCM 16-bit)
 */
static bool wav_test_parse_header(void)
{
    printf("\n[WAV] ====== wav_test_03: parse 44-byte RIFF header ======\n");

    /* Step 1: rewind (wav_test_02 left fptr at 12). */
    FRESULT fr = pff_lseek(0);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek(0) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }

    /* Step 2: read first 256 bytes (one FAT cluster worth). */
    u8 raw[256] __attribute__((aligned(4)));
    UINT br = 0;
    fr = pff_read(raw, sizeof(raw), &br);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_read(256) -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }
    printf("[WAV] PASS: read %u bytes from file head\n", br);

    /* Step 3: parse. */
    if (!wav_parse_header(raw, br)) {
        printf("[WAV] FAIL: header parse failed.\n");
        return false;
    }

    /* Step 4: report. */
    printf("[WAV] PASS: header parsed:\n");
    printf("[WAV]   format      = %u (1 = PCM)\n",       s_hdr.format);
    printf("[WAV]   channels    = %u (1 = mono, 2 = stereo)\n",
           s_hdr.channels);
    printf("[WAV]   sample_rate = %u Hz\n",             s_hdr.sample_rate);
    printf("[WAV]   bits        = %u (must be 16)\n",   s_hdr.bits);
    printf("[WAV]   data_offset = %u (header size)\n",  s_hdr.data_offset);
    printf("[WAV]   data_size   = %u bytes\n",          s_hdr.data_size);
    u32 play_seconds = (s_hdr.data_size) /
                       (s_hdr.channels * (s_hdr.bits / 8) * s_hdr.sample_rate);
    printf("[WAV]   duration    = ~%u seconds\n",       play_seconds);

    /* Sanity check: data_offset + data_size <= fsize */
    if ((u32)s_hdr.data_offset + s_hdr.data_size > (u32)s_fs.fsize + 44) {
        /* data_size 字段可能 > 实际剩余字节（小尾数某些工具不写满），
         * 仅打印警告，不 fail。 */
        printf("[WAV]   WARN: data_size > fsize (will stop at EOF).\n");
    }
    return true;
}
#endif

#if 1   // wav_test_04: obuf one-shot play (first sound)
/**
 * @brief wav_test_04: 首次出声 — 喂 ~5 秒音频到 obuf 后退出
 *
 * 测试原理：
 *   1. dac_spr_set(SPR)         — 把 DAC 采样率配到 wav 的 sample_rate
 *   2. dac_aubuf_clr()           — 清 obuf（防止旧数据残留）
 *   3. pff_lseek(s_hdr.data_offset) — 跳过头
 *   4. 喂数 ~1MB（约 5 秒音频），进度打印
 *
 * **为什么限制 1MB 而不是整首 52MB**：
 *   pff_read(256) 一次读 1 个 FAT sector，平均 5~10ms。52MB / 256 = 200K+
 *   次 read = **数十分钟**。而且 DAC obuf 只有 ~2KB，新数据到达前早就被 DMA
 *   抽空，根本播不出声音。所以 wav_test_04 只验证"5 秒能出声"就够，剩下的
 *   完整播放交给 wav_test_05 的 5ms tick 流式喂数。
 *
 *   喂完后退出到 wav_test_run() 的 halt 循环，DMA 自己继续 drain obuf，
 *   不需要 CPU 再做什么。
 *
 * 前置：s_hdr 必须已由 wav_test_03 解析好。
 */
#define WAV_TEST04_FEED_BYTES   (1024 * 1024)   /* 1 MB ≈ 5 秒 44.1kHz 立体声 */

static bool wav_test_play_one_shot(void)
{
    printf("\n[WAV] ====== wav_test_04: obuf one-shot play ======\n");
    printf("[WAV]   wav sample_rate = %u Hz\n", s_hdr.sample_rate);
    printf("[WAV]   wav channels    = %u\n", s_hdr.channels);
    printf("[WAV]   wav data_offset = %u\n", s_hdr.data_offset);
    printf("[WAV]   wav data_size   = %u bytes (~%u seconds)\n",
           s_hdr.data_size,
           s_hdr.data_size / (s_hdr.channels * (s_hdr.bits / 8) * s_hdr.sample_rate));
    printf("[WAV]   feed limit     = %u bytes (~5 seconds)\n",
           WAV_TEST04_FEED_BYTES);

    /* Step 0: 唤醒 DAC 输出通路（关键！）
     *
     * 开机默认播完 power_on 音后，DAC 会停在 fade_out / mute 状态：
     *   - sys_cb.dac_sta = 0  (DAC powered off)
     *   - sys_cb.mute = 0     (mute flag 通常被清)
     *   - loudspeaker_mute 可能还置位
     *   - obuf 没有被 DMA 主动 drain
     *
     * 解决方案：仿照 bsp_sys_unmute() 的流程，手动打开：
     *   - sys_cb.dac_sta == 0 → dac_restart() (power on)
     *   - sys_cb.mute    == 1 → bsp_sys_unmute() (loudspeaker_unmute + dac_fade_in)
     *
     * 不调用这个，obuf_put_samples 喂进去的 PCM 永远不会被 DAC 播放。
     */
    printf("[WAV] Step 0: wake up DAC output path...\n");
    if (sys_cb.dac_sta == 0) {
        printf("[WAV]   dac_sta=0 -> dac_restart()\n");
        sys_cb.dac_sta = 1;
        dac_restart();
    }
    if (sys_cb.mute) {
        printf("[WAV]   mute=1 -> bsp_sys_unmute()\n");
        bsp_sys_unmute();  /* 这一步内部调用 loudspeaker_unmute + dac_fade_in */
    } else {
        /* 即便 mute=0，也确保 loudspeaker 解 mute + DAC fade in */
        printf("[WAV]   explicit loudspeaker_unmute + dac_fade_in\n");
        bsp_loudspeaker_unmute();
        dac_fade_in();
    }
    /* 确保音量不为 0（防止 dac_init 后 vol=0 听不到） */
    if (sys_cb.vol == 0) {
        sys_cb.vol = 8;
    }
    dac_set_volume(sys_cb.vol);

    /* Step 1: DAC 采样率匹配 wav */
    printf("[WAV] Step 1: dac_spr_set(SPR_xxx for %u Hz)...\n", s_hdr.sample_rate);
    dac_spr_set(hz_to_spr(s_hdr.sample_rate));

    /* Step 2: 清 obuf（重要，防止上一次播放的残留数据） */
    printf("[WAV] Step 2: dac_aubuf_clr()...\n");
    dac_aubuf_clr();

    /* Step 3: 跳过 RIFF 头 */
    printf("[WAV] Step 3: pff_lseek(%u) jump header...\n", s_hdr.data_offset);
    FRESULT fr = pff_lseek(s_hdr.data_offset);
    if (fr != FR_OK) {
        printf("[WAV] FAIL: pff_lseek -> %d (%s)\n", fr, fr_str(fr));
        return false;
    }

    /* Step 4: 喂数到 WAV_TEST04_FEED_BYTES 上限就退出。
     * 进度打印每 256KB 一次，避免用户以为程序卡死。 */
    printf("[WAV] Step 4: feeding PCM (capped at %u bytes)...\n",
           WAV_TEST04_FEED_BYTES);
    UINT br = 0;
    u32 total = 0;
    u32 last_print = 0;
    while (total < WAV_TEST04_FEED_BYTES) {
        fr = pff_read(s_pcm, sizeof(s_pcm), &br);
        if (fr != FR_OK || br == 0) break;
        obuf_put_samples(s_pcm, br);
        total += br;
        /* 进度打印：每 256KB 一次（~1.5 秒音频） */
        if (total - last_print >= 256 * 1024) {
            printf("[WAV]   fed %u / %u bytes (%u%%)\n",
                   total, WAV_TEST04_FEED_BYTES,
                   (total * 100) / WAV_TEST04_FEED_BYTES);
            last_print = total;
        }
    }
    printf("[WAV] PASS: feed done, total %u bytes (~%u seconds)\n",
           total,
           total / (s_hdr.channels * (s_hdr.bits / 8) * s_hdr.sample_rate));
    printf("[WAV]   (obuf now has data; DMA will drain in background)\n");
    printf("[WAV]   Listen on 3.5mm headphone jack NOW.\n");
    return true;
}
#endif

#if 0   // wav_test_05: 5ms tick streaming
static bool wav_test_stream_start(void);
#endif

#if 0   // wav_test_06: PLAY/PREV/NEXT key control
static bool wav_test_keys_init(void);
#endif

#if 0   // wav_test_07: root dir scan + playlist
static bool wav_test_scan_root(void);
#endif

/* ---------------------------------------------------------------------------
 * 入口：wav_test_run
 * ---------------------------------------------------------------------------
 *
 * 每个 wav_test_NN 子任务是一个 #if 闸门控制的测试函数，按顺序串联，
 * 任一失败立即跳到 halt。当前激活 wav_test_01 + wav_test_02。
 */
void wav_test_run(void)
{
#if 1
    if (!wav_test_banner())      goto halt;   // wav_test_01 ACTIVE
#endif

#if 1
    if (!wav_test_open_fixed())  goto halt;   // wav_test_02 ACTIVE
#endif

#if 1
    if (!wav_test_parse_header())  goto halt; // wav_test_03 ACTIVE
#endif

#if 1
    if (!wav_test_play_one_shot()) goto halt; // wav_test_04 ACTIVE
#endif

#if 0
    if (!wav_test_stream_start())  goto halt;  // wav_test_05
    if (!wav_test_keys_init())     goto halt;  // wav_test_06
    if (!wav_test_scan_root())     goto halt;  // wav_test_07
#endif

    printf("\n[WAV] ====== All active tests PASSED ======\n");

halt:
    // WDT 由 SDK tick 自动喂（bsp_sys_init 里 sys_set_tmr_enable(1,1)）。
    // delay_5ms(200) 进入 200ms 周期短延时，不会触发 WDT 复位。
    printf("[WAV] Halt. 5ms loop.\n");
    while (1) {
        delay_5ms(200);
    }
}
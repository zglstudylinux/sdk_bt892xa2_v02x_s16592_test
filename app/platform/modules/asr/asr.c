#include "include.h"
#include "asr.h"
#if ASR_EN

void asr_prefetch_init(void);


extern u8 asr_prefetch_kisck;
extern u8 ws_sysclk;


#define TRACE_EN                    0
#define DEBUG_EN					0

#define ASR_2_LEVEL_DEAL_EN         0       //唤醒词是否分两级处理
#define ASR_WARNNINGS_EN            1       //是否打开唤醒的提示音
#define ASR_CMD_LEN                 20      //唤醒词的最大长度（字节）

#if TRACE_EN
#define TRACE(...)                  printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif


//和asr_cmd_list对应
enum {
    ///唤醒词
    ASR_WAKE_UP_XIAO_LAN_TONG_XUE,
    ASR_WAKE_UP_XIAO_LAN_XIAO_LAN,
    ASR_WAKE_UP_NI_HAO_XIAO_LAN,
    ASR_WAKE_UP_ANSWER_INCOM,
    ASR_WAKE_UP_CALL_TERMINATE,

    ///二级命令词
    ASR_CMD_VOL_UP,
    ASR_CMD_VOL_DOWN,
    ASR_CMD_MUSIC_PLAY,
    ASR_CMD_MUSIC_PAUSE,
    ASR_CMD_MUSIC_STOP,
    ASR_CMD_MUSIC_NEXT,
    ASR_CMD_MUSIC_PREV,
    ASR_CMD_ANC_MODE,
    ASR_CMD_TRANSPARENCY_MODE,
    ASR_CMD_NORMAL_MODE,
    ASR_CMD_LIGHT_TURN_ON,
    ASR_CMD_LIGHT_TURN_OFF,
    ASR_CMD_VOICE_ASSISTANT,
    ASR_CMD_COLLECT_MUSIC,
    ASR_CMD_GAME_MODE,
    ASR_CMD_MUSIC_MODE,

    ASR_RESPONSE_RESULT_INDEX
};

const char asr_cmd_list[ASR_RESPONSE_RESULT_INDEX][ASR_CMD_LEN] = {
    ///唤醒词
    "小蓝同学",
    "小蓝小蓝",
    "你好小蓝",
    "接听电话",
    "挂断电话",

    ///二级命令词
    "增大音量",
    "减小音量",
    "播放歌曲",
    "暂停播放",
    "停止播放",
    "下一曲",
    "上一曲",
    "降噪模式",
    "通透模式",
    "普通模式",
    "打开灯光",
    "关闭灯光",
    "打开语音助手",
    "收藏音乐",
    "游戏模式",
    "音乐模式",
};


#if ASR_WARNNINGS_EN
static void asr_warning_play(u16 num)
{
#if BT_TWS_EN
    bsp_tws_tone_play(num);
#else
    bsp_piano_warning_play(WARNING_TONE, num);
#endif // BT_TWS_EN
}
#endif // ASR_WARNNINGS_EN


#if ASR_2_LEVEL_DEAL_EN ///唤醒词分两级处理
static u8 asr_wake_up_flag = 0;
static u32 asr_wake_ticks = 0;

#define ASR_WAKE_UP_WORD_NUM        3
#define ASR_WAKE_UP_TICKS_THRESOLD  100//4s

static void asr_2_level_deal(const char *result)
{
    uint32_t item = 0;
    for (; item < ASR_RESPONSE_RESULT_INDEX; item++) {
        if (strcmp(result, (const char *)&asr_cmd_list[item]) == 0) {
            break;
        }
    }
    TRACE("asr_deal %d\n", item);

    //wake up word
    if (item < ASR_WAKE_UP_WORD_NUM) {
        asr_wake_up_flag = 1;
        asr_wake_ticks = 0;
        #if ASR_WARNNINGS_EN
        asr_warning_play(2);
        #endif // ASR_WARNNINGS_EN
        return;
    }

    //incoming
    if (f_bt.disp_status == BT_STA_INCOMING) {
        if (item == ASR_WAKE_UP_ANSWER_INCOM) {
            bt_call_answer_incoming();
        } else if (item == ASR_WAKE_UP_CALL_TERMINATE) {
            bt_call_terminate();
        }
        asr_wake_up_flag = 0;
        asr_wake_ticks = 0;
        return;
    }

    if (asr_wake_up_flag) {
        switch (item) {
        case ASR_CMD_VOL_UP:
            msg_enqueue(KU_VOL_UP);
            break;

        case ASR_CMD_VOL_DOWN:
            msg_enqueue(KU_VOL_DOWN);
            break;

        case ASR_CMD_MUSIC_PLAY:
            bt_music_play();
            break;

        case ASR_CMD_MUSIC_PAUSE:
            bt_music_pause();
            break;

        case ASR_CMD_MUSIC_STOP:
            bt_music_stop();
            break;

        case ASR_CMD_MUSIC_NEXT:
            bt_music_next();
            break;

        case ASR_CMD_MUSIC_PREV:
            bt_music_prev();
            break;

        case ASR_CMD_ANC_MODE:

            break;

        case ASR_CMD_TRANSPARENCY_MODE:

            break;

        case ASR_CMD_NORMAL_MODE:

            break;

        case ASR_CMD_LIGHT_TURN_ON:
            break;

        case ASR_CMD_LIGHT_TURN_OFF:
            break;

        case ASR_CMD_VOICE_ASSISTANT:
            bt_hfp_siri_switch();
            break;

        case ASR_CMD_COLLECT_MUSIC:
            break;

        case ASR_CMD_GAME_MODE:
            if (!bt_is_low_latency()) {
                bsp_tws_res_music_play(TWS_RES_GAME_MODE);
            }
            break;

        case ASR_CMD_MUSIC_MODE:
            if (bt_is_low_latency()) {
                bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
            }
            break;

        default:
            break;
        }

        asr_wake_up_flag = 0;
        asr_wake_ticks = 0;
        #if ASR_WARNNINGS_EN
        asr_warning_play(3);
        #endif // ASR_WARNNINGS_EN
    }
}
#else                   ///唤醒词一级处理
static void asr_deal(const char *result)
{
    uint32_t item = 0;
    for (; item < ASR_RESPONSE_RESULT_INDEX; item++) {
        if (strcmp(result, (const char *)&asr_cmd_list[item]) == 0) {
            break;
        }
    }
    TRACE("asr_deal %d\n", item);

    switch (item) {
    case ASR_WAKE_UP_XIAO_LAN_TONG_XUE:
    case ASR_WAKE_UP_XIAO_LAN_XIAO_LAN:
    case ASR_WAKE_UP_NI_HAO_XIAO_LAN:
        bt_hfp_siri_switch();
        break;

    case ASR_WAKE_UP_ANSWER_INCOM:
        if (f_bt.disp_status == BT_STA_INCOMING) {
            bt_call_answer_incoming();
        }
        break;

    case ASR_WAKE_UP_CALL_TERMINATE:
        if (f_bt.disp_status == BT_STA_INCOMING) {
            bt_call_terminate();
        }
        break;

    case ASR_CMD_VOL_UP:
        msg_enqueue(KU_VOL_UP);
        break;

    case ASR_CMD_VOL_DOWN:
        msg_enqueue(KU_VOL_DOWN);
        break;

    case ASR_CMD_MUSIC_PLAY:
        bt_music_play();
        break;

    case ASR_CMD_MUSIC_PAUSE:
        bt_music_pause();
        break;

    case ASR_CMD_MUSIC_STOP:
        bt_music_stop();
        break;

    case ASR_CMD_MUSIC_NEXT:
        bt_music_next();
        break;

    case ASR_CMD_MUSIC_PREV:
        bt_music_prev();
        break;

    case ASR_CMD_ANC_MODE:

        break;

    case ASR_CMD_TRANSPARENCY_MODE:

        break;

    case ASR_CMD_NORMAL_MODE:

        break;

    case ASR_CMD_LIGHT_TURN_ON:
        break;

    case ASR_CMD_LIGHT_TURN_OFF:
        break;

    case ASR_CMD_VOICE_ASSISTANT:
        bt_hfp_siri_switch();
        break;

    case ASR_CMD_COLLECT_MUSIC:
        break;

    case ASR_CMD_GAME_MODE:
        if (!bt_is_low_latency()) {
            bsp_tws_res_music_play(TWS_RES_GAME_MODE);
        }
        break;

    case ASR_CMD_MUSIC_MODE:
        if (bt_is_low_latency()) {
            bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
        }
        break;

    default:
        break;
    }
    #if ASR_WARNNINGS_EN
    if (item == ASR_CMD_VOICE_ASSISTANT || item == ASR_CMD_GAME_MODE || item == ASR_CMD_MUSIC_MODE || (item <= ASR_WAKE_UP_CALL_TERMINATE)) {
        return;
    }
    asr_warning_play(2);
    #endif // ASR_WARNNINGS_EN
}
#endif // ASR_2_LEVEL_DEAL_EN

u32 asr_alg_process(short *ptr)
{
    const char *text;
    float score;

    ///asr recog
#if DEBUG_EN
    GPIOBSET = BIT(2);
#endif // DEBUG_EN

    sys_cb.asr_flag |= ASR_FLAG_ALG_PROC;
    int ret = Wanson_ASR_Recog(ptr, 400, &text, &score);
    sys_cb.asr_flag &= ~ASR_FLAG_ALG_PROC;

    if(sys_cb.asr_flag & ASR_FLAG_CLK_RESTORE){
#if ASR_VAD_EN
        set_sys_clk(ws_sysclk);
#endif
        sys_cb.asr_flag &= ~ASR_FLAG_CLK_RESTORE;
        sys_cb.asr_flag &= ~ASR_FLAG_CLK_INC;
    }

#if DEBUG_EN
    GPIOBCLR = BIT(2);
#endif // DEBUG_EN

#if ASR_PREFETCH_EN
    if (asr_prefetch_kisck) {
        asr_prefetch_kisck --;
    }
#endif

#if ASR_2_LEVEL_DEAL_EN
    if (asr_wake_up_flag) {
        if (asr_wake_ticks < ASR_WAKE_UP_TICKS_THRESOLD) {
            asr_wake_ticks++;
        } else {
            asr_wake_up_flag = 0;
            asr_wake_ticks = 0;
            #if ASR_WARNNINGS_EN
            asr_warning_play(3);
            #endif // ASR_WARNNINGS_EN
        }
    }
#endif // ASR_2_LEVEL_DEAL_EN

    if (ret == 1) {
        TRACE("ASR Result: %s\n", text);
        #if ASR_2_LEVEL_DEAL_EN
        asr_2_level_deal(text);
        #else
        asr_deal(text);
        #endif // ASR_2_LEVEL_DEAL_EN
#if ASR_PREFETCH_EN
        asr_prefetch_kisck = 5;
#endif
    }

    return ret;
}

void asr_alg_stop(void)
{
    Wanson_ASR_Release();
}

void asr_alg_start(void)
{
    if (Wanson_ASR_Init() < 0) {
        TRACE("Wanson_ASR_Init Failed!\n");
        while (1) WDT_CLR();
    }
    Wanson_ASR_Reset();
    TRACE("Wanson_ASR_Init OK!\n");
#if ASR_PREFETCH_EN
    asr_prefetch_init();
#endif
}

void asr_alg_init(void)
{
#if DEBUG_EN
    GPIOBDE |= BIT(1);
    GPIOBCLR = BIT(1);
    GPIOBDIR &= ~BIT(1);

    GPIOBDE |= BIT(2);
    GPIOBCLR = BIT(2);
    GPIOBDIR &= ~BIT(2);

    GPIOEDE |= BIT(4);
    GPIOECLR = BIT(4);
    GPIOEDIR &= ~BIT(4);
#endif // DEBUG_EN
}

#endif

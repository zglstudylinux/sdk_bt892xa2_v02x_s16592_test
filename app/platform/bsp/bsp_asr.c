#include "include.h"


#if ASR_EN

#define TRACE_EN                    0

#if TRACE_EN
#define TRACE(...)                  printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

u32 asr_alg_process(short *ptr);
void asr_alg_stop(void);
void asr_alg_start(void);
void asr_alg_init(void);
int vad_process(int16_t *ptr);
int vad_init(u32 start_threshold_value, u32 stop_threshold_value);
u32 vad_get_ste_value(void);
void alg_user_kick_start(void);

static u8 asr_sta;
#if ASR_VAD_EN
volatile int vad_state AT(.ws_asr.vad);
u8 ws_sysclk AT(.ws_asr.data);
#endif

#define ASR_PCM_BUFF_NUM                5
#define ASR_PRE_READ                    3
#define ASR_PCM_NUM                     400
#define VAD_START_THRESHOLD_VALUE       300000
#define VAD_STOP_THRESHOLD_VALUE        2000000
#define VAD_DMA_SIZE                    1280

uint8_t cfg_asr_stop_delay_threshold_value = 10;
u16 vad_dma_buf[VAD_DMA_SIZE] AT(.vad.dma);

struct asr_buf_t {
    volatile s16 wptr;
    volatile s16 rptr;
    short pcm_buf[ASR_PCM_BUFF_NUM][ASR_PCM_NUM];
} asr_buf AT(.ws_asr.data);

u32 asr_get_adc_buff_addr(void)
{
    return DMA_ADR(vad_dma_buf);
}

u32 asr_get_adc_buff_len(void)
{
    return VAD_DMA_SIZE;
}

AT(.text.sys_clk)
uint8_t sysclk_get_spiflash_clkdiv(u32 sys_clk)
{
    if (sys_clk == SYS_160M) {
        return 1;
    } else {
        return 0;
    }
}

AT(.com_text.alg)
static s16 alg_ptr_to_next(s16 ptr)
{
    s16 res = ptr + 1;
    if (res >= ASR_PCM_BUFF_NUM) {
        res = 0;
    }
    return res;
}

AT(.com_text.asr.adc_process)
void asr_sdadc_process(u8 *ptr, u32 samples, int ch_mode)
{
    struct asr_buf_t *p = &asr_buf;
    memcpy(p->pcm_buf[p->wptr], ptr, ASR_PCM_NUM * 2);
    p->wptr = alg_ptr_to_next(p->wptr);
    ///vad
#if ASR_VAD_EN
    vad_state = vad_process((int16_t*)ptr);
    if (vad_state == VAD_STATE_NO_VOICE) {
        return;
    } else if (vad_state == VAD_STATE_VOICE_START) {
        p->rptr = p->wptr - (ASR_PRE_READ + 1);
        if (p->rptr < 0) {
            p->rptr += ASR_PCM_BUFF_NUM;
        }
        ws_sysclk = get_cur_sysclk();
        set_sys_clk(SYS_160M);
        sys_cb.asr_flag |= ASR_FLAG_CLK_INC;
        sys_cb.asr_flag &= ~ASR_FLAG_CLK_RESTORE;
        alg_user_kick_start();
        TRACE("VAD START\n");
    } else if (vad_state == VAD_STATE_VOICE_SPEAKING) {
        alg_user_kick_start();
    } else if (vad_state == VAD_STATE_VOICE_STOP) {
        if(sys_cb.asr_flag & ASR_FLAG_ALG_PROC){
            sys_cb.asr_flag |= ASR_FLAG_CLK_RESTORE;
        }else{
            set_sys_clk(ws_sysclk);
            sys_cb.asr_flag &= ~ASR_FLAG_CLK_INC;
            sys_cb.asr_flag &= ~ASR_FLAG_CLK_RESTORE;
        }
        TRACE("VAD STOP %d\n", ws_sysclk);
        return;
    }
#endif // ASR_VAD_EN
}

AT(.com_text.alg)
void alg_user_process(void)
{
#if ASR_VAD_EN
    if (vad_state == VAD_STATE_VOICE_STOP) {
        return;
    }
#endif // ASR_VAD_EN
    if(sys_cb.sys_is_sleep){
        delay_5ms(3);
    }
    ///kws
    if(sys_cb.asr_flag & ASR_FLAG_CLK_INC){
        struct asr_buf_t *p = &asr_buf;
        asr_alg_process(p->pcm_buf[p->rptr]);
        p->rptr = alg_ptr_to_next(p->rptr);
    }
}

AT(.com_text.asr)
u8 bsp_asr_get_statue(void)
{
    return asr_sta;
}

void bsp_asr_start(void)
{
    if (asr_sta == ASR_START) {
        return;
    }

#if ASR_VAD_EN
    ws_sysclk = get_cur_sysclk();
    vad_state = VAD_STATE_NO_VOICE;
#endif // ASR_VAD_EN
    memset(&asr_buf, 0, sizeof(struct asr_buf_t));

    asr_alg_start();
    audio_path_init(AUDIO_PATH_ASR);
    audio_path_start(AUDIO_PATH_ASR);
#if ASR_VAD_EN
    vad_init(VAD_START_THRESHOLD_VALUE, VAD_STOP_THRESHOLD_VALUE);
#endif // ASR_VAD_EN
    asr_sta = ASR_START;
    TRACE("bsp_asr_start\n");
}

void bsp_asr_stop(void)
{
    if (asr_sta == ASR_IDLE) {
        return;
    }
    asr_alg_stop();
    audio_path_exit(AUDIO_PATH_ASR);
    asr_sta = ASR_IDLE;
    TRACE("bsp_asr_stop\n");
}

void bsp_asr_init(void)
{
    asr_alg_init();
#if ASR_VAD_EN
    vad_init(VAD_START_THRESHOLD_VALUE, VAD_STOP_THRESHOLD_VALUE);
#endif // ASR_VAD_EN
}

void asr_bt_evt_notice(uint evt, void *params)
{
    switch (evt) {
    case BT_NOTICE_TWS_CONNECTED:
        if (bt_tws_is_slave()) {
            msg_enqueue(EVT_ASR_STOP);
        }
        break;

    case BT_NOTICE_TWS_ROLE_CHANGE:
        if (bt_tws_is_slave()) {
            msg_enqueue(EVT_ASR_STOP);
        } else {
            msg_enqueue(EVT_ASR_START);
        }
        break;
    }
}

#endif // ASR_EN

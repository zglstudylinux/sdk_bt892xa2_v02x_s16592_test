#ifndef __BSP_ASR_H
#define __BSP_ASR_H

enum {
    ASR_IDLE = 0,
    ASR_START,
};

enum {
    ASR_FLAG_ALG_PROC = BIT(0),
    ASR_FLAG_CLK_RESTORE = BIT(1),
    ASR_FLAG_CLK_INC = BIT(2),
};

void bsp_asr_init(void);
void bsp_vad_start(void);
void bsp_vad_stop(void);
void bsp_asr_start(void);
void bsp_asr_stop(void);
void asr_bt_evt_notice(uint evt, void *params);
u8 bsp_asr_get_statue(void);
void vad_enter_sleep(u8 sleep);
bool is_vad_wake(void);

enum {
    VAD_STATE_NO_VOICE,
    VAD_STATE_VOICE_START,
    VAD_STATE_VOICE_SPEAKING,
    VAD_STATE_VOICE_STOP,
};

#endif

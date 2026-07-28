#include "include.h"

#if OPUS_ENC_EN

extern u16 opus_pcm_len(void);
extern int opus_enc_frame(s16 *pcm, u8 *packet);
extern void opus_kick_start(void);
extern int sco_set_mic_gain_after_aec(void);
extern void mic_post_gain_process_s(s16 *in_ptr, u32 *out_ptr, int gain, int samples);
extern bool opus_enc_init(u32 spr, u32 nch);
extern void opus_enc_exit(void);

static u8 opus_sysclk, opus_init = 0;
bool opus_start;
static uint8_t opus_skip_frame = 0; 

static u32 post_proc_buf[64] AT(.nr_buf.sco_proc);
//---------------------------------------------------------------------
//
#define MAX_PCM_BUF_SIZE        2560//1280            //50ms
typedef struct {
    s16 pcm[MAX_PCM_BUF_SIZE];
    u32 len;
    u16 widx;
    u16 ridx;
} opus_cb_t;

static opus_cb_t opus_pcm_cb AT(.opus.buf.bsp);
static volatile int opus_enc_sta;
static u8 opus_channel;
static u8 opus_packet[256] AT(.opus.buf.bsp);

//---------------------------------------------------------------------
//缓存压缩数据
#define OPUS_CACHE_LEN       480

struct opus_mcb_t {
    u16 wptr;
    u16 rptr;
    volatile u16 len;
    u8 output[OPUS_CACHE_LEN];

    u8 nr_type;
    int gain;
} opus_mcb AT(.opus.buf.bsp);

//--------------------------------------------------------------------------------
AT(.com_text.opus)
void opus_skip_frame_do(s16 *ptr, u32 samples, int ch_mode)
{
    opus_skip_frame--;
    memset(ptr, 0, samples << (1+ch_mode));
}

//--------------------------------------------------------------------------------
//128点输入一次
AT(.com_text.sndp)
WEAK void opus_nr_process(s16 *buf)
{

}

//---------------------------------------------------------------------
AT(.com_text.opus)
void opus_put_pcm(u8 *buf, u16 len, int ch_mode)
{
    opus_channel = ch_mode + 1;
    opus_cb_t *p = &opus_pcm_cb;

    if (p->len <= MAX_PCM_BUF_SIZE) {
        memcpy(&p->pcm[p->widx], buf, len * 2 * opus_channel);
        p->widx += (len * opus_channel);
        if (p->widx >= MAX_PCM_BUF_SIZE) {
            p->widx = 0;
        }
        GLOBAL_INT_DISABLE();
        p->len += (len * opus_channel);
        GLOBAL_INT_RESTORE();
    }else{
        uart_putchar('!');
    }
}

//---------------------------------------------------------------------
AT(.com_text.opus)
static s16* opus_get_pcm(void)
{
    s16 *buf = NULL;
    u32 frame_size = opus_pcm_len();
    frame_size = opus_channel * frame_size;
    opus_cb_t *p = &opus_pcm_cb;

    if (p->len >= frame_size) {
        buf = &p->pcm[p->ridx];
        p->ridx += frame_size;
        if (p->ridx >= MAX_PCM_BUF_SIZE) {
            p->ridx = 0;
        }
        GLOBAL_INT_DISABLE();
        p->len -= frame_size;
        GLOBAL_INT_RESTORE();
    }

    return buf;
}

//---------------------------------------------------------------------
//缓存压缩好的数据
AT(.com_text.opus)
void opus_put_frame(u8 *buf, u16 len)
{
    struct opus_mcb_t *p = &opus_mcb;

    if (p->len <= OPUS_CACHE_LEN) {
        memcpy(&p->output[p->wptr], buf, len);
        p->wptr += len;
        if (p->wptr >= OPUS_CACHE_LEN) {
            p->wptr = 0;
        }
        GLOBAL_INT_DISABLE();
        p->len += len;
        GLOBAL_INT_RESTORE();
    }else{
        uart_putchar('*');
    }
}

//---------------------------------------------------------------------
//
AT(.com_text.opus)
u8 opus_get_frame(u8 *rx_buff, u16 opus_frame_len)
{
	struct opus_mcb_t * p = &opus_mcb;

	if (p->len < opus_frame_len) {	   //1帧
		return 0;
	}

	memcpy(rx_buff, &p->output[p->rptr], opus_frame_len);

	p->rptr += opus_frame_len;
	if (p->rptr >= OPUS_CACHE_LEN) {
		p->rptr = 0;
	}

	GLOBAL_INT_DISABLE();
	p->len -= opus_frame_len;
	GLOBAL_INT_RESTORE();

	return opus_frame_len;
}


//---------------------------------------------------------------------
//
void opus_start_init(u8 nr_type)
{
    memset(&opus_mcb, 0, sizeof(struct opus_mcb_t));
    memset(&opus_pcm_cb, 0, sizeof(opus_pcm_cb));

    opus_enc_sta = 0;
    opus_mcb.nr_type = nr_type;
    opus_mcb.gain = sco_set_mic_gain_after_aec();
}

//---------------------------------------------------------------------
//
void opus_stop_end(void)
{
    if (opus_mcb.nr_type) {
        opus_mcb.nr_type = 0;
    }
}

//---------------------------------------------------------------------
//opus_sdadc_process
AT(.com_text.opus)
WEAK void opus_sdadc_process(u8 *ptr, u32 samples, int ch_mode)
{
    if (opus_skip_frame) {
        opus_skip_frame_do((s16*)ptr, samples, ch_mode);
        return;
    }
	
    if(opus_mcb.nr_type){
        opus_nr_process((void *)ptr);
        if(opus_mcb.gain){
            mic_post_gain_process_s((s16 *)ptr, post_proc_buf, opus_mcb.gain, samples);   //数字后增益
        }
        if(opus_mcb.nr_type == 2){
            ch_mode = 0;
        }
    }

    opus_put_pcm(ptr, samples, ch_mode);

    if (!opus_enc_sta && opus_pcm_cb.len >= (opus_pcm_len()<< ch_mode)) {
        opus_kick_start();
    }

#if TME_APP_EN
    tme_opus_frame_energy_cal(ptr,samples);
#endif
}

//---------------------------------------------------------------------
//

//---------------------------------------------------------------------
//
AT(.com_text.opus)
void opus_enc_process(void)
{
    int rlen;
    s16 *in = opus_get_pcm();
    if (in == NULL) {
        return;
    }

    opus_enc_sta = 1;
    rlen = opus_enc_frame(in, opus_packet);
    if(rlen < 0 || rlen > (128*opus_channel)) {
        uart_putchar('&');
        //printf("opus encode failed: %d\n",rlen);
    } else {
        //printf("opus:%d\n", rlen);
        opus_put_frame(opus_packet, rlen);
    }
    opus_enc_sta = 0;
}

AT(.text.opus.bsp)
bool bsp_opus_is_encode(void)
{
    return (opus_init == 1);
}

u16 opus_enc_data_len_get(void)
{
    return opus_mcb.len;
}

AT(.text.opus.bsp)
void bsp_opus_encode_start(void)
{
    if(opus_init == 0 && f_bt.disp_status < BT_STA_INCOMING){
        printf("--->bsp_opus_encode_start\n");
        bt_tws_user_key(OPUS_ENC_START);
        bt_audio_bypass();
        if(opus_enc_init(SPR_16000, 1)){
            opus_sysclk = get_cur_sysclk();
            set_sys_clk(SYS_160M);
            opus_skip_frame = 125; //丢掉MIC刚启动时不稳定的数据
            opus_start_init(NR_TYPE_SMIC);          
            audio_path_init(AUDIO_PATH_OPUS);
            audio_path_start(AUDIO_PATH_OPUS);
            opus_init = 1;
        }
     }
}

AT(.text.opus.bsp)
void bsp_opus_encode_stop(void)
{
    if(opus_init){
        printf("--->bsp_opus_encode_stop\n");
        opus_init = 0;
        opus_skip_frame = 0;
        bt_tws_user_key(OPUS_ENC_STOP);
        audio_path_exit(AUDIO_PATH_OPUS);
        opus_enc_exit();
        opus_stop_end();
        set_sys_clk(opus_sysclk);
        bt_audio_enable();
    }
}

AT(.text.opus.bsp)
u8 bsp_opus_get_enc_frame(u8 *buff, u16 len)
{
    if(opus_init && (buff != NULL)){
        return opus_get_frame(buff,len);
    }

    return 0;
}


#endif

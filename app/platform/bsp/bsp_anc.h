#ifndef __BSP_ANC_H
#define __BSP_ANC_H

#define  ANC_FADE_IN_STEP    20     //时间计算方法：ancvol变化0-->32767，淡入时间为（32767/FADE_IN_STEP + 1）*1ms
#define  ANC_FADE_OUT_STEP   20     //时间计算方法：ancvol变化32767-->0，淡出时间为（32767/FADE_IN_STEP + 1）*1ms
extern u8 anc_soft_loc_sta;                //用于ANC软件淡入淡出时，同步状态
extern u8 anc_soft_rem_sta;
void anc_soft_set_mode(u8 mode);
void anc_soft_mode_detect(void);
void anc_soft_vol_fade_process(void);
void bsp_anc_soft_set_mode(u8 mode);
void bsp_anc_soft_mode_detect(void);
void bsp_anc_soft_ancvol_detect(void);

void anc_soft_fade_init(u8 fade_en, u16 fade_in_step, u16 fade_out_step);
void bsp_anc_dbg_eq_param(u8 packet, u8 band_cnt, u32 *eq_buf);
void bsp_anc_start(void);
void bsp_anc_stop(void);
void bsp_anc_set_mode(u8 mode);
void bsp_anc_init(void);
void bsp_anc_exit(void);
void bsp_anc_mic_mute(u8 ch, u8 mute);
void bsp_anc_set_mic_gain(u8 ch, u8 anl, u8 gain);
void bsp_anc_gain_adjust(u8 ch, s8 value);
bool bsp_anc_res_check_crc(u32 addr, u32 len);
void bsp_anc_fade_enable(u8 enable);
void bsp_anc_dc_check(void);

void bsp_ttp_init(void);
void bsp_ttp_exit(void);
void bsp_ttp_start(void);
void bsp_ttp_stop(void);
#endif

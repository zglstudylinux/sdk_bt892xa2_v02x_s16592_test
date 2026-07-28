#include "include.h"
#include "bsp_effect.h"


#if BT_MUSIC_AUDIO_EN

#define TRACE_EN                            0

#if TRACE_EN
#define TRACE(...)                          printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif // TRACE_EN


typedef struct {
    u8 bt_music_audio_sta;      //进入SCO时保存的当前的音乐音效状态
    u8 dynamic_bass_state;      //动态低音状态
    u8 dbb_level_cnt;           //动态低音等级总数
    dbb_param_cb_t dbb_param;   //动态低音参数
    u8 dynamic_eq_state;        //动态EQ状态
} music_effect_t;
music_effect_t music_effect;


void music_effect_sco_audio_init_do(void)
{
    music_effect.bt_music_audio_sta = bt_music_audio_get_state();
#if BT_DYNAMIC_BASS_BOOST_EN
    music_dbb_audio_stop();
#elif BT_DYNAMIC_EQ_EN
    music_dyeq_audio_stop();
#else
    bt_music_audio_set_state(MUSIC_AUDIO_NONE);
#endif
}

void music_effect_sco_audio_exit_do(void)
{
    bt_music_audio_init(BT_MUSIC_AUDIO_TYPE);
#if BT_DYNAMIC_BASS_BOOST_EN
    if (music_effect.bt_music_audio_sta == MUSIC_AUDIO_DBB) {
        music_dbb_audio_start();
    }
#elif BT_DYNAMIC_EQ_EN
    if (music_effect.bt_music_audio_sta == MUSIC_AUDIO_DYEQ) {
        music_dyeq_audio_start();
    }
#else
    bt_music_audio_set_state(music_effect.bt_music_audio_sta);
#endif
}

///动态低音音效
#if BT_DYNAMIC_BASS_BOOST_EN

extern const u8  *dac_avol_table;
extern const u16 *dac_dvol_table;
extern const u16 dig_vol_tbl[61];

int bsp_dig_vol_convert_to_db(u16 dig_vol);
void dynamic_bass_set_coef_param(const u8* param, u32 len);
void dynamic_bass_set_param(dbb_param_cb_t* p);


#define DEFAULT_BASS_LEVEL                  10   //默认低音等级

const u8 dbb_coef_param[453] = {
/* 等级总数：11    	*/
/* 滤波器类型：low shelf  	*/
/* 中心频率：200    	*/
/* Q值：0.75      	*/
/* 增益：-10, -6, -2, 2, 4, 6, 8, 10, 12, 14, 16,   	*/
	0x02, 0x0b, 0xf6, 0xfa, 0xfe, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x7e, 0x33, 0xeb,
	0x07, 0xfe, 0xd2, 0xb6, 0x07, 0x96, 0x6b, 0xa0, 0x0f, 0xf6, 0xbe, 0x5e, 0xf0, 0x0f, 0x24, 0x5d,
	0xf8, 0xe3, 0x9a, 0xf3, 0x07, 0x15, 0xb0, 0xb8, 0x07, 0xc4, 0xd5, 0xaa, 0x0f, 0x3b, 0xae, 0x54,
	0xf0, 0x08, 0x39, 0x53, 0xf8, 0xd7, 0xe0, 0xfb, 0x07, 0x5a, 0x9f, 0xb9, 0x07, 0x0f, 0x1d, 0xb4,
	0x0f, 0x3e, 0xba, 0x4b, 0xf0, 0x1c, 0x57, 0x4a, 0xf8, 0x4a, 0x21, 0x04, 0x08, 0xce, 0xa3, 0xb9,
	0x07, 0x3f, 0x61, 0xbc, 0x0f, 0x8c, 0xc7, 0x43, 0xf0, 0xb3, 0x63, 0x42, 0xf8, 0x3e, 0x48, 0x08,
	0x08, 0x29, 0x4e, 0xb9, 0x07, 0x26, 0x2b, 0xc0, 0x0f, 0x0c, 0x27, 0x40, 0xf0, 0xcc, 0xbb, 0x3e,
	0xf8, 0x70, 0x78, 0x0c, 0x08, 0xae, 0xbd, 0xb8, 0x07, 0xa5, 0xbe, 0xc3, 0x0f, 0x1f, 0xbe, 0x3c,
	0xf0, 0xa6, 0x46, 0x3b, 0xf8, 0x81, 0xb5, 0x10, 0x08, 0xf8, 0xf1, 0xb7, 0x07, 0xca, 0x1e, 0xc7,
	0x0f, 0x44, 0x8a, 0x39, 0xf0, 0x96, 0x01, 0x38, 0xf8, 0x23, 0x03, 0x15, 0x08, 0x72, 0xea, 0xb6,
	0x07, 0x77, 0x4e, 0xca, 0x0f, 0x30, 0x89, 0x36, 0xf0, 0x12, 0xea, 0x34, 0xf8, 0x1e, 0x65, 0x19,
	0x08, 0x5d, 0xa6, 0xb5, 0x07, 0x67, 0x50, 0xcd, 0x0f, 0xc6, 0xb8, 0x33, 0xf0, 0xb2, 0xfd, 0x31,
	0xf8, 0x50, 0xdf, 0x1d, 0x08, 0xcc, 0x24, 0xb4, 0x07, 0x2a, 0x27, 0xd0, 0x0f, 0x1d, 0x17, 0x31,
	0xf0, 0x2c, 0x3a, 0x2f, 0xf8, 0xb3, 0x75, 0x22, 0x08, 0xa7, 0x64, 0xb2, 0x07, 0x2e, 0xd5, 0xd2,
	0x0f, 0x7d, 0xa2, 0x2e, 0xf0, 0x52, 0x9d, 0x2c, 0xf8, 0x40, 0x5e, 0xe9, 0x07, 0xe8, 0x79, 0xb0,
	0x07, 0x5d, 0xf2, 0x97, 0x0f, 0x46, 0x11, 0x67, 0xf0, 0x7a, 0x2b, 0x65, 0xf8, 0x9f, 0x82, 0xf2,
	0x07, 0xf5, 0x7f, 0xb2, 0x07, 0x41, 0x49, 0xa3, 0x0f, 0x1b, 0x24, 0x5c, 0xf0, 0xc9, 0x6a, 0x5a,
	0xf8, 0x70, 0x83, 0xfb, 0x07, 0xfd, 0x83, 0xb3, 0x07, 0x62, 0x63, 0xad, 0x0f, 0x79, 0x6c, 0x52,
	0xf0, 0x6e, 0xc8, 0x50, 0xf8, 0x15, 0x7f, 0x04, 0x08, 0x41, 0x89, 0xb3, 0x07, 0x5f, 0x63, 0xb6,
	0x0f, 0xe3, 0xcc, 0x49, 0xf0, 0xec, 0x27, 0x48, 0xf8, 0x8b, 0x04, 0x09, 0x08, 0x8c, 0x2c, 0xb3,
	0x07, 0x55, 0x83, 0xba, 0x0f, 0xea, 0xdd, 0x45, 0xf0, 0x28, 0x30, 0x44, 0xf8, 0x47, 0x94, 0x0d,
	0x08, 0x06, 0x90, 0xb2, 0x07, 0x0c, 0x68, 0xbe, 0x0f, 0x92, 0x2b, 0x42, 0xf0, 0x52, 0x6f, 0x40,
	0xf8, 0x3f, 0x32, 0x12, 0x08, 0x3f, 0xb3, 0xb1, 0x07, 0xd9, 0x14, 0xc2, 0x0f, 0x30, 0xb3, 0x3e,
	0xf0, 0x8b, 0xe2, 0x3c, 0xf8, 0x7d, 0xe2, 0x16, 0x08, 0x9c, 0x95, 0xb0, 0x07, 0xe0, 0x8c, 0xc5,
	0x0f, 0x51, 0x72, 0x3b, 0xf0, 0x18, 0x87, 0x39, 0xf8, 0x21, 0xa9, 0x1b, 0x08, 0x4f, 0x36, 0xaf,
	0x07, 0x18, 0xd3, 0xc8, 0x0f, 0xb9, 0x66, 0x38, 0xf0, 0x60, 0x5a, 0x36, 0xf8, 0x66, 0x8a, 0x20,
	0x08, 0x5d, 0x94, 0xad, 0x07, 0x4e, 0xea, 0xcb, 0x0f, 0x61, 0x8e, 0x35, 0xf0, 0xeb, 0x59, 0x33,
	0xf8, 0xa7, 0x8a, 0x25, 0x08, 0x98, 0xae, 0xab, 0x07, 0x25, 0xd5, 0xce, 0x0f, 0x7b, 0xe7, 0x32,
	0xf0, 0x61, 0x83, 0x30, 0xf8,
};


void music_dbb_audio_start(void)
{
    if (music_effect.dynamic_bass_state == 1) {
        return;
    }

    //调整主频
    sys_clk_req(INDEX_KARAOK, SYS_60M);             						//借用一下INDEX_KARAOK

    //MUTE一下去掉杂音
    dac_fade_out();
    dac_fade_wait();

    //设置参数
    dynamic_bass_set_coef_param(dbb_coef_param, sizeof(dbb_coef_param));
    music_effect.dbb_level_cnt = dbb_coef_param[1];
    music_effect.dbb_param.bass_level = DEFAULT_BASS_LEVEL;
    if (music_effect.dbb_param.bass_level >= music_effect.dbb_level_cnt) {  //bass等级范围限制
        music_effect.dbb_param.bass_level = music_effect.dbb_level_cnt - 1;
    }
#if !SYS_ADJ_DIGVOL_EN
    u8 anl_vol = 0;
    if (sys_cb.vol <= VOL_MAX) {
        if (sys_cb.vol > 0) {
            anl_vol = dac_avol_table[sys_cb.vol] + sys_cb.anl_gain_offset;
        }
    }
    music_effect.dbb_param.vol_level = 54 - (anl_vol);
    if (anl_vol == 0) {
        music_effect.dbb_param.vol = 0;
    } else {
        music_effect.dbb_param.vol = dig_vol_tbl[music_effect.dbb_param.vol_level];
    }
#else
    u16 dig_vol = dac_dvol_table[sys_cb.vol];
    int vol_level = bsp_dig_vol_convert_to_db(dig_vol);
    if (vol_level < 0) {
        printf("ERROR: DIGITAL GAIN VALUE ERR!\n");
        return;
    }
    music_effect.dbb_param.vol_level = vol_level;
    music_effect.dbb_param.vol = dig_vol;
#endif
    music_effect.dbb_param.vol_direct_set = 1;
    music_effect.dbb_param.sample_rate = SPR_44100; 						 //初始先设44.1K，后续流程会自动读寄存器的值进行调整
    dynamic_bass_set_param(&music_effect.dbb_param);

    //开启动态低音
    bt_music_audio_set_state(MUSIC_AUDIO_DBB);
    music_effect.dynamic_bass_state = 1;

    //调整音量
    sys_cb.soft_vol_en = 1;
#if !SYS_ADJ_DIGVOL_EN
    dac_set_volume(54);
#else
    dac_set_dvol(DIG_N0DB);
#endif
    delay_5ms(8);
    dac_fade_in();

    TRACE("music_dbb_audio_start\n");
}

void music_dbb_audio_stop(void)
{
    if (music_effect.dynamic_bass_state == 0) {
        return;
    }

    //MUTE一下去掉杂音
    dac_fade_out();
    dac_fade_wait();

    //关闭动态低音
    bt_music_audio_set_state(MUSIC_AUDIO_NONE);
    music_effect.dynamic_bass_state = 0;

    //调整音量
    sys_cb.soft_vol_en = 0;
    bsp_change_volume(sys_cb.vol);
    delay_5ms(8);
    dac_fade_in();

    //调整主频
    sys_clk_free(INDEX_KARAOK);

    TRACE("music_dbb_audio_stop\n");
}

void music_dbb_audio_set_bass_level(u8 bass_level)
{
    if (music_effect.dynamic_bass_state == 0) {
        return;
    }

    //bass等级范围限制
    if (bass_level >= music_effect.dbb_level_cnt) {
        bass_level = music_effect.dbb_level_cnt - 1;
    }
    music_effect.dbb_param.bass_level = bass_level;
//    music_effect.dbb_param.sample_rate = (u8)((DACDIGCON0 >> 2) & 0xF); //DAC SRC0输入采样率
    dynamic_bass_set_param(&music_effect.dbb_param);

    TRACE("bass_level: %d, vol: -%d dB 0x%04x, %d, spr: %d\n",
          music_effect.dbb_param.bass_level,
          music_effect.dbb_param.vol_level,
          music_effect.dbb_param.vol,
          music_effect.dbb_param.vol_direct_set,
          music_effect.dbb_param.sample_rate
    );
}

AT(.com_text.effect.dbb)
void music_dbb_audio_set_vol(u8 sys_vol, u8 vol_direct_set)
{
    if (music_effect.dynamic_bass_state == 0) {
        return;
    }

#if !SYS_ADJ_DIGVOL_EN
    u8 anl_vol = 0;
    if (sys_vol <= VOL_MAX) {
        if (sys_vol > 0) {
            anl_vol = dac_avol_table[sys_vol] + sys_cb.anl_gain_offset;
        }
    }
    music_effect.dbb_param.vol_level = 54 - (anl_vol);
    if (anl_vol == 0) {
        music_effect.dbb_param.vol = 0;
    } else {
        music_effect.dbb_param.vol = dig_vol_tbl[music_effect.dbb_param.vol_level];
    }
#else
    u16 dig_vol = dac_dvol_table[sys_vol];
    int vol_level = bsp_dig_vol_convert_to_db(dig_vol);
    if (vol_level < 0) {
        printf("ERROR: DIGITAL GAIN VALUE ERR!\n");
        return;
    }
    music_effect.dbb_param.vol_level = vol_level;
    music_effect.dbb_param.vol = dig_vol;
#endif
    music_effect.dbb_param.vol_direct_set = vol_direct_set;
//    music_effect.dbb_param.sample_rate = (u8)((DACDIGCON0 >> 2) & 0xF); //DAC SRC0输入采样率
    dynamic_bass_set_param(&music_effect.dbb_param);

    TRACE("bass_level: %d, vol: -%d dB 0x%04x, %d, spr: %d\n",
          music_effect.dbb_param.bass_level,
          music_effect.dbb_param.vol_level,
          music_effect.dbb_param.vol,
          music_effect.dbb_param.vol_direct_set,
          music_effect.dbb_param.sample_rate
    );
}

#if SYS_ADJ_DIGVOL_EN
AT(.com_text.effect.dbb)
void music_dbb_audio_set_vol_by_digvol(u16 digvol, u8 vol_direct_set)
{
    if (music_effect.dynamic_bass_state == 0) {
        return;
    }

    int vol_level = bsp_dig_vol_convert_to_db(digvol);
    if (vol_level < 0) {
        printf("ERROR: DIGITAL GAIN VALUE ERR!\n");
        return;
    }
    music_effect.dbb_param.vol_level = vol_level;
    music_effect.dbb_param.vol = digvol;

    music_effect.dbb_param.vol_direct_set = vol_direct_set;
//    music_effect.dbb_param.sample_rate = (u8)((DACDIGCON0 >> 2) & 0xF); //DAC SRC0输入采样率
    dynamic_bass_set_param(&music_effect.dbb_param);

    TRACE("bass_level: %d, vol: -%d dB 0x%04x, %d, spr: %d\n",
          music_effect.dbb_param.bass_level,
          music_effect.dbb_param.vol_level,
          music_effect.dbb_param.vol,
          music_effect.dbb_param.vol_direct_set,
          music_effect.dbb_param.sample_rate
    );
}
#endif // SYS_ADJ_DIGVOL_EN

AT(.com_text.effect.dbb)
u8 music_dbb_get_state(void)
{
    return music_effect.dynamic_bass_state;
}

#endif // BT_DYNAMIC_BASS_BOOST_EN

///动态EQ音效
#if BT_DYNAMIC_EQ_EN
extern const u8  *dac_avol_table;
extern const u16 *dac_dvol_table;
extern const u16 dig_vol_tbl[61];

u8 dyeq_cb[200] AT(.music_buff.dyeq);
u8 dyeq_drc_cb[84] AT(.music_buff.dyeq);
soft_vol_t dyeq_soft_vol AT(.music_buff.dyeq);

AT(.audio_text.dyeq)
ALWAYS_INLINE s32 dyeq_mono_process(s16 data)
{
    s32 tmp32;
    soft_vol_process_mono_one_sample(&dyeq_soft_vol, &data);
    tmp32 = (s32)data;
    dynamic_eq_process(dyeq_cb, &tmp32);
    tmp32 = dyeq_drc_v3_calc(tmp32, dyeq_drc_cb);
    return tmp32;
}

AT(.audio_text.dyeq)
void dynamic_eq_frame_process(s16* buf_l, s16* buf_r)
{
    int i;
    s32 tmp32;
    s16 tmp16;
#if BT_TWS_EN
    u8 ch_idx = sys_cb.tws_left_channel ? 0 : 1;     //in_nch: 0=L, 1=R, 2=(L+R)/2
    if (!bt_tws_is_connected()) {
        ch_idx = 2;
    }
#else
    u8 ch_idx = 2;
#endif // BT_TWS_EN

    if (ch_idx == 0) {
        for (i = 0; i < 256; i++) {
            tmp32 = dyeq_mono_process(buf_l[i]);
            buf_l[i] = (s16)tmp32;
            buf_r[i] = (s16)tmp32;
        }
    } else if (ch_idx == 1) {
        for (i = 0; i < 256; i++) {
            tmp32 = dyeq_mono_process(buf_r[i]);
            buf_l[i] = (s16)tmp32;
            buf_r[i] = (s16)tmp32;
        }
    } else if (ch_idx == 2) {
        for (i = 0; i < 256; i++) {
            tmp16 = (s16)(((s32)buf_l[i] + buf_r[i]) >> 1);
            tmp32 = dyeq_mono_process(tmp16);
            buf_l[i] = (s16)tmp32;
            buf_r[i] = (s16)tmp32;
        }
    }
}

void music_dyeq_audio_start(void)
{
    if (music_effect.dynamic_eq_state == 1) {
        return;
    }

    //调整主频
    sys_clk_req(INDEX_KARAOK, SYS_60M);             //借用一下INDEX_KARAOK

    //MUTE一下去掉杂音
    dac_fade_out();
    dac_fade_wait();

    //设置参数
    memset(dyeq_cb, 0, sizeof(dyeq_cb));
    memset(dyeq_drc_cb, 0, sizeof(dyeq_drc_cb));
    bsp_set_effect_by_abt();
    soft_vol_init(&dyeq_soft_vol);

    //开启动态EQ
    bt_music_audio_set_state(MUSIC_AUDIO_DYEQ);
    music_effect.dynamic_eq_state = 1;

    //调整音量
    sys_cb.soft_vol_en = 1;
    music_dyeq_audio_set_vol(sys_cb.vol, 1);
#if !SYS_ADJ_DIGVOL_EN
    dac_set_volume(54);
#else
    dac_set_dvol(DIG_N0DB);
#endif
    delay_5ms(8);
    dac_fade_in();

    TRACE("music_dyeq_audio_start\n");
}

void music_dyeq_audio_stop(void)
{
    if (music_effect.dynamic_eq_state == 0) {
        return;
    }

    //MUTE一下去掉杂音
    dac_fade_out();
    dac_fade_wait();

    //关闭动态低音
    bt_music_audio_set_state(MUSIC_AUDIO_NONE);
    music_effect.dynamic_eq_state = 0;

    //调整音量
    sys_cb.soft_vol_en = 0;
    bsp_change_volume(sys_cb.vol);
    delay_5ms(8);
    dac_fade_in();

    //调整主频
    sys_clk_free(INDEX_KARAOK);

    TRACE("music_dyeq_audio_stop\n");
}

void music_dyeq_set_param(u8 *buf)
{
    dyeq_coef_update(dyeq_cb, buf);
}

void music_dyeq_drc_set_param(void *buf)
{
    dyeq_drc_v3_set_param(buf, dyeq_drc_cb);
}

AT(.audio_text.dyeq)
void dynamic_eq_clear_cache(void)
{
    dyeq_clear_cache(dyeq_cb);
}

AT(.com_text.effect.dyeq)
void music_dyeq_audio_set_vol(u8 sys_vol, u8 vol_direct_set)
{
    u16 vol;
    if (music_effect.dynamic_eq_state == 0) {
        return;
    }

#if !SYS_ADJ_DIGVOL_EN
    u8 anl_vol = 0;
    u8 vol_level;
    if (sys_vol <= VOL_MAX) {
        if (sys_vol > 0) {
            anl_vol = dac_avol_table[sys_vol] + sys_cb.anl_gain_offset;
        }
    }
    vol_level = 54 - (anl_vol);
    if (anl_vol == 0) {
        vol = 0;
    } else {
        vol = dig_vol_tbl[vol_level];
    }
#else
    vol = dac_dvol_table[sys_vol];
#endif
    soft_vol_set_vol_param(&dyeq_soft_vol, vol, vol_direct_set);
    TRACE("music_dyeq_audio_set_vol : %x %x\n", vol, vol_direct_set);
}

#if SYS_ADJ_DIGVOL_EN
AT(.com_text.effect.dyeq)
void music_dyeq_audio_set_vol_by_digvol(u16 digvol, u8 vol_direct_set)
{
    if (music_effect.dynamic_eq_state == 0) {
        return;
    }

    soft_vol_set_vol_param(&dyeq_soft_vol, digvol, vol_direct_set);
    TRACE("music_dyeq_audio_set_vol_by_digvol : %x %x\n", digvol, vol_direct_set);
}
#endif // SYS_ADJ_DIGVOL_EN

AT(.com_text.effect.dyeq)
u8 music_dyeq_get_state(void)
{
    return music_effect.dynamic_eq_state;
}

#endif // BT_DYNAMIC_EQ_EN

#endif // BT_MUSIC_AUDIO_EN

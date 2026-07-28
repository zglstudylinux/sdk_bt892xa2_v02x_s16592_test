#ifndef _BSP_EFFECT_H
#define _BSP_EFFECT_H

void music_effect_sco_audio_init_do(void);
void music_effect_sco_audio_exit_do(void);

#if BT_DYNAMIC_BASS_BOOST_EN
#define music_soft_vol_get_state()       music_dbb_get_state()
#elif BT_DYNAMIC_EQ_EN
#define music_soft_vol_get_state()       music_dyeq_get_state()
#endif

//动态低音音效
void music_dbb_audio_start(void);
void music_dbb_audio_stop(void);
void music_dbb_audio_set_bass_level(u8 bass_level);
void music_dbb_audio_set_vol(u8 sys_vol, u8 vol_direct_set);
void music_dbb_audio_set_vol_by_digvol(u16 digvol, u8 vol_direct_set);
void bt_music_audio_clear_cache(void);
u8 music_dbb_get_state(void);

//动态EQ音效
void music_dyeq_audio_start(void);
void music_dyeq_audio_stop(void);
void music_dyeq_set_param(u8 *buf);
void music_dyeq_drc_set_param(void *buf);
void music_dyeq_audio_set_vol(u8 sys_vol, u8 vol_direct_set);
void music_dyeq_audio_set_vol_by_digvol(u16 digvol, u8 vol_direct_set);
u8 music_dyeq_get_state(void);

#endif // _BSP_EFFECT_H

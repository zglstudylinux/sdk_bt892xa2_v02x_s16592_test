#include "include.h"
#include "api.h"

//const uint8_t cfg_bt_a2dp_media_discard = 3;        //华为手机挂断通话恢复音乐有漏音（前3帧），可配置丢弃前几帧
//const uint8_t cfg_a2dp_sbc_max_bitpool = 53;        //default=53
uint8_t cfg_bt_a2dp_id3_en = BT_ID3_TAG_EN;

#if !BT_A2DP_RECON_EN
void a2dp_recon_hook(void)
{
}
#endif

uint a2dp_get_vol(void)
{
    uint vol;

    vol = ((u32)sys_cb.vol * 1280L /VOL_MAX) /10;

    //32段音量时，同步手机音量由98%->100%调整为97%->100%(98%->100%有时会出现同步不了的情况:耳机音量32了，但手机音量97%)
    if(VOL_MAX==32 && vol==0x7C) {
        vol=0x7B;
    }

    if(vol > 0x7f) {
        vol = 0x7f;
    }

    return vol;
}

void a2dp_vol_conver(u8 *vol)
{
	u8 vol_unit = (1280L / VOL_MAX) / 10;
	if (*vol > 0 && *vol < vol_unit) {
		*vol = vol_unit;
	}
	*vol = (*vol + 1) * VOL_MAX / 128;
}

//检查a2dp音量
void a2dp_vol_check(u8 *vol)
{
    u8 vol_unit = (1280L / VOL_MAX) / 10;
    if (*vol > 0 && *vol < vol_unit) {
        *vol = vol_unit;
    }
}

u16 a2dp_change_volume(u8 vol)
{
    a2dp_vol_check(&vol);
    uint8_t vol_set = (vol + 1) * VOL_MAX / 128;
    sys_cb.vol = vol_set;
	msg_enqueue(EVT_A2DP_VOL_SAVE);
#if !SYS_ADJ_DIGVOL_EN
    if (sys_cb.incall_flag & INCALL_FLAG_SCO) {
        return 0xffff;
    }
#endif
    return bsp_volume_convert(vol_set);
}

AT(.com_text.a2dp)
void a2dp_tws_sync_vol(uint16_t vol)
{
    if (bsp_tws_sync_is_play()) {
        msg_enqueue(EVT_A2DP_SET_VOL);
    } else if ((sys_cb.incall_flag & INCALL_FLAG_SCO) == 0) {
#if !SYS_ADJ_DIGVOL_EN
    #if BT_DYNAMIC_BASS_BOOST_EN
        if (sys_cb.soft_vol_en) {
            music_dbb_audio_set_vol(sys_cb.vol, f_bt.disp_status != BT_STA_PLAYING);
            return;
        }
    #endif
    #if BT_DYNAMIC_EQ_EN
        if (sys_cb.soft_vol_en) {
            music_dyeq_audio_set_vol(sys_cb.vol, f_bt.disp_status != BT_STA_PLAYING);
            return;
        }
    #endif // BT_DYNAMIC_EQ_EN
        if (vol != 0xffff) {
            dac_set_volume(vol);
        } else {
            bsp_change_volume(sys_cb.vol);
        }
#else
    #if BT_DYNAMIC_BASS_BOOST_EN
        if (sys_cb.soft_vol_en) {
            music_dbb_audio_set_vol_by_digvol(vol, f_bt.disp_status != BT_STA_PLAYING);
            return;
        }
    #endif
    #if BT_DYNAMIC_EQ_EN
        if (sys_cb.soft_vol_en) {
            music_dyeq_audio_set_vol_by_digvol(vol, f_bt.disp_status != BT_STA_PLAYING);
            return;
        }
    #endif // BT_DYNAMIC_EQ_EN
        dac_set_dvol(vol);
#endif
    }
}


AT(.com_text.a2dp)
void a2dp_tws_sync_close(void)
{
    msg_enqueue(EVT_A2DP_SYNC_CLOSE);
}


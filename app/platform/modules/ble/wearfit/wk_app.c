#include "include.h"
#include "wk_app.h"
#include "wk_ota.h"
#include "wk_tws.h"

#if LE_WK_APP_EN

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#define TRACE_R(...)            print_r(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACE_R(...)
#endif


static const char ascii_buff[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
const u8 app_cmd_id[4] = {0x55, 0xAA, 0x41, 0x42};

static u8 be_frame_mem[BLE_FRMAE_MEM_SIZE] AT(.ble_buf.app);
wkapp_cb_st wkapp_cb AT(.ble_buf.app);
//-------------------------------------------------------------------

AT(.app_text.param)
void wk_flash_write(void *buf, u32 addr, uint len)
{
    cm_write(buf, APP_CM_PAGE(addr), len);
    wkapp_cb.param_save_update = 2;          //2s
}

AT(.app_text.param)
void wk_flash_read(void *buf, u32 addr, uint len)
{
    cm_read(buf, APP_CM_PAGE(addr), len);
}

AT(.app_text.param)
void wk_param_sync(void)
{
    if(wkapp_cb.param_save_update){
        wkapp_cb.param_save_update--;
        if(wkapp_cb.param_save_update == 0x00){
            param_sync();
        }
    }
}

AT(.app_text.param)
void wk_param_flag_load(void)
{
    wk_flash_read(&wkapp_cb.param_flag,WK_CM_FLAG_ADDR,WK_CM_FLAG_LEN);
}

AT(.app_text.param)
void wk_param_flag_save(void)
{
    if(wkapp_cb.param_flag != PARAM_FLAG){
        wkapp_cb.param_flag = PARAM_FLAG;
        wk_flash_write(&wkapp_cb.param_flag,WK_CM_FLAG_ADDR,WK_CM_FLAG_LEN);
    }
}

u8 wk_app_mtu_get(void)
{
    u16 mtu = 0;

    mtu = ble_get_gatt_mtu();

    if(mtu > 0xFF){
        mtu = 0xFF;
    }
    return (u8)mtu;
}

AT(.app_text.init)
void wk_sn_init(void)
{
    u8 buff[WK_CM_SN_LEN];
    u16 crc, crc_sn;

    wk_flash_read(buff, WK_CM_SN_ADDR, WK_CM_SN_LEN);
    crc = calc_crc(buff, WK_CM_SN_LEN, 0xFFFF);
    wk_flash_read((u8 *)&crc_sn, WK_CM_SN_CRC_ADDR, WK_CM_SN_CRC_LEN);

    if(crc == crc_sn){
        memcpy(wkapp_cb.sn_l, buff, SN_LEN);
        memcpy(wkapp_cb.sn_r, &buff[SN_LEN], SN_LEN);
    }else{
        TRACE("SN CRC ERR!\n");

        u8 *sn_temp = buff;

        for(u8 i=0; i<(SN_LEN/2); i++){
            sn_temp[i*2] = ascii_buff[(xcfg_cb.bt_addr[i]>>4)&0x0F];
            sn_temp[i*2+1] = ascii_buff[(xcfg_cb.bt_addr[i])&0x0F];
        }

        if(sys_cb.tws_left_channel){
            memcpy(wkapp_cb.sn_l,  sn_temp,  SN_LEN);
        }else{
            memcpy(wkapp_cb.sn_r,  sn_temp,  SN_LEN);
        }
        TRACE_R(wkapp_cb.sn_l, 24);
    }
}


AT(.app_text.init)
void wk_key_init(void)
{
    if(WK_CM_IS_EXIST()){
        wk_flash_read(&wkapp_cb.key_local.key_short, WK_CM_KEY_ADDR, WK_CM_KEY_LEN);
    }else{
        TRACE("Key Default\n");
        if(sys_cb.tws_left_channel){
            wkapp_cb.key_local.key_short    = KEY_SHORT_LEFT_DEF;
            wkapp_cb.key_local.key_long     = KEY_LONG_LEFT_DEF;
            wkapp_cb.key_local.key_double   = KEY_DOUBLE_LEFT_DEF;
            wkapp_cb.key_local.key_three    = KEY_THREE_LEFT_DEF;
            wkapp_cb.key_local.key_four     = KEY_FOUR_LEFT_DEF;
            wkapp_cb.key_local.key_five     = KEY_FIVE_LEFT_DEF;

            wkapp_cb.key_remote.key_short    = KEY_SHORT_RIGHT_DEF;
            wkapp_cb.key_remote.key_long     = KEY_LONG_RIGHT_DEF;
            wkapp_cb.key_remote.key_double   = KEY_DOUBLE_RIGHT_DEF;
            wkapp_cb.key_remote.key_three    = KEY_THREE_RIGHT_DEF;
            wkapp_cb.key_remote.key_four     = KEY_FOUR_RIGHT_DEF;
            wkapp_cb.key_remote.key_five     = KEY_FIVE_RIGHT_DEF;
        }else{
            wkapp_cb.key_local.key_short    = KEY_SHORT_RIGHT_DEF;
            wkapp_cb.key_local.key_long     = KEY_LONG_RIGHT_DEF;
            wkapp_cb.key_local.key_double   = KEY_DOUBLE_RIGHT_DEF;
            wkapp_cb.key_local.key_three    = KEY_THREE_RIGHT_DEF;
            wkapp_cb.key_local.key_four     = KEY_FOUR_RIGHT_DEF;
            wkapp_cb.key_local.key_five     = KEY_FIVE_RIGHT_DEF;

            wkapp_cb.key_remote.key_short    = KEY_SHORT_LEFT_DEF;
            wkapp_cb.key_remote.key_long     = KEY_LONG_LEFT_DEF;
            wkapp_cb.key_remote.key_double   = KEY_DOUBLE_LEFT_DEF;
            wkapp_cb.key_remote.key_three    = KEY_THREE_LEFT_DEF;
            wkapp_cb.key_remote.key_four     = KEY_FOUR_LEFT_DEF;
            wkapp_cb.key_remote.key_five     = KEY_FIVE_LEFT_DEF;
        }
        wk_flash_write(&wkapp_cb.key_local.key_short, WK_CM_KEY_ADDR, WK_CM_KEY_LEN);
    }

    xcfg_cb.user_def_ks_sel = wkapp_cb.key_local.key_short;
    xcfg_cb.user_def_kl_sel = wkapp_cb.key_local.key_long;
    xcfg_cb.user_def_kd_sel = wkapp_cb.key_local.key_double;
    xcfg_cb.user_def_kt_sel = wkapp_cb.key_local.key_three;
    xcfg_cb.user_def_kfour_sel = wkapp_cb.key_local.key_four;
    xcfg_cb.user_def_kfive_sel = wkapp_cb.key_local.key_five;
}

AT(.app_text.key)
void wk_key_info_save(void)
{
    TRACE("wk_key_info_save:");

    u8 key_temp[WK_CM_KEY_LEN];

    memcpy(key_temp, &wkapp_cb.key_local.key_short, WK_CM_KEY_LEN);
    wk_flash_write(key_temp, WK_CM_KEY_ADDR, WK_CM_KEY_LEN);

    TRACE_R(key_temp, WK_CM_KEY_LEN);
}


AT(.app_text.init)
void wk_poweroff_time_init(void)
{
    if(WK_CM_IS_EXIST()){
        wk_flash_read(&wkapp_cb.poweroff_time, WK_CM_OFF_TIME_ADDR, WK_CM_OFF_TIME_LEN);
        if(wkapp_cb.poweroff_time == 0x00){
            sys_cb.pwroff_time   = -1L;
        }else{
            sys_cb.pwroff_time = (u32)wkapp_cb.poweroff_time * 60 * 10;
        }
    }else{
        TRACE("Poweroff Time Default\n");
        if(xcfg_cb.sys_off_time){
            wkapp_cb.poweroff_time = xcfg_cb.sys_off_time/60;
            sys_cb.pwroff_time = (u32)wkapp_cb.poweroff_time * 60 * 10;
        }else{
            sys_cb.pwroff_time = -1L;
            wkapp_cb.poweroff_time = 0;
        }
        reset_pwroff_delay();
        wk_flash_write(&wkapp_cb.poweroff_time, WK_CM_OFF_TIME_ADDR, WK_CM_OFF_TIME_LEN);
    }
}


AT(.app_text.param)
void wk_poweroff_time_save(u8 time)
{
    TRACE("poweroff time save:%d\n", time);

    wkapp_cb.poweroff_time = time;

    if(wkapp_cb.poweroff_time == 0x00){
        sys_cb.pwroff_time   = -1;
    }else{
        sys_cb.pwroff_time = (u32)wkapp_cb.poweroff_time * 60 * 10;
    }

    wk_flash_write(&wkapp_cb.poweroff_time, WK_CM_OFF_TIME_ADDR, WK_CM_OFF_TIME_LEN);

    reset_pwroff_delay();

    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_SENT_OFF_TIMES, &time, 1);
    }
}

//------------------------------------------------------------------------
//EAR DET
#if WK_CTL_IN_EAR_EN
AT(.app_text.param)
void wk_ear_det_init(void)
{
    if(WK_CM_IS_EXIST()){
        wk_flash_read(&sys_cb.in_ear_en, WK_CM_EAR_DET_ADDR, WK_CM_EAR_DET_LEN);
    }else{
        TRACE("Ear Det Default\n");
        sys_cb.in_ear_en = 1;
        wk_flash_write(&sys_cb.in_ear_en, WK_CM_EAR_DET_ADDR, WK_CM_EAR_DET_LEN);
    }
}

AT(.app_text.param)
void wk_ear_det_save(u8 ear_en)
{
    sys_cb.in_ear_en = ear_en;

    wk_flash_write(&sys_cb.in_ear_en, WK_CM_EAR_DET_ADDR, WK_CM_EAR_DET_LEN);

    if(IS_APP_MASTER()){
        wk_tws_ear_det_sync();
    }
}
#endif

//------------------------------------------------------------------------
//EQ
AT(.app_text.param)
void wk_eq_init(void)
{
    if(WK_CM_IS_EXIST()){
        wk_flash_read(&sys_cb.eq_mode, WK_CM_EQ_ADDR, WK_CM_EQ_LEN);
    }else{
        TRACE("Eq Default\n");
        sys_cb.eq_mode = 0;
        wk_flash_write(&sys_cb.eq_mode, WK_CM_EQ_ADDR, WK_CM_EQ_LEN);
        music_set_eq_by_num(sys_cb.eq_mode);
    }
}

AT(.app_text.param)
void wk_eq_save(u8 eq)
{
    sys_cb.eq_mode = eq;

    wk_flash_write(&sys_cb.eq_mode, WK_CM_EQ_ADDR, WK_CM_EQ_LEN);

    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_SENT_EQ, &sys_cb.eq_mode, 1);
        delay_5ms(30);
    }

    music_set_eq_by_num(sys_cb.eq_mode);
}

void wk_bt_name_init(void)
{
    u8 bt_name_len = 0;

    if(WK_CM_IS_EXIST()){
        wk_flash_read(&bt_name_len,WK_CM_BT_NAME_SIZE_ADDR,WK_CM_BT_NAME_SIZE_LEN);
        if((bt_name_len > 0) && (bt_name_len <= 32)){
            memset(xcfg_cb.bt_name,0,sizeof(xcfg_cb.bt_name));
            wk_flash_read(xcfg_cb.bt_name,WK_CM_BT_NAME_ADDR,bt_name_len);
        }
    }else{
        TRACE("Bt Name Default\n");
        memcpy(xcfg_cb.bt_name,wkapp_cb.bt_name,32);
        bt_name_len = strlen(xcfg_cb.bt_name);
        wk_flash_write(xcfg_cb.bt_name,WK_CM_BT_NAME_ADDR,bt_name_len);
        wk_flash_write(&bt_name_len,WK_CM_BT_NAME_SIZE_ADDR,WK_CM_BT_NAME_SIZE_LEN);
    }
}

void wk_bt_name_save(u8 *name, u8 len)
{
    memset(xcfg_cb.bt_name,0,sizeof(xcfg_cb.bt_name));
    memcpy(xcfg_cb.bt_name,name,len);

    wk_flash_write(xcfg_cb.bt_name,WK_CM_BT_NAME_ADDR,len);
    wk_flash_write(&len,WK_CM_BT_NAME_SIZE_ADDR,WK_CM_BT_NAME_SIZE_LEN);

    bt_set_stack_local_name(xcfg_cb.bt_name);
}

#if WK_CTL_TONE_EN
void wk_tone_init(void)
{
    u8 lang_id;

    if (xcfg_cb.lang_id == 2) {
        lang_id = 0;             //出厂默认英文
    } else if (xcfg_cb.lang_id == 3) {
        lang_id = 1;             //出厂默认中文
    } else {
        lang_id = xcfg_cb.lang_id;
    }

    if(sys_cb.lang_id != lang_id){
        sys_cb.lang_id = lang_id;
        param_lang_id_write();
        param_sync();
        bt_tws_sync_setting();
        wkapp_cb.lang_id = sys_cb.lang_id;
    }
}
#endif

//
void wk_app_init(void)
{
    u8 version[] = SW_VERSION;
    memset(&wkapp_cb, 0, sizeof(wkapp_cb));

    memset(be_frame_mem, 0 ,BLE_FRMAE_MEM_SIZE);
    mem_init((uint32_t)&be_frame_mem[0], (uint32_t)&be_frame_mem[BLE_FRMAE_MEM_SIZE-1]);

    wk_ota_init();

    if(xcfg_cb.bt_tws_lr_mode > 8) {
        tws_lr_xcfg_sel();
    }

    memcpy(wkapp_cb.bt_name,bt_get_local_name(),32);
//VER
    wkapp_cb.version[0] =  0x00;
    wkapp_cb.version[1] =  version[1] - '0';
    wkapp_cb.version[2] =  version[3] - '0';
    wkapp_cb.version[3] =  version[5] - '0';

#if WK_CTL_TONE_EN
    wkapp_cb.lang_id = sys_cb.lang_id;
#endif

#if WK_CTL_ANC_EN
    wkapp_cb.anc_mode = sys_cb.anc_user_mode;
#endif

    wk_param_flag_load();

//SN
    wk_sn_init();

//KEY
    wk_key_init();

//POWEROFF TIME
    wk_poweroff_time_init();

//EAR DET
#if WK_CTL_IN_EAR_EN
    wk_ear_det_init();
#endif

    wk_bt_name_init();

//EQ
    wk_eq_init();

    wk_param_flag_save();
}

void wk_device_reset_2_factory(void)
{
    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_RESET_FACTORY, NULL, 0);
    }

    wkapp_cb.param_flag = 0xFFFFFFFF;
    wk_flash_write(&wkapp_cb.param_flag,WK_CM_FLAG_ADDR,WK_CM_FLAG_LEN);
    param_sync();

    wk_key_init();

    wk_poweroff_time_init();

#if WK_CTL_IN_EAR_EN
    wk_ear_det_init();
#endif

    wk_bt_name_init();
    bt_set_stack_local_name(xcfg_cb.bt_name);

    wk_eq_init();

#if WK_CTL_TONE_EN
    wk_tone_init();
#endif

    wk_param_flag_save();
}


AT(.app_text.wkapp)
static void wk_app_process_1s(void)
{
    wk_param_sync();
    wk_app_find_ear_process();

    if(!wk_ota_is_start()){
        wk_app_vbat_process();
        wk_app_state_sync();
    }

    if(wkapp_cb.ble_param_update){
        wkapp_cb.ble_param_update--;
        if(wkapp_cb.ble_param_update == 0x00){
            ble_update_conn_param(80, 0, 200);
        }
    }

    wkapp_cb.sec_count++;

    if(wkapp_cb.sec_count >= 240){
        wkapp_cb.sec_count = 0;
    }
}

//-------------------------------------------------------------------
//
AT(.com_text.wkapp)
void wk_app_process(void)
{
    static u32 ticks = 0;

    if(frame_ble_num_get()){
        frame_ble_proce();
    }

    if(wk_tws_msg_is_active()){
        wk_tws_msg_process();
    }

    wk_ota_process();

    if(tick_check_expire(ticks, 1000)){
        ticks = tick_get();
        wk_app_process_1s();
    }
}

//-------------------------------------------------------
//
AT(.app_text.ble)
void wk_app_find_ear(u8 cmd)
{
    TRACE("find_era:%d\n", cmd);
    wkapp_cb.find_ear = cmd;

    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_SENT_FIND_EAR, &cmd, 1);
    }
}

AT(.app_text.ble)
void wk_app_find_ear_process(void)
{
    if(wkapp_cb.find_ear){
        reset_sleep_delay();
        bsp_piano_warning_play(WARNING_TONE, 0);
    }
}

//-------------------------------------------------------
//
AT(.app_text.ble)
void wk_app_vbat_process(void)
{
#if VUSB_SMART_VBAT_HOUSE_EN
    wkapp_cb.vbat_box = sys_cb.loc_house_bat & 0x7F;
#else
    wkapp_cb.vbat_box = 0;
#endif

    if((wkapp_cb.sec_count % VBAT_UPDATE_TIME) == 0){
        wkapp_cb.vbat_local = hfp_get_bat_level_ex();
        if(bt_tws_is_connected()){
            wk_tws_vbat_exchange();
        }else{
            wk_app_vbat_notify();
        }
    }
}

//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_auth_req(u8 *buff, u8 len)
{
    TRACE("%s\n", __func__);

    u8 payload_len = buff[5];

    if((payload_len != 64) || (len != 70)){
        return;
    }

    wk_app_auth_op(buff+6, buff+6);

    buff[4] = WK_CMD_AUTH_RSP;
    ble_wk_app_send_packet(buff, 70);
}

//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_sn_req(u8 *buff, u8 len)
{
    TRACE("%s\n", __func__);

    u8 sn_ack[30];

    memcpy(sn_ack, app_cmd_id, 4);
    sn_ack[4] = WK_CMD_SN_RSP;
    sn_ack[5] = 24;

    memcpy(sn_ack+6, wkapp_cb.sn_l, 12);
    memcpy(sn_ack+6+12, wkapp_cb.sn_r, 12);
    ble_wk_app_send_packet(sn_ack, 30);
}

//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_music_ctrl_req(u8 *buff, u8 len)
{
    u8 cmd_ack[7];
    u8 cmd = buff[4];

    TRACE("%s\n", __func__);

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01;
    cmd_ack[6] = cmd;

    if((f_bt.disp_status == BT_STA_CONNECTED) || (f_bt.disp_status == BT_STA_PLAYING)){
        switch(cmd){
            case WK_CMD_PLAY:
                bt_music_play();
                break;

            case WK_CMD_PAUSE:
                bt_music_pause();
                break;

            case WK_CMD_NEXT:
                bt_music_next();
                break;

            case WK_CMD_PRVE:
                bt_music_prev();
                break;

            case WK_CMD_VOL:
                sys_cb.vol = VOL_MAX*buff[6]/100;
                bsp_set_volume(sys_cb.vol);
                bsp_bt_vol_change();
                break;
        }
    }else{
        cmd_ack[4] = WK_CMD_ACK_FAIL;
    }

    ble_wk_app_send_packet(cmd_ack, 7);
}


//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_key_set_req(u8 *buff, u8 len)
{
    u8 cmd_ack[7];
    u8 cmd = buff[4];
    u8 key_val = buff[6];

    TRACE("%s:%d %x %x\n", __func__, func_bt_tws_get_channel(), cmd, key_val);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;

    if((func_bt_tws_get_channel() == TWS_CHANNEL_LEFT) && (cmd <= WK_CMD_SET_L_LONG)){
        switch(cmd){
            case WK_CMD_SET_L_SHORT:
                wkapp_cb.key_local.key_short = key_val;
                xcfg_cb.user_def_ks_sel = wkapp_cb.key_local.key_short;
                break;

            case WK_CMD_SET_L_DOUBLE:
                wkapp_cb.key_local.key_double = key_val;
                xcfg_cb.user_def_kd_sel = wkapp_cb.key_local.key_double;
                break;

            case WK_CMD_SET_L_THREE:
                wkapp_cb.key_local.key_three = key_val;
                xcfg_cb.user_def_kt_sel = wkapp_cb.key_local.key_three;
                break;

            case WK_CMD_SET_L_FOUR:
                wkapp_cb.key_local.key_four = key_val;
                xcfg_cb.user_def_kfour_sel = wkapp_cb.key_local.key_four;
                break;

            case WK_CMD_SET_L_FIVE:
                wkapp_cb.key_local.key_five = key_val;
                xcfg_cb.user_def_kfive_sel = wkapp_cb.key_local.key_five;
                break;

            case WK_CMD_SET_L_LONG:
                wkapp_cb.key_local.key_long = key_val;
                xcfg_cb.user_def_kl_sel = wkapp_cb.key_local.key_long;
                break;
        }
        wk_key_info_save();
    }else if((func_bt_tws_get_channel() == TWS_CHANNEL_RIGHT) && (cmd >= WK_CMD_SET_R_SHORT)){
        switch(cmd){
            case WK_CMD_SET_R_SHORT:
                wkapp_cb.key_local.key_short = key_val;
                xcfg_cb.user_def_ks_sel = wkapp_cb.key_local.key_short;
                break;

            case WK_CMD_SET_R_DOUBLE:
                wkapp_cb.key_local.key_double = key_val;
                xcfg_cb.user_def_kd_sel = wkapp_cb.key_local.key_double;
                break;

            case WK_CMD_SET_R_THREE:
                wkapp_cb.key_local.key_three = key_val;
                xcfg_cb.user_def_kt_sel = wkapp_cb.key_local.key_three;
                break;

            case WK_CMD_SET_R_FOUR:
                wkapp_cb.key_local.key_four = key_val;
                xcfg_cb.user_def_kfour_sel = wkapp_cb.key_local.key_four;
                break;

            case WK_CMD_SET_R_FIVE:
                wkapp_cb.key_local.key_five = key_val;
                xcfg_cb.user_def_kfive_sel = wkapp_cb.key_local.key_five;
                break;

            case WK_CMD_SET_R_LONG:
                wkapp_cb.key_local.key_long = key_val;
                xcfg_cb.user_def_kl_sel = wkapp_cb.key_local.key_long;
                break;
        }
        wk_key_info_save();
    }else{
        u8 *p_key = &wkapp_cb.key_remote.key_short;

        if(cmd >= WK_CMD_SET_R_SHORT){
            p_key[cmd - WK_CMD_SET_R_SHORT] = key_val;
        }else{
            p_key[cmd - WK_CMD_SET_L_SHORT] = key_val;
        }
        if(wk_tws_set_remote_key(cmd, key_val) == false){
            cmd_ack[4] = WK_CMD_ACK_FAIL;
        }
    }

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[5] = 0x01;
    cmd_ack[6] = cmd;

    ble_wk_app_send_packet(cmd_ack, 7);
}



//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_dev_info_set_req(u8 *buff, u8 len)
{
    u8 cmd_ack[7];
    u8 cmd = buff[4];
    u8 payload_len = buff[5];
    u8 *payload = &buff[6];
    TRACE("%s\n", __func__);

    switch(cmd){
        case WK_CMD_SET_POWEROFF_TIME:
            TRACE("-->WK_CMD_SET_POWEROFF_TIME\n");
            wk_poweroff_time_save(payload[0]);
            break;

        case WK_CMD_SET_FACTORY:
            TRACE("-->WK_CMD_SET_FACTORY\n");
            wk_device_reset_2_factory();
            break;

        case WK_CMD_SET_WORK_MODE:
            TRACE("-->WK_CMD_SET_WORK_MODE\n");
            wkapp_cb.work_mode = payload[0];
            if (wkapp_cb.work_mode == 0x00){
                bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
            } else {
                bsp_tws_res_music_play(TWS_RES_GAME_MODE);
            }
            break;

#if WK_CTL_IN_EAR_EN
        case WK_CMD_SET_EAR_DET:
            TRACE("-->WK_CMD_SET_EAR_DET\n");
            wk_ear_det_save(payload[0]);
            break;
#endif

        case WK_CMD_SET_EQ_MODE:
            TRACE("-->WK_CMD_SET_EQ_MODE\n");
            wk_eq_save(payload[0]);
            break;

#if WK_CTL_TONE_EN
        case WK_CMD_SET_LANGUAGE:
            TRACE("-->WK_CMD_SET_LANGUAGE\n");
            wkapp_cb.lang_id = payload[0];
            if(sys_cb.lang_id != payload[0]){
                func_bt_switch_voice_lang();
            }
            break;
#endif

        case WK_CMD_SET_LED:
            TRACE("-->WK_CMD_SET_LED\n");
            wkapp_cb.led_en = payload[0];
            break;

        case WK_CMD_SET_FIND_EAR:
            TRACE("-->WK_CMD_SET_FIND_EAR\n");
            wk_app_find_ear(payload[0]);
            break;

        case WK_CMD_SET_ANC_MODE:
            TRACE("-->WK_CMD_SET_ANC_MODE\n");
#if WK_CTL_ANC_EN
            sys_cb.anc_user_mode = payload[0];
            bsp_tws_res_music_play(TWS_RES_NR_DISABLE + sys_cb.anc_user_mode);
#endif
            break;

        case WK_CMD_SET_BT_NAME:
            TRACE("-->WK_CMD_SET_BT_NAME\n");
            if(payload_len > 31){
                payload_len = 31;
            }
            wk_bt_name_save(payload,payload_len);
            wk_tws_bt_name_sync();
            break;
    }

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01;
    cmd_ack[6] = cmd;

    ble_wk_app_send_packet(cmd_ack, 7);
}

void wk_app_vbat_notify(void)
{
    u8 cmd_ack[10];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x04;
    cmd_ack[6] = WK_CMD_GET_VBAT;
    if(func_bt_tws_get_channel() == TWS_CHANNEL_LEFT){
        cmd_ack[7] = wkapp_cb.vbat_local;
        cmd_ack[8] = wkapp_cb.vbat_remote;
    }else{
        cmd_ack[7] = wkapp_cb.vbat_remote;
        cmd_ack[8] = wkapp_cb.vbat_local;
    }
    cmd_ack[9] = wkapp_cb.vbat_box;

    ble_wk_app_send_packet(cmd_ack, 10);
}

void wk_app_device_version_notify(void)
{
    u8 cmd_ack[11];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x05;
    cmd_ack[6] = WK_CMD_GET_VERSION;
    memcpy(&cmd_ack[7], wkapp_cb.version, 4);

    ble_wk_app_send_packet(cmd_ack, 11);
}

void wk_app_bt_name_notify(void)
{
    u8 cmd_ack[40];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + strlen(xcfg_cb.bt_name);
    cmd_ack[6] = WK_CMD_GET_EDR_NAME;
    memcpy(&cmd_ack[7], xcfg_cb.bt_name, strlen(xcfg_cb.bt_name));

    ble_wk_app_send_packet(cmd_ack, strlen(xcfg_cb.bt_name)+7);
}

void wk_app_key_left_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x06;
    cmd_ack[6] = WK_CMD_GET_L_KEY;
    if(func_bt_tws_get_channel() == TWS_CHANNEL_LEFT){
        memcpy(&cmd_ack[7], &wkapp_cb.key_local.key_short, 6);
    }else{
        memcpy(&cmd_ack[7], &wkapp_cb.key_remote.key_short, 6);
        if(!bt_tws_is_connected()){
            cmd_ack[4] = WK_CMD_ACK_FAIL;
            cmd_ack[5] = 0x01;
        }
    }

    ble_wk_app_send_packet(cmd_ack, 6 + cmd_ack[5]);
}

void wk_app_key_right_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x06;
    cmd_ack[6] = WK_CMD_GET_R_KEY;
    if(func_bt_tws_get_channel() == TWS_CHANNEL_RIGHT){
        memcpy(&cmd_ack[7], &wkapp_cb.key_local.key_short, 6);
    }else{
        memcpy(&cmd_ack[7], &wkapp_cb.key_remote.key_short, 6);
        if(!bt_tws_is_connected()){
            cmd_ack[4] = WK_CMD_ACK_FAIL;
            cmd_ack[5] = 0x01;
        }
    }

    ble_wk_app_send_packet(cmd_ack, 6 + cmd_ack[5]);
}

void wk_app_eq_mode_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_EQ;
    cmd_ack[7] = sys_cb.eq_mode;

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_device_vol_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_VOL;
    cmd_ack[7] = (u16)sys_cb.vol * 100 / VOL_MAX;

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_play_sta_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_PLAY_STA;
    cmd_ack[7] = bt_is_playing();

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_latency_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_WORK_MODE;
    cmd_ack[7] = bt_is_low_latency();

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_in_ear_sta_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_EAR_DET;
#if WK_CTL_IN_EAR_EN
    cmd_ack[7] = sys_cb.in_ear_en;
#else
    cmd_ack[7] = 0;
#endif
    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_tone_mode_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_LANGUAGE;
    cmd_ack[7] = sys_cb.lang_id;

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_anc_mode_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_ANC;
#if WK_CTL_ANC_EN
    cmd_ack[7] = sys_cb.anc_user_mode;
#else
    cmd_ack[7] = 0;
#endif
    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_tws_sta_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x02;
    cmd_ack[6] = WK_CMD_GET_TWS;
    cmd_ack[7] = bt_tws_is_connected();
    cmd_ack[8] = func_bt_tws_get_channel();

    ble_wk_app_send_packet(cmd_ack, 9);
}

void wk_app_mtu_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_MTU;
    cmd_ack[7] = wk_app_mtu_get();

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_led_sta_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_LED;
    cmd_ack[7] = wkapp_cb.led_en;

    ble_wk_app_send_packet(cmd_ack, 8);
}

void wk_app_poweroff_time_notify(void)
{
    u8 cmd_ack[15];

    memcpy(cmd_ack, app_cmd_id, 4);
    cmd_ack[4] = WK_CMD_ACK_SUCCEED;
    cmd_ack[5] = 0x01 + 0x01;
    cmd_ack[6] = WK_CMD_GET_OFF_TIME;
    cmd_ack[7] = wkapp_cb.poweroff_time;

    ble_wk_app_send_packet(cmd_ack, 8);
}

//---------------------------------------------------------------------
AT(.app_text.ble)
static void wk_app_dev_info_get_req(u8 *buff, u8 len)
{
    u8 cmd = buff[4];
    TRACE("%s\n", __func__);

    switch(cmd){
        case WK_CMD_GET_VBAT:
            TRACE("-->WK_CMD_GET_VBAT\n");
            wk_app_vbat_notify();
            break;

        case WK_CMD_GET_VERSION:
            TRACE("-->WK_CMD_GET_VERSION\n");
            wk_app_device_version_notify();
            break;

        case WK_CMD_GET_EDR_NAME:
            TRACE("-->WK_CMD_GET_EDR_NAME\n");
            wk_app_bt_name_notify();
            break;

        case WK_CMD_GET_L_KEY:
            TRACE("-->WK_CMD_GET_L_KEY\n");
            wk_app_key_left_notify();
            break;

        case WK_CMD_GET_R_KEY:
            TRACE("-->WK_CMD_GET_R_KEY\n");
            wk_app_key_right_notify();
            break;

        case WK_CMD_GET_EQ:
            TRACE("-->WK_CMD_GET_EQ\n");
            wk_app_eq_mode_notify();
            break;

        case WK_CMD_GET_VOL:
            TRACE("-->WK_CMD_GET_VOL\n");
            wk_app_device_vol_notify();
            break;

        case WK_CMD_GET_PLAY_STA:
            TRACE("-->WK_CMD_GET_PLAY_STA\n");
            wk_app_play_sta_notify();
            break;

        case WK_CMD_GET_WORK_MODE:
            TRACE("-->WK_CMD_GET_WORK_MODE\n");
            wk_app_latency_notify();
            break;

        case WK_CMD_GET_EAR_DET:
            TRACE("-->WK_CMD_GET_EAR_DET\n");
            wk_app_in_ear_sta_notify();
            break;

        case WK_CMD_GET_LANGUAGE:
            TRACE("-->WK_CMD_GET_LANGUAGE\n");
            wk_app_tone_mode_notify();
            break;

        case WK_CMD_GET_ANC:
            TRACE("-->WK_CMD_GET_ANC\n");
            wk_app_anc_mode_notify();
            break;

        case WK_CMD_GET_TWS:
            TRACE("-->WK_CMD_GET_TWS\n");
            wk_app_tws_sta_notify();
            break;

        case WK_CMD_GET_MTU:
            TRACE("-->WK_CMD_GET_MTU\n");
            wk_app_mtu_notify();
            break;

        case WK_CMD_GET_LED:
            TRACE("-->WK_CMD_GET_LED\n");
            wk_app_led_sta_notify();
            break;

        case WK_CMD_GET_OFF_TIME:
            TRACE("-->WK_CMD_GET_OFF_TIME\n");
            wk_app_poweroff_time_notify();
            break;

        default:
            break;
    }
}

//-------------------------------------------------------------------
//
AT(.app_text.ble)
void wk_app_cmd_write(u8 *buff, u8 len)
{
    TRACE("CMD->");
    TRACE_R(buff, len);

    if(memcmp(app_cmd_id, buff, 4)){
        return;
    }

    u8 cmd = buff[4];

    switch(cmd){
        case WK_CMD_AUTH_REQ:
            wk_app_auth_req(buff, len);
            break;

        case WK_CMD_SN_REQ:
            wk_app_sn_req(buff, len);
            break;

        case WK_CMD_PLAY:
        case WK_CMD_PAUSE:
        case WK_CMD_NEXT:
        case WK_CMD_PRVE:
        case WK_CMD_VOL:
            wk_app_music_ctrl_req(buff, len);
            break;

        case WK_CMD_SET_L_SHORT:
        case WK_CMD_SET_L_DOUBLE:
        case WK_CMD_SET_L_THREE:
        case WK_CMD_SET_L_FOUR:
        case WK_CMD_SET_L_FIVE:
        case WK_CMD_SET_L_LONG:
        case WK_CMD_SET_R_SHORT:
        case WK_CMD_SET_R_DOUBLE:
        case WK_CMD_SET_R_THREE:
        case WK_CMD_SET_R_FOUR:
        case WK_CMD_SET_R_FIVE:
        case WK_CMD_SET_R_LONG:
            wk_app_key_set_req(buff, len);
            break;


        case WK_CMD_SET_POWEROFF_TIME:
        case WK_CMD_SET_FACTORY:
        case WK_CMD_SET_WORK_MODE:
        case WK_CMD_SET_EAR_DET:
        case WK_CMD_SET_EQ_MODE:
        case WK_CMD_SET_LANGUAGE:
        case WK_CMD_SET_LED:
        case WK_CMD_SET_FIND_EAR:
        case WK_CMD_SET_ANC_MODE:
        case WK_CMD_SET_BT_NAME:
            wk_app_dev_info_set_req(buff, len);
            break;

        case WK_CMD_GET_VBAT:
        case WK_CMD_GET_VERSION:
        case WK_CMD_GET_EDR_NAME:
        case WK_CMD_GET_L_KEY:
        case WK_CMD_GET_R_KEY:
        case WK_CMD_GET_EQ:
        case WK_CMD_GET_VOL:
        case WK_CMD_GET_PLAY_STA:
        case WK_CMD_GET_WORK_MODE:
        case WK_CMD_GET_EAR_DET:
        case WK_CMD_GET_LANGUAGE:
        case WK_CMD_GET_ANC:
        case WK_CMD_GET_TWS:
        case WK_CMD_GET_MTU:
        case WK_CMD_GET_LED:
        case WK_CMD_GET_OFF_TIME:
            wk_app_dev_info_get_req(buff, len);
            break;
    }
}

//-------------------------------------------------------
//
AT(.app_text.ble)
void wk_app_state_sync(void)
{
    if(wkapp_cb.vol != sys_cb.vol){
        wkapp_cb.vol = sys_cb.vol;
        wk_app_device_vol_notify();
    }

    if(wkapp_cb.play_sta != bt_is_playing()){
        wkapp_cb.play_sta = bt_is_playing();
        wk_app_play_sta_notify();
    }

#if WK_CTL_TONE_EN
    if(wkapp_cb.lang_id != sys_cb.lang_id){
        wkapp_cb.lang_id = sys_cb.lang_id;
        wk_app_tone_mode_notify();
    }
#endif

#if WK_CTL_ANC_EN
    if(wkapp_cb.anc_mode != sys_cb.anc_user_mode){
        wkapp_cb.anc_mode = sys_cb.anc_user_mode;
        wk_app_anc_mode_notify();
    }
#endif
}

//-------------------------------------------------------
//
AT(.app_text.ble)
void wk_app_bt_evt_notice(u8 evt, void *params)
{
    //u8 *packet = params;

    switch(evt){
        case BT_NOTICE_TWS_CONNECTED:
            if(IS_APP_MASTER()){
                wk_tws_all_info_sync_req();
            }
            wk_app_tws_sta_notify();
            break;

        case BT_NOTICE_TWS_DISCONNECT:
            wkapp_cb.vbat_remote = 0;
            if(ble_is_connect()){
                wk_app_tws_sta_notify();
                wk_app_vbat_notify();
            }
            break;

        case BT_NOTICE_CONNECTED:
#if !LE_ADV_POWERON_EN
            ble_adv_en();
#endif
            break;

        case BT_NOTICE_DISCONNECT:
#if !LE_ADV_POWERON_EN
            ble_adv_dis();
#endif
            break;
    }
}

void wk_ble_connect_callback(void)
{
    wkapp_cb.ble_param_update = 2;
}

void wk_ble_disconnect_callback(void)
{
    wk_ota_disconnect_callback();
}

uint16_t wk_ble_role_switch_get_data(uint8_t *data_ptr)
{
    return 0;
}

uint16_t wk_ble_role_switch_set_data(uint8_t *data_ptr, uint16_t len)
{
    return 0;
}

AT(.app_text.wk_app)
void wk_app_enter_sleep(void)
{
    wkapp_cb.active_flag = 0;
}

AT(.com_text.wk_app)
bool wk_app_need_wakeup(void)
{
    return wkapp_cb.active_flag ;
}

#endif

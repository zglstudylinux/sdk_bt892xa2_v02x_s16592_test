#include "include.h"
#include "wk_app.h"
#include "wk_ota.h"
#include "wk_tws.h"

#if LE_WK_APP_EN

static u8 tws_recv_msg_data(u8 cmd, u8 *ptr, u8 len);

//--------------------------------------------------------------------------
#define TWS_MSG_MEM_MASK            5
#define TWS_MSG_LEN                 36

static u8 wk_tws_msg_mem_sent[TWS_MSG_LEN] AT(.ble_buf.app);
static u8 wk_tws_msg_mem_rece[TWS_MSG_MEM_MASK][TWS_MSG_LEN] AT(.ble_buf.app);
static u16 wk_tws_msg_len;
static volatile u32 wk_tws_msg_r = 0, wk_tws_msg_w = 0;

u8 tws_sync_ota[512+16] AT(.fot_data.buf);
u16 tws_sync_data_len;

uint16_t tws_set_fot_data(uint8_t *data_ptr, uint16_t size)
{
    u8 cmd = data_ptr[0];

    reset_sleep_delay();
    wkapp_cb.active_flag = 1;

    if(cmd == TWS_INFO_OTA){
        wk_ota_tws_data_proc(&data_ptr[1], size - 1);
    }else{
        if((wk_tws_msg_w-wk_tws_msg_r) < TWS_MSG_MEM_MASK){
            wk_tws_msg_mem_rece[wk_tws_msg_w%TWS_MSG_MEM_MASK][0] = size - 1;
            memcpy(&wk_tws_msg_mem_rece[wk_tws_msg_w%TWS_MSG_MEM_MASK][1], data_ptr, size);
            wk_tws_msg_w++;
        }
    }
    return 0;
}


uint16_t tws_get_fot_data(uint8_t *buf)
{
    if(wk_ota_is_start()){
        memcpy(buf, tws_sync_ota, tws_sync_data_len);
        return tws_sync_data_len;
    }else{
        memcpy(buf, wk_tws_msg_mem_sent, wk_tws_msg_len);
        return wk_tws_msg_len;
    }
}

void tws_data_sync_do(void)
{
    if(bt_tws_is_connected()){
        bt_tws_sync_fot_data();
    }
}

AT(.com_text.detect)
bool wk_tws_msg_is_active(void)
{
    return (wk_tws_msg_w != wk_tws_msg_r);
}

AT(.com_text.detect)
void wk_tws_msg_process(void)
{
    tws_recv_msg_data(wk_tws_msg_mem_rece[wk_tws_msg_r%TWS_MSG_MEM_MASK][1], &wk_tws_msg_mem_rece[wk_tws_msg_r%TWS_MSG_MEM_MASK][2],
                       wk_tws_msg_mem_rece[wk_tws_msg_r%TWS_MSG_MEM_MASK][0]);
    wk_tws_msg_r++;
}


AT(.app_text.tws)
void tws_send_msg_data(u8 cmd, uint8_t *data, uint16_t len)
{
    static u32 ticks = 0;

    if(!bt_tws_is_connected()){
        return;
    }
    if(len > (TWS_MSG_LEN - 1)){
        len = (TWS_MSG_LEN - 1);
    }

    printf("--> tws sent CMD %d ", cmd);
    print_r(data, len);

    while(!tick_check_expire(ticks, 50)){
        delay_5ms(1);
    }
    ticks = tick_get();

    wk_tws_msg_mem_sent[0] = cmd;

    if(len){
        memcpy(wk_tws_msg_mem_sent+1, data, len);
    }

    wk_tws_msg_len = 1 + len;

    tws_data_sync_do();
}

AT(.app_text.tws)
void wk_tws_all_info_sync_req(void)
{
    u8 data[32];
    u8 offset = 0;

    if(sys_cb.tws_left_channel){
        memcpy(&data[offset], wkapp_cb.sn_l, SN_LEN);
    }else{
        memcpy(&data[offset], wkapp_cb.sn_r, SN_LEN);
    }
    offset += SN_LEN;

    data[offset++] = wkapp_cb.poweroff_time;

#if WK_CTL_IN_EAR_EN
    data[offset++] = sys_cb.in_ear_en;
#endif

    data[offset++] = hfp_get_bat_level_ex();

    memcpy(&data[offset], &wkapp_cb.key_local,WK_CM_KEY_LEN);
    offset += WK_CM_KEY_LEN;

    tws_send_msg_data(TWS_REQ_ALL_INFO_SYNC, data, offset);
}

AT(.app_text.tws)
void wk_tws_all_info_sync_rsp(void)
{
    u8 rsp_data[32];
    u8 offset = 0;

    rsp_data[offset++] = hfp_get_bat_level_ex();

    memcpy(&rsp_data[offset], &wkapp_cb.key_local,WK_CM_KEY_LEN);
    offset += WK_CM_KEY_LEN;

    tws_send_msg_data(TWS_RSP_ALL_INFO_SYNC, rsp_data, offset);
}

void wk_tws_info_sync_req_proc(u8 *data, u8 len)
{
    u8 offset = 0;

    wk_tws_sn_sync_rsp(&data[offset], SN_LEN);
    offset += SN_LEN;

    wk_poweroff_time_save(data[offset]);
    offset++;

#if WK_CTL_IN_EAR_EN
    wk_ear_det_save(data[offset]);
    offset++;
#endif

    wkapp_cb.vbat_remote = data[offset++];

    memcpy(&wkapp_cb.key_remote,&data[offset],WK_CM_KEY_LEN);
    offset += WK_CM_KEY_LEN;

    wk_tws_all_info_sync_rsp();
}

void wk_tws_info_sync_rsp_proc(u8 *data, u8 len)
{
    u8 offset = 0;

    wkapp_cb.vbat_remote = data[offset++];

    memcpy(&wkapp_cb.key_remote,&data[offset],WK_CM_KEY_LEN);
    offset += WK_CM_KEY_LEN;

    wk_app_vbat_notify();
}

AT(.app_text.tws)
void wk_tws_sn_sync_req(void)
{
    if(sys_cb.tws_left_channel){
        tws_send_msg_data(TWS_REQ_SENT_SN_INFO, wkapp_cb.sn_l, SN_LEN);
    }else{
        tws_send_msg_data(TWS_REQ_SENT_SN_INFO, wkapp_cb.sn_r, SN_LEN);
    }
}

AT(.app_text.tws)
void wk_tws_sn_sync_rsp(u8 *sn, u8 len)
{
    u8 *sn_remote;
    u16 crc_sn;

    if(sys_cb.tws_left_channel){
        sn_remote = wkapp_cb.sn_r;
    }else{
        sn_remote = wkapp_cb.sn_l;
    }

    if(memcmp(sn_remote, sn, SN_LEN)){
        printf("TWS SN Save\n");
        memcpy(sn_remote, sn, SN_LEN);
        crc_sn = calc_crc(wkapp_cb.sn_l, WK_CM_SN_LEN, 0xFFFF);
        wk_flash_write(wkapp_cb.sn_l, WK_CM_SN_ADDR, WK_CM_SN_LEN);
        wk_flash_write((u8 *)&crc_sn, WK_CM_SN_CRC_ADDR, WK_CM_SN_CRC_LEN);
        param_sync();
    }
}

AT(.app_text.param)
void wk_tws_poweroff_time_sync(void)
{
    if(IS_APP_MASTER()){
        if((xcfg_cb.sys_off_time != -1) &&  xcfg_cb.sys_off_time){
            tws_send_msg_data(TWS_REQ_SENT_OFF_TIMES, &wkapp_cb.poweroff_time, 1);
        }
    }
}

#if WK_CTL_IN_EAR_EN
AT(.app_text.param)
void wk_tws_ear_det_sync(void)
{
    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_SENT_EAR_DET, &sys_cb.in_ear_en, 1);
    }
}
#endif

void wk_tws_bt_name_sync(void)
{
    u16 name_len = strlen(xcfg_cb.bt_name);

    if(IS_APP_MASTER()){
        tws_send_msg_data(TWS_REQ_SENT_BT_NAME, (u8*)xcfg_cb.bt_name, name_len);
    }
}


//-------------------------------------------------------
//
AT(.app_text.ble)
bool wk_tws_get_remote_key(u8 cmd)
{
    if(!bt_tws_is_connected()){
        return false;
    }
    tws_send_msg_data(TWS_REQ_KEY_INFO_GET, &cmd, 1);
    return true;
}

AT(.app_text.ble)
static void wk_tws_key_info_req_proc(void)
{
    tws_send_msg_data(TWS_RSP_KEY_INFO_GET, &wkapp_cb.key_local.key_short, 6);
}

AT(.app_text.ble)
static void wk_tws_key_info_rsp_proc(u8 *buff, u8 len)
{
    memcpy(&wkapp_cb.key_remote.key_short, buff, len);
}

//-------------------------------------------------------
//
AT(.app_text.ble)
bool wk_tws_set_remote_key(u8 cmd, u8 key_vlaue)
{
    u8 key_info[2];

    if(!bt_tws_is_connected()){
        return false;
    }
    key_info[0] = cmd;
    key_info[1] = key_vlaue;

    tws_send_msg_data(TWS_REQ_KEY_INFO_SET, key_info, 2);

    return true;
}

AT(.app_text.ble)
static void wk_app_key_info_set(u8 cmd, u8 key_vlaue)
{
    switch(cmd){
        case WK_CMD_SET_L_SHORT:
        case WK_CMD_SET_R_SHORT:
            wkapp_cb.key_local.key_short = key_vlaue;
            xcfg_cb.user_def_ks_sel = wkapp_cb.key_local.key_short;
            break;
        case WK_CMD_SET_L_DOUBLE:
        case WK_CMD_SET_R_DOUBLE:
            wkapp_cb.key_local.key_double = key_vlaue;
            xcfg_cb.user_def_kd_sel = wkapp_cb.key_local.key_double;
            break;
        case WK_CMD_SET_L_THREE:
        case WK_CMD_SET_R_THREE:
            wkapp_cb.key_local.key_three = key_vlaue;
            xcfg_cb.user_def_kt_sel = wkapp_cb.key_local.key_three;
            break;
        case WK_CMD_SET_L_FOUR:
        case WK_CMD_SET_R_FOUR:
            wkapp_cb.key_local.key_four = key_vlaue;
            xcfg_cb.user_def_kfour_sel = wkapp_cb.key_local.key_four;
            break;
        case WK_CMD_SET_L_FIVE:
        case WK_CMD_SET_R_FIVE:
            wkapp_cb.key_local.key_five = key_vlaue;
            xcfg_cb.user_def_kfive_sel = wkapp_cb.key_local.key_five;
            break;
        case WK_CMD_SET_L_LONG:
        case WK_CMD_SET_R_LONG:
            wkapp_cb.key_local.key_long = key_vlaue;
            xcfg_cb.user_def_kl_sel = wkapp_cb.key_local.key_long;
            break;
    }

    wk_key_info_save();
}

AT(.app_text.ble)
void wk_tws_vbat_exchange(void)
{
    tws_send_msg_data(TWS_REQ_SENT_VBAT, &wkapp_cb.vbat_local, 1);
}

AT(.app_text.ble)
static void wk_tws_vbat_exchange_proc(u8 *buff, u8 len)
{
    wkapp_cb.vbat_remote = buff[0];

    printf("wkapp_cb.vbat_local:%d,wkapp_cb.vbat_remote:%d\n",wkapp_cb.vbat_local,wkapp_cb.vbat_remote);

    if(IS_APP_MASTER()){
        wk_app_vbat_notify();
    }
}


//-------------------------------------------------------
//tws -> tws
AT(.app_text.ble)
static u8 tws_recv_msg_data(u8 cmd, u8 *ptr, u8 len)
{
    printf("-->tws recv CMD %d ", cmd);
    print_r(ptr, len);

    switch(cmd){
        case TWS_REQ_ALL_INFO_SYNC:
            wk_tws_info_sync_req_proc(ptr,len);
            break;

        case TWS_RSP_ALL_INFO_SYNC:
            wk_tws_info_sync_rsp_proc(ptr,len);
            break;

        case TWS_REQ_SENT_SN_INFO:
            wk_tws_sn_sync_rsp(ptr, len);
            break;

        case TWS_REQ_KEY_INFO_GET:
            wk_tws_key_info_req_proc();
            break;

        case TWS_RSP_KEY_INFO_GET:
            wk_tws_key_info_rsp_proc(ptr, len);
            break;

        case TWS_REQ_KEY_INFO_SET:
            wk_app_key_info_set(ptr[0], ptr[1]);
            break;

        case TWS_REQ_SENT_OFF_TIMES:
            wk_poweroff_time_save(ptr[0]);
            break;

#if WK_CTL_IN_EAR_EN
        case TWS_REQ_SENT_EAR_DET:
            wk_ear_det_save(ptr[0]);
            break;
#endif

        case TWS_REQ_SENT_EQ:
            wk_eq_save(ptr[0]);
            break;

        case TWS_REQ_SENT_FIND_EAR:
            wk_app_find_ear(ptr[0]);
            break;

        case TWS_REQ_SENT_VBAT:
            wk_tws_vbat_exchange_proc(ptr, len);
            break;

        case TWS_REQ_RESET_FACTORY:
            wk_device_reset_2_factory();
            break;

        case TWS_REQ_SENT_BT_NAME:
            wk_bt_name_save(ptr,len);
            break;
    }

    return true;
}
#endif


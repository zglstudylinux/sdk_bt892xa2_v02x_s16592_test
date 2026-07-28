#include "include.h"
#include "qcy_app.h"
#include "qcy_tws.h"

#if LE_QCY_APP_EN && QCY_TWS_EN

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#define TRACE_R(...)            print_r(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACE_R(...)
#endif

static u8 tws_recv_msg_data(u8 cmd, u8 *ptr, u8 len);

//--------------------------------------------------------------------------
#define TWS_MSG_MEM_MASK            5
#define TWS_MSG_LEN                 36

static u8 qcy_tws_msg_mem_sent[TWS_MSG_LEN] AT(.ble_buf.app);
static u8 qcy_tws_msg_mem_rece[TWS_MSG_MEM_MASK][TWS_MSG_LEN] AT(.ble_buf.app);
static u16 qcy_tws_msg_len;
static volatile u32 qcy_tws_msg_r = 0, qcy_tws_msg_w = 0;

u8 tws_sync_ota[512+16] AT(.fot_data.buf);
u16 tws_sync_data_len;

uint16_t tws_set_fot_data(uint8_t *data_ptr, uint16_t size)
{
    printf("%s\n", __func__);
    reset_sleep_delay();
    qcy_app_cb.active_flag = 1;
    if ((qcy_tws_msg_w-qcy_tws_msg_r) < TWS_MSG_MEM_MASK) {
        qcy_tws_msg_mem_rece[qcy_tws_msg_w%TWS_MSG_MEM_MASK][0] = size - 1;
        memcpy(&qcy_tws_msg_mem_rece[qcy_tws_msg_w%TWS_MSG_MEM_MASK][1], data_ptr, size);
        qcy_tws_msg_w++;
    }
    return 0;
}

uint16_t tws_get_fot_data(uint8_t *buf)
{
    printf("%s\n", __func__);
    memcpy(buf, qcy_tws_msg_mem_sent, qcy_tws_msg_len);
    return qcy_tws_msg_len;
}

void tws_data_sync_do(void)
{
    if (bt_tws_is_connected()) {
        bt_tws_sync_fot_data();
    }
}

AT(.com_text.detect)
bool qcy_tws_msg_is_active(void)
{
    return (qcy_tws_msg_w != qcy_tws_msg_r);
}

AT(.app_text.qcyapp)
void qcy_tws_msg_process(void)
{
    tws_recv_msg_data(qcy_tws_msg_mem_rece[qcy_tws_msg_r%TWS_MSG_MEM_MASK][1], &qcy_tws_msg_mem_rece[qcy_tws_msg_r%TWS_MSG_MEM_MASK][2],
                       qcy_tws_msg_mem_rece[qcy_tws_msg_r%TWS_MSG_MEM_MASK][0]);
    qcy_tws_msg_r++;
}

AT(.app_text.tws)
void tws_send_msg_data(u8 cmd, uint8_t *data, uint16_t len)
{
    static u32 ticks = 0;

    if (!bt_tws_is_connected()) {
        return;
    }
    if (len > (TWS_MSG_LEN - 1)) {
        len = (TWS_MSG_LEN - 1);
    }

    printf("--> tws sent CMD %d ", cmd);
    print_r(data, len);

    while(!tick_check_expire(ticks, 50)){
        delay_5ms(1);
    }
    ticks = tick_get();

    qcy_tws_msg_mem_sent[0] = cmd;

    if (len) {
        memcpy(qcy_tws_msg_mem_sent+1, data, len);
    }

    qcy_tws_msg_len = 1 + len;

    tws_data_sync_do();
}

// 主耳发起请求信息同步
AT(.app_text.tws)
void qcy_tws_all_info_sync_req(void)
{
    u8 data[32];
    u8 offset = 0;

#if QCY_CTL_IN_EAR_EN
    data[offset++] = qcy_app_cb.in_ear_detect;
#endif

    data[offset++] = qcy_app_cb.game_mode;

    data[offset++] = bsp_get_bat_level();
    data[offset++] = 0;// 充电标志位

#if QCY_VOL_BALANCE
    data[offset++] = qcy_app_cb.vol_balance_val;
#endif

    memcpy(&data[offset], &qcy_app_cb.eq_info, QCY_CM_EQ_LEN);
    offset += QCY_CM_EQ_LEN;

    memcpy(&data[offset], &qcy_app_cb.version_local, QCY_SW_VER_LEN);
    offset += QCY_SW_VER_LEN;

    memcpy(&data[offset], &qcy_app_cb.key_local,QCY_CM_KEY_LEN);
    offset += QCY_CM_KEY_LEN;

    tws_send_msg_data(TWS_REQ_ALL_INFO_SYNC, data, offset);
}

// 副耳收到主耳同步消息回应
AT(.app_text.tws)
void qcy_tws_all_info_sync_rsp(void)
{
    u8 rsp_data[32];
    u8 offset = 0;

    rsp_data[offset++] = bsp_get_bat_level();
    rsp_data[offset++] = 0;// 充电标志位

#if QCY_VOL_BALANCE
    rsp_data[offset++] = qcy_app_cb.vol_balance_val;
#endif

    memcpy(&rsp_data[offset], &qcy_app_cb.version_local, QCY_SW_VER_LEN);
    offset += QCY_SW_VER_LEN;

    memcpy(&rsp_data[offset], &qcy_app_cb.key_local,QCY_CM_KEY_LEN);
    offset += QCY_CM_KEY_LEN;

    tws_send_msg_data(TWS_RSP_ALL_INFO_SYNC, rsp_data, offset);
}

// 收到对方的同步信息
void qcy_tws_info_sync_req_proc(u8 *data, u8 len)
{
    u8 offset = 0;

#if QCY_CTL_IN_EAR_EN
    qcy_ear_det_save(data[offset]);
    offset++;
#endif

    if (data[offset] != qcy_app_cb.game_mode) {
        qcy_app_cb.do_flag |= FLAG_GAME_MODE_SET;
//        sys_cb.gm_first_flag  = 1;      //首次设置游戏模式不响提示音
    }
    qcy_app_cb.game_mode = data[offset++];

    qcy_app_cb.vbat_remote = data[offset++];
    qcy_app_cb.vbat_ischarge_remote = data[offset++];

#if QCY_VOL_BALANCE
    qcy_app_cb.vol_balance_val = data[offset++];
#endif

    memcpy(&qcy_app_cb.eq_info, &data[offset], QCY_CM_EQ_LEN);
    offset += QCY_CM_EQ_LEN;
    qcy_app_cb.do_flag |= FLAG_EQ_SET;

    memcpy(&qcy_app_cb.version_remote, &data[offset], QCY_SW_VER_LEN);
    offset += QCY_SW_VER_LEN;

    memcpy(&qcy_app_cb.key_remote,&data[offset],QCY_CM_KEY_LEN);
    qcy_app_key_tbl_set();
    offset += QCY_CM_KEY_LEN;

    qcy_tws_all_info_sync_rsp();
}

// 主耳收到副耳同步消息处理
void qcy_tws_info_sync_rsp_proc(u8 *data, u8 len)
{
    u8 offset = 0;

    qcy_app_cb.vbat_remote = data[offset++];
    qcy_app_cb.vbat_ischarge_remote = data[offset++];

#if QCY_VOL_BALANCE
    qcy_app_cb.vol_balance_val = data[offset++];
#endif

    memcpy(&qcy_app_cb.version_remote, &data[offset], QCY_SW_VER_LEN);
    offset += QCY_SW_VER_LEN;

    memcpy(&qcy_app_cb.key_remote,&data[offset],QCY_CM_KEY_LEN);
    offset += QCY_CM_KEY_LEN;
    qcy_app_key_tbl_set();
    qcy_app_vbat_notify();
}

// AT(.app_text.param)
// void qcy_tws_poweroff_time_sync(void)
// {
//     if (IS_APP_MASTER()) {
//         if ((xcfg_cb.sys_off_time != -1) &&  xcfg_cb.sys_off_time) {
//             tws_send_msg_data(TWS_REQ_SENT_OFF_TIMES, &qcy_app_cb.poweroff_time, 1);
//         }
//     }
// }

#if QCY_CTL_IN_EAR_EN
AT(.app_text.param)
void qcy_tws_ear_det_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SENT_EAR_DET, &qcy_app_cb.in_ear_detect, 1);
    }
}
#endif

void qcy_tws_earphone_mode_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SENT_EARPHONE_MODE, &qcy_app_cb.earphone_mode, 1);
    }
}

void qcy_tws_anc_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SENT_ANC_INFO, (u8*)&qcy_app_cb.anc_info, 3);
    }
}

void qcy_tws_bt_name_sync(void)
{
    u16 name_len = strlen(xcfg_cb.bt_name);

    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SENT_BT_NAME, (u8*)xcfg_cb.bt_name, name_len);
    }
}

void qcy_tws_reset_default_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_RESET_DEFAULT, NULL, 0);
    }
}

void qcy_tws_delete_link_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_DELETE_LINK, NULL, 0);
    }
}

void qcy_tws_reset_factory_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_RESET_FACTORY, NULL, 0);
    }
}

void qcy_tws_sleep_mode_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SLEEP_MODE_SYNC, &qcy_app_cb.sleep_mode, 1);
    }
}

//-------------------------------------------------------
//
AT(.app_text.ble)
bool qcy_tws_get_remote_key(u8 cmd)
{
    if (!bt_tws_is_connected()) {
        return false;
    }
    tws_send_msg_data(TWS_REQ_KEY_INFO_GET, &cmd, 1);
    return true;
}

AT(.app_text.ble)
static void qcy_tws_key_info_req_proc(void)
{
    tws_send_msg_data(TWS_RSP_KEY_INFO_GET, &qcy_app_cb.key_local.key_short, 6);
}

AT(.app_text.ble)
static void qcy_tws_key_info_rsp_proc(u8 *buff, u8 len)
{
    memcpy(&qcy_app_cb.key_remote.key_short, buff, len);
}

#if QCY_VOL_BALANCE
AT(.app_text.ble)
void qcy_tws_vol_balance_sync(void)
{
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_VOL_BALANCE_SYNC, (u8*)&qcy_app_cb.vol_balance_val, 2);
    }
}
#endif

//-------------------------------------------------------
//
AT(.app_text.ble)
bool qcy_tws_set_remote_key(u8 cmd, u8 key_vlaue)
{
    u8 key_info[2];

    if (!bt_tws_is_connected()) {
        return false;
    }
    key_info[0] = cmd;
    key_info[1] = key_vlaue;

    tws_send_msg_data(TWS_REQ_KEY_INFO_SET, key_info, 2);

    return true;
}

AT(.app_text.ble)
static void qcy_app_key_info_set(u8 cmd, u8 key_vlaue)
{
    u8 temp_key_value = 0;
    temp_key_value = qcy_to_ab_keydef(key_vlaue);
    TRACE("key set:%02x - %02x\n", cmd, temp_key_value);
    switch (cmd) {
    case QCY_MUSIC_EVT_L_SINGLE:
    case QCY_MUSIC_EVT_R_SINGLE:
        qcy_app_cb.key_local.key_short = temp_key_value;
        xcfg_cb.user_def_ks_sel = qcy_app_cb.key_local.key_short;
        qcy_evt_fun_set(cmd, key_vlaue);
        break;
    case QCY_MUSIC_EVT_L_DOUBLE:
    case QCY_MUSIC_EVT_R_DOUBLE:
        qcy_app_cb.key_local.key_double = temp_key_value;
        xcfg_cb.user_def_kd_sel = qcy_app_cb.key_local.key_double;
        qcy_evt_fun_set(cmd, key_vlaue);
        break;
    case QCY_MUSIC_EVT_L_THREE:
    case QCY_MUSIC_EVT_R_THREE:
        qcy_app_cb.key_local.key_three = temp_key_value;
        xcfg_cb.user_def_kt_sel = qcy_app_cb.key_local.key_three;
        qcy_evt_fun_set(cmd, key_vlaue);
        break;
    case QCY_MUSIC_EVT_L_FOUR:
    case QCY_MUSIC_EVT_R_FOUR:
        qcy_app_cb.key_local.key_four = temp_key_value;
        xcfg_cb.user_def_kfour_sel = qcy_app_cb.key_local.key_four;
        qcy_evt_fun_set(cmd, key_vlaue);
        break;
    case QCY_MUSIC_EVT_L_LONG:
    case QCY_MUSIC_EVT_R_LONG:
        qcy_app_cb.key_local.key_long = temp_key_value;
        xcfg_cb.user_def_kl_sel = qcy_app_cb.key_local.key_long;
        qcy_evt_fun_set(cmd, key_vlaue);
        break;
    // case QCY_CALL_EVT_L_SINGLE:
    // case QCY_CALL_EVT_R_SINGLE:
    //     qcy_evt_fun_set(cmd, key_vlaue);
    //     break;
    // case QCY_CALL_EVT_L_DOUBLE:
    // case QCY_CALL_EVT_R_DOUBLE:
    //     qcy_evt_fun_set(cmd, key_vlaue);
    //     break;
    // case QCY_CALL_EVT_L_THREE:
    // case QCY_CALL_EVT_R_THREE:
    //     qcy_evt_fun_set(cmd, key_vlaue);
    //     break;
    // case QCY_CALL_EVT_L_FOUR:
    // case QCY_CALL_EVT_R_FOUR:
    //     qcy_evt_fun_set(cmd, key_vlaue);
    //     break;
    // case QCY_CALL_EVT_L_LONG:
    // case QCY_CALL_EVT_R_LONG:
    //     qcy_evt_fun_set(cmd, key_vlaue);
    //     break;
    }

    qcy_key_info_save();
}

AT(.app_text.ble)
void qcy_tws_vbat_exchange(void)
{
    qcy_app_cb.vbat_ischarge_local = sys_cb.charge_sta;// 充电标志位
    tws_send_msg_data(TWS_REQ_SENT_VBAT, &qcy_app_cb.vbat_local, 2);
}

AT(.app_text.ble)
static void qcy_tws_vbat_exchange_proc(u8 *buff, u8 len)
{
    qcy_app_cb.vbat_remote = buff[0];
    qcy_app_cb.vbat_ischarge_remote = buff[1];
    TRACE("vb_local[%d]:%d,vb_remote[%d]:%d\n", qcy_app_cb.vbat_ischarge_local, qcy_app_cb.vbat_local, qcy_app_cb.vbat_ischarge_remote, qcy_app_cb.vbat_remote);
    if (IS_APP_MASTER()) {
        qcy_app_vbat_notify();
    }
}


//-------------------------------------------------------
//tws -> tws
AT(.app_text.ble)
static u8 tws_recv_msg_data(u8 cmd, u8 *ptr, u8 len)
{
    printf("-->tws recv CMD %d ", cmd);
    print_r(ptr, len);

    switch (cmd) {
    case TWS_REQ_ALL_INFO_SYNC:
        qcy_tws_info_sync_req_proc(ptr,len);
        break;
    case TWS_RSP_ALL_INFO_SYNC:
        qcy_tws_info_sync_rsp_proc(ptr,len);
        qcy_scan_data_update(1);
        break;
    case TWS_REQ_KEY_INFO_GET:
        qcy_tws_key_info_req_proc();
        break;
    case TWS_RSP_KEY_INFO_GET:
        qcy_tws_key_info_rsp_proc(ptr, len);
        break;
    case TWS_REQ_KEY_INFO_SET:
        qcy_app_key_info_set(ptr[0], ptr[1]);
        break;
    case TWS_REQ_RESET_DEFAULT:
        qcy_reset_default_do();
        break;
    case TWS_REQ_DELETE_LINK:
        qcy_delete_link_do();
        break;
    case TWS_REQ_RESET_FACTORY:
        qcy_reset_factory_do();
        break;
//         case TWS_REQ_SENT_OFF_TIMES:
//             qcy_poweroff_time_save(ptr[0]);
//             break;
#if QCY_CTL_IN_EAR_EN
    case TWS_REQ_SENT_EAR_DET:
        qcy_ear_det_save(ptr[0]);
        break;
#endif
    case TWS_REQ_SENT_EARPHONE_MODE:
        qcy_earphone_mode_save(ptr[0]);
        break;
#if QCY_ANC_EN
    case TWS_REQ_SENT_ANC_INFO:
        memcpy((u8*)&qcy_app_cb.anc_info, ptr, QCY_CM_ANC_LEN);
        qcy_anc_info_save();
        break;
#endif
    case TWS_REQ_SENT_EQ:
        memcpy((u8*)&qcy_app_cb.eq_info, ptr, QCY_CM_EQ_LEN);
        qcy_eq_info_save();
        break;

    case TWS_REQ_VOL_BALANCE_SYNC:
#if QCY_VOL_BALANCE
        qcy_app_cb.vol_balance_val = ptr[0];
        dac_set_dvol(bsp_volume_convert(sys_cb.vol));
        qcy_vol_balance_save();
#endif
        break;
    case TWS_REQ_VOL_DB_SYNC:
        break;
    case TWS_REQ_SLEEP_MODE_SYNC:
        if(ptr[0] == 1){
            qcy_sleep_mode_enter();
        } else {
            qcy_sleep_mode_exit();
        }
        break;
    case TWS_REQ_SENT_VBAT:
        qcy_tws_vbat_exchange_proc(ptr, len);
        break;
    case TWS_REQ_SENT_BT_NAME:
        qcy_bt_name_save(ptr,len);
        break;
    }
    return true;
}
#endif


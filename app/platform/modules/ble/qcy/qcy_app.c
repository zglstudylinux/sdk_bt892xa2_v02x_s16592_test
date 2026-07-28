#include "qcy_app.h"
#include "qcy_tws.h"

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#define TRACE_R(...)            print_r(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACE_R(...)
#endif

#if LE_QCY_APP_EN

qcy_scan_rsp_data_t qcy_scan_data = {
    .len = 0x1b,
    .type_manufacturer = 0xff,
    .manufacturer_id = {0x1c, 0x52},
    .earphone_id = {0x4d, 0x51}, // QCY DT10
    .reserved = 0,
    .reserved1 = 0,
    .reserved2 = 0,
};

qcy_evt_fun_t qcy_evt_fun_tbl[QCY_EVT_FUN_TBL_LEN] = {

    {QCY_MUSIC_EVT_L_SINGLE,0},
    {QCY_MUSIC_EVT_R_SINGLE,0},
    {QCY_MUSIC_EVT_L_DOUBLE,0},
    {QCY_MUSIC_EVT_R_DOUBLE,0},
    {QCY_MUSIC_EVT_L_THREE,0},
    {QCY_MUSIC_EVT_R_THREE,0},
    {QCY_MUSIC_EVT_L_FOUR,0},
    {QCY_MUSIC_EVT_R_FOUR,0},
    {QCY_MUSIC_EVT_L_LONG,0},
    {QCY_MUSIC_EVT_R_LONG,0},

    // {QCY_CALL_EVT_L_SINGLE,0},
    // {QCY_CALL_EVT_R_SINGLE,0},
    // {QCY_CALL_EVT_L_DOUBLE,0},
    // {QCY_CALL_EVT_R_DOUBLE,0},
    // {QCY_CALL_EVT_L_THREE,0},
    // {QCY_CALL_EVT_R_THREE,0},
    // {QCY_CALL_EVT_L_FOUR,0},
    // {QCY_CALL_EVT_R_FOUR,0},
    // {QCY_CALL_EVT_L_LONG,0},
    // {QCY_CALL_EVT_R_LONG,0},

};

qcy_app_cb_st qcy_app_cb AT(.ble_buf.app) = {0};

#if QCY_TWS_EN
const u16 qcy_dvol_tbl_60[60 + 1] = {
    DIG_N60DB,DIG_N59DB,DIG_N58DB,DIG_N57DB,DIG_N56DB,DIG_N55DB,DIG_N54DB,DIG_N53DB,DIG_N52DB,DIG_N51DB,
    DIG_N50DB,DIG_N49DB,DIG_N48DB,DIG_N47DB,DIG_N46DB,DIG_N45DB,DIG_N44DB,DIG_N43DB,DIG_N42DB,DIG_N41DB,
    DIG_N40DB,DIG_N39DB,DIG_N38DB,DIG_N37DB,DIG_N36DB,DIG_N35DB,DIG_N34DB,DIG_N33DB,DIG_N32DB,DIG_N31DB,
    DIG_N30DB,DIG_N29DB,DIG_N28DB,DIG_N27DB,DIG_N26DB,DIG_N25DB,DIG_N24DB,DIG_N23DB,DIG_N22DB,DIG_N21DB,
    DIG_N20DB,DIG_N19DB,DIG_N18DB,DIG_N17DB,DIG_N16DB,DIG_N15DB,DIG_N14DB,DIG_N13DB,DIG_N12DB,DIG_N11DB,
    DIG_N10DB,DIG_N9DB,DIG_N8DB,DIG_N7DB,DIG_N6DB,DIG_N5DB,DIG_N4DB,DIG_N3DB,DIG_N2DB,DIG_N1DB,DIG_N0DB
};
#endif

AT(.app_text.param)
void qcy_flash_write(void *buf, u32 addr, uint len)
{
    cm_write(buf, APP_CM_PAGE(addr), len);
    qcy_app_cb.param_save_update = 2;          //2s
}

AT(.app_text.param)
void qcy_flash_read(void *buf, u32 addr, uint len)
{
    cm_read(buf, APP_CM_PAGE(addr), len);
}

AT(.app_text.param)
void qcy_param_sync(void)
{
    if (qcy_app_cb.param_save_update) {
        qcy_app_cb.param_save_update--;
        if (qcy_app_cb.param_save_update == 0x00) {
            TRACE("param sync\n");
            param_sync();
        }
    }
}

AT(.app_text.param)
static void qcy_param_flag_load(void)
{
    qcy_flash_read(&qcy_app_cb.param_flag,QCY_CM_FLAG_ADDR,QCY_CM_FLAG_LEN);
}

AT(.app_text.param)
static void qcy_param_flag_save(void)
{
    if (qcy_app_cb.param_flag != PARAM_FLAG) {
        qcy_app_cb.param_flag = PARAM_FLAG;
        qcy_flash_write(&qcy_app_cb.param_flag,QCY_CM_FLAG_ADDR,QCY_CM_FLAG_LEN);
    }
}

void qcy_app_key_tbl_set(void)
{
    if (IS_CHANNEL_LEFT()) {
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_SINGLE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_short));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_DOUBLE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_double));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_THREE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_three));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_FOUR, ab_to_qcy_keydef(qcy_app_cb.key_local.key_four));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_LONG, ab_to_qcy_keydef(qcy_app_cb.key_local.key_long));

        qcy_evt_fun_set(QCY_MUSIC_EVT_R_SINGLE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_short));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_DOUBLE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_double));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_THREE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_three));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_FOUR, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_four));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_LONG, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_long));
        // 此处需要补充通话模式副耳按键
    } else {
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_SINGLE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_short));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_DOUBLE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_double));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_THREE, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_three));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_FOUR, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_four));
        qcy_evt_fun_set(QCY_MUSIC_EVT_L_LONG, ab_to_qcy_keydef(qcy_app_cb.key_remote.key_long));

        qcy_evt_fun_set(QCY_MUSIC_EVT_R_SINGLE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_short));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_DOUBLE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_double));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_THREE, ab_to_qcy_keydef(qcy_app_cb.key_local.key_three));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_FOUR, ab_to_qcy_keydef(qcy_app_cb.key_local.key_four));
        qcy_evt_fun_set(QCY_MUSIC_EVT_R_LONG, ab_to_qcy_keydef(qcy_app_cb.key_local.key_long));
        // 此处需要补充通话模式副耳按键
    }
}

AT(.app_text.init)
void qcy_key_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.key_local.key_short, QCY_CM_KEY_ADDR, QCY_CM_KEY_LEN);
    }else {
        TRACE("Key Default\n");
        if (IS_CHANNEL_LEFT()) {
            qcy_app_cb.key_local.key_short    = KEY_SHORT_LEFT_DEF;
            qcy_app_cb.key_local.key_long     = KEY_LONG_LEFT_DEF;
            qcy_app_cb.key_local.key_double   = KEY_DOUBLE_LEFT_DEF;
            qcy_app_cb.key_local.key_three    = KEY_THREE_LEFT_DEF;
            qcy_app_cb.key_local.key_four     = KEY_FOUR_LEFT_DEF;
            qcy_app_cb.key_local.key_five     = KEY_FIVE_LEFT_DEF;

            qcy_app_cb.key_remote.key_short    = KEY_SHORT_RIGHT_DEF;
            qcy_app_cb.key_remote.key_long     = KEY_LONG_RIGHT_DEF;
            qcy_app_cb.key_remote.key_double   = KEY_DOUBLE_RIGHT_DEF;
            qcy_app_cb.key_remote.key_three    = KEY_THREE_RIGHT_DEF;
            qcy_app_cb.key_remote.key_four     = KEY_FOUR_RIGHT_DEF;
            qcy_app_cb.key_remote.key_five     = KEY_FIVE_RIGHT_DEF;
        } else {
            qcy_app_cb.key_remote.key_short    = KEY_SHORT_LEFT_DEF;
            qcy_app_cb.key_remote.key_long     = KEY_LONG_LEFT_DEF;
            qcy_app_cb.key_remote.key_double   = KEY_DOUBLE_LEFT_DEF;
            qcy_app_cb.key_remote.key_three    = KEY_THREE_LEFT_DEF;
            qcy_app_cb.key_remote.key_four     = KEY_FOUR_LEFT_DEF;
            qcy_app_cb.key_remote.key_five     = KEY_FIVE_LEFT_DEF;

            qcy_app_cb.key_local.key_short    = KEY_SHORT_RIGHT_DEF;
            qcy_app_cb.key_local.key_long     = KEY_LONG_RIGHT_DEF;
            qcy_app_cb.key_local.key_double   = KEY_DOUBLE_RIGHT_DEF;
            qcy_app_cb.key_local.key_three    = KEY_THREE_RIGHT_DEF;
            qcy_app_cb.key_local.key_four     = KEY_FOUR_RIGHT_DEF;
            qcy_app_cb.key_local.key_five     = KEY_FIVE_RIGHT_DEF;
        }
        qcy_flash_write(&qcy_app_cb.key_local.key_short, QCY_CM_KEY_ADDR, QCY_CM_KEY_LEN);
    }

    xcfg_cb.user_def_ks_sel = qcy_app_cb.key_local.key_short;
    xcfg_cb.user_def_kl_sel = qcy_app_cb.key_local.key_long;
    xcfg_cb.user_def_kd_sel = qcy_app_cb.key_local.key_double;
    xcfg_cb.user_def_kt_sel = qcy_app_cb.key_local.key_three;
    xcfg_cb.user_def_kfour_sel = qcy_app_cb.key_local.key_four;
    xcfg_cb.user_def_kfive_sel = qcy_app_cb.key_local.key_five;
    qcy_app_key_tbl_set();
}

AT(.app_text.key)
// QCY按键定义转蓝汛SDK定义
u8 qcy_to_ab_keydef(u8 qcy_key)
{
    u8 ab_key = 0xff;
    switch (qcy_key) {
    case QCY_EVT_FUN_NONE:
        ab_key = UDK_NONE;
        break;
    case QCY_EVT_FUN_PP:
        ab_key = UDK_PLAY_PAUSE;
        break;
    case QCY_EVT_FUN_PREV:
        ab_key = UDK_PREV;
        break;
    case QCY_EVT_FUN_NEXT:
        ab_key = UDK_NEXT;
        break;
    case QCY_EVT_FUN_SIRI:
        ab_key = UDK_SIRI;
        break;
    case QCY_EVT_FUN_VOL_UP:
        ab_key = UDK_VOL_UP;
        break;
    case QCY_EVT_FUN_VOL_DOWN:
        ab_key = UDK_VOL_DOWN;
        break;
    case QCY_EVT_FUN_GAME_MODE:
        ab_key = UDK_LOW_LATENCY;
        break;
    case QCY_EVT_FUN_LISTEN_MODE:
        ab_key = 15;
        break;
    case QCY_EVT_FUN_ANC_CHANGE:
        ab_key = UDK_NR;
        break;
    default:
        break;
    }
    return ab_key;
}

AT(.app_text.key)
// 蓝汛SDK按键定义转QCY定义
u8 ab_to_qcy_keydef(u8 ab_key)
{
    u8 qcy_key = 0;
    switch (ab_key) {
    case UDK_NONE:
        qcy_key = QCY_EVT_FUN_NONE;
        break;
    case UDK_PLAY_PAUSE:
        qcy_key = QCY_EVT_FUN_PP;
        break;
    case UDK_PREV:
        qcy_key = QCY_EVT_FUN_PREV;
        break;
    case UDK_NEXT:
        qcy_key = QCY_EVT_FUN_NEXT;
        break;
    case UDK_SIRI:
        qcy_key = QCY_EVT_FUN_SIRI;
        break;
    case UDK_VOL_UP:
        qcy_key = QCY_EVT_FUN_VOL_UP;
        break;
    case UDK_VOL_DOWN:
        qcy_key = QCY_EVT_FUN_VOL_DOWN;
        break;
    case UDK_LOW_LATENCY:
        qcy_key = QCY_EVT_FUN_GAME_MODE;
        break;
    case UDK_NR:
        qcy_key = QCY_EVT_FUN_ANC_CHANGE;
        break;
    default:
        break;
    }
    return qcy_key;
}

// QCY EQ定义转蓝汛EQ定义
u8 qcy_to_ab_eq_def(u8 qcy_eq_id)
{
    u8 ab_eq = 0;
    switch (qcy_eq_id) {
    case QCY_EQ_CUSTOM:
        ab_eq = 0xff;
        break;
    case QCY_EQ_DEFAULT:
        ab_eq = 0x00;
        break;
    case QCY_EQ_POP:
        ab_eq = 0x01;
        break;
    case QCY_EQ_MEGA_BASS:
        ab_eq = 0xff;
        break;
    case QCY_EQ_ROCK:
        ab_eq = 0x02;
        break;
    case QCY_EQ_SOFT:
        ab_eq = 0xff;
        break;
    case QCY_EQ_CLASSICAL:
        ab_eq = 0x04;
        break;
    default:
        break;
    }
    return ab_eq;
}

// 进入睡眠模式
void qcy_sleep_mode_enter(void)
{
    TRACE("qcy enter sleep\n");
#if QCY_TWS_EN
    qcy_tws_sleep_mode_sync();
#endif
    // 关闭按键
    xcfg_cb.user_def_ks_sel = UDK_NONE;
    xcfg_cb.user_def_kl_sel = UDK_NONE;
    xcfg_cb.user_def_kd_sel = UDK_NONE;
    xcfg_cb.user_def_kt_sel = UDK_NONE;
    xcfg_cb.user_def_kfour_sel = UDK_NONE;
    xcfg_cb.user_def_kfive_sel = UDK_NONE;
}

// 退出睡眠模式
void qcy_sleep_mode_exit(void)
{
    TRACE("qcy exit sleep\n");
#if QCY_TWS_EN
    qcy_tws_sleep_mode_sync();
#endif
    // 恢复按键
    xcfg_cb.user_def_ks_sel = qcy_app_cb.key_local.key_short;
    xcfg_cb.user_def_kl_sel = qcy_app_cb.key_local.key_long;
    xcfg_cb.user_def_kd_sel = qcy_app_cb.key_local.key_double;
    xcfg_cb.user_def_kt_sel = qcy_app_cb.key_local.key_three;
    xcfg_cb.user_def_kfour_sel = qcy_app_cb.key_local.key_four;
    xcfg_cb.user_def_kfive_sel = qcy_app_cb.key_local.key_five;
}

// 进入触控测试模式
void qcy_touch_test_mode_enter(void)
{

}

// 退出触控测试模式
void qcy_touch_test_mode_exit(void)
{

}

// 进入贴合度检测模式
void qcy_compactness_detect_mode_enter(void)
{

}

// 退出贴合度检测模式
void qcy_compactness_detect_mode_exit(void)
{

}

// 设置EQ
void qcy_eq_set_do(void)
{
    if (qcy_app_cb.eq_info.mode == QCY_EQ_CUSTOM) {
        music_set_eq_overall_gain(-8);//设置自定义EQ时，整体EQ降低8DB
        for (u8 i=0; i< QCY_EQ_BAND_CNT; i++) {
            music_set_eq_for_index(i,qcy_app_cb.eq_info.gain[i]);
        }
        music_set_eq_for_index_do();
    } else if (qcy_app_cb.eq_info.mode >= 1 && qcy_app_cb.eq_info.mode <= 6) {
        music_set_eq_by_num(qcy_to_ab_eq_def(qcy_app_cb.eq_info.mode));
    }
}

#if QCY_ANC_EN
u8 qcy_anc_def_to_ab(void)
{
    if (qcy_app_cb.anc_info.mode == 1) {
        return 1;
    } else if (qcy_app_cb.anc_info.mode == 2) {
        return 0;
    } else {
        return 2;
    }
}

u8 ab_anc_def_to_qcy(void)
{
    if (sys_cb.anc_mode == 0) {
        return 2;
    } else if (sys_cb.anc_mode == 1) {
        return 1;
    } else {
        return 3;
    }
}

void qcy_anc_set_level(void)
{
    s8 anc_value;
    if (qcy_app_cb.anc_info.mode == QCY_MODE_ANC_ON) {
        anc_value = (qcy_app_cb.anc_default_gain - QCY_ANC_VAL_MIN)*qcy_app_cb.anc_info.anc_on_level/100;
        anc_value = QCY_ANC_VAL_MIN + anc_value;
        TRACE("set anc val:%d\n", anc_value);
        bsp_anc_gain_adjust(0, anc_value);
    } else if (qcy_app_cb.anc_info.mode == QCY_MODE_TRANSPARENCY) {
        anc_value = 10*qcy_app_cb.anc_info.transparency_level/50;
        anc_value = qcy_app_cb.tp_default_gain - 10 + anc_value;
        TRACE("set anc tp val:%d\n", anc_value);
        bsp_anc_gain_adjust(0, anc_value);
    }
}

void qcy_anc_set_do(void)
{
    u8 anc_change_flag;
    if (sys_cb.anc_mode != qcy_anc_def_to_ab()) {
        anc_change_flag = 1;
    } else {
        anc_change_flag = 0;
    }

    switch (qcy_app_cb.anc_info.mode) {
    case QCY_MODE_ANC_ON:
        if (anc_change_flag) {
            bsp_tws_res_wav_play(TWS_RES_ANC_ON);
        }
        break;

    case QCY_MODE_ANC_OFF:
        if (anc_change_flag) {
            bsp_tws_res_wav_play(TWS_RES_ANC_OFF);
        }
        break;

    case QCY_MODE_TRANSPARENCY:
        if (anc_change_flag) {
            bsp_tws_res_wav_play(TWS_RES_TRANSPARENCY_ON);
        }
        break;

    default:
        break;
    }
    qcy_anc_set_level();
    sys_cb.anc_mode = qcy_anc_def_to_ab();
    sys_cb.anc_user_mode = qcy_app_cb.anc_info.mode;
}
#endif

// 恢复默认设置
void qcy_reset_default_do(void)
{
    TRACE("%s\n", __func__);
#if QCY_TWS_EN
    qcy_tws_reset_default_sync();
#endif
    qcy_app_cb.param_flag = 0xFFFF;
    qcy_sleep_mode_exit();
    if(IS_GAME_MODE() == 0x01) {
        bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
    }
    qcy_key_init();
    qcy_eq_init();
#if QCY_ANC_EN
    qcy_anc_info_init();
#endif
#if QCY_VOL_BALANCE
    qcy_vol_balance_init();
#endif
    qcy_language_init();
    qcy_param_flag_save();
}

// 清除配对信息
void qcy_delete_link_do(void)
{
    TRACE("%s\n", __func__);
    // qcy_tws_delete_link_sync();
    bt_nor_unpair_device();
}

// 恢复出厂设置
void qcy_reset_factory_do(void)
{
    TRACE("%s\n", __func__);
#if QCY_TWS_EN
    qcy_tws_reset_factory_sync();
#endif
}

AT(.app_text.key)
void qcy_key_info_save(void)
{
    TRACE("qcy_key_info_save:");

    u8 temp[QCY_CM_KEY_LEN];

    memcpy(temp, (u8*)&qcy_app_cb.key_local, QCY_CM_KEY_LEN);
    qcy_flash_write(temp, QCY_CM_KEY_ADDR, QCY_CM_KEY_LEN);

    TRACE_R(temp, QCY_CM_KEY_LEN);
}

void qcy_eq_info_save(void)
{
    TRACE("qcy_eq_info_save:");

    u8 temp[QCY_CM_EQ_LEN];

    memcpy(temp, (u8*)&qcy_app_cb.eq_info, QCY_CM_EQ_LEN);
    qcy_flash_write(temp, QCY_CM_EQ_ADDR, QCY_CM_EQ_LEN);
    TRACE_R(temp, QCY_CM_EQ_LEN);
#if QCY_TWS_EN
    if (IS_APP_MASTER()) {
        tws_send_msg_data(TWS_REQ_SENT_EQ, temp, QCY_CM_EQ_LEN);
        delay_5ms(30);
    }
#endif // QCY_TWS_EN
    qcy_app_cb.do_flag |= FLAG_EQ_SET;
}

void qcy_eq_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.eq_info, QCY_CM_EQ_ADDR, QCY_CM_EQ_LEN);
    } else {
        TRACE("Eq Default\n");
        u8 default_eq_info[QCY_CM_EQ_LEN] = {1,0,0,0,0,0,0,0,0,0,0};
        memcpy((u8*)&qcy_app_cb.eq_info, default_eq_info, QCY_CM_EQ_LEN);
        qcy_flash_write((u8*)&qcy_app_cb.eq_info, QCY_CM_EQ_ADDR, QCY_CM_EQ_LEN);
    }
    if (!sco_is_connected()) {
        qcy_eq_set_do();
    }
}

#if QCY_VOL_BALANCE
void qcy_vol_balance_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.vol_balance_val, QCY_CM_VOL_BALANCE_ADDR, QCY_CM_VOL_BALANCE_LEN);
    } else {
        TRACE("Vol Balance Default\n");
        qcy_app_cb.vol_balance_val = 50;
        qcy_flash_write((u8*)&qcy_app_cb.vol_balance_val, QCY_CM_VOL_BALANCE_ADDR, QCY_CM_VOL_BALANCE_LEN);
    }
}

AT(.app_text.param)
void qcy_vol_balance_save(void)
{
    TRACE("%s:%d\n", __func__, qcy_app_cb.vol_balance_val);
    qcy_flash_write(&qcy_app_cb.vol_balance_val, QCY_CM_VOL_BALANCE_ADDR, QCY_CM_VOL_BALANCE_LEN);
#if QCY_TWS_EN
    if(IS_APP_MASTER()){
        qcy_tws_vol_balance_sync();
    }
#endif
}
#endif

#if QCY_TWS_EN
u16 qcy_dvol_tbl_get_val(u8 id)
{
    return qcy_dvol_tbl_60[60 - id];
}
#endif

#if QCY_CTL_IN_EAR_EN
AT(.app_text.param)
void qcy_ear_det_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.in_ear_detect, QCY_CM_EAR_DET_ADDR, QCY_CM_EAR_DET_LEN);
    } else {
        TRACE("Ear Det Default\n");
        qcy_app_cb.in_ear_detect = 1;
        qcy_flash_write(&qcy_app_cb.in_ear_detect, QCY_CM_EAR_DET_ADDR, QCY_CM_EAR_DET_LEN);
    }
}

AT(.app_text.param)
void qcy_ear_det_save(u8 ear_en)
{
    qcy_app_cb.in_ear_detect = ear_en;
    qcy_flash_write(&qcy_app_cb.in_ear_detect, QCY_CM_EAR_DET_ADDR, QCY_CM_EAR_DET_LEN);
#if QCY_TWS_EN
    if (IS_APP_MASTER()) {
        qcy_tws_ear_det_sync();
    }
#endif
}
#endif

AT(.app_text.param)
void qcy_earphone_mode_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.earphone_mode, QCY_CM_EARPHONE_MODE_ADDR, QCY_CM_EARPHONE_MODE_LEN);
    }else{
        TRACE("Earphone Mode Default\n");
        qcy_app_cb.earphone_mode = 1;
        qcy_flash_write(&qcy_app_cb.earphone_mode, QCY_CM_EARPHONE_MODE_ADDR, QCY_CM_EARPHONE_MODE_LEN);
    }
}

AT(.app_text.param)
void qcy_earphone_mode_save(u8 mode)
{
    TRACE("%s\n", __func__);
    qcy_app_cb.earphone_mode = mode;
    qcy_flash_write(&qcy_app_cb.earphone_mode, QCY_CM_EARPHONE_MODE_ADDR, QCY_CM_EARPHONE_MODE_LEN);
#if QCY_TWS_EN
    if (IS_APP_MASTER()) {
        qcy_tws_earphone_mode_sync();
    }
#endif
}

#if QCY_ANC_EN
AT(.app_text.param)
void qcy_anc_info_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&qcy_app_cb.anc_info, QCY_CM_ANC_ADDR, QCY_CM_ANC_LEN);
    } else {
        TRACE("ANC Info Default\n");
        qcy_app_cb.anc_info.mode = QCY_MODE_ANC_ON;
        qcy_app_cb.anc_info.anc_on_level = 100;
        qcy_app_cb.anc_info.transparency_level = 50;
        qcy_flash_write(&qcy_app_cb.anc_info, QCY_CM_ANC_ADDR, QCY_CM_ANC_LEN);
    }
    qcy_app_cb.anc_default_gain = sys_cb.adjust_val[0];
    qcy_app_cb.tp_default_gain = sys_cb.adjust_val[2];
}

AT(.app_text.param)
void qcy_anc_info_save()
{
    TRACE("%s\n", __func__);
    qcy_flash_write(&qcy_app_cb.anc_info, QCY_CM_ANC_ADDR, QCY_CM_ANC_LEN);
#if QCY_TWS_EN
    if (IS_APP_MASTER()) {
        qcy_tws_anc_sync();
        qcy_app_cb.do_flag |= FLAG_ANC_SET;
    }
#endif
}

void qcy_anc_init(void)
{
    if (QCY_CM_IS_EXIST()) {
        bsp_param_read(&sys_cb.anc_mode, PARAM_ANC_NR_STA, 1);
    } else {
        sys_cb.anc_mode = 0;
        sys_cb.anc_user_mode = 0;
        bsp_param_write(&sys_cb.anc_mode, PARAM_ANC_NR_STA, 1);
    }
}
#endif // QCY_ANC_EN

void qcy_language_init(void)
{
    if (QCY_CM_IS_EXIST()) {

    } else {
        TRACE("Language Default\n");
    }
}

void qcy_bt_name_init(void)
{
    u8 bt_name_len = 0;

    if (QCY_CM_IS_EXIST()) {
        qcy_flash_read(&bt_name_len,QCY_CM_BT_NAME_SIZE_ADDR,QCY_CM_BT_NAME_SIZE_LEN);
        if((bt_name_len > 0) && (bt_name_len <= 32)) {
            memset(xcfg_cb.bt_name,0,sizeof(xcfg_cb.bt_name));
            qcy_flash_read(xcfg_cb.bt_name,QCY_CM_BT_NAME_ADDR,bt_name_len);
        }
    } else {
        TRACE("Bt Name Default\n");
        memcpy(xcfg_cb.bt_name,qcy_app_cb.bt_name,32);
        bt_name_len = strlen(xcfg_cb.bt_name);
        qcy_flash_write(xcfg_cb.bt_name,QCY_CM_BT_NAME_ADDR,bt_name_len);
        qcy_flash_write(&bt_name_len,QCY_CM_BT_NAME_SIZE_ADDR,QCY_CM_BT_NAME_SIZE_LEN);
    }
}

void qcy_bt_name_save(u8 *name, u8 len)
{
    memset(xcfg_cb.bt_name,0,sizeof(xcfg_cb.bt_name));
    memcpy(xcfg_cb.bt_name,name,len);

    qcy_flash_write(xcfg_cb.bt_name,QCY_CM_BT_NAME_ADDR,len);
    qcy_flash_write(&len,QCY_CM_BT_NAME_SIZE_ADDR,QCY_CM_BT_NAME_SIZE_LEN);

    bt_set_stack_local_name(xcfg_cb.bt_name);
}

#if QCY_VOL_BALANCE
// 声道平衡 balance<50返回右耳db值 balance>50返回左耳db值
int qcy_cal_vol_balance(int balance, int db_current)
{
    int res;
    int temp1;
    int temp2;
    int temp3;
    if (balance < 50) {
        temp1 = ((-db_current) - QCY_DB_VOL0)*100;
        //避免除0
        if (balance == 0) {
            temp2 = 0;
            temp3 = 0;
        } else {
            temp2 = (100 - balance)*100/balance;
            temp3 = temp1/temp2;
        }
        res = temp3 + QCY_DB_VOL0;
        TRACE("balance=%d db_cur=%d right%d\n", balance, db_current, res);
    } else {
        res = ((-db_current) - QCY_DB_VOL0)*((100-balance)*1000/balance)/1000 + QCY_DB_VOL0;
        TRACE("balance=%d db_cur=%d left%d\n", balance, db_current, res);
    }
    return -res;
}

#if QCY_TWS_EN
void dac_dvol_process(u16 *vol)
{
    if (bt_tws_is_connected() && qcy_app_cb.vol_balance_val != 50) {
        if (IS_CHANNEL_LEFT()) {
            if(qcy_app_cb.vol_balance_val > 50) {
                *vol = qcy_dvol_tbl_get_val(qcy_cal_vol_balance(qcy_app_cb.vol_balance_val, qcy_vol_dig2db(*vol)));
            }
        } else {
            if(qcy_app_cb.vol_balance_val < 50) {
                *vol = qcy_dvol_tbl_get_val(qcy_cal_vol_balance(qcy_app_cb.vol_balance_val, qcy_vol_dig2db(*vol)));
            }
        }
    }
    TRACE("dac_dvol_process:%d\n", *vol);
}
#endif
#endif

void qcy_scan_data_set_checksum(void)
{
    u16 temp = 0;
    u8 *p_adv = (u8*)&qcy_scan_data;
    for (u8 i=7; i<=14; i++) {
        temp += p_adv[i];
    }
    qcy_scan_data.checksum = (u8)(temp & 0x00ff);
}

void qcy_scan_data_update(u8 popup_flag)
{
    if (popup_flag == 0) {
        qcy_scan_data.box_is_open = 0;
        qcy_scan_data.outside_flag = 0;
    } else {
        qcy_scan_data.box_is_open = 1;
        qcy_scan_data.outside_flag = 1;
    }
    qcy_app_cb.vbat_local = bsp_get_bat_level();

    qcy_scan_data.tws_is_left = func_bt_tws_get_channel();
    qcy_scan_data.color_is_black = QCY_COLOR_IS_BLACK();
    qcy_scan_data.connect_sta = bt_is_connected();
    qcy_scan_data.pair_flag = bt_nor_get_link_info(NULL);
    qcy_scan_data.tws_connect_sta = bt_tws_is_connected();
    if (IS_CHANNEL_LEFT()) {
        qcy_scan_data.left_discover_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.left_connect_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_discover_mode = 0;
        qcy_scan_data.right_connect_mode = 0;
        qcy_scan_data.left_charge_sta = qcy_app_cb.vbat_ischarge_local;
        qcy_scan_data.left_vbat = qcy_app_cb.vbat_local;
        if (qcy_scan_data.tws_connect_sta == 1) {
            qcy_scan_data.right_charge_sta = qcy_app_cb.vbat_ischarge_remote;
            qcy_scan_data.right_vbat = qcy_app_cb.vbat_remote;
        } else {
            qcy_scan_data.right_charge_sta = 0;
            qcy_scan_data.right_vbat = 0;
        }
    } else {
        qcy_scan_data.left_discover_mode = 0;
        qcy_scan_data.left_connect_mode = 0;
        qcy_scan_data.right_discover_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_connect_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_charge_sta = qcy_app_cb.vbat_ischarge_local;
        qcy_scan_data.right_vbat = qcy_app_cb.vbat_local;
        if (qcy_scan_data.tws_connect_sta == 1) {
            qcy_scan_data.left_charge_sta = qcy_app_cb.vbat_ischarge_remote;
            qcy_scan_data.left_vbat = qcy_app_cb.vbat_remote;
        } else {
            qcy_scan_data.left_charge_sta = 0;
            qcy_scan_data.left_vbat = 0;
        }
    }
    qcy_scan_data.box_charge_sta = qcy_app_cb.vbat_ischarge_box;
    qcy_scan_data.box_vbat = qcy_app_cb.vbat_box;
    qcy_scan_data.adv_count = (qcy_scan_data.adv_count+1)%255;
    qcy_scan_data_set_checksum();
    ble_set_scan_rsp_data((u8*)&qcy_scan_data, sizeof(qcy_scan_data));
}

void qcy_adv_init(void)
{
    u8 addr[6];
    qcy_app_cb.vbat_local = bsp_get_bat_level();

    qcy_scan_data.tws_is_left = IS_CHANNEL_LEFT();
    qcy_scan_data.color_is_black = QCY_COLOR_IS_BLACK();
    qcy_scan_data.box_is_open = 1;
    qcy_scan_data.connect_sta = bt_is_connected();
    qcy_scan_data.pair_flag = bt_nor_get_link_info(NULL);
    qcy_scan_data.mac_encrypt_flag = 0;
    qcy_scan_data.outside_flag = 1;
#if QCY_TWS_EN
    qcy_scan_data.tws_connect_sta = bt_tws_is_connected();
#else
    qcy_scan_data.tws_connect_sta = 0;
#endif
    if (IS_CHANNEL_LEFT()) {
        qcy_scan_data.left_discover_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.left_connect_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_discover_mode = 0;
        qcy_scan_data.right_connect_mode = 0;
        qcy_scan_data.left_charge_sta = sys_cb.charge_sta;
        qcy_scan_data.left_vbat = qcy_app_cb.vbat_local;
        if (qcy_scan_data.tws_connect_sta == 1) {
            qcy_scan_data.right_charge_sta = qcy_app_cb.vbat_ischarge_remote;
            qcy_scan_data.right_vbat = qcy_app_cb.vbat_remote;
        } else {
            qcy_scan_data.right_charge_sta = 0;
            qcy_scan_data.right_vbat = 0;
        }
    } else {
        qcy_scan_data.left_discover_mode = 0;
        qcy_scan_data.left_connect_mode = 0;
        qcy_scan_data.right_discover_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_connect_mode = IS_CAN_BE_SCAN();
        qcy_scan_data.right_charge_sta = sys_cb.charge_sta;
        qcy_scan_data.right_vbat = qcy_app_cb.vbat_local;
        if (qcy_scan_data.tws_connect_sta == 1) {
            qcy_scan_data.left_charge_sta = qcy_app_cb.vbat_ischarge_remote;
            qcy_scan_data.left_vbat = qcy_app_cb.vbat_remote;
        } else {
            qcy_scan_data.left_charge_sta = 0;
            qcy_scan_data.left_vbat = 0;
        }
    }
    qcy_app_cb.vbat_ischarge_box = 0;
    qcy_scan_data.box_charge_sta = qcy_app_cb.vbat_ischarge_box;
    qcy_scan_data.box_vbat = 0;
    bt_get_local_bd_addr(addr);
    qcy_scan_data.master_changed_mac[0] = addr[1];
    qcy_scan_data.master_changed_mac[1] = addr[0];
    qcy_scan_data.master_changed_mac[2] = addr[2];
    qcy_scan_data.master_changed_mac[3] = addr[5];
    qcy_scan_data.master_changed_mac[4] = addr[4];
    qcy_scan_data.master_changed_mac[5] = addr[3];
    qcy_scan_data.adv_count = 1;
    qcy_scan_data.slave_changed_mac[0] = addr[1];
    qcy_scan_data.slave_changed_mac[1] = addr[0];
    qcy_scan_data.slave_changed_mac[2] = addr[2];
    qcy_scan_data.slave_changed_mac[3] = addr[5];
    qcy_scan_data.slave_changed_mac[4] = addr[4];
    qcy_scan_data.slave_changed_mac[5] = addr[3];
    qcy_scan_data_set_checksum();
}

// 上报电池电量
void qcy_app_vbat_notify(void)
{
    qcy_vbat_rsp_data_t qcy_vbat_data;
    if (!ble_is_connect()) {
        return;
    }
    qcy_vbat_data.box_charge_sta = qcy_app_cb.vbat_ischarge_box;
    qcy_vbat_data.box_vbat = qcy_app_cb.vbat_box;
    qcy_vbat_data.box_charge_sta = 0;
    qcy_app_cb.vbat_ischarge_local = 0;
    qcy_app_cb.vbat_ischarge_remote = 0;
    if (IS_CHANNEL_LEFT()) {
        qcy_vbat_data.left_charge_sta = qcy_app_cb.vbat_ischarge_local;
        qcy_vbat_data.left_vbat = qcy_app_cb.vbat_local;
#if QCY_TWS_EN
        qcy_vbat_data.right_charge_sta = qcy_app_cb.vbat_ischarge_remote;
        qcy_vbat_data.right_vbat = qcy_app_cb.vbat_remote;
#else
        qcy_vbat_data.right_charge_sta = 0;
        qcy_vbat_data.right_vbat = 0;
#endif
    } else {
        qcy_vbat_data.left_charge_sta = qcy_app_cb.vbat_ischarge_remote;
        qcy_vbat_data.left_vbat = qcy_app_cb.vbat_remote;
        qcy_vbat_data.right_charge_sta = qcy_app_cb.vbat_ischarge_local;
        qcy_vbat_data.right_vbat = qcy_app_cb.vbat_local;
    }
    ble_qcy_report_status_send_packet((u8*)&qcy_vbat_data, sizeof(qcy_vbat_data));
}

// QCY设置功能上报，耳机功能改变要主动上报
void qcy_app_setting_notify(u8 cmd)
{
    u8 setting_data[34];
    u8 offset = 0;
    u8 len;
    if (!ble_is_connect()) {
        return;
    }
    memset(setting_data, 0, 34);
    switch(cmd) {
    case QCY_CMD_LED_BLINK:
        len = 1;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        setting_data[offset++] = get_led_sta(0);
        break;
    case QCY_CMD_VOL:
        len = 3;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        setting_data[offset++] = sys_cb.vol;
        setting_data[offset++] = sys_cb.vol;
        setting_data[offset++] = VOL_MAX;
        break;
    case QCY_CMD_GAME_MODE:
        len = 1;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        setting_data[offset++] = qcy_app_cb.game_mode;
        break;
    case QCY_CMD_SLEEP_MODE:
        len = 1;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        setting_data[offset++] = qcy_app_cb.sleep_mode;
        break;
    case QCY_CMD_READ_CLASIC_ADDR:
        len = 6;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        memcpy(&setting_data[offset], xcfg_cb.bt_addr, len);
        offset += len;
        break;
#if QCY_VOL_BALANCE
    case QCY_CMD_CHANNEL_BALANCE:
        len = 1;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        setting_data[offset++] = qcy_app_cb.vol_balance_val;
        break;
#endif
    case QCY_CMD_ANC:
        len = 3;
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        memcpy(&setting_data[offset], (u8*)&qcy_app_cb.anc_info, len);
        offset += len;
        break;
    case QCY_CMD_SET_BT_NAME:
        len = strlen(xcfg_cb.bt_name);
        setting_data[offset++] = 0xff;
        setting_data[offset++] = len+2;
        setting_data[offset++] = cmd;
        setting_data[offset++] = len;
        memcpy(&setting_data[offset], xcfg_cb.bt_name, len);
        offset += len;
        break;
    default:
        break;
    }
    if(offset > 0) {
        ble_qcy_get_func_setting_send_packet((u8*)&setting_data, offset);
    }
}

AT(.app_text.ble)
void qcy_app_vbat_process(void)
{
#if (VUSB_SMART_VBAT_HOUSE_EN)
    qcy_app_cb.vbat_box = sys_cb.loc_house_bat;
#else
    qcy_app_cb.vbat_box = 0;
#endif

    if ((qcy_app_cb.sec_count % VBAT_UPDATE_TIME) == 0) {
        qcy_app_cb.vbat_local = bsp_get_bat_level();
#if QCY_TWS_EN
        if (bt_tws_is_connected()) {
            qcy_tws_vbat_exchange();
        } else {
            qcy_app_vbat_notify();
        }
#else
        qcy_app_vbat_notify();
#endif
    }
}

void qcy_setting_data_process(u8 *ptr, u8 len)
{
    u8 i;
    u8 cmd_index = 2;
    u8 *cmd;
    u8 cmd_len;
    if (ptr[0] != 0xff) {
        return;
    }
    for (i=0; i<len; i++) {
        if (i == cmd_index) {
            TRACE("setting cmd = %02x\n",ptr[i]);
            cmd_len = ptr[i+1];
            cmd = &ptr[i+2];
            switch (ptr[i]) {
            case QCY_CMD_DEFAULT:
                qcy_reset_default_do();
                break;
            case QCY_CMD_CLEAR_PAIR:
                qcy_delete_link_do();
                break;
            case QCY_CMD_RECOVERY:
                qcy_reset_factory_do();
                break;
            case QCY_CMD_PLAY_PAUSE:
                break;
#if QCY_CTL_IN_EAR_EN
            case QCY_CMD_IN_EAR_DETECT:
                qcy_ear_det_save(cmd[0]);
                break;
#endif
            case QCY_CMD_DENOISE:
                break;
            case QCY_CMD_VOL:
                sys_cb.vol = cmd[1];
                qcy_app_cb.vol = sys_cb.vol;
                bsp_set_volume(sys_cb.vol);
                bsp_bt_vol_change();
                break;
            case QCY_CMD_GAME_MODE:
                qcy_app_cb.game_mode = cmd[0];
                qcy_app_cb.game_mode_set_flag = 1;
                if (qcy_app_cb.game_mode == 0x01) {
                    bsp_tws_res_music_play(TWS_RES_GAME_MODE);
                } else {
                    bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
                }
                // TRACE("game mode:%02x %02x\n",qcy_app_cb.game_mode, IS_GAME_MODE());
                break;
            case QCY_CMD_LISTEN_MODE:
                break;
            case QCY_CMD_TOUCH_SENSITIVITY:
                break;
            case QCY_CMD_EARPHONE_MODE:
                qcy_earphone_mode_save(cmd[0]);
                break;
            case QCY_CMD_TOUCH_TEST_MODE:
                break;
            case QCY_CMD_OTA:
                break;
            case QCY_CMD_TIME_SYNC:
                break;
            case QCY_CMD_SLEEP_MODE:
                qcy_app_cb.sleep_mode = cmd[0];
                qcy_app_cb.sleep_mode_set_flag = 1;
                if (qcy_app_cb.sleep_mode == 1) {
                    qcy_sleep_mode_enter();
                } else {
                    qcy_sleep_mode_exit();
                }
                break;
            case QCY_CMD_COMPACTNESS_DETECT:
                break;
            case QCY_CMD_LED_MODE_SW:
                break;
            case QCY_CMD_READ_CLASIC_ADDR:
                qcy_app_setting_notify(QCY_CMD_READ_CLASIC_ADDR);
                break;
#if QCY_VOL_BALANCE
            case QCY_CMD_CHANNEL_BALANCE:
                qcy_app_cb.vol_balance_val = cmd[0];
#if QCY_TWS_EN
                dac_set_dvol(bsp_volume_convert(sys_cb.vol));
#else

#endif
                qcy_vol_balance_save();
                break;
#endif
            case QCY_CMD_MULTI_POINT_CON:
                break;
#if QCY_ANC_EN
            case QCY_CMD_ANC:
                qcy_app_cb.anc_info.mode = cmd[0];
                TRACE("user=%d,anc=%d,info=%d\n",sys_cb.anc_user_mode,sys_cb.anc_mode,qcy_app_cb.anc_info.mode);
                if (sys_cb.anc_mode != qcy_app_cb.anc_info.mode) {
                    qcy_app_cb.anc_mode_set_flag = 1;
                }
                if (cmd[1] == 0x01) {
                    if (qcy_app_cb.anc_info.mode == QCY_MODE_ANC_ON) {
                        qcy_app_cb.anc_info.anc_on_level = cmd[2];
                    } else if (qcy_app_cb.anc_info.mode == QCY_MODE_TRANSPARENCY) {
                        qcy_app_cb.anc_info.transparency_level = cmd[2];
                    }
                }
                qcy_anc_info_save();
                break;
#endif
            case QCY_CMD_SET_BT_NAME:
                if (ptr[i+1] > 32) {
                    qcy_bt_name_save(cmd, 32);
                } else {
                    qcy_bt_name_save(cmd, cmd_len);
                }
#if QCY_TWS_EN
                qcy_tws_bt_name_sync();
#endif
                break;
            case QCY_CMD_CHANGE_TONE:
                break;
            case QCY_CMD_GET_TONE_STR:
                break;
            case QCY_CMD_SHUTDOWN_TIMER:
                break;
            case QCY_CMD_GET_SETTING:
                qcy_app_setting_notify(cmd[0]);
                break;
            default:
                break;
            }
            cmd_index += cmd_len + 2;
        }
    }
}

// QCY EQ1设置数据处理函数
void qcy_eq1_data_process(u8 *ptr, u8 len)
{
    if (qcy_app_cb.eq_info.mode == 0xff && ptr[0] != 0xff) {
        eq_var_init(NULL);
    }
    qcy_app_cb.eq_info.mode = ptr[0];
    memcpy((u8*)&qcy_app_cb.eq_info.gain, &ptr[1], QCY_EQ_BAND_CNT);
    qcy_eq_info_save();
}

void qcy_evt_fun_set(u8 evt_id, u8 fun_id)
{
    for (u8 i=0; i<QCY_EVT_FUN_TBL_LEN; i++) {
        if (evt_id == qcy_evt_fun_tbl[i].evt_id) {
            qcy_evt_fun_tbl[i].fun_id = fun_id;
        }
    }
}

// QCY按键功能设置处理函数
void qcy_key_set_process(u8 *ptr, u8 len)
{
    u8 key_val = 0;
    TRACE("%s:%d\n", __func__, func_bt_tws_get_channel());
    if (len != 2) {
        TRACE("event setting len error!\n");
    }
    for (u8 i=0; i<QCY_EVT_FUN_TBL_LEN; i++) {
        if (ptr[0] == qcy_evt_fun_tbl[i].evt_id) {
            qcy_evt_fun_tbl[i].fun_id = ptr[1];
            key_val = qcy_to_ab_keydef(ptr[1]);
            // 设置左耳
            if(i%2 == 0 && IS_CHANNEL_LEFT()) {
                TRACE("set left key:%d\n", key_val);
                switch (qcy_evt_fun_tbl[i].evt_id){
                case QCY_MUSIC_EVT_L_SINGLE:
                    qcy_app_cb.key_local.key_short = key_val;
                    xcfg_cb.user_def_ks_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_L_DOUBLE:
                    qcy_app_cb.key_local.key_double = key_val;
                    xcfg_cb.user_def_kd_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_L_THREE:
                    qcy_app_cb.key_local.key_three = key_val;
                    xcfg_cb.user_def_kt_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_L_FOUR:
                    qcy_app_cb.key_local.key_four = key_val;
                    xcfg_cb.user_def_kfour_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_L_LONG:
                    qcy_app_cb.key_local.key_long = key_val;
                    xcfg_cb.user_def_kl_sel = key_val;
                    break;
                // case QCY_CALL_EVT_L_SINGLE:
                //     break;
                // case QCY_CALL_EVT_L_DOUBLE:
                //     break;
                // case QCY_CALL_EVT_L_THREE:
                //     break;
                // case QCY_CALL_EVT_L_FOUR:
                //     break;
                // case QCY_CALL_EVT_L_LONG:
                //     break;
                default:
                    break;
                }
                qcy_key_info_save();
            }
            // 设置右耳
            else if (i%2 == 1 && IS_CHANNEL_LEFT()) {
                TRACE("set right key:%d\n", key_val);
                switch (qcy_evt_fun_tbl[i].evt_id) {
                case QCY_MUSIC_EVT_R_SINGLE:
                    qcy_app_cb.key_local.key_short = key_val;
                    xcfg_cb.user_def_ks_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_R_DOUBLE:
                    qcy_app_cb.key_local.key_double = key_val;
                    xcfg_cb.user_def_kd_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_R_THREE:
                    qcy_app_cb.key_local.key_three = key_val;
                    xcfg_cb.user_def_kt_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_R_FOUR:
                    qcy_app_cb.key_local.key_four = key_val;
                    xcfg_cb.user_def_kfour_sel = key_val;
                    break;
                case QCY_MUSIC_EVT_R_LONG:
                    qcy_app_cb.key_local.key_long = key_val;
                    xcfg_cb.user_def_kl_sel = key_val;
                    break;
                // case QCY_CALL_EVT_R_SINGLE:
                //     break;
                // case QCY_CALL_EVT_R_DOUBLE:
                //     break;
                // case QCY_CALL_EVT_R_THREE:
                //     break;
                // case QCY_CALL_EVT_R_FOUR:
                //     break;
                // case QCY_CALL_EVT_R_LONG:
                //     break;
                default:
                    break;
                }
                qcy_key_info_save();
            }
            // 同步到副耳
            else {
                switch (qcy_evt_fun_tbl[i].evt_id) {
                case QCY_MUSIC_EVT_L_SINGLE:
                case QCY_MUSIC_EVT_R_SINGLE:
                    qcy_app_cb.key_remote.key_short = key_val;
                    break;
                case QCY_MUSIC_EVT_L_DOUBLE:
                case QCY_MUSIC_EVT_R_DOUBLE:
                    qcy_app_cb.key_remote.key_double = key_val;
                    break;
                case QCY_MUSIC_EVT_L_THREE:
                case QCY_MUSIC_EVT_R_THREE:
                    qcy_app_cb.key_remote.key_three = key_val;
                    break;
                case QCY_MUSIC_EVT_L_FOUR:
                case QCY_MUSIC_EVT_R_FOUR:
                    qcy_app_cb.key_remote.key_four = key_val;
                    break;
                case QCY_MUSIC_EVT_L_LONG:
                case QCY_MUSIC_EVT_R_LONG:
                    qcy_app_cb.key_remote.key_long = key_val;
                    break;
                // case QCY_CALL_EVT_L_SINGLE:
                // case QCY_CALL_EVT_R_SINGLE:
                //     break;
                // case QCY_CALL_EVT_L_DOUBLE:
                // case QCY_CALL_EVT_R_DOUBLE:
                //     break;
                // case QCY_CALL_EVT_L_THREE:
                // case QCY_CALL_EVT_R_THREE:
                //     break;
                // case QCY_CALL_EVT_L_FOUR:
                // case QCY_CALL_EVT_R_FOUR:
                //     break;
                // case QCY_CALL_EVT_L_LONG:
                // case QCY_CALL_EVT_R_LONG:
                //     break;
                default:
                    break;
                }
#if QCY_TWS_EN
                TRACE("sync key:%d\n", key_val);
                qcy_tws_set_remote_key(qcy_evt_fun_tbl[i].evt_id, ptr[1]);
#endif
            }
            qcy_app_key_tbl_set();
        }
    }
}

void qcy_bt_message_process(u16 msg)
{
    switch(msg) {
    case EVT_CHARGE_INBOX:
        break;

    default:
        break;
    }
}

AT(.app_text.ble)
void qcy_app_bt_evt_notice(u8 evt, void *params)
{
    //u8 *packet = params;

    switch (evt) {
    case BT_NOTICE_TWS_CONNECTED:
#if QCY_TWS_EN
        if(IS_APP_MASTER()){
            qcy_tws_all_info_sync_req();
        }
#endif
        break;

    case BT_NOTICE_TWS_DISCONNECT:
        qcy_app_cb.vbat_remote = 0;
        qcy_sleep_mode_exit();
        if(ble_is_connect()) {
            qcy_app_vbat_notify();
        }
        qcy_scan_data_update(1);
        break;

    // case BT_NOTICE_CONNECTED:
    //     ble_adv_en();
    //     break;

    // case BT_NOTICE_DISCONNECT:
    //     ble_adv_dis();
    //     break;
    }
}

void qcy_ble_connect_callback(void)
{
    ble_update_conn_param(80, 0, 200);
    // qcy_app_cb.ble_param_update = 1;
}

void qcy_ble_disconnect_callback(void)
{

}

AT(.com_text.qcyapp)
void qcy_app_enter_sleep(void)
{
    TRACE("%s\n", __func__);
}

AT(.com_text.qcyapp)
bool qcy_app_wakeup(void)
{
    return qcy_app_cb.active_flag;
}

AT(.app_text.qcyapp)
static void qcy_app_process_1s(void)
{
    qcy_param_sync();

    if (!QCY_OTA_IS_START()) {
        qcy_app_vbat_process();
    }

    if (qcy_app_cb.ble_param_update) {
        qcy_app_cb.ble_param_update--;
        if (qcy_app_cb.ble_param_update == 0x00){
            ble_update_conn_param(80, 0, 200);
        }
    }

    qcy_app_cb.sec_count++;
    if (qcy_app_cb.sec_count >= 240){
        qcy_app_cb.sec_count = 0;
    }
}

#if QCY_VOL_BALANCE &&  QCY_TWS_EN
u8 qcy_vol_dig2db(u16 dig_vol)
{
    u8 res = 0;
    for (u8 i = 0; i < 61; i++) {
        if (qcy_dvol_tbl_60[i] >= dig_vol) {
            res = 60 - i;
            break;
        }
    }
    return res;
}
#endif

void qcy_device_info_update(void)
{
    if (qcy_app_cb.vol != sys_cb.vol) {
        qcy_app_cb.vol = sys_cb.vol;
        qcy_app_setting_notify(QCY_CMD_VOL);
    }

#if QCY_ANC_EN
    if (qcy_app_cb.anc_mode_set_flag == 1) {
        // 等待设置完成
        if (sys_cb.anc_mode == qcy_app_cb.anc_info.mode) {
            qcy_app_cb.anc_mode_set_flag = 0;
        }
    } else {
        if (ab_anc_def_to_qcy() != qcy_app_cb.anc_info.mode) {
            qcy_app_cb.anc_info.mode = ab_anc_def_to_qcy();
            qcy_app_setting_notify(QCY_CMD_ANC);
        }
    }
#endif

#if USER_TKEY
    if (qcy_app_cb.sleep_mode_set_flag == 1) {
        // 等待设置完成
        if (qcy_app_cb.sleep_mode == IS_SLEEP_MODE()) {
            qcy_app_cb.sleep_mode_set_flag = 0;
        }
    } else {
        if (qcy_app_cb.sleep_mode != IS_SLEEP_MODE()) {
            qcy_app_cb.sleep_mode = IS_SLEEP_MODE();
            qcy_app_setting_notify(QCY_CMD_SLEEP_MODE);
        }
    }
#endif

    if (qcy_app_cb.game_mode_set_flag == 1) {
        // 等待设置完成
        if (qcy_app_cb.game_mode == IS_GAME_MODE()) {
            qcy_app_cb.game_mode_set_flag = 0;
        }
    } else {
        if (qcy_app_cb.game_mode != IS_GAME_MODE()) {
            qcy_app_cb.game_mode = IS_GAME_MODE();
            qcy_app_setting_notify(QCY_CMD_GAME_MODE);
        }
    }

}

AT(.app_text.qcyapp)
void qcy_app_process(void)
{
    static u32 ticks = 0;
#if QCY_TWS_EN
    if (qcy_tws_msg_is_active()){
        qcy_tws_msg_process();
    }
#endif // QCY_TWS_EN

    if (tick_check_expire(ticks, 1000)) {
        ticks = tick_get();
        qcy_app_process_1s();
    }

    if (qcy_app_cb.do_flag & FLAG_EQ_SET) {
        if (!sco_is_connected()) {
            qcy_app_cb.do_flag &= ~FLAG_EQ_SET;
            qcy_eq_set_do();
        }
    }

#if QCY_ANC_EN
    if (qcy_app_cb.do_flag & FLAG_ANC_SET) {
        if (!sco_is_connected()) {
            qcy_app_cb.do_flag &= ~FLAG_ANC_SET;
            qcy_anc_set_do();
        }
    }
#endif

    if (qcy_app_cb.do_flag & FLAG_GAME_MODE_SET) {
        qcy_app_cb.do_flag &= ~FLAG_GAME_MODE_SET;
        if (qcy_app_cb.game_mode == 0x01) {
            bsp_tws_res_music_play(TWS_RES_GAME_MODE);
        } else {
            bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
        }
    }
    qcy_device_info_update();
}

uint16_t qcy_ble_role_switch_get_data(uint8_t *data_ptr)
{
    return 0;
}

uint16_t qcy_ble_role_switch_set_data(uint8_t *data_ptr, uint16_t len)
{
    return 0;
}

void qcy_app_init(void)
{

// VERSION
    // qcy_app_cb.version_local[0] = (QCY_SW_VERSION >> 16) & 0xff;
    // qcy_app_cb.version_local[1] = (QCY_SW_VERSION >> 8) & 0xff;
    // qcy_app_cb.version_local[2] = QCY_SW_VERSION & 0xff;

    u8 version[] = SW_VERSION;

    qcy_app_cb.version_local[0] = version[1] - '0';
    qcy_app_cb.version_local[1] = version[3] - '0';
    qcy_app_cb.version_local[2] = version[5] - '0';

    qcy_app_cb.game_mode = IS_GAME_MODE();
    qcy_app_cb.sleep_mode = 0x02;
#if QCY_TWS_EN
    if (xcfg_cb.bt_tws_lr_mode > 8) {
        tws_lr_xcfg_sel();
    }
#endif
    memcpy(qcy_app_cb.bt_name, bt_get_local_name(), 32);

    qcy_param_flag_load();
// In ear detect
#if QCY_CTL_IN_EAR_EN
    qcy_ear_det_init();
#endif
#if QCY_VOL_BALANCE
//声道平衡
    qcy_vol_balance_init();
#endif
//KEY
    qcy_key_init();
// Earphone mode
    qcy_earphone_mode_init();
#if QCY_ANC_EN
// ANC
    qcy_anc_info_init();
#endif
// EQ
    qcy_eq_init();
// NAME
    qcy_bt_name_init();

    qcy_param_flag_save();
}
#endif


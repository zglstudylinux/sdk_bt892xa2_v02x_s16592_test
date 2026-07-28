#include "include.h"
#include "qcy_app.h"

#if LE_QCY_APP_EN

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#define TRACE_R(...)            print_r(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACE_R(...)
#endif


///////////////////////////////////////////////////////////////////////////

const uint8_t adv_data_const[] = {
    // Name
    0x0b, 0x09, 'Q', '-', 'E', 'a', 'r', 'B', 'u','d', 's', '\0',
    // uuid
    0x03, 0x03, 0xe8, 0xfe,
};

u32 ble_get_scan_data(u8 *scan_buf, u32 buf_size)
{
    memset(scan_buf, 0, buf_size);
    u32 data_len = sizeof(qcy_scan_data);
    memcpy(scan_buf, (u8*)&qcy_scan_data, data_len);
    return data_len;
}

u32 ble_get_adv_data(u8 *adv_buf, u32 buf_size)
{
    memset(adv_buf, 0, buf_size);
    memcpy(adv_buf, adv_data_const, sizeof(adv_data_const));
    return sizeof(adv_data_const);
}

///////////////////////////////////////////////////////////////////////////
#define MAX_NOTIFY_NUM          6
#define MAX_NOTIFY_LEN          185     //max=256
#define NOTIFY_POOL_SIZE       (MAX_NOTIFY_LEN + sizeof(struct txbuf_tag)) * MAX_NOTIFY_NUM

AT(.ble_cache.att)
uint8_t notify_tx_pool[NOTIFY_POOL_SIZE];

void ble_txpkt_init(void)
{
    txpkt_init(&notify_tx, notify_tx_pool, MAX_NOTIFY_NUM, MAX_NOTIFY_LEN);
    notify_tx.send_kick = ble_send_kick;
}


//----------------------------------------------------------------------------
//qcy app service
static const uint8_t qcy_app_primay_uuid16[2]={0x01, 0xa0};
static const gatts_uuid_base_st uuid_qcy_app_primay_base = {
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_app_primay_uuid16,
};

// qcy耳机功能设置
static const uint8_t qcy_func_setting_uuid16[2]={0x01, 0x10};
static const gatts_uuid_base_st uuid_qcy_func_setting_base = {
    .props = ATT_WRITE,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_func_setting_uuid16,
};
static gatts_service_base_st gatts_qcy_func_setting_write_base;
static uint8_t gatt_qcy_func_setting_write_callback(u8 *ptr, u8 len)
{
    TRACE("FUNC SET<-");
    TRACE_R(ptr, len);
    qcy_setting_data_process(ptr, len);
    return 0;
}

// qcy获取耳机功能设置
static const uint8_t qcy_get_func_setting_uuid16[2]={0x02, 0x10};
static const gatts_uuid_base_st uuid_qcy_get_func_setting_base = {
    .props = ATT_NOTIFY | ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_get_func_setting_uuid16,
};

static gatts_service_base_st gatts_qcy_get_func_setting_base;
bool ble_qcy_get_func_setting_send_packet(u8 *buf, u8 len)
{
    TRACE("SET<-");
    TRACE_R(buf, len);

    return ble_tx_notify(gatts_qcy_get_func_setting_base.att_index, buf, len);
}

// qcy耳机事件功能设置
static const uint8_t qcy_event_setting_uuid16[2]={0x0d, 0x00};
static const gatts_uuid_base_st uuid_qcy_event_setting_base = {
    .props = ATT_WRITE | ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_event_setting_uuid16,
};

static gatts_service_base_st gatts_qcy_event_setting_base;
static uint8_t gatt_qcy_event_setting_write_callback(u8 *ptr, u8 len)
{
    TRACE("EVT SET<-");
    TRACE_R(ptr, len);
    qcy_key_set_process(ptr, len);
    return 0;
}

// qcy耳机获取版本号
static const uint8_t qcy_get_version_uuid16[2]={0x07, 0x00};
static const gatts_uuid_base_st uuid_qcy_get_version_base = {
    .props = ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_get_version_uuid16,
};
static gatts_service_base_st gatts_qcy_get_version_base;

// qcy耳机上报状态（电量、充电灯）
static const uint8_t qcy_vbat_sta_uuid16[2]={0x08, 0x00};
static const gatts_uuid_base_st uuid_qcy_vbat_sta_base = {
    .props = ATT_NOTIFY | ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_vbat_sta_uuid16,
};

static gatts_service_base_st gatts_qcy_vbat_sta_base;
bool ble_qcy_report_status_send_packet(u8 *buf, u8 len)
{
    TRACE("STA<-");
    TRACE_R(buf, len);

    return ble_tx_notify(gatts_qcy_vbat_sta_base.att_index, buf, len);
}
#if QCY_CHAR_LANGUAGE
// qcy耳机语言切换
static const uint8_t qcy_language_uuid16[2]={0x09, 0x00};
static const gatts_uuid_base_st uuid_qcy_language_base = {
   .props = ATT_WRITE | ATT_READ,
   .type = BLE_GATTS_UUID_TYPE_16BIT,
   .uuid = qcy_language_uuid16,
};

static gatts_service_base_st gatts_qcy_language_base;
static uint8_t gatt_qcy_language_write_callback(u8 *ptr, u8 len)
{
    TRACE("lan:");
    TRACE_R(ptr, len);
    return true;
}
#endif

// qcy EQ1 EQ系数由耳机计算（在EQ2无法实现的情况下，使用EQ1）
static const uint8_t qcy_eq1_uuid16[2]={0x0b, 0x00};
static const gatts_uuid_base_st uuid_qcy_eq1_base = {
    .props = ATT_WRITE | ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = qcy_eq1_uuid16,
};

static gatts_service_base_st gatts_qcy_eq1_base;
static uint8_t gatt_qcy_eq1_write_callback(u8 *ptr, u8 len)
{
    TRACE("write EQ1:");
    TRACE_R(ptr, len);
    qcy_eq1_data_process(ptr, len);
    return true;
}

#if QCY_CHAR_EQ2
// qcy EQ2 EQ系数由APP计算
static const uint8_t qcy_eq2_uuid16[2]={0x03, 0x10};
static const gatts_uuid_base_st uuid_qcy_eq2_base = {
   .props = ATT_WRITE | ATT_READ,
   .type = BLE_GATTS_UUID_TYPE_16BIT,
   .uuid = qcy_eq2_uuid16,
};

static gatts_service_base_st gatts_qcy_eq2_base;
static uint8_t gatt_qcy_eq2_write_callback(u8 *ptr, u8 len)
{
    TRACE("EQ2:");
    TRACE_R(ptr, len);
    return true;
}
#endif
//----------------------------------------------------------------------------
//FOTA
#if QCY_FOT_EN

const uint8_t app_primay_uuid16[2]={0x12, 0xff};
static const gatts_uuid_base_st uuid_app_primay_base = {
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = app_primay_uuid16,
};

const uint8_t app_notify_uuid16[2]={0x14, 0xff};
static const gatts_uuid_base_st uuid_app_notify_base = {
    .props = ATT_NOTIFY,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = app_notify_uuid16,
};
static gatts_service_base_st gatts_app_notify_base;

const uint8_t fota_uuid16[2]={0x15, 0xff};
static const gatts_uuid_base_st uuid_fota_base = {
    .props = ATT_READ|ATT_WRITE_WITHOUT_RESPONSE,
    .type  = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid  = fota_uuid16,
};
static gatts_service_base_st gatts_fota_base;

static u8 gatt_callback_fota(u8 *ptr, u8 len)
{
    if(fot_app_connect_auth(ptr,len)){
        fot_recv_proc(ptr,len);
    }
    return true;
}

bool ble_fot_send_packet(u8 *buf, u8 len)
{
    return ble_tx_notify(gatts_app_notify_base.att_index, buf, len);
}
#endif // QCY_FOT_EN

u8 ble_att_write_callback(u16 handle, u8 *ptr, u8 len)
{
    if (handle == gatts_qcy_func_setting_write_base.handle) {
        return gatt_qcy_func_setting_write_callback(ptr, len);
    } else if (handle == gatts_qcy_event_setting_base.handle) {
        return gatt_qcy_event_setting_write_callback(ptr, len);
    }
#if QCY_CHAR_LANGUAGE
    else if (handle == gatts_qcy_language_base.handle) {
        return gatt_qcy_language_write_callback(ptr, len);
    }
#endif
    else if (handle == gatts_qcy_eq1_base.handle) {
        return gatt_qcy_eq1_write_callback(ptr, len);
    }
#if QCY_CHAR_EQ2
    else if (handle == gatts_qcy_eq2_base.handle) {
        return gatt_qcy_eq2_write_callback(ptr, len);
    }
#endif
#if QCY_FOT_EN
    else if (handle == gatts_fota_base.handle) {
        return gatt_callback_fota(ptr, len);
    }
#endif // QCY_FOT_EN
    return true;
}

u8 ble_att_read_callback(u16 handle, u8 *ptr, u8 len)
{
    u8 read_data[100];
    u8 offset = 0;
    if (handle == gatts_qcy_get_func_setting_base.handle) {
        TRACE("get settings\n");
        read_data[offset++] = 0xff;
        read_data[offset++] = 0;// 数据总长度
#if QCY_CTL_IN_EAR_EN
        read_data[offset++] = QCY_CMD_IN_EAR_DETECT;
        read_data[offset++] = 1;
        read_data[offset++] = qcy_app_cb.in_ear_detect;
#endif
        read_data[offset++] = QCY_CMD_SLEEP_MODE;
        read_data[offset++] = 1;
        read_data[offset++] = qcy_app_cb.sleep_mode;

        read_data[offset++] = QCY_CMD_ANC;
        read_data[offset++] = 3;
        read_data[offset++] = qcy_app_cb.anc_info.mode;
        if (qcy_app_cb.anc_info.mode == 0x01) {
            read_data[offset++] = 0x01;//子场景模式
            read_data[offset++] = qcy_app_cb.anc_info.anc_on_level;
        } else if (qcy_app_cb.anc_info.mode == 0x03){
            read_data[offset++] = 0x01;//子场景模式
            read_data[offset++] = qcy_app_cb.anc_info.transparency_level;
        } else {
            read_data[offset++] = 0x00;//子场景模式
            read_data[offset++] = 0x00;
        }
        read_data[offset++] = QCY_CMD_VOL;
        read_data[offset++] = 3;
        read_data[offset++] = sys_cb.vol;
        read_data[offset++] = sys_cb.vol;
        read_data[offset++] = VOL_MAX;
        read_data[offset++] = QCY_CMD_GAME_MODE;
        read_data[offset++] = 1;
        read_data[offset++] = qcy_app_cb.game_mode;
        read_data[offset++] = QCY_CMD_EARPHONE_MODE;
        read_data[offset++] = 1;
        read_data[offset++] = qcy_app_cb.earphone_mode;
        read_data[1] = offset - 2;                  // 数据总长度
        memcpy(ptr, read_data, offset);
    } else if (handle == gatts_qcy_event_setting_base.handle) {
        TRACE("get event settings\n");
        memcpy(ptr, (u8*)&qcy_evt_fun_tbl, sizeof(qcy_evt_fun_tbl));
    } else if (handle == gatts_qcy_get_version_base.handle) {
        TRACE("get version\n");
        if (IS_CHANNEL_LEFT()) {
            memcpy(read_data, qcy_app_cb.version_local, QCY_SW_VER_LEN);
            offset += QCY_SW_VER_LEN;
            memcpy(&read_data[offset], qcy_app_cb.version_remote, QCY_SW_VER_LEN);
            offset += QCY_SW_VER_LEN;
        } else {
            memcpy(read_data, qcy_app_cb.version_remote, QCY_SW_VER_LEN);
            offset += QCY_SW_VER_LEN;
            memcpy(&read_data[offset], qcy_app_cb.version_local, QCY_SW_VER_LEN);
            offset += QCY_SW_VER_LEN;
        }
        memcpy(ptr, read_data, offset);
    } else if (handle == gatts_qcy_vbat_sta_base.handle) {
        TRACE("get vbat sta\n");
        qcy_vbat_rsp_data_t qcy_vbat_data;
        qcy_vbat_data.box_charge_sta = 0;
        qcy_vbat_data.box_vbat = qcy_app_cb.vbat_box;
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
        memcpy(ptr, (u8*)&qcy_vbat_data, sizeof(qcy_vbat_data));
    }
#if QCY_CHAR_LANGUAGE
    else if (handle == gatts_qcy_language_base.handle) {
        TRACE("get language\n");
    }
#endif
    else if (handle == gatts_qcy_eq1_base.handle) {
        TRACE("get eq1\n");
        memcpy(ptr, (u8*)&qcy_app_cb.eq_info, sizeof(qcy_app_cb.eq_info));
    }
#if QCY_CHAR_EQ2
    else if(handle == gatts_qcy_eq2_base.handle) {
        TRACE("get eq2\n");
    }
#endif
#if QCY_FOT_EN
    else if(handle == gatts_fota_base.handle) {

    }
#endif // QCY_FOT_EN
    return true;
}
//----------------------------------------------------------------------------
//

void ble_init_att_for_handle(u16 handle, uint8_t *buf, uint16_t len);

void ble_app_gatts_service_init(void)
{
    int ret = 0;

    ble_gatts_init();

    ret = ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                uuid_qcy_app_primay_base.uuid,
                                uuid_qcy_app_primay_base.type,
                                NULL);

    ret = ble_gatts_characteristic_add(uuid_qcy_func_setting_base.uuid,
                                       uuid_qcy_func_setting_base.type,
                                       uuid_qcy_func_setting_base.props,
                                       &gatts_qcy_func_setting_write_base.handle,
                                       &gatts_qcy_func_setting_write_base.att_index,
                                       &gatt_qcy_func_setting_write_callback);

    ret = ble_gatts_characteristic_add(uuid_qcy_get_func_setting_base.uuid,
                                       uuid_qcy_get_func_setting_base.type,
                                       uuid_qcy_get_func_setting_base.props,
                                       &gatts_qcy_get_func_setting_base.handle,
                                       &gatts_qcy_get_func_setting_base.att_index,
                                       NULL);
    ble_init_att_for_handle(gatts_qcy_get_func_setting_base.handle, NULL, sizeof(qcy_fun_setting_rsp_data_t));

    ret = ble_gatts_characteristic_add(uuid_qcy_event_setting_base.uuid,
                                       uuid_qcy_event_setting_base.type,
                                       uuid_qcy_event_setting_base.props,
                                       &gatts_qcy_event_setting_base.handle,
                                       &gatts_qcy_event_setting_base.att_index,
                                       &gatt_qcy_event_setting_write_callback);
    ble_init_att_for_handle(gatts_qcy_event_setting_base.handle,(u8*)&qcy_evt_fun_tbl, sizeof(qcy_evt_fun_tbl));

    ret = ble_gatts_characteristic_add(uuid_qcy_get_version_base.uuid,
                                       uuid_qcy_get_version_base.type,
                                       uuid_qcy_get_version_base.props,
                                       &gatts_qcy_get_version_base.handle,
                                       &gatts_qcy_get_version_base.att_index,
                                       NULL);

    ble_init_att_for_handle(gatts_qcy_get_version_base.handle, NULL, sizeof(qcy_version_rsp_data_t));

    ret = ble_gatts_characteristic_add(uuid_qcy_vbat_sta_base.uuid,
                                       uuid_qcy_vbat_sta_base.type,
                                       uuid_qcy_vbat_sta_base.props,
                                       &gatts_qcy_vbat_sta_base.handle,
                                       &gatts_qcy_vbat_sta_base.att_index,
                                       NULL);
    ble_init_att_for_handle(gatts_qcy_vbat_sta_base.handle, NULL, sizeof(qcy_vbat_rsp_data_t));

#if QCY_CHAR_LANGUAGE
    ret = ble_gatts_characteristic_add(uuid_qcy_language_base.uuid,
                                       uuid_qcy_language_base.type,
                                       uuid_qcy_language_base.props,
                                       &gatts_qcy_language_base.handle,
                                       &gatts_qcy_language_base.att_index,
                                       &gatt_qcy_language_write_callback);
#endif

    ret = ble_gatts_characteristic_add(uuid_qcy_eq1_base.uuid,
                                       uuid_qcy_eq1_base.type,
                                       uuid_qcy_eq1_base.props,
                                       &gatts_qcy_eq1_base.handle,
                                       &gatts_qcy_eq1_base.att_index,
                                       &gatt_qcy_eq1_write_callback);
    ble_init_att_for_handle(gatts_qcy_eq1_base.handle, (u8*)&qcy_app_cb.eq_info, sizeof(qcy_eq_info_t));

#if QCY_CHAR_EQ2
    ret = ble_gatts_characteristic_add(uuid_qcy_eq2_base.uuid,
                                       uuid_qcy_eq2_base.type,
                                       uuid_qcy_eq2_base.props,
                                       &gatts_qcy_eq2_base.handle,
                                       &gatts_qcy_eq2_base.att_index,
                                       &gatt_qcy_eq2_write_callback);
#endif

#if QCY_FOT_EN
    ret = ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                uuid_app_primay_base.uuid,
                                uuid_app_primay_base.type,
                                NULL);            //PRIMARY

    ret = ble_gatts_characteristic_add(uuid_app_notify_base.uuid,
                                       uuid_app_notify_base.type,
                                       uuid_app_notify_base.props,
                                       &gatts_app_notify_base.handle,
                                       &gatts_app_notify_base.att_index,
                                       NULL);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_fota_base.uuid,
                                       uuid_fota_base.type,
                                       uuid_fota_base.props,
                                       &gatts_fota_base.handle,
                                       &gatts_fota_base.att_index,
                                       &gatt_callback_fota);      //characteristic
#endif // QCY_FOT_EN

   if (ret != BLE_GATTS_SUCCESS) {
       TRACE("gatt err: %d\n", ret);
       return;
   }

}

void ble_app_init(void)
{
    qcy_adv_init();
    ble_app_gatts_service_init();
}

#endif

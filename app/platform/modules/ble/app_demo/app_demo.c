#include "include.h"
#include "app_demo.h"

#if LE_APP_EN
///////////////////////////////////////////////////////////////////////////
const uint8_t adv_data_const[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, 0x01, 0x02,
    // Manufacturer Specific Data
    0x03, 0xff, 0x42, 0x06,
};

const uint8_t scan_data_const[] = {
    // Name
    0x08, 0x09, 'B', 'L', 'E', '-', 'B', 'O', 'X',
};

u32 ble_get_scan_data(u8 *scan_buf, u32 buf_size)
{
    memset(scan_buf, 0, buf_size);
    u32 data_len = sizeof(scan_data_const);
    memcpy(scan_buf, scan_data_const, data_len);

    //读取BLE配置的蓝牙名称
    int len;
    len = strlen(xcfg_cb.le_name);
    if (len > 0) {
        memcpy(&scan_buf[2], xcfg_cb.le_name, len);
        data_len = 2 + len;
        scan_buf[0] = len + 1;
    }
    return data_len;
}

u32 ble_get_adv_data(u8 *adv_buf, u32 buf_size)
{
    memset(adv_buf, 0, buf_size);
    u32 data_len = sizeof(adv_data_const);
    memcpy(adv_buf, adv_data_const, data_len);

    return data_len;
}

///////////////////////////////////////////////////////////////////////////
#define MAX_NOTIFY_NUM          8
#define MAX_NOTIFY_LEN          256     //max=256
#define NOTIFY_POOL_SIZE       (MAX_NOTIFY_LEN + sizeof(struct txbuf_tag)) * MAX_NOTIFY_NUM

AT(.ble_cache.att)
uint8_t notify_tx_pool[NOTIFY_POOL_SIZE];

void ble_txpkt_init(void)
{
    txpkt_init(&notify_tx, notify_tx_pool, MAX_NOTIFY_NUM, MAX_NOTIFY_LEN);
    notify_tx.send_kick = ble_send_kick;
}

/***
*
*/
#define BLE_CMD_BUF_LEN     4
#define BLE_CMD_BUF_MASK    (BLE_CMD_BUF_LEN - 1)
#define BLE_RX_BUF_LEN      20

struct ble_cmd_t{
    u8 len;
    u8 buf[BLE_RX_BUF_LEN];
};

struct ble_cb_t {
    struct ble_cmd_t cmd[BLE_CMD_BUF_LEN];
    u8 cmd_rptr;
    u8 cmd_wptr;
} ble_cb;

//----------------------------------------------------------------------------
//app service
const uint8_t app_primay_uuid16[2]={0x12, 0xff};
static const gatts_uuid_base_st uuid_app_primay_base = {
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = app_primay_uuid16,
};

const uint8_t app_write_uuid16[2]={0x13, 0xff};
static const gatts_uuid_base_st uuid_app_write_base = {
    .props = ATT_WRITE,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = app_write_uuid16,
};
static gatts_service_base_st gatts_app_write_base;

const uint8_t app_notify_uuid16[2]={0x14, 0xff};
static const gatts_uuid_base_st uuid_app_notify_base = {
    .props = ATT_NOTIFY,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = app_notify_uuid16,
};
static gatts_service_base_st gatts_app_notify_base;

//----------------------------------------------------------------------------
//app
static uint8_t gatt_write_callback(u8 *ptr, u8 len)
{
    printf("BLE RX [%d]: \n", len);
    print_r(ptr, len);

    u8 wptr = ble_cb.cmd_wptr & BLE_CMD_BUF_MASK;
    ble_cb.cmd_wptr++;
    if (len > BLE_RX_BUF_LEN) {
        len = BLE_RX_BUF_LEN;
    }
    memcpy(ble_cb.cmd[wptr].buf, ptr, len);
    ble_cb.cmd[wptr].len = len;

    return true;
}

void app_demo_process_do(u8 *ptr, u16 size)
{

}

void app_demo_process(void)
{
    if (ble_cb.cmd_rptr == ble_cb.cmd_wptr) {
        return;
    }
    u8 rptr = ble_cb.cmd_rptr & BLE_CMD_BUF_MASK;
    ble_cb.cmd_rptr++;
    u8 *ptr = ble_cb.cmd[rptr].buf;
    u8 len = ble_cb.cmd[rptr].len;

    app_demo_process_do(ptr, len);
}

bool ble_send_packet(u8 *buf, u8 len)
{
    static u32 ble_tick = 0;
    while (!tick_check_expire(ble_tick, 30)) {     //延时30ms再发
        delay_5ms(2);
    }
    ble_tick = tick_get();
    printf("BLE TX [%d]: \n", len);
    print_r(buf, len);
    return (ble_tx_notify(gatts_app_notify_base.att_index, buf, len) == 0);
}

//----------------------------------------------------------------------------
//FOTA
#if LE_APP_FOT_EN
const uint8_t fota_uuid16[2]={0x15, 0xff};
static const gatts_uuid_base_st uuid_fota_base = {
    .props = ATT_READ|ATT_WRITE_WITHOUT_RESPONSE,
    .type  = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid  = fota_uuid16,
};
static gatts_service_base_st gatts_fota_base;

static u8 gatt_callback_fota(u8 *ptr, u8 len)
{
    if(fot_app_connect_auth(ptr,len,FOTA_CON_BLE)){
        fot_recv_proc(ptr,len);
    }
    return true;
}

bool ble_fot_send_packet(u8 *buf, u8 len)
{
    return ble_tx_notify(gatts_app_notify_base.att_index, buf, len);
}
#endif

//----------------------------------------------------------------------------
//
void ble_app_gatts_service_init(void)
{
    int ret = 0;

    ble_gatts_init();

    ret = ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                uuid_app_primay_base.uuid,
                                uuid_app_primay_base.type,
                                NULL);            //PRIMARY

    ret = ble_gatts_characteristic_add(uuid_app_write_base.uuid,
                                       uuid_app_write_base.type,
                                       uuid_app_write_base.props,
                                       &gatts_app_write_base.handle,
                                       &gatts_app_write_base.att_index,
                                       &gatt_write_callback);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_app_notify_base.uuid,
                                       uuid_app_notify_base.type,
                                       uuid_app_notify_base.props,
                                       &gatts_app_notify_base.handle,
                                       &gatts_app_notify_base.att_index,
                                       NULL);      //characteristic

#if LE_APP_FOT_EN
    ret = ble_gatts_characteristic_add(uuid_fota_base.uuid,
                                       uuid_fota_base.type,
                                       uuid_fota_base.props,
                                       &gatts_fota_base.handle,
                                       &gatts_fota_base.att_index,
                                       &gatt_callback_fota);      //characteristic
#endif

    if(ret != BLE_GATTS_SUCCESS){
        printf("gatt err: %d\n", ret);
        return;
    }
}


//----------------------------------------------------------------------------
//
void ble_app_init(void)
{
    ble_app_gatts_service_init();
}

#endif // LE_APP_EN

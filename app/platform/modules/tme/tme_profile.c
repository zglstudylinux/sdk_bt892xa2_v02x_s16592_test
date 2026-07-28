#include "include.h"
#include "tme_app.h"

#if TME_APP_EN
///////////////////////////////////////////////////////////////////////////
ALIGNED(4)
AT(.rodata.sdp.tbl)
const uint8_t sdp_spp_tme_service_record[] = {
    0x36, 0x00, 0x54, 0x09, 0x00, 0x00, 0x0a, 0x50, 0x01, 0x00, 0x02, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x03, 0x19, 0xfd, 0x90, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0e, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00,
    0x36, 0x00, 0x05, 0x19, 0x00, 0x03, 0x08, 0x05, 0x09, 0x00, 0x05, 0x36, 0x00, 0x03, 0x19, 0x10,
    0x02, 0x09, 0x00, 0x06, 0x36, 0x00, 0x09, 0x09, 0x65, 0x6e, 0x09, 0x00, 0x6a, 0x09, 0x01, 0x00,
    0x09, 0x00, 0x09, 0x36, 0x00, 0x09, 0x36, 0x00, 0x06, 0x19, 0xfd, 0x90, 0x09, 0x01, 0x02, 0x09,
    0x01, 0x00, 0x25, 0x03, 0x53, 0x50, 0x50,
};

uint8_t adv_data_buf[31] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, 0x01, 0x02,
    //len,      BID,       PID,                 MAC                            status     cookie                         version
    0x17, 0xff, 0x11,0x27, 0xA6,0x86,0x01,0x00, 0xBE,0xF6,0x0f,0x39,0xD0,0x88, 0x01,0x00, 0xcc,0xcc,0xcc,0xcc,0xcc,0xcc, 0x01,0x00,
    //UUID
    0x03, 0x03, 0x90,0xFD,
};

const uint8_t scan_data_const[] = {
    //TX Power Level
    0x02, 0x0A, 0xC5,
    //Name
    0x04, 0x09, 'B','L','E',
};

u32 ble_get_scan_data(u8 *scan_buf, u32 buf_size)
{
    memset(scan_buf, 0, buf_size);
    u32 data_len = sizeof(scan_data_const);
    memcpy(scan_buf, scan_data_const, data_len);

    scan_buf[2] = 0xC0;     //TX Power Level  -64dbm

    //读取BLE配置的蓝牙名称
    int len;
    len = strlen(xcfg_cb.le_name);
    if (len > 0) {
        memcpy(&scan_buf[5], xcfg_cb.le_name, len);
        data_len = 5 + len;
        scan_buf[3] = len + 1;
    }
    return data_len;
}

u32 ble_get_adv_data(u8 *adv_buf, u32 buf_size)
{
    u8 i = 0;
    u8 edr_addr[6];
    u8 edr_addr_temp[6];
    u32 pid = tme_pid_get();

    memset(adv_buf, 0, buf_size);
    u32 data_len = sizeof(adv_data_buf);

    bt_get_local_bd_addr(edr_addr_temp);

    for(i=0;i<6;i++){
        edr_addr[i] = edr_addr_temp[5-i];     //大小端转换
    }

    //print_r(edr_addr,6);

    memcpy(&adv_data_buf[7], &pid, 4);

    memcpy(&adv_data_buf[11], edr_addr, 6);

    memcpy(&adv_data_buf[25], tme_firmware_version_get(), 2);

    memcpy(adv_buf, adv_data_buf, data_len);

    return data_len;
}


///////////////////////////////////////////////////////////////////////////
#define MAX_NOTIFY_NUM          6
#define MAX_NOTIFY_LEN          200     //max=256
#define NOTIFY_POOL_SIZE       (MAX_NOTIFY_LEN + sizeof(struct txbuf_tag)) * MAX_NOTIFY_NUM

AT(.ble_cache.att)
uint8_t notify_tx_pool[NOTIFY_POOL_SIZE];

void ble_txpkt_init(void)
{
    txpkt_init(&notify_tx, notify_tx_pool, MAX_NOTIFY_NUM, MAX_NOTIFY_LEN);
    notify_tx.send_kick = ble_send_kick;
}

const uint8_t tme_primay_uuid128[16]={0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x90,0xFD,0x00,0x00};
static const gatts_uuid_base_st uuid_tme_primay_base = {
    .type = BLE_GATTS_UUID_TYPE_128BIT,
    .uuid = tme_primay_uuid128,
};

const uint8_t tme_notify_uuid128[16]={0xce, 0x45,0xbc,0xa3,0x8c,0xfc,0x26,0x83,0xe6,0x45,0x10,0x18,0x43,0x59,0x23,0xca};
static const gatts_uuid_base_st uuid_tme_notify_base = {
    .props = ATT_NOTIFY | ATT_READ,
    .type = BLE_GATTS_UUID_TYPE_128BIT,
    .uuid = tme_notify_uuid128,
};
static gatts_service_base_st gatts_tme_notify_base;

const uint8_t tme_write_uuid128[16]={0x9b, 0x2c,0x65,0x1b,0xb2,0xc1,0xa2,0x83,0x13,0x4b,0x10,0x18,0x53,0x53,0x74,0x68};
static const gatts_uuid_base_st uuid_tme_write_base = {
    .props = ATT_WRITE,
    .type = BLE_GATTS_UUID_TYPE_128BIT,
    .uuid = tme_write_uuid128,
};

static gatts_service_base_st gatts_tme_write_base;

static u8 gatt_callback_tme_write(u8 *ptr, u8 len)
{
    tme_recv_proc(ptr,len);

    return true;
}


bool ble_send_packet(u8 *buf, u8 len)
{
    static u32 ble_tick = 0;

    while (!tick_check_expire(ble_tick, 20)) {
        delay_5ms(2);
    }
    ble_tick = tick_get();

    u16 remain_len = len;

    u8 *p_data = buf;

    u16 mtu = ble_get_gatt_mtu();

    bool result = false;

    while(remain_len){
        if(remain_len > mtu){
            result = ble_tx_notify(gatts_tme_notify_base.att_index, p_data, mtu);
            remain_len -= mtu;
            p_data += mtu;
            delay_5ms(4);
        }else{
            result = ble_tx_notify(gatts_tme_notify_base.att_index, p_data, remain_len);
            remain_len = 0;
        }
    }

    return result;
}


//----------------------------------------------------------------------------
//
void ble_app_gatts_service_init(void)
{
    int ret = 0;

    ble_gatts_init();

    ret = ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                uuid_tme_primay_base.uuid,
                                uuid_tme_primay_base.type,
                                NULL);            //PRIMARY

    ret = ble_gatts_characteristic_add(uuid_tme_notify_base.uuid,
                                       uuid_tme_notify_base.type,
                                       uuid_tme_notify_base.props,
                                       &gatts_tme_notify_base.handle,
                                       &gatts_tme_notify_base.att_index,
                                       NULL);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_tme_write_base.uuid,
                                       uuid_tme_write_base.type,
                                       uuid_tme_write_base.props,
                                       &gatts_tme_write_base.handle,
                                       &gatts_tme_write_base.att_index,
                                       &gatt_callback_tme_write);      //characteristic


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

#endif // TME_APP_EN

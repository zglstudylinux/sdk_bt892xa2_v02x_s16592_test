#include "include.h"
#include "wk_app.h"
#include "wk_ota.h"

#if LE_WK_APP_EN

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#define TRACE_R(...)            print_r(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACE_R(...)
#endif

static u8 ble_frame_puts(u8 handle, u8 *cmd, u8 len);
static ble_massage_st ble_massge_frame  AT(.ble_buf.app);

///////////////////////////////////////////////////////////////////////////
const uint8_t adv_data_const[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, 0x01, 0x02,
    // Manufacturer Specific Data
    0x10, 0xff, 0x00, 0x4c, 0x31, 0x41, 0x42, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

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
    memcpy(adv_buf, adv_data_const, sizeof(adv_data_const));

    adv_buf[5]          = ADV_TYPE_NORMAL;
    *(u32 *)(adv_buf+8) = PRODUCT_ID;
    *(u16 *)(adv_buf+12) = LE_ADV_EXT;
    bt_get_local_bd_addr(adv_buf+14);

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
//wk app service
static const uint8_t wk_app_primay_uuid16[2]={0xf1, 0xaa};
static const gatts_uuid_base_st uuid_wk_app_primay_base = {
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = wk_app_primay_uuid16,
};

static const uint8_t wk_cmd_uuid16[2]={0xf2, 0xaa};
static const gatts_uuid_base_st uuid_wk_cmd_base = {
    .props = ATT_WRITE,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = wk_cmd_uuid16,
};
static uint8_t gatt_wk_app_write_callback(u8 *ptr, u8 len)
{
    ble_frame_puts(WK_APP_CMD, ptr, len);
    return true;
}

static const uint8_t wk_cmd_ack_uuid16[2]={0xf3, 0xaa};
static const gatts_uuid_base_st uuid_wk_cmd_ack_base = {
    .props = ATT_INDICATE,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = wk_cmd_ack_uuid16,
};
static gatts_service_base_st gatts_wk_cmd_ack_base;
bool ble_wk_app_send_packet(u8 *buf, u8 len)
{
    TRACE("ACK<-");
    TRACE_R(buf, len);

    return ble_tx_notify(gatts_wk_cmd_ack_base.att_index, buf, len);
}


static const uint8_t wk_ota_uuid16[2]={0xf4, 0xaa};
static const gatts_uuid_base_st uuid_wk_ota_base = {
    .props = ATT_WRITE_WITHOUT_RESPONSE,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = wk_ota_uuid16,
};
static uint8_t gatt_wk_ota_callback(u8 *ptr, u8 len)
{
    wk_ota_recv_proc(ptr,len);
    return true;
}

static const uint8_t wk_voice_uuid16[2]={0xf5, 0xaa};
static const gatts_uuid_base_st uuid_wk_voice_base = {
    .props = ATT_NOTIFY,
    .type = BLE_GATTS_UUID_TYPE_16BIT,
    .uuid = wk_voice_uuid16,
};
static gatts_service_base_st gatts_wk_voice_base;
bool ble_wk_voice_send_packet(u8 *buf, u8 len)
{
    TRACE("VCE<-");
    TRACE_R(buf, len);

    return ble_tx_notify(gatts_wk_voice_base.att_index, buf, len);
}


//----------------------------------------------------------------------------
//
void ble_app_gatts_service_init(void)
{
    int ret = 0;

    ble_gatts_init();

    ret = ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                uuid_wk_app_primay_base.uuid,
                                uuid_wk_app_primay_base.type,
                                NULL);            //PRIMARY

    ret = ble_gatts_characteristic_add(uuid_wk_cmd_base.uuid,
                                       uuid_wk_cmd_base.type,
                                       uuid_wk_cmd_base.props,
                                       NULL,
                                       NULL,
                                       &gatt_wk_app_write_callback);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_wk_cmd_ack_base.uuid,
                                       uuid_wk_cmd_ack_base.type,
                                       uuid_wk_cmd_ack_base.props,
                                       &gatts_wk_cmd_ack_base.handle,
                                       &gatts_wk_cmd_ack_base.att_index,
                                       NULL);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_wk_ota_base.uuid,
                                       uuid_wk_ota_base.type,
                                       uuid_wk_ota_base.props,
                                       NULL,
                                       NULL,
                                       &gatt_wk_ota_callback);      //characteristic

    ret = ble_gatts_characteristic_add(uuid_wk_voice_base.uuid,
                                       uuid_wk_voice_base.type,
                                       uuid_wk_voice_base.props,
                                       &gatts_wk_voice_base.handle,
                                       &gatts_wk_voice_base.att_index,
                                       NULL);      //characteristic


   if(ret != BLE_GATTS_SUCCESS){
       printf("gatt err: %d\n", ret);
       return;
   }

}

void ble_app_init(void)
{
    ble_app_gatts_service_init();

    memset(&ble_massge_frame, 0, sizeof(ble_massge_frame));
}

//-------------------------------------------------------------
//
static u8 ble_frame_puts(u8 index, u8 *cmd, u8 len)
{
    static u8 i = 0;
    u8 *ptr;
    reset_sleep_delay();
    wkapp_cb.active_flag = 1;

    if(ble_massge_frame.num >= MAX_MASSAGE_NUM){
        printf("err massage full\n");
        return true;
    }

    for(;i<MAX_MASSAGE_NUM; i++){
        if(ble_massge_frame.msg[i] == 0){
            break;
        }
    }
    if(i >= MAX_MASSAGE_NUM){
        for(i=0; i<MAX_MASSAGE_NUM; i++){
            if(ble_massge_frame.msg[i] == 0){
                break;
            }
        }
    }

    if(i >= MAX_MASSAGE_NUM){
        return true;
    }

    ptr = (u8 *)mem_malloc(len+2);

    if(ptr != NULL){
        memcpy(ptr+2, cmd, len);
        ptr[0] = index;
        ptr[1] = len;

        ble_massge_frame.msg[i] = (u32)ptr;
        ble_massge_frame.num++;
    }

    return true;
}

AT(.com_text.ble)
u8 frame_ble_num_get(void)
{
    return ble_massge_frame.num;
}


AT(.app_text.ble)
void frame_ble_proce(void)
{
    static u8 i = 0;
    u8 *ptr;

    for(; i<MAX_MASSAGE_NUM; i++){
        if(ble_massge_frame.msg[i] != 0){
            break;
        }
    }
    if(i >= MAX_MASSAGE_NUM){
        for(i=0; i<MAX_MASSAGE_NUM; i++){
            if(ble_massge_frame.msg[i] != 0){
                break;
            }
        }
    }

    if(i >= MAX_MASSAGE_NUM){
        ble_massge_frame.num--;
        return;
    }

    ptr = (void *)ble_massge_frame.msg[i];

    if(ptr[0] == WK_APP_CMD){
        wk_app_cmd_write(ptr+2, ptr[1]);
    }

    mem_free(ptr);

    ble_massge_frame.msg[i] = 0;
    ble_massge_frame.num--;
}

#endif


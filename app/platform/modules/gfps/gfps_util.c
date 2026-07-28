#include "include.h"

#if GFPS_EN

#define GFPS_DISCON_2_DISCOVER        0

#define GFPS_FIND_DEV_RING_INTERVAL   3000 // GFPS查找设备响铃的间隔，单位ms

enum{
    GFPS_CMD_RING_ON = 0xA0,
    GFPS_CMD_RING_OFF,
    GFPS_CMD_BAT_REQ,
    GFPS_CMD_BAT_RSP,
    GFPS_CMD_BT_NAME,
    GFPS_CMD_ACCOUNT_KEY,
    GFPS_CMD_FACTORY_RESET
};

enum{
    GFPS_ADV_TO_DISCOVER = 1,
    GFPS_ADV_TO_NON_DISCOVER,
};

enum{
    GFPS_RING_STATUS_NONE,
    GFPS_RING_STATUS_ON,
    GFPS_RING_STATUS_OFF
};

typedef struct{
    u8 ring_do;
    u8 remote_ring_do;
    u8 ring_timeout;
    u8 sec_count;
    u8 vol;
    u8 bat_local;
    u8 bat_house;
    u8 bat_remote;
    u8 bat_update;
    bool bat_display;
    u8 wakeup;
    u32 tick;
    u32 find_dev_ring_tick;
    u8 adv_do;      //0: not do   1:to discoverable mode  2:to non_discoverable mode
    u8 adv_cnt;
}gfps_var_t;

typedef struct{
    u8 data[93];
    u16 len;
}gfps_tws_sync_t;

// ---------------------------------------------------------------------------
uint hfp_get_bat_level_ex(void);

// ---------------------------------------------------------------------------
// Google Fast Pair Service型号ID和ECC私钥，注册后得到
//ECC私钥需进行base64解码，推荐网站：http://docs.gizwits.com/zh-cn/tools/Base64_encode_decode.html

// Model ID
static const uint8_t fast_pair_model_id[3] = {
#if BT_TWS_EN
    0x46, 0x17, 0x80
#else
    0x66, 0xFA, 0x80
#endif // BT_TWS_EN
};

// ECC Private Key
static const uint8_t ecc_private_key[32] = {
#if BT_TWS_EN
    0x8D, 0x99, 0x05, 0x36, 0xE8, 0xE0, 0xB6, 0x73, 0x83, 0x09, 0x14, 0x19, 0xC8, 0x45, 0x4B, 0x07,
    0x19, 0xBE, 0xA3, 0x1A, 0x7C, 0xC4, 0x46, 0x73, 0xD8, 0x98, 0x19, 0x66, 0xF0, 0xF9, 0xA6, 0xC7
#else
    0xde, 0x67, 0xbb, 0xc4, 0xaf, 0x91, 0x2b, 0x52, 0xba, 0xef, 0xf7, 0x0e, 0x5f, 0x64, 0xef, 0xc7,
    0x44, 0x6e, 0xa5, 0x40, 0xb4, 0xa9, 0x6f, 0x58, 0xda, 0x6c, 0x09, 0xa2, 0x65, 0xe6, 0xd4, 0x66
#endif // BT_TWS_EN
};

static gfps_var_t gfps_var AT(.gfps.var);
static gfps_tws_sync_t gfps_tws_sync AT(.gfps.var);

const uint8_t *get_fast_pair_model_id(void)
{
    return fast_pair_model_id;
}

const uint8_t *get_ecc_private_key(void)
{
    return ecc_private_key;
}

// ---------------------------------------------------------------------------

// 这两个一定要实现，不然ECDH慢到怀疑人生

void run_at_highest_speed(void)
{
    // 设置最高速度
    sys_clk_req(INDEX_GFPS, SYS_160M);
}

void run_at_normal_speed(void)
{
    // 设置普通速度
    sys_clk_free(INDEX_GFPS);
}

//---------------------------------------------------------------------------

static bool gfps_tws_data_sync(u8 cmd, u8* val, u16 len)
{
    if(bt_tws_is_connected()){
        gfps_tws_sync.data[0] = cmd;
        if(len <= (sizeof(gfps_tws_sync.data) - 1)){ //cmd占用了1Byte
            memcpy(&gfps_tws_sync.data[1], val, len);
        }else{
            printf("gfps tws sync data len overflow !!!\n");
        }
        gfps_tws_sync.len = 1 + len;
        bt_tws_sync_custom_data();
        return true;
    }
    return false;
}

uint16_t google_fast_pair_tws_get_data(uint8_t *buf)
{
    memcpy(buf, gfps_tws_sync.data, gfps_tws_sync.len);

    return gfps_tws_sync.len;
}

static void google_fast_pair_bt_name_sync(u8 *name, u8 len)
{
    gfps_tws_data_sync(GFPS_CMD_BT_NAME, name, len);
}

static void google_fast_pair_account_key_sync(u8 *data, u8 len)
{
    gfps_tws_data_sync(GFPS_CMD_ACCOUNT_KEY, data, len);
}


static void google_fast_pair_factory_reset_sync(u8 *data, u8 len)
{
    gfps_tws_data_sync(GFPS_CMD_FACTORY_RESET, data, len);
}

// ---------------------------------------------------------------------------
static uint8_t get_flash_account_key_data_len(void)
{
    return sizeof(struct account_key_list);
}

static bool account_key_read_do(void)
{
    account_key_list_t account_key_data = (account_key_list_t)get_account_key_info();
    uint8_t account_key_data_len = *(uint8_t *)account_key_data; // 第一个字节是结构体大小
    uint8_t flash_data_len = get_flash_account_key_data_len();

    if (account_key_data_len != flash_data_len) {
        printf("Re(%d,%d)\n", account_key_data_len, flash_data_len);
        WDT_RST();
        return false;
    }

    cm_read(account_key_data, GFPS_PAGE(PARAM_ACCOUNT_KEY), account_key_data_len);

    if(account_key_data->count > ACCOUNT_KEY_MAX_COUNT){
        account_key_data->count = 0;
    }

//    printf("%s:",__func__);
//    print_r(account_key_data,account_key_data_len);
//    printf("\n");
    return true;
}

bool account_key_write_do(void *data, uint8_t data_len)
{
    uint8_t flash_data_len = get_flash_account_key_data_len();

    if (data_len != flash_data_len) {
        printf("We(%d,%d)\n", data_len, flash_data_len);
        return false;
    }

    cm_write(data, GFPS_PAGE(PARAM_ACCOUNT_KEY), data_len);
    cm_sync();
//    printf("%s:",__func__);
//    print_r(data,data_len);
//    printf("\n");

    if(!bt_tws_is_slave()){
        google_fast_pair_account_key_sync(data, data_len);
    }

    return true;
}

void read_personalized_name_do(uint8_t *data, uint8_t *data_len)
{
    uint8_t flash_name_len = cm_read8(GFPS_PAGE(PARAM_PERSONALIZED_NAME_LEN));

    if (flash_name_len == 0) {
        // 如果flash还没有保存个性化名称，可以返回默认名称，或者留空
        return;
    }

    if (*data_len > flash_name_len) {
        *data_len = flash_name_len;
    }
    cm_read(data, GFPS_PAGE(PARAM_PERSONALIZED_NAME), *data_len);
}

bool write_personalized_name_do(uint8_t *data, uint8_t data_len)
{
    cm_write8(GFPS_PAGE(PARAM_PERSONALIZED_NAME_LEN), data_len);
    cm_write(data, GFPS_PAGE(PARAM_PERSONALIZED_NAME), data_len);
    cm_sync();

    if(!bt_tws_is_slave()){
        google_fast_pair_bt_name_sync(data, data_len);
    }

    return true;
}

///复位个性化名称、account key
void reset_personalized_name_account_key(void)
{
    cm_clear(GFPS_PAGE(0));

    if(bt_tws_is_connected()){
        google_fast_pair_factory_reset_sync(NULL,0);
    }
}


// ---------------------------------------------------------------------------

bool is_support_tws(void)
{
    return xcfg_cb.bt_tws_en && BT_TWS_EN;
}

bool is_in_bluetooth_silence_mode(void)
{
    return false;
}

static void device_bat_info_sent(void)
{
    printf("device_bat_info_sent:0x%x,0x%x\n",gfps_var.bat_local,gfps_var.bat_remote);

#if BT_TWS_EN
    if(sys_cb.tws_left_channel){
        send_msg_dev_info_battery_updated(gfps_var.bat_local, 0, gfps_var.bat_remote, 0, gfps_var.bat_house, 0);
    }else{
        send_msg_dev_info_battery_updated(gfps_var.bat_remote, 0, gfps_var.bat_local, 0, gfps_var.bat_house, 0);
    }
#else
    u8 bat_info[3];
    u8 bat_info_len=0;

    bat_info[bat_info_len++] = 0x00;
    bat_info[bat_info_len++] = 0x01;///bat data len
    bat_info[bat_info_len++] = gfps_var.bat_local;

    send_msg_dev_info_battery_updated2(bat_info, bat_info_len);
#endif // BT_TWS_EN

    if(!ble_is_connect()){
        switch_ble_adv_to_non_discoverable_mode(false);
    }
}

static void device_ring_status_sent(void)
{
    bool ring_do = (gfps_var.ring_do == GFPS_RING_STATUS_ON) ? 1 : 0;
#if BT_TWS_EN
    bool remote_ring_do = (gfps_var.remote_ring_do == GFPS_RING_STATUS_ON) ? 1 : 0;
    if(sys_cb.tws_left_channel){
        send_msg_device_action_ring2(ring_do, remote_ring_do); // 两个组件（对耳）
    }else{
        send_msg_device_action_ring2(remote_ring_do, ring_do); // 两个组件（对耳）
    }
#else
    send_msg_device_action_ring(ring_do);
#endif // BT_TWS_EN
}

void device_ring(bool left_ring, bool right_ring, uint16_t timeout)
{
    // todo: 响铃(if not support dual, use right_ring)
    printf("-->device_ring:%d, %d, %d\n",left_ring, right_ring, timeout);

    if(timeout){
        gfps_var.ring_timeout = timeout;
    }else{
        gfps_var.ring_timeout = 0xff;
    }
#if BT_TWS_EN
    if(left_ring){
        if(sys_cb.tws_left_channel){
            if(gfps_var.ring_do != GFPS_RING_STATUS_ON){
                gfps_var.ring_do = GFPS_RING_STATUS_ON;
                gfps_var.vol = xcfg_cb.vol_max / 2;
            }
        }else{
            if(gfps_tws_data_sync(GFPS_CMD_RING_ON, &gfps_var.ring_timeout, 1)){
                gfps_var.remote_ring_do = GFPS_RING_STATUS_ON;
            }
        }
    }else{
        if(sys_cb.tws_left_channel){
            gfps_var.ring_do = GFPS_RING_STATUS_OFF;
        }else{
            if(gfps_tws_data_sync(GFPS_CMD_RING_OFF, NULL, 0)){
                gfps_var.remote_ring_do = GFPS_RING_STATUS_OFF;
            }
        }
    }

    if(right_ring){
        if(sys_cb.tws_left_channel){
            if(gfps_tws_data_sync(GFPS_CMD_RING_ON, &gfps_var.ring_timeout, 1)){
                gfps_var.remote_ring_do = GFPS_RING_STATUS_ON;
            }
        }else{
            if(gfps_var.ring_do != GFPS_RING_STATUS_ON){
                gfps_var.ring_do = GFPS_RING_STATUS_ON;
                gfps_var.vol = xcfg_cb.vol_max / 2;
            }
        }
    }else{
        if(sys_cb.tws_left_channel){
            if(gfps_tws_data_sync(GFPS_CMD_RING_OFF, NULL, 0)){
                gfps_var.remote_ring_do = GFPS_RING_STATUS_OFF;
            }
        }else{
            gfps_var.ring_do = GFPS_RING_STATUS_OFF;
        }
    }
#else
    if(left_ring | right_ring){
        if(gfps_var.ring_do != GFPS_RING_STATUS_ON){
            gfps_var.ring_do = GFPS_RING_STATUS_ON;
            gfps_var.vol = xcfg_cb.vol_max / 2;
        }
    }else{
            gfps_var.ring_do = GFPS_RING_STATUS_OFF;
    }
#endif // BT_TWS_EN

    // todo: 响铃后，使用以下API通知手机同步响铃状态
//    void send_msg_device_action_ring(bool ring);                        // 单个组件
    device_ring_status_sent();

}

#if BT_TWS_EN
/// TWS使用
bool get_active_status_left_ear(void)
{
    if(sys_cb.tws_left_channel || bt_tws_is_connected()){
        return true;
    }

    return false;
}

bool get_active_status_right_ear(void)
{
    if(!sys_cb.tws_left_channel || bt_tws_is_connected()){
        return true;
    }

    return false;
}
#else
/// 非TWS使用
bool get_active_status(void)
{
    return true;
}
#endif // BT_TWS_EN

void gen_acc_key_filter_get_bat_info(u8 *bat_info, u8 *bat_info_len, u8 *bat_ui_display)
{
#if BT_TWS_EN
    if(sys_cb.tws_left_channel){
        bat_info[0] = gfps_var.bat_local;
        bat_info[1] = gfps_var.bat_remote;
        bat_info[2] = gfps_var.bat_house;
    }else{
        bat_info[0] = gfps_var.bat_remote;
        bat_info[1] = gfps_var.bat_local;
        bat_info[2] = gfps_var.bat_house;
    }
    *bat_info_len = 3;

#else
    bat_info[0] = gfps_var.bat_local;
    *bat_info_len = 1;
#endif // BT_TWS_EN

    *bat_ui_display = gfps_var.bat_display;
}

AT(.com_text.gfps.wakeup)
bool google_fast_pair_need_wakeup(void)
{
    return gfps_var.wakeup;
}


static void google_fast_pair_vbat_check_process(void)
{
    static u8 local_bat = 0;
    static u8 remote_bat = 0x7F;
    static u8 box_bat = 0x7F;
    u8 sync = 1;

#if VUSB_SMART_VBAT_HOUSE_EN
    gfps_var.bat_house = bsp_vhouse_get_house_bat_level();
    gfps_var.bat_local = sys_cb.loc_bat;
    if((sys_cb.loc_bat & BIT(7)) && vhouse_bat_is_ready()){
        gfps_var.bat_remote = sys_cb.rem_bat;
        sync = 0;
    }
#else
    gfps_var.bat_local = hfp_get_bat_level_ex();
#endif

    if((local_bat != gfps_var.bat_local) || (box_bat != gfps_var.bat_house)){
        local_bat = gfps_var.bat_local;
        box_bat = gfps_var.bat_house;
        if(sync){
            gfps_tws_data_sync(GFPS_CMD_BAT_REQ, &gfps_var.bat_local, 2);
        }
        gfps_var.bat_update = 1;
    }

    if(remote_bat != gfps_var.bat_remote){
        remote_bat = gfps_var.bat_remote;
        gfps_var.bat_update = 1;
    }

    if(gfps_var.bat_update){
        gfps_var.bat_update = 0;
        if(spp_custom1_connected() && gfps_var.bat_display){
            device_bat_info_sent();
        }
    }
}

static void google_fast_pair_ring_process(void)
{
    if(gfps_var.ring_do != GFPS_RING_STATUS_NONE){
        if((gfps_var.ring_do == GFPS_RING_STATUS_ON) && gfps_var.ring_timeout){
            reset_sleep_delay();
            bsp_piano_warning_play(WARNING_TONE, 0);
            if(gfps_var.vol < xcfg_cb.vol_max){
                gfps_var.vol += 2;
                bsp_change_volume(gfps_var.vol);
            }
        }else{
            if(gfps_var.ring_do == GFPS_RING_STATUS_OFF){
                if(sys_cb.incall_flag & INCALL_FLAG_SCO){
                    bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
                }else{
                    bsp_change_volume(sys_cb.vol);
                }
                gfps_var.ring_do = GFPS_RING_STATUS_NONE;
            }
        }
    }
}

static void google_fast_pair_ring_timeout_check_process(void)
{
    if((gfps_var.ring_do == GFPS_RING_STATUS_ON) && gfps_var.ring_timeout){
        if(gfps_var.ring_timeout != 0xff){
            gfps_var.ring_timeout--;
        }
    }
}

static void google_fast_pair_adv_process(void)
{
    if(gfps_var.adv_cnt){
        gfps_var.adv_cnt--;
    }else{
        if(gfps_var.adv_do == GFPS_ADV_TO_DISCOVER){
            switch_ble_adv_to_discoverable_mode(false);
        }else if(gfps_var.adv_do == GFPS_ADV_TO_NON_DISCOVER){
            switch_ble_adv_to_non_discoverable_mode(false);
        }
        gfps_var.adv_do = 0;
    }
}

static void google_fast_pair_process_do(void)
{
    google_fast_pair_vbat_check_process();

    if(gfps_var.ring_do != GFPS_RING_STATUS_NONE){
        google_fast_pair_ring_timeout_check_process();
    }

    if(gfps_var.adv_do && !bt_tws_is_slave()){
        google_fast_pair_adv_process();
    }

    gfps_var.sec_count++;

    if(gfps_var.sec_count >= 240){
        gfps_var.sec_count = 0;
    }

    gfps_var.wakeup = 0;
}

void google_fast_pair_process(void)
{
    if(tick_check_expire(gfps_var.tick, 1000)){
        gfps_var.tick = tick_get();

        google_fast_pair_process_do();
    }

    if(tick_check_expire(gfps_var.find_dev_ring_tick, GFPS_FIND_DEV_RING_INTERVAL)){
        gfps_var.find_dev_ring_tick = tick_get();
        
        google_fast_pair_ring_process();
    }
}

void google_fast_pair_tws_set_data(uint8_t *data, uint16_t len)
{
    u8 cmd = data[0];
    u8 *data_ptr = &data[1];
    u8 data_len = len - 1;

    gfps_var.wakeup = 1;

    switch(cmd){
        case GFPS_CMD_RING_ON:
            if(gfps_var.ring_do != GFPS_RING_STATUS_ON){
                gfps_var.ring_do = GFPS_RING_STATUS_ON;
                gfps_var.ring_timeout = data[1];
                gfps_var.vol = xcfg_cb.vol_max / 2;
                bsp_change_volume(gfps_var.vol);
            }
            break;

        case GFPS_CMD_RING_OFF:
            gfps_var.ring_do = GFPS_RING_STATUS_OFF;
            break;

        case GFPS_CMD_BAT_REQ:
            gfps_var.bat_remote = data[1];
            if(data[2] != 0x7F){
                gfps_var.bat_house = data[2];
            }
#if VUSB_SMART_VBAT_HOUSE_EN
            sys_cb.rem_bat = gfps_var.bat_remote;
            sys_cb.rem_house_bat = gfps_var.bat_house;
#endif
            break;

        case GFPS_CMD_BT_NAME:
            write_personalized_name_do(data_ptr, data_len);
            break;

        case GFPS_CMD_ACCOUNT_KEY:
            account_key_write_do(data_ptr, data_len);
            account_key_read_do();
            break;

        case GFPS_CMD_FACTORY_RESET:
            cm_clear(GFPS_PAGE(0));
            break;

        default:
            break;
    }
}

void google_fast_pair_evt_notice(u8 evt, void *params)
{
    u8 *packet = params;

    switch(evt){
        case BT_NOTICE_DISCONNECT:
            fp_bt_disconnected_callback();
#if GFPS_DISCON_2_DISCOVER
            gfps_var.adv_do = GFPS_ADV_TO_DISCOVER;
            gfps_var.adv_cnt = 4;
#endif
            break;

        case BT_NOTICE_CONNECTED:
            fp_bt_connected_callback();
#if GFPS_DISCON_2_DISCOVER
            gfps_var.adv_do = 0;
#endif
            break;

        case BT_NOTICE_TWS_CONNECTED:
            if(bt_tws_is_slave()){
                gfps_var.adv_do = 0;
            }
            gfps_var.bat_local = hfp_get_bat_level_ex();
            gfps_tws_data_sync(GFPS_CMD_BAT_REQ, &gfps_var.bat_local, 2);
            break;

        case BT_NOTICE_TWS_DISCONNECT:
            gfps_var.bat_remote = 0x7F;
            gfps_var.remote_ring_do = GFPS_RING_STATUS_OFF;
            if(spp_custom1_connected() == 0){
                if(gfps_var.ring_do == GFPS_RING_STATUS_ON){
                    gfps_var.ring_do = GFPS_RING_STATUS_OFF;
                }
                if(gfps_var.bat_display){
                    switch_ble_adv_to_non_discoverable_mode(false);
                }
            }else{
                if(gfps_var.bat_display){
                    device_ring_status_sent();
                    device_bat_info_sent();
                }
            }
            break;

        case BT_NOTICE_TWS_ROLE_CHANGE:
            if(packet[0] == 0){     //role switch to master,adv change to non discoverable mode
                gfps_var.adv_do = GFPS_ADV_TO_NON_DISCOVER;
                gfps_var.adv_cnt = 1;
            }
            break;

        case BT_NOTICE_CONNECT_FAIL:
            gfps_var.adv_do = GFPS_ADV_TO_DISCOVER;
            gfps_var.adv_cnt = 0;
            break;

        default:
            break;
    }
}

void google_fast_pair_init(void)
{
    memset(&gfps_var, 0, sizeof(gfps_var));

    gfps_var.bat_local = hfp_get_bat_level_ex();
    gfps_var.bat_remote = 0x7F;     //0x7F表示未知电量
    gfps_var.bat_house = 0x7F;
    gfps_var.bat_display = true;

    account_key_read_do();
}

void google_fast_pair_factory_reset(void)
{
    reset_personalized_name_account_key();
}

AT(.text.gg.fp)
void fp_spp_send_do(uint8_t *data, uint16_t data_len)
{
    bt_spp_custom1_tx(data, data_len);
}

bool google_fast_pair_is_enable(void)
{
    return true;
}

void google_fast_pair_spp_recv_proc(uint8_t *data, uint16_t data_len)
{
    fp_spp_receive_callback(data, data_len);

    gfps_var.wakeup = 1;
}

void google_fast_pair_spp_connected_callback(void)
{
    gfps_var.bat_update = 1;

    fp_spp_connected_callback();
}

void google_fast_pair_ble_connect_callback(void)
{
    gfps_var.wakeup = 1;

    fp_ble_connected_callback();
}

void google_fast_pair_ble_disconnect_callback(void)
{
    fp_ble_disconnected_callback();
}

u8 lc_get_lmp_rsp_timeout(void)
{
    return 10;
}

bool lc_lmp_is_timeout_2_detach(void)
{
    return true;
}

AT(.text.gg.fp)
void google_fast_pair_adv_init(void)
{
    if(bt_nor_get_link_info(NULL)) {
        //gfps_var.adv_do = GFPS_ADV_TO_NON_DISCOVER;
    }else{
        gfps_var.adv_do = GFPS_ADV_TO_DISCOVER;
    }
    gfps_var.adv_cnt = 1;
}

// ---------------------------------------------------------------------------

void google_fast_pair_adv_2_discoverable_mode(void)
{
#if BT_TWS_EN
    if(!bt_tws_is_slave()){
        if(bt_nor_is_connected()){
            bt_nor_disconnect(); // 先断开手机
        }
    }
#else
    if(bt_is_connected()){
        bt_disconnect(0xff);
    }
#endif

    printf("GFPS start\n");
    switch_ble_adv_to_discoverable_mode(false);
}

void gfps_bat_ui_open(void)
{
    if(!gfps_var.bat_display){
        gfps_var.bat_display = true;
        if(gfps_get_cur_adv_mode() == GFPS_MODE_NON_DISCOVER){
            switch_ble_adv_to_non_discoverable_mode(false);
        }
    }
}

void gfps_bat_ui_close(void)
{
    if(gfps_var.bat_display){
        gfps_var.bat_display = false;
        if(gfps_get_cur_adv_mode() == GFPS_MODE_NON_DISCOVER){
            switch_ble_adv_to_non_discoverable_mode(false);
        }
    }
}

#if VUSB_SMART_VBAT_HOUSE_EN
void google_fast_pair_vhouse_cmd_notice(u8 cmd)
{
    switch(cmd){
    case VHOUSE_CMD_OPEN_WINDOW:
        gfps_bat_ui_open();
        break;

    case VHOUSE_CMD_CLOSE_WINDOW:
        gfps_bat_ui_close();
        break;
    }
}
#endif

#endif

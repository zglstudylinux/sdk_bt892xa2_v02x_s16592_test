#ifndef _WK_APP_H_
#define _WK_APP_H_

#if LE_WK_APP_EN
//
#define BLE_FRMAE_MEM_SIZE          512
#define MAX_MASSAGE_NUM             40
#define VBAT_UPDATE_TIME            60          //second
#define IS_APP_MASTER()             (!bt_tws_is_slave())
#define WK_CM_IS_EXIST()            (wkapp_cb.param_flag == PARAM_FLAG)

//const
#define PARAM_FLAG          0x5AC35AC5
#define SN_LEN              12
#define PRODUCT_ID          0x01004241
#define LE_ADV_EXT          0x0000

//function config
#define WK_CTL_TONE_EN          0
#define WK_CTL_ANC_EN           ANC_EN
#define WK_CTL_IN_EAR_EN        USER_INEAR_DETECT_EN


//key config
#define KEY_SHORT_LEFT_DEF      UDK_PLAY_PAUSE
#define KEY_SHORT_RIGHT_DEF     UDK_PLAY_PAUSE
#define KEY_DOUBLE_LEFT_DEF     UDK_REDIALING
#define KEY_DOUBLE_RIGHT_DEF    UDK_REDIALING
#define KEY_THREE_LEFT_DEF      UDK_NONE
#define KEY_THREE_RIGHT_DEF     UDK_NONE
#define KEY_FOUR_LEFT_DEF       UDK_NONE
#define KEY_FOUR_RIGHT_DEF      UDK_NONE
#define KEY_FIVE_LEFT_DEF       UDK_NONE
#define KEY_FIVE_RIGHT_DEF      UDK_NONE
#define KEY_LONG_LEFT_DEF       UDK_SIRI
#define KEY_LONG_RIGHT_DEF      UDK_SIRI

//Cm Addr Config
#define WK_CM_SN_CRC_ADDR           0
#define WK_CM_SN_CRC_LEN            2                     //2bytes

#define WK_CM_SN_ADDR               (WK_CM_SN_CRC_ADDR+WK_CM_SN_CRC_LEN)
#define WK_CM_SN_LEN                (SN_LEN+SN_LEN)     //24bytes

#define WK_CM_KEY_ADDR              (WK_CM_SN_ADDR+WK_CM_SN_LEN)
#define WK_CM_KEY_LEN               (6)

#define WK_CM_OFF_TIME_ADDR         (WK_CM_KEY_ADDR+WK_CM_KEY_LEN)
#define WK_CM_OFF_TIME_LEN          (1)

#define WK_CM_EAR_DET_ADDR          (WK_CM_OFF_TIME_ADDR+WK_CM_OFF_TIME_LEN)
#define WK_CM_EAR_DET_LEN           (1)

#define WK_CM_EQ_ADDR               (WK_CM_EAR_DET_ADDR+WK_CM_EAR_DET_LEN)
#define WK_CM_EQ_LEN                (1)

#define WK_CM_FLAG_ADDR             (WK_CM_EQ_ADDR + WK_CM_EQ_LEN)
#define WK_CM_FLAG_LEN              (4)

#define WK_CM_BT_NAME_SIZE_ADDR     (WK_CM_FLAG_ADDR + WK_CM_FLAG_LEN)
#define WK_CM_BT_NAME_SIZE_LEN      (1)
#define WK_CM_BT_NAME_ADDR          (WK_CM_BT_NAME_SIZE_ADDR + WK_CM_BT_NAME_SIZE_LEN)
#define WK_CM_BT_NAME_LEN           (32)
//---------------------------------------------------------------------------------

typedef struct __attribute__((packed)) _ble_massage {
	u32 msg[MAX_MASSAGE_NUM];
    volatile u8  num;
}ble_massage_st;


enum {
    ADV_TYPE_NORMAL,
};

enum {
    WK_APP_CMD,
    WK_APP_OTA,
};

enum {
    TWS_CHANNEL_RIGHT = 0x00,
    TWS_CHANNEL_LEFT,
};

enum {
    WK_CMD_AUTH_REQ = 0x10,
    WK_CMD_AUTH_RSP = 0x11,
    WK_CMD_SN_REQ   = 0x12,
    WK_CMD_SN_RSP   = 0x13,

    WK_CMD_PLAY     = 0x20,
    WK_CMD_PAUSE    = 0x21,
    WK_CMD_NEXT     = 0x22,
    WK_CMD_PRVE     = 0x23,
    WK_CMD_VOL      = 0x24,

    WK_CMD_SET_L_SHORT  = 0x30,     //左耳点击
    WK_CMD_SET_L_DOUBLE = 0x31,     //左耳双击
    WK_CMD_SET_L_THREE  = 0x32,     //左耳三击
    WK_CMD_SET_L_FOUR   = 0x33,     //左耳四击
    WK_CMD_SET_L_FIVE   = 0x34,     //左耳五击
    WK_CMD_SET_L_LONG   = 0x35,     //左耳长按
    WK_CMD_SET_R_SHORT  = 0x36,     //右耳点击
    WK_CMD_SET_R_DOUBLE = 0x37,     //右耳双击
    WK_CMD_SET_R_THREE  = 0x38,     //右耳三击
    WK_CMD_SET_R_FOUR   = 0x39,     //右耳四击
    WK_CMD_SET_R_FIVE   = 0x3A,     //右耳五击
    WK_CMD_SET_R_LONG   = 0x3B,     //右耳长按

    WK_CMD_SET_POWEROFF_TIME  = 0x40,     //自动关机时间
    WK_CMD_SET_FACTORY        = 0x41,     //恢复出厂化设置
    WK_CMD_SET_WORK_MODE      = 0x42,     //工作模式
    WK_CMD_SET_EAR_DET        = 0x43,     //入耳检测
    WK_CMD_SET_EQ_MODE        = 0x44,     //EQ模式
    WK_CMD_SET_LANGUAGE       = 0x45,     //语言类型
    WK_CMD_SET_LED            = 0x46,     //LED
    WK_CMD_SET_FIND_EAR       = 0x47,     //查找耳机
    WK_CMD_SET_ANC_MODE       = 0x48,     //降噪模式
    WK_CMD_SET_BT_NAME        = 0x49,     //经典蓝牙名称设置

    WK_CMD_GET_VBAT           = 0x50,     //获取电池电量
    WK_CMD_GET_VERSION        = 0x51,     //获取固件版本号
    WK_CMD_GET_EDR_NAME       = 0x52,     //获取经典蓝牙的名字
    WK_CMD_GET_L_KEY          = 0x53,     //获取左耳按键配置
    WK_CMD_GET_R_KEY          = 0x54,     //获取右耳按键配置
    WK_CMD_GET_EQ             = 0x55,     //获取EQ配置
    WK_CMD_GET_VOL            = 0x56,     //获取音量
    WK_CMD_GET_PLAY_STA       = 0x57,     //获取播放状态
    WK_CMD_GET_WORK_MODE      = 0x58,     //获取工作模式
    WK_CMD_GET_EAR_DET        = 0x59,     //获取入耳检测是否打开
    WK_CMD_GET_LANGUAGE       = 0x5A,     //获取语言设置
    WK_CMD_GET_ANC            = 0x5B,     //获取降噪模式
    WK_CMD_GET_TWS            = 0x5C,     //获取TWS
    WK_CMD_GET_MTU            = 0x5D,     //获取一次最大传输大小
    WK_CMD_GET_LED            = 0x5E,     //获取LED状态
    WK_CMD_GET_OFF_TIME       = 0x5F,     //获取关机时间


    WK_CMD_OTA_START_REQ      = 0xA0,
    WK_CMD_OTA_START_RSP,
    WK_CMD_OTA_DATA_START,
    WK_CMD_OTA_DATA_CONTINUE,
    WK_CMD_OTA_STA,

    WK_CMD_ACK_SUCCEED = 0xF0,
    WK_CMD_ACK_FAIL    = 0xF1,
};

typedef struct __attribute__((packed)) {
    u8 key_short;
    u8 key_double;
    u8 key_three;
    u8 key_four;
    u8 key_five;
    u8 key_long;
}key_info_st;

typedef struct __attribute__((packed)) {
	u8 sn_l[SN_LEN];
	u8 sn_r[SN_LEN];

    key_info_st key_local;
    key_info_st key_remote;

    u8 poweroff_time;
    u8 work_mode;
    u8 led_en;
    u8 find_ear;
    u8 anc_mode;
    u8 play_sta;
    u8 vol;
    u8 lang_id;

    u8 vbat_local;
    u8 vbat_remote;
    u8 vbat_box;
    u8 version[4];
    char bt_name[32];

    u32 param_flag;
    u8 param_save_update;
    u8 sec_count;
    u8 ble_param_update;
    u8 active_flag;
}wkapp_cb_st;


extern wkapp_cb_st wkapp_cb;
extern const u8 app_cmd_id[4];

//-----------------------------------------------------
void mem_init(uint32_t free_memory_start, uint32_t free_memory_end);
void * mem_malloc(uint32_t size);
void mem_free(void *ptr);

void wk_app_auth_op(u8 *input, u8 *output);
uint calc_crc(void *buf, uint len, uint seed);


uint hfp_get_bat_level_ex(void);


//-----------------------------------------------------
bool ble_wk_app_send_packet(u8 *buf, u8 len);
bool ble_wk_voice_send_packet(u8 *buf, u8 len);

u8 frame_ble_num_get(void);
void frame_ble_proce(void);
void wk_app_cmd_write(u8 *buff, u8 len);

void tws_send_msg_data(u8 cmd, uint8_t *data, uint16_t len);
bool wk_tws_msg_is_active(void);
void wk_tws_msg_process(void);

void wk_bt_msg_process(u8 evt, void *params);

bool wk_tws_get_remote_key(u8 cmd);
bool wk_tws_set_remote_key(u8 cmd, u8 key_vlaue);

void wk_app_find_ear(u8 cmd);
void wk_app_find_ear_process(void);

void wk_app_vbat_process(void);
void wk_app_vbat_notify(void);

void wk_app_state_sync(void);
void wk_app_enter_sleep(void);
bool wk_app_need_wakeup(void);

void wk_sn_init(void);

void wk_key_init(void);
void wk_key_info_save(void);


void wk_poweroff_time_init(void);
void wk_poweroff_time_save(u8 time);

void wk_ear_det_init(void);
void wk_ear_det_save(u8 ear_en);

void wk_eq_init(void);
void wk_eq_save(u8 eq);

void wk_bt_name_init(void);
void wk_bt_name_save(u8 *name, u8 len);

void wk_device_reset_2_factory(void);

void wk_app_process(void);
void wk_app_init(void);
void wk_app_bt_evt_notice(u8 evt, void *params);

void wk_flash_write(void *buf, u32 addr, uint len);
void wk_flash_read(void *buf, u32 addr, uint len);

void wk_ble_connect_callback(void);
void wk_ble_disconnect_callback(void);
uint16_t wk_ble_role_switch_get_data(uint8_t *data_ptr);
uint16_t wk_ble_role_switch_set_data(uint8_t *data_ptr, uint16_t len);

#endif

#endif

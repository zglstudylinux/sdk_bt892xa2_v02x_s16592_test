#ifndef _QCY_APP_H_
#define _QCY_APP_H_
#include "include.h"

#if LE_QCY_APP_EN

// function config
#define QCY_CTL_IN_EAR_EN           USER_INEAR_DETECT_EN
#define QCY_ANC_EN                  ANC_EN                  //ANC控制
#define QCY_VOL_BALANCE             0
#define QCY_TWS_EN                  BT_TWS_EN
#define QCY_FOT_EN                  LE_APP_FOT_EN

// 由于QCY协议占用8个特征值，蓝牙库最大只支持8个，省略
#define QCY_CHAR_LANGUAGE           0
#define QCY_CHAR_EQ2                0

//固件版本号设置
#define QCY_SW_VERSION              0x010002 //当前固件版本号,V1.0.2,0x010201对应V1.2.1

//const
#define PARAM_FLAG                  0x5AC3

#define VBAT_UPDATE_TIME            60      // second
#define QCY_SW_VER_LEN              3       // QCY软件版本号长度
#define QCY_EVT_FUN_TBL_LEN         10      // 按键功能设置表长度
#define QCY_KEY_MAP_TAB_LEN         10      // 按键功能映射表长度
#define QCY_EQ_MAP_TAB_LEN          7       // EQ功能映射表长度
#define QCY_EQ_BAND_CNT             10      // EQ段数
#define QCY_DB_VOL0                 -60     // 音量为0时DB值
#define QCY_ANC_VAL_MIN             -20     // ANC最小gain值

#define FLAG_EQ_SET                 BIT(0)
#define FLAG_DEVICE_RESET           BIT(1)
#define FLAG_BT_LINK_INFO_CLEAR     BIT(2)
#define FLAG_TWS_STA_NOTIFY         BIT(3)
#define FLAG_GAME_MODE_SET          BIT(4)
#define FLAG_ANC_SET                BIT(5)

#define IS_APP_MASTER()             (!bt_tws_is_slave())
#if QCY_TWS_EN
#define IS_CHANNEL_LEFT()           func_bt_tws_get_channel()
#else
#define IS_CHANNEL_LEFT()           1
#endif
#define IS_GAME_MODE()              (bt_is_low_latency()?1:2)
#define IS_CAN_BE_SCAN()            (bt_get_scan()&0x01)
#define IS_CAN_BE_CON()             ((bt_get_scan()>>1)&0x01)
#if USER_TKEY
#define IS_SLEEP_MODE()             (!tk_cb.tk_en?1:2)
#endif

#define QCY_COLOR_IS_BLACK()        1
#define QCY_CM_IS_EXIST()           (qcy_app_cb.param_flag == PARAM_FLAG)
#if QCY_FOT_EN
#define QCY_OTA_IS_START()          is_fot_start()
#else
#define QCY_OTA_IS_START()          0
#endif


//key config
#define KEY_SHORT_LEFT_DEF          UDK_NONE
#define KEY_SHORT_RIGHT_DEF         UDK_NONE
#define KEY_DOUBLE_LEFT_DEF         UDK_PLAY_PAUSE
#define KEY_DOUBLE_RIGHT_DEF        UDK_PLAY_PAUSE
#define KEY_THREE_LEFT_DEF          UDK_SIRI
#define KEY_THREE_RIGHT_DEF         UDK_NEXT
#define KEY_FOUR_LEFT_DEF           UDK_VOL_DOWN
#define KEY_FOUR_RIGHT_DEF          UDK_VOL_UP
#define KEY_FIVE_LEFT_DEF           UDK_DUT
#define KEY_FIVE_RIGHT_DEF          UDK_DUT
#define KEY_LONG_LEFT_DEF           UDK_LOW_LATENCY
#define KEY_LONG_RIGHT_DEF          UDK_NR

//Cm Addr Config

#define QCY_CM_FLAG_ADDR             0
#define QCY_CM_FLAG_LEN              (2)

#define QCY_CM_KEY_ADDR              (QCY_CM_FLAG_ADDR + QCY_CM_FLAG_LEN)
#define QCY_CM_KEY_LEN               (6)

#define QCY_CM_EAR_DET_ADDR          (QCY_CM_KEY_ADDR + QCY_CM_KEY_LEN)
#define QCY_CM_EAR_DET_LEN           (1)

#define QCY_CM_EQ_ADDR               (QCY_CM_EAR_DET_ADDR + QCY_CM_EAR_DET_LEN)
#define QCY_CM_EQ_LEN                (11)

#define QCY_CM_EARPHONE_MODE_ADDR    (QCY_CM_EQ_ADDR + QCY_CM_EQ_LEN)
#define QCY_CM_EARPHONE_MODE_LEN     (1)

#define QCY_CM_ANC_ADDR              (QCY_CM_EARPHONE_MODE_ADDR + QCY_CM_EARPHONE_MODE_LEN)
#define QCY_CM_ANC_LEN               (3)

#define QCY_CM_BT_NAME_SIZE_ADDR     (QCY_CM_ANC_ADDR + QCY_CM_ANC_LEN)
#define QCY_CM_BT_NAME_SIZE_LEN      (1)
#define QCY_CM_BT_NAME_ADDR          (QCY_CM_BT_NAME_SIZE_ADDR + QCY_CM_BT_NAME_SIZE_LEN)
#define QCY_CM_BT_NAME_LEN           (32)

#define QCY_CM_VOL_BALANCE_ADDR      (QCY_CM_BT_NAME_ADDR + QCY_CM_BT_NAME_LEN)
#define QCY_CM_VOL_BALANCE_LEN       (1)

enum {
    QCY_CMD_DEFAULT                 = 0x01, // 还原默认设置
    QCY_CMD_CLEAR_PAIR              = 0x02, // 清除配对记录
    QCY_CMD_RECOVERY                = 0x03, // 恢复出厂设置
    QCY_CMD_PLAY_PAUSE              = 0x04, // 音乐播放暂停
    QCY_CMD_LED_BLINK               = 0x05, // 耳机灯闪烁
    QCY_CMD_IN_EAR_DETECT           = 0x06, // 入耳检测
    QCY_CMD_DENOISE                 = 0x07, // 降噪
    QCY_CMD_VOL                     = 0x08, // 耳机音量设置
    QCY_CMD_GAME_MODE               = 0x09, // 低延时(游戏)模式
    QCY_CMD_LISTEN_MODE             = 0x0A, // 兼听功能
    QCY_CMD_TOUCH_SENSITIVITY       = 0x0B, // 触控灵敏度调节
    QCY_CMD_EARPHONE_MODE           = 0x0C, // 耳机模式
    QCY_CMD_TOUCH_TEST_MODE         = 0x0D, // 触控测试模式
    QCY_CMD_OTA                     = 0x0E, // 进入OTA模式
    QCY_CMD_TIME_SYNC               = 0x0F, // 时间同步
    QCY_CMD_SLEEP_MODE              = 0x10, // 睡眠模式
    QCY_CMD_COMPACTNESS_DETECT      = 0x11, // 贴合度检测
    QCY_CMD_LED_MODE_SW             = 0x12, // lED模式开关
    QCY_CMD_READ_CLASIC_ADDR        = 0x13, // 读取经典蓝牙地址
    QCY_CMD_CHANNEL_BALANCE         = 0x16, // 声道平衡
    QCY_CMD_MULTI_POINT_CON         = 0x15, // 多点连接
    QCY_CMD_ANC                     = 0x17, // ANC功能
    QCY_CMD_SET_BT_NAME             = 0x18, // 修改配对名
    QCY_CMD_CHANGE_TONE             = 0x19, // 开始切换提示音
    QCY_CMD_GET_TONE_STR            = 0x1a, // 获取提示音字符串标识
    QCY_CMD_SHUTDOWN_TIMER          = 0x14, // 定时关机
    QCY_CMD_GET_SETTING             = 0xfE, // APP获取耳机设定数据
};

// 按键事件类型
enum {
    QCY_MUSIC_EVT_L_SINGLE          = 0x01,
    QCY_MUSIC_EVT_R_SINGLE          = 0x02,
    QCY_MUSIC_EVT_L_DOUBLE          = 0x03,
    QCY_MUSIC_EVT_R_DOUBLE          = 0x04,
    QCY_MUSIC_EVT_L_THREE           = 0x05,
    QCY_MUSIC_EVT_R_THREE           = 0x06,
    QCY_MUSIC_EVT_L_FOUR            = 0x07,
    QCY_MUSIC_EVT_R_FOUR            = 0x08,
    QCY_MUSIC_EVT_L_LONG            = 0x09,
    QCY_MUSIC_EVT_R_LONG            = 0x0A,

    QCY_CALL_EVT_L_SINGLE           = 0x41,
    QCY_CALL_EVT_R_SINGLE           = 0x42,
    QCY_CALL_EVT_L_DOUBLE           = 0x43,
    QCY_CALL_EVT_R_DOUBLE           = 0x44,
    QCY_CALL_EVT_L_THREE            = 0x45,
    QCY_CALL_EVT_R_THREE            = 0x46,
    QCY_CALL_EVT_L_FOUR             = 0x47,
    QCY_CALL_EVT_R_FOUR             = 0x48,
    QCY_CALL_EVT_L_LONG             = 0x49,
    QCY_CALL_EVT_R_LONG             = 0x4A,
};

// 按键功能设置
enum {
    QCY_EVT_FUN_NONE                = 0x00,
    QCY_EVT_FUN_PP                  = 0x01,
    QCY_EVT_FUN_PREV                = 0x02,
    QCY_EVT_FUN_NEXT                = 0x03,
    QCY_EVT_FUN_SIRI                = 0x04,
    QCY_EVT_FUN_VOL_UP              = 0x05,
    QCY_EVT_FUN_VOL_DOWN            = 0x06,
    QCY_EVT_FUN_GAME_MODE           = 0x07,
    QCY_EVT_FUN_LISTEN_MODE         = 0x0A,
    QCY_EVT_FUN_ANC_CHANGE          = 0x0B,
    QCY_EVT_FUN_EQ_CHANGE           = 0x0C,
    QCY_EVT_FUN_EQ_SWITCH           = 0x0D,

    QCY_EVT_FUN_EQ_DEFAULT          = 0x51,
    QCY_EVT_FUN_EQ_SHOT             = 0x52,
    QCY_EVT_FUN_EQ_FOOTSTEPS        = 0x53,
    QCY_EVT_FUN_EQ_BURST            = 0x54,
    QCY_EVT_FUN_EQ_VEHICLE          = 0x55,
    QCY_EVT_FUN_EQ_AIRPLANE         = 0x56,
    QCY_EVT_FUN_EQ_BROKEN           = 0x57,
};

// EQ设置种类
enum {
    QCY_EQ_CUSTOM                   = 0xFF,// 自定义EQ

    QCY_EQ_DEFAULT                  = 0x01,// 默认
    QCY_EQ_POP                      = 0x02,// 流行
    QCY_EQ_MEGA_BASS                = 0x03,// 重低音
    QCY_EQ_ROCK                     = 0x04,// 摇滚
    QCY_EQ_SOFT                     = 0x05,// 柔和
    QCY_EQ_CLASSICAL                = 0x06,// 古典

    QCY_EQ_GAME_POP                 = 0x10,
    QCY_EQ_GAME_MEGA_BASS           = 0x11,
    QCY_EQ_GAME_ROCK                = 0x12,
    QCY_EQ_GAME_SOFT                = 0x13,
    QCY_EQ_GAME_CLASSICAL           = 0x14,

    QCY_EQ_GAME_DEFAULT             = 0x41,// 游戏模式默认
    QCY_EQ_GAME_SHOT                = 0x42,// 枪声
    QCY_EQ_GAME_FOOTSTEPS           = 0x43,// 脚步声
    QCY_EQ_GAME_BURST               = 0x44,// 爆破声
    QCY_EQ_GAME_VEHICLE             = 0x45,// 载具声
    QCY_EQ_GAME_AIRPLANE            = 0x46,// 飞机声
    QCY_EQ_GAME_BROKEN              = 0x47,// 破窗声

};

//  耳机模式
enum {
    QCY_MODE_ANC_ON                 = 0x01,// ANC 0N
    QCY_MODE_ANC_OFF                = 0x02,// ANC OFF
    QCY_MODE_TRANSPARENCY           = 0x03,// 通透
    QCY_MODE_OUTDOORS               = 0x04,// 户外
    QCY_MODE_AIRPLANE               = 0x0A,// 飞机
    QCY_MODE_TRAIN                  = 0x0B,// 火车
    QCY_MODE_SUBWAY                 = 0x0C,// 地铁
    QCY_MODE_BUS                    = 0x0D,// 公交
    QCY_MODE_WALK                   = 0x0E,// 步行
    QCY_MODE_OFFICE                 = 0x0F,// 办公室
    QCY_MODE_ROOM                   = 0x10,// 房间
    QCY_MODE_COFFEE_HOUSE           = 0x11,// 咖啡厅
    QCY_MODE_TALK                   = 0x12,// 对话
    QCY_MODE_SQUARE                 = 0x13,// 广场
    QCY_MODE_BAR                    = 0x14,// 酒吧
    QCY_MODE_CONCERT                = 0x15,// 演唱会
    QCY_MODE_SLEEP                  = 0x16,// 睡眠
};

typedef struct __attribute__((packed)) {

    u8 len;                             // 长度
    u8 type_manufacturer;               // 产商信息 0xff
    u8 manufacturer_id[2];              // 耳机产商标识
    u8 earphone_id[2];                  // 产品ID,由QCY分配
    u8 checksum;                        // 校验字节

    u8 tws_is_left              : 1;    // 左右耳机标志位 1:左耳 0:右耳
    u8 color_is_black           : 1;    // 耳机颜色标志位 1:黑色 0:白色
    u8 reserved                 : 2;
    u8 left_discover_mode       : 1;    // 左耳机可发现标志位 1:可发现 0:不可发现
    u8 right_discover_mode      : 1;    // 右耳机可发现标志位 1:可发现 0:不可发现
    u8 left_connect_mode        : 1;    // 左耳机可连接模式
    u8 right_connect_mode       : 1;    // 右耳机可连接模式

    u8 reserved1                : 1;
    u8 box_is_open              : 1;    // 充电盒是否开盖状态标志位 1:开盖 0:关闭
    u8 connect_sta              : 1;    // 耳机连接标志位
    u8 pair_flag                : 1;    // 耳机是否有配对记录
    u8 mac_encrypt_flag         : 1;    // mac地址段是否加密
    u8 outside_flag             : 1;    // 耳机是否被拿出 1:拿出了盒子 0:在盒子里面
    u8 tws_connect_sta          : 1;    // 是否成功连上TWS
    u8 reserved2                : 1;

    u8 left_vbat                : 7;    // 左耳机电量
    u8 left_charge_sta          : 1;    // 左耳机充电状态标志位

    u8 right_vbat               : 7;    // 右耳机电量
    u8 right_charge_sta         : 1;    // 右耳机充电状态标志位
    
    u8 box_vbat                 : 7;    // 盒子电量
    u8 box_charge_sta           : 1;    // 盒子充电状态标志位

    u8 master_mac_lap[3];               // 主机（配对手机）的mac地址LAP
    u8 master_changed_mac[6];           // 主耳机经过转变后的mac地址
    u8 adv_count;                       // 广播计数 1-255 循环
    u8 slave_changed_mac[6];            // 副耳机经过转变后的mac地址

} qcy_scan_rsp_data_t;

typedef struct __attribute__((packed)) {

    u8 left_version[3];
    u8 right_version[3];

} qcy_version_rsp_data_t;

typedef struct __attribute__((packed)) {

    u8 left_vbat                : 7;    // 左耳机电量
    u8 left_charge_sta          : 1;    // 左耳机充电状态标志位

    u8 right_vbat               : 7;    // 右耳机电量
    u8 right_charge_sta         : 1;    // 右耳机充电状态标志位

    u8 box_vbat                 : 7;    // 盒子电量
    u8 box_charge_sta           : 1;    // 盒子充电状态标志位

} qcy_vbat_rsp_data_t;

typedef struct __attribute__((packed)) {

    u8 evt_id;
    u8 fun_id;

}qcy_evt_fun_t;

typedef struct __attribute__((packed)) {

    u8 head;
    u8 len_total;
#if QCY_CTL_IN_EAR_EN
    u8 cmd_in_ear_detect;
    u8 len_in_ear_detect;
    u8 data_in_ear_detect;
#endif
    u8 cmd_sleep_mode;
    u8 len_sleep_mode;
    u8 data_sleep_mode;
    u8 cmd_anc;
    u8 len_anc;
    u8 data_anc_sub_mode;
    u8 data_anc_mode;
    u8 data_anc_level;
    u8 cmd_vol;
    u8 len_vol;
    u8 data_left_vol;
    u8 data_right_vol;
    u8 data_vol_max;
    u8 cmd_game_mode;
    u8 len_game_mode;
    u8 data_game_mode;
    u8 cmd_earphone_mode;
    u8 len_earphone_mode;
    u8 data_earphone_mode;
} qcy_fun_setting_rsp_data_t;

typedef struct __attribute__((packed)) {
    u8 key_short;
    u8 key_double;
    u8 key_three;
    u8 key_four;
    u8 key_five;
    u8 key_long;
}key_info_st;

typedef struct __attribute__((packed)){
    u8 mode;
    s8 gain[QCY_EQ_BAND_CNT];
}qcy_eq_info_t;

typedef struct __attribute__((packed)){
    u8 mode;
    u8 anc_on_level;
    u8 transparency_level;
}qcy_anc_info_t;

typedef struct __attribute__((packed)) {

    key_info_st key_local;
    key_info_st key_remote;
    qcy_eq_info_t eq_info;
    qcy_anc_info_t anc_info;

    // u8 poweroff_time;
    u8 game_mode;
    u8 game_mode_set_flag       : 1,
       anc_mode_set_flag        : 1,
       sleep_mode_set_flag      : 1,
       anc_gain_set_flag        : 1;

    s8 anc_default_gain;
    s8 tp_default_gain;
#if QCY_VOL_BALANCE
    u8 vol_balance_val;
#endif
    u8 sleep_mode;
#if QCY_CTL_IN_EAR_EN
    u8 in_ear_detect;
#endif
    u8 earphone_mode;
    u8 vol;

    u8 vbat_local;
    u8 vbat_ischarge_local;
    u8 vbat_remote;
    u8 vbat_ischarge_remote;
    u8 vbat_box;
    u8 vbat_ischarge_box;
    u8 version_local[QCY_SW_VER_LEN];
    u8 version_remote[QCY_SW_VER_LEN];
    char bt_name[32];

    u16 do_flag;
    u16 param_flag;
    u8 param_save_update;
    u8 ble_param_update;
    u8 sec_count;
    u8 active_flag;
}qcy_app_cb_st;

extern qcy_app_cb_st qcy_app_cb;
extern qcy_scan_rsp_data_t qcy_scan_data;
extern qcy_evt_fun_t qcy_evt_fun_tbl[QCY_EVT_FUN_TBL_LEN];

void qcy_adv_init(void);
void qcy_scan_data_set_checksum(void);
void qcy_scan_data_update(u8 popup_flag);
void qcy_bt_message_process(u16 msg);

u8 qcy_to_ab_keydef(u8 key_value);
u8 ab_to_qcy_keydef(u8 qcy_key);
u8 qcy_to_ab_eq_def(u8 qcy_eq_id);
void qcy_app_key_tbl_set(void);


void qcy_sleep_mode_enter(void);
void qcy_sleep_mode_exit(void);
void qcy_touch_test_mode_enter(void);
void qcy_touch_test_mode_exit(void);
void qcy_compactness_detect_mode_enter(void);
void qcy_compactness_detect_mode_exit(void);

#if QCY_ANC_EN
void qcy_eq_set_do(void);
void qcy_anc_set_do(void);
#endif
void qcy_reset_default_do(void);
void qcy_delete_link_do(void);
void qcy_reset_factory_do(void);

void qcy_key_info_save(void);
void qcy_eq_info_save(void);
void qcy_bt_name_save(u8 *name, u8 len);

u8 qcy_vol_dig2db(u16 dig_vol);

void qcy_key_init(void);
void qcy_eq_init(void);
void qcy_language_init(void);
void qcy_bt_name_init(void);
void qcy_app_init(void);
#if QCY_CTL_IN_EAR_EN
void qcy_ear_det_init(void);
void qcy_ear_det_save(u8 ear_en);
#endif
#if QCY_ANC_EN
void qcy_anc_info_init(void);
void qcy_anc_info_save();
void qcy_anc_set_level(void);
#endif
void qcy_earphone_mode_init(void);
void qcy_earphone_mode_save(u8 mode);

bool qcy_tws_get_remote_key(u8 cmd);
bool qcy_tws_set_remote_key(u8 cmd, u8 key_vlaue);
#if QCY_VOL_BALANCE
void qcy_vol_balance_init(void);
void qcy_vol_balance_save(void);
int qcy_cal_vol_balance(int balance, int db_current);
u16 qcy_dvol_tbl_get_val(u8 id);
#endif

void qcy_app_bt_evt_notice(u8 evt, void *params);

void qcy_ble_connect_callback(void);
void qcy_ble_disconnect_callback(void);

void qcy_app_enter_sleep(void);
bool qcy_app_wakeup(void);

void qcy_app_vbat_notify(void);
void qcy_app_setting_notify(u8 cmd);

void qcy_evt_fun_set(u8 evt_id, u8 fun_id);
void qcy_setting_data_process(u8 *ptr, u8 len);
void qcy_key_set_process(u8 *ptr, u8 len);
void qcy_eq1_data_process(u8 *ptr, u8 len);

void qcy_app_process(void);

void qcy_app_bt_evt_notice(u8 evt, void *params);
void qcy_flash_write(void *buf, u32 addr, uint len);
void qcy_flash_read(void *buf, u32 addr, uint len);

uint16_t qcy_ble_role_switch_get_data(uint8_t *data_ptr);
uint16_t qcy_ble_role_switch_set_data(uint8_t *data_ptr, uint16_t len);

#if QCY_TWS_EN
bool qcy_tws_msg_is_active(void);
void qcy_tws_msg_process(void);
void tws_send_msg_data(u8 cmd, uint8_t *data, uint16_t len);
#endif

bool ble_qcy_get_func_setting_send_packet(u8 *buf, u8 len);
bool ble_qcy_report_status_send_packet(u8 *buf, u8 len);

uint hfp_get_bat_level_ex(void);
#endif
#endif

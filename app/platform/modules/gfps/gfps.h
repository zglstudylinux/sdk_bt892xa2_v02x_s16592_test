#ifndef __GFPS_H
#define __GFPS_H

#include "include.h"

//CM Param
#define PARAM_PERSONALIZED_NAME_LEN     0           // 1byte
#define PARAM_PERSONALIZED_NAME         1           // 64 Bytes
#define PARAM_ACCOUNT_KEY               0x41        // 92 bytes, sizeof(struct account_key_list), 5 keys


//Define in libs, Modify the library file synchronously
//*******************************************************************************************/
#define ACCOUNT_KEY_LENGTH      16
#define ACCOUNT_KEY_MAX_COUNT   5 // 至少5个

typedef struct account_key_list {
    uint8_t keys[ACCOUNT_KEY_LENGTH * ACCOUNT_KEY_MAX_COUNT]; // each account key has 16 bytes
    uint16_t use_freq[ACCOUNT_KEY_MAX_COUNT]; // 用来记录使用频次，fixme: uint16_t应该够了吧？
    uint8_t count;
} *account_key_list_t;

enum{
    GFPS_MODE_NONE = 0,
    GFPS_MODE_DISCOVER,
    GFPS_MODE_NON_DISCOVER,
};
//*******************************************************************************************/

void *get_account_key_info();   // 获取账户秘钥结构体指针，第一个字节会放置结构体大小，开机调用后要读取flash信息覆盖

void switch_ble_adv_to_discoverable_mode(bool update_addr);     // BLE Adv切换到discoverable模式，蓝牙线程调用才可更新地址，其他情况update_addr需传0
void switch_ble_adv_to_non_discoverable_mode(bool update_addr); // BLE Adv切换到non-discoverable模式，蓝牙线程调用才可更新地址，其他情况update_addr需传0
void switch_ble_adv_off(void);
u8 gfps_get_cur_adv_mode(void);

void fp_spp_connected_callback(void);
void fp_spp_receive_callback(uint8_t *data, uint16_t data_len);
void fp_ble_disconnected_callback(void);
void fp_ble_connected_callback(void);
void fp_bt_disconnected_callback(void);
void fp_bt_connected_callback(void);

void google_fast_pair_init();
void google_fast_pair_bt_init(void);
void google_fast_pair_spp_recv_proc(uint8_t *data, uint16_t data_len);
void google_fast_pair_spp_connected_callback(void);
void google_fast_pair_ble_connect_callback(void);
void google_fast_pair_ble_disconnect_callback(void);
bool google_fast_pair_need_wakeup(void);

/// 消息流

/// 手机能力
bool is_installed_companion_app();  // 手机是否已安装Companion App
bool is_support_silence_mode();     // 手机是否支持静音模式
uint8_t get_platform_name();        // 获取手机平台
uint8_t get_android_version();      // 获取手机Android版本

/// 消息
void send_msg_bluetooth_silence_mode(bool silent); // 启用/禁用静音模式后调用，发送状态给手机。使用场景，耳机从头上拿下/戴上

// Companion App Event消息，使用自定义消息需要注意msg_buffer长度（MSG_BUFFER_MAX_LEN=64）
void send_msg_companion_app(uint8_t msg_code, uint8_t *data, uint16_t data_len);
void send_msg_companion_app_log_buffer_full(uint8_t *data, uint16_t data_len);

// Device Information Event消息，电池电量更新，电量变化时发送，根据每个组件状态传参
void send_msg_dev_info_battery_updated(uint8_t left_value, bool left_charging,uint8_t right_value, bool right_charging,uint8_t case_value, bool case_charging);
void send_msg_dev_info_battery_updated2(uint8_t *battery_data,uint8_t battery_data_len);

// Device Information Event消息，电池剩余时间更新，剩余时间变化时发送
void send_msg_dev_info_battery_time(uint8_t time);      // 8位时间
void send_msg_dev_info_battery_time2(uint16_t time);    // 16位时间

// Device Action Event消息，响铃，同步响铃状态到手机使用
void send_msg_device_action_ring(bool ring);                        // 单个组件
void send_msg_device_action_ring2(bool left_ring, bool right_ring); // 两个组件（对耳）

//
void google_fast_pair_evt_notice(u8 evt, void *params);
void google_fast_pair_process(void);
uint16_t google_fast_pair_tws_get_data(uint8_t *buf);
void google_fast_pair_tws_set_data(uint8_t *data, uint16_t len);
void google_fast_pair_vhouse_cmd_notice(u8 cmd);

#endif // __GFPS_H


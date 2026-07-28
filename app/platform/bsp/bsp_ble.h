#ifndef _BSP_BLE_H
#define _BSP_BLE_H

//快速配对相关接口
#if GFPS_EN
#include "gfps.h"
#define le_popup_init()                     google_fast_pair_init()
#define le_popup_process()                  google_fast_pair_process()
#define le_popup_evt_notice(a,b)            google_fast_pair_evt_notice(a,b)
#define le_popup_vhouse_cmd_notice(a)       google_fast_pair_vhouse_cmd_notice(a)
#define le_popup_need_wakeup()              google_fast_pair_need_wakeup()
#else
#define le_popup_init()
#define le_popup_process()
#define le_popup_evt_notice(a,b)
#define le_popup_vhouse_cmd_notice(a)
#define le_popup_need_wakeup()              0
#endif

//手机APP相关接口
#if AB_MATE_APP_EN
#include "ab_mate_app.h"
    #define app_ble_disconnect_callback()           ab_mate_ble_disconnect_callback()
    #define app_ble_connect_callback()              ab_mate_ble_connect_callback()
    #define app_process()                           ab_mate_process()
    #define app_var_init()                          ab_mate_var_init()
    #define app_init_do()                           ab_mate_init_do()
    #define app_eq_set()                            ab_mate_eq_set_do()
    #define app_enter_sleep()                       ab_mate_enter_sleep()
    #define app_exit_sleep()                        ab_mate_exit_sleep()
    #define app_need_wakeup()                       ab_mate_system_need_wakeup()
    #define app_bt_evt_notice(a,b)                  ab_mate_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         ab_mate_ble_role_switch_get_data(a)
    #define app_ble_role_switch_set_data(a,b)       ab_mate_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)
#elif LE_APP_EN
#include "app_demo.h"
    #define app_ble_disconnect_callback()
    #define app_ble_connect_callback()
    #define app_process()                           app_demo_process()
    #define app_var_init()
    #define app_init_do()
    #define app_eq_set()                            0
    #define app_enter_sleep()
    #define app_exit_sleep()
    #define app_need_wakeup()                       0
    #define app_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         0
    #define app_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)
#elif LE_WK_APP_EN
#include "wk_app.h"
    #define app_ble_disconnect_callback()           wk_ble_disconnect_callback()
    #define app_ble_connect_callback()              wk_ble_connect_callback()
    #define app_process()                           wk_app_process()
    #define app_var_init()                          wk_app_init()
    #define app_init_do()
    #define app_eq_set()                            0
    #define app_enter_sleep()                       wk_app_enter_sleep()
    #define app_exit_sleep()
    #define app_need_wakeup()                       wk_app_need_wakeup()
    #define app_bt_evt_notice(a,b)                  wk_app_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         wk_ble_role_switch_get_data(a)
    #define app_ble_role_switch_set_data(a,b)       wk_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)
#elif LE_TUYA_EN
#include "tuya_ble_app.h"
    #define app_ble_disconnect_callback()           tuya_ble_disconnected_handler()
    #define app_ble_connect_callback()              tuya_ble_connected_handler()
    #define app_process()                           tuya_ble_process()
    #define app_var_init()                          tuya_ble_var_init()
    #define app_init_do()                           tuya_ble_app_init()
    #define app_eq_set()                            tuya_ble_eq_set()
    #define app_enter_sleep()                       tuya_ble_enter_sleep()
    #define app_exit_sleep()                        tuya_ble_exit_sleep()
    #define app_need_wakeup()                       tuya_ble_data_receive_flag_get()
    #define app_bt_evt_notice(a,b)                  tuya_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         tuya_ble_role_switch_get_data(a)
    #define app_ble_role_switch_set_data(a,b)       tuya_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()                tuya_ble_sleep_mode_process()
    #define app_tws_user_key_process(a)
#elif LE_QCY_APP_EN
#include "qcy_app.h"
    #define app_ble_disconnect_callback()           qcy_ble_disconnect_callback()
    #define app_ble_connect_callback()              qcy_ble_connect_callback()
    #define app_process()                           qcy_app_process()
    #define app_var_init()                          qcy_app_init()
    #define app_init_do()
    #define app_eq_set()                            0
    #define app_enter_sleep()                       qcy_app_enter_sleep()
    #define app_exit_sleep()
    #define app_need_wakeup()                       qcy_app_wakeup()
    #define app_bt_evt_notice(a,b)                  qcy_app_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         qcy_ble_role_switch_get_data(a)
    #define app_ble_role_switch_set_data(a,b)       qcy_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)
#elif TME_APP_EN
#include "tme_app.h"
    #define app_ble_disconnect_callback()           tme_ble_disconnect_callback()
    #define app_ble_connect_callback()              tme_ble_connect_callback()
    #define app_process()                           tme_process()
    #define app_var_init()                          tme_init()
    #define app_init_do()
    #define app_eq_set()                            0
    #define app_enter_sleep()                       tme_enter_sleep()
    #define app_exit_sleep()                        tme_exit_sleep()
    #define app_need_wakeup()                       tme_is_need_wakeup()
    #define app_bt_evt_notice(a,b)                  tme_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         0
    #define app_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)             tme_tws_user_key_process(a)
#elif LE_DUEROS_DMA_EN
#include "dueros_dma_app.h"
    #define app_ble_disconnect_callback()           dueros_dma_app_ble_disconnect_callback()
    #define app_ble_connect_callback()              dueros_dma_app_ble_connect_callback()
    #define app_process()                           dueros_dma_app_process()
    #define app_var_init()                          dueros_dma_var_init()
    #define app_init_do()                           dueros_dma_app_init()
    #define app_eq_set()                            0
    #define app_enter_sleep()                       dueros_dma_app_enter_sleep()
    #define app_exit_sleep()                        dueros_dma_app_exit_sleep()
    #define app_need_wakeup()                       dueros_dma_app_is_need_wakeup()
    #define app_bt_evt_notice(a,b)                  dueros_dma_app_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         0
    #define app_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)             dueros_dma_tws_user_key_process(a)
#else
    #define app_ble_disconnect_callback()
    #define app_ble_connect_callback()
    #define app_process()
    #define app_var_init()
    #define app_init_do()
    #define app_eq_set()                            0
    #define app_enter_sleep()
    #define app_exit_sleep()
    #define app_need_wakeup()                       0
    #define app_bt_evt_notice(a,b)
    #define app_ble_role_switch_get_data(a)         0
    #define app_ble_role_switch_set_data(a,b)
    #define app_sleep_mode_process()
    #define app_tws_user_key_process(a)
#endif


void bt_app_cmd_process(u8 *ptr, u16 size, u8 type);
bool ble_send_packet(u8 *buf, u8 len);
bool ble_fot_send_packet(u8 *buf, u8 len);

#endif

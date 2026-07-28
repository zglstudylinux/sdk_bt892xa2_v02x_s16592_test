#ifndef _QCY_TWS_H_
#define _QCY_TWS_H_

enum {
    TWS_REQ_ALL_INFO_SYNC = 0x01,
    TWS_RSP_ALL_INFO_SYNC,
    TWS_REQ_KEY_INFO_GET,
    TWS_RSP_KEY_INFO_GET,
    TWS_REQ_KEY_INFO_SET,
    TWS_REQ_SENT_OFF_TIMES,
    TWS_REQ_SENT_EAR_DET,
    TWS_REQ_SENT_EARPHONE_MODE,
    TWS_REQ_SENT_ANC_INFO,
    TWS_REQ_SENT_EQ,
    TWS_REQ_VOL_BALANCE_SYNC,
    TWS_REQ_VOL_DB_SYNC,
    TWS_REQ_SLEEP_MODE_SYNC,
    TWS_REQ_SENT_FIND_EAR,
    TWS_REQ_SENT_VBAT,
    TWS_REQ_RESET_DEFAULT,
    TWS_REQ_DELETE_LINK,
    TWS_REQ_RESET_FACTORY,
    TWS_REQ_SENT_BT_NAME,
};

uint16_t qcy_tws_set_data(uint8_t *data_ptr, uint16_t size);
uint16_t qcy_tws_get_data(uint8_t *buf);

void qcy_tws_all_info_sync_req(void);
void qcy_tws_vbat_exchange(void);
void qcy_tws_sn_sync_req(void);
void qcy_tws_poweroff_time_sync(void);
void qcy_tws_ear_det_sync(void);
void qcy_tws_earphone_mode_sync(void);
void qcy_tws_anc_sync(void);
void qcy_tws_bt_name_sync(void);
void qcy_tws_reset_default_sync(void);
void qcy_tws_delete_link_sync(void);
void qcy_tws_reset_factory_sync(void);
void qcy_tws_sleep_mode_sync(void);
void qcy_tws_vol_balance_sync(void);
#endif

#ifndef _WK_TWS_H_
#define _WK_TWS_H_


enum {
    TWS_REQ_ALL_INFO_SYNC = 0x01,
    TWS_RSP_ALL_INFO_SYNC,
    TWS_REQ_SENT_SN_INFO,
    TWS_REQ_KEY_INFO_GET,
    TWS_RSP_KEY_INFO_GET,
    TWS_REQ_KEY_INFO_SET,
    TWS_REQ_SENT_OFF_TIMES,
    TWS_REQ_SENT_EAR_DET,
    TWS_REQ_SENT_EQ,
    TWS_REQ_SENT_FIND_EAR,
    TWS_REQ_SENT_VBAT,
    TWS_REQ_RESET_FACTORY,
    TWS_REQ_SENT_BT_NAME,

    TWS_INFO_OTA = 0x20,
    TWS_INFO_OTA_REQ,
    TWS_INFO_OTA_DATA,
    TWS_INFO_OTA_FILE_ADDR,
    TWS_INFO_OTA_UPDATE_DONE,
    TWS_INFO_OTA_ERR,
};

void wk_tws_all_info_sync_req(void);
void wk_tws_vbat_exchange(void);
void wk_tws_sn_sync_req(void);
void wk_tws_sn_sync_rsp(u8 *sn, u8 len);
void wk_tws_poweroff_time_sync(void);
void wk_tws_ear_det_sync(void);
void wk_tws_bt_name_sync(void);

#endif

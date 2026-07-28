#ifndef _WK_OTA_H_
#define _WK_OTA_H_

bool wk_ota_is_start(void);
void wk_ota_init(void);
bool wk_ota_is_start(void);
void wk_ota_tws_data_proc(uint8_t *data_ptr, u16 size);
void wk_ota_process(void);
void wk_ota_disconnect_callback(void);
void wk_ota_recv_proc(u8 *ptr, u8 len);


#endif

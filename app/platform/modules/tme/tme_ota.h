#ifndef __TME_OTA_H
#define __TME_OTA_H

#define TME_TWS_OTA_EN          FOT_SUPPORT_TWS

#define TME_OTA_MTU             128

#define TME_OTA_START_ADDR      (FLASH_SIZE/2)
#define TME_OTA_END_ADDR        (FLASH_SIZE - 0x5000 - 0x1000)
#define TME_OTA_FILE_MAX_LEN    (TME_OTA_END_ADDR - TME_OTA_START_ADDR + 0x1000)

typedef struct{
    u32 new_firmware_size;
    u32 new_firmware_crc;
    uint firmware_whole_crc;
    u8 new_firmware_version[2];         //固件版本号，2.0.3.15表示成0x3f,0x20,小端格式
    u16 data_len;
    u32 data_offset;
    u8 progress;
    u8 update_success;
}tme_ota_var_t;


void tme_ota_init(void);
void tme_cmd_ota_proc(u8 cmd,u8 *buf,u16 len);
void tme_ota_tws_data_proc(uint8_t *data_ptr, u16 size);

#endif

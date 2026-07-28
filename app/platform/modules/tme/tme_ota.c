#include "include.h"
#include "tme_ota.h"
#include "tme_timer.h"

#if TME_APP_EN && TME_OTA_EN

static tme_ota_var_t tme_ota_var AT(.tme_buf.ota);
static p_soft_timer tme_ota_reset_timer AT(.tme_buf.ota);

u8 tme_tws_data[512+4] AT(.tme_buf.app);
u16 tme_tws_data_len AT(.tme_buf.app);

u8 is_fot_update_en(void)
{
    return 1;
}

void tme_ota_init(void)
{
    memset(&tme_ota_var,0,sizeof(tme_ota_var_t));

    tme_ota_var.data_len = TME_OTA_MTU;
}

void tme_ota_reset_do(p_soft_timer timer)
{
    printf("tme_ota_reset_do:wdt reset\n");
    WDT_RST();
}

void tme_ota_reset_timer_start(void)
{
    soft_timer_create(&tme_ota_reset_timer, 2000, TIMER_SINGLE_SHOT, tme_ota_reset_do);
    soft_timer_start(tme_ota_reset_timer);
}

uint16_t tws_set_fot_data(uint8_t *data_ptr, uint16_t size)
{
#if TME_TWS_OTA_EN
    tme_ota_tws_data_proc(data_ptr,size);
#endif
    return 0;
}


uint16_t tws_get_fot_data(uint8_t *buf)
{
    memcpy(buf, tme_tws_data, tme_tws_data_len);

    return tme_tws_data_len;
}

void tme_tws_data_sync(void)
{
    bt_tws_sync_fot_data();
}

AT(.text.fot.cache)
void tme_ota_transport_segment_response_do(u8 err)
{
    //printf("offset:0x%x, pro:%d\n",tme_ota_var.data_offset,tme_ota_var.progress);

    if(err){
        tme_cmd_tx.data[0] = RESPONSE_ERR_OTA_UPDATE_FILE;
    }else{
        tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
    }
    memcpy(&tme_cmd_tx.data[1],&tme_ota_var.data_offset,4);
    memcpy(&tme_cmd_tx.data[5],&tme_ota_var.data_len,2);
    tme_cmd_tx.data[7] = tme_ota_var.progress;
    tme_cmd_tx.data_len = 8;
    tme_response_send();
}

#if TME_TWS_OTA_EN

u8 is_fot_tws_support(void)
{
    return 1;
}

static void tme_tws_ota_info_sync(u8* buf)
{
    u8 offset = 0;
    tme_tws_data[offset++] = TME_TWS_OTA_QUERY_INFO;
    memcpy(&tme_tws_data[offset],buf,10);
    offset += 10;
    tme_tws_data_len = offset;
    tme_tws_data_sync();
}

static void tme_tws_ota_transport_start_sync(u8 rsp)
{
    u8 offset = 0;
    tme_tws_data[offset++] = TME_TWS_OTA_TRANSPORT_START | rsp;
    memcpy(&tme_tws_data[offset],&tme_ota_var.data_offset,4);
    offset += 4;
    tme_tws_data_len = offset;
    tme_tws_data_sync();
}

AT(.text.fot.cache)
u8 tme_tws_ota_transport_sync(uint8_t *data_ptr, u16 size)
{
    static u16 offset = 0;
    if(offset == 0){
        tme_tws_data[offset++] = TME_TWS_OTA_TRANSPORT_SEGMENT;
    }
    memcpy(&tme_tws_data[offset],data_ptr,size);
    offset += size;
    if(offset == 512+1){
        tme_tws_data_len = offset;
        tme_tws_data_sync();
        offset = 0;
        return 0;
    }
    return 1;
}

AT(.com_text.fot)
void tws_fot_resp(void)
{
    u8 data[2];
    data[0] = TME_TWS_OTA_TRANSPORT_SEGMENT | 0x80;
    data[1] = FOT_ERR_OK;

    tme_ota_tws_data_proc(data,2);
}

AT(.text.fot.cache)
void tme_tws_ota_transport_reply(void)
{
    u8 offset = 0;

    if(fot_get_err()){
        tme_tws_data[offset++] = TME_TWS_OTA_TRANSPORT_SEGMENT | 0x80;
        tme_tws_data[offset++] = fot_get_err();
        tme_tws_data_len = offset;
        tme_tws_data_sync();
    }else{
        bt_fot_tws_resp();
    }
}

static void tme_tws_ota_update_done_sync(void)
{
    tme_tws_data[0] = TME_TWS_OTA_UPDATE_DONE;
    tme_tws_data_len = 1;
    tme_tws_data_sync();
}

static void tme_tws_ota_err_sync(void)
{
    tme_tws_data[0] = TME_TWS_OTA_ERR;
    tme_tws_data_len = 1;
    tme_tws_data_sync();
}

AT(.text.fot.cache)
void tme_ota_tws_data_proc(uint8_t *data_ptr, u16 size)
{
    printf("ota_tws:");
    //print_r(data_ptr,size);
    print_r(data_ptr,2);

    u8 cmd = data_ptr[0] & 0x7f;
    u8 rsp = (data_ptr[0] & 0x80) ? 1:0;

    switch(cmd){
        case TME_TWS_OTA_QUERY_INFO:
            memcpy(&tme_ota_var.new_firmware_size,&data_ptr[1],4);
            memcpy(&tme_ota_var.new_firmware_crc,&data_ptr[5],4);
            memcpy(tme_ota_var.new_firmware_version,&data_ptr[9],2);
            break;
        case TME_TWS_OTA_TRANSPORT_START:
            if(rsp){
                u32 remote_offset;
                memcpy(&remote_offset,&data_ptr[1],4);
                if(tme_ota_var.data_offset > remote_offset){
                    tme_ota_var.data_offset = remote_offset;
                    param_fot_addr_write((u8*)&tme_ota_var.data_offset);
                    fot_breakpoint_info_read();
                }
                tme_ota_var.progress =  tme_ota_var.data_offset * 100 / tme_ota_var.new_firmware_size;
                tme_ota_transport_segment_response_do(fot_get_err());
            }else{
                u32 last_crc32,remote_offset;
                memcpy(&remote_offset,&data_ptr[1],4);
                tme_ota_var.data_offset = 0;
                fot_init();
                set_sys_clk(SYS_120M);
                tme_cm_read(&last_crc32,TME_CM_OTA_CRC32,4);
                //printf("-->last_crc32:%x, remote CRC32:%x\n",last_crc32,tme_ota_var.new_firmware_crc);
                if(last_crc32 == tme_ota_var.new_firmware_crc){
                    if(fot_breakpoint_info_read() == true){
                        tme_ota_var.data_offset = fot_get_curaddr();
                    }
                }else{
                    tme_cm_write(&tme_ota_var.new_firmware_crc,TME_CM_OTA_CRC32,4);
                }
                if(tme_ota_var.data_offset > remote_offset){
                    tme_ota_var.data_offset = remote_offset;
                    param_fot_addr_write((u8*)&tme_ota_var.data_offset);
                    fot_breakpoint_info_read();
                }
                tme_tws_ota_transport_start_sync(0x80);
            }
            break;
        case TME_TWS_OTA_TRANSPORT_SEGMENT:
            if(rsp){
                u8 remote_err = data_ptr[1];

                if(is_fot_update_success()){
                    if(remote_err == FOT_ERR_OK){
                        tme_ota_var.update_success = 1;
                        tme_ota_var.progress = 100;
                        tme_ota_var.data_offset = tme_ota_var.new_firmware_size;
                        tme_ota_var.data_len = 0;
                        tme_ota_transport_segment_response_do(RESPONSE_ERR_NONE);
                        fot_tws_done();
                        tme_tws_ota_update_done_sync();
                        return;
                    }
                }
                //printf("offset:0x%x, pro:%d\n",tme_ota_var.data_offset,tme_ota_var.progress);
                if(fot_get_err() || remote_err){
                    if(remote_err){
                        fot_tws_done_sync_err_deal();
                    }
                    if(fot_get_err()){
                        tme_tws_ota_err_sync();
                    }
                    tme_ota_transport_segment_response_do(RESPONSE_ERR_OTA_UPDATE_FILE);
                }else{
                    tme_ota_transport_segment_response_do(RESPONSE_ERR_NONE);
                }
            }else{
                fot_write(&data_ptr[1], fot_get_curaddr(), size-1);
                tme_ota_var.data_offset = fot_get_curaddr();
                tme_tws_ota_transport_reply();
            }
            break;
        case TME_TWS_OTA_UPDATE_DONE:
            fot_tws_done();
            tme_ota_reset_timer_start();
            break;
        case TME_TWS_OTA_ERR:
            fot_tws_done_sync_err_deal();
            break;
        default:
            break;
    }
}
#endif

static void tme_ota_info_response(u8* buf)
{
    u16 tme_ota_mtu = TME_OTA_MTU;

    memcpy(&tme_ota_var.new_firmware_size,&buf[0],4);
    memcpy(&tme_ota_var.new_firmware_crc,&buf[4],4);
    memcpy(tme_ota_var.new_firmware_version,&buf[8],2);

#if TME_TWS_OTA_EN
    tme_tws_ota_info_sync(buf);
#endif

    tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
    memcpy(&tme_cmd_tx.data[1],&tme_ota_mtu,2);
    memcpy(&tme_cmd_tx.data[3],tme_firmware_version_get(),2);
    tme_cmd_tx.data[5] = (bsp_get_bat_level() > 100) ? 100: bsp_get_bat_level();
    if(bsp_get_bat_level() < 50){
        tme_cmd_tx.data[0] = RESPONSE_ERR_OTA_LOW_POWER;
    }
    if(tme_ota_var.new_firmware_size > TME_OTA_FILE_MAX_LEN){
        tme_cmd_tx.data[0] = RESPONSE_ERR_OTA_FIRMWARE_LEN;
    }
#if TME_TWS_OTA_EN
    if(!bt_tws_is_connected()){
        tme_cmd_tx.data[0] = RESPONSE_ERR_OTA_UPDATE_FAIL;
    }
#endif
    tme_cmd_tx.data_len = 6;
    tme_response_send();
}

static void tme_ota_query_version_response(void)
{
    tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
    memcpy(&tme_cmd_tx.data[1],tme_firmware_version_get(),2);
    tme_cmd_tx.data_len = 3;
    tme_response_send();
}

static void tme_ota_transport_segment_response(u8* buf, u16 len)
{
    u32 last_crc32 = 0;

    if(len == 0){
        tme_ota_var.data_offset = 0;
        if(ble_is_connect()){
            ble_update_conn_param(12,0,400);
        }
        fot_init();
        set_sys_clk(SYS_120M);
        tme_cm_read(&last_crc32,TME_CM_OTA_CRC32,4);
        //printf("-->last_crc32:%x, remote CRC32:%x\n",last_crc32,tme_ota_var.new_firmware_crc);
        if(last_crc32 == tme_ota_var.new_firmware_crc){
            if(fot_breakpoint_info_read() == true){
                tme_ota_var.data_offset = fot_get_curaddr();
            }
        }else{
            tme_cm_write(&tme_ota_var.new_firmware_crc,TME_CM_OTA_CRC32,4);
        }
#if TME_TWS_OTA_EN
        if(bt_tws_is_connected()){
            tme_tws_ota_transport_start_sync(0);
        }else{
            tme_ota_transport_segment_response_do(RESPONSE_ERR_OTA_UPDATE_FILE);
        }
#endif
    }else{
#if TME_TWS_OTA_EN
        u8 flag = tme_tws_ota_transport_sync(buf,len);
#endif
        fot_write(buf, fot_get_curaddr(), len);
        tme_ota_var.data_offset = fot_get_curaddr();
        tme_ota_var.progress =  tme_ota_var.data_offset * 100 / tme_ota_var.new_firmware_size;
#if TME_TWS_OTA_EN
        if(flag){
            tme_ota_transport_segment_response_do(fot_get_err());
        }else{
            if(!bt_tws_is_connected()){
                tme_ota_transport_segment_response_do(RESPONSE_ERR_OTA_UPDATE_FILE);
            }
        }
#endif
    }

#if (TME_TWS_OTA_EN == 0)
    if(is_fot_update_success()){
        tme_ota_var.update_success = 1;
        tme_ota_var.progress = 100;
        tme_ota_var.data_offset = tme_ota_var.new_firmware_size;
        tme_ota_var.data_len = 0;
    }else{
        tme_ota_var.progress =  tme_ota_var.data_offset * 100 / tme_ota_var.new_firmware_size;
    }
    tme_ota_transport_segment_response_do(fot_get_err());
#endif
}

static void tme_ota_check_crc_response(void)
{
    if(tme_ota_var.update_success){
        tme_error_response(RESPONSE_ERR_NONE);
        tme_ota_reset_timer_start();
    }else{
        tme_error_response(RESPONSE_ERR_OTA_CRC_FIRMWARE_DATA_WHOLE);
    }
}

void tme_cmd_ota_proc(u8 cmd,u8 *buf,u16 len)
{
//    if(cmd != CMD_OTA_TRANSPORT_SEGMENT){
//        printf("ota_cmd:%d,len:%d\n",cmd,len);
//        print_r(buf,len);
//    }else{
//        printf("len:%d\n",len);
//    }

    switch(cmd){
        case CMD_OTA_QUERY_VERSION:   //查询当前固件版本号
            tme_ota_query_version_response();
            break;
        case CMD_OTA_CHECK_CRC:   //固件完整校验命令
            tme_ota_check_crc_response();
            break;
        case CMD_OTA_CANCAL:   //取消OTA升级
            tme_error_response(RESPONSE_ERR_NONE);
            break;
        case CMD_OTA_QUERY_INFO:   //OTA配置信息获取命令
            tme_ota_info_response(buf);
            break;
        case CMD_OTA_MODIFY_NAME_MAC:   //更改固件蓝牙信息
            break;
        case CMD_OTA_TRANSPORT_SEGMENT:   //OTA数据传输命令
            tme_ota_transport_segment_response(buf,len);
            break;
        default:
            break;
    }
}

#endif

#include "include.h"
#include "wk_app.h"
#include "wk_ota.h"
#include "wk_tws.h"

#if LE_WK_APP_EN

#define FOT_DEBUG_EN                    0

#if FOT_DEBUG_EN
#define FOT_DEBUG(...)                  printf(__VA_ARGS__)
#define FOT_DEBUG_R(...)                print_r(__VA_ARGS__)
#else
#define FOT_DEBUG(...)
#define FOT_DEBUG_R(...)
#endif

enum{
    FOT_STA_STOP = 0,
    FOT_STA_START_REQ,
    FOT_STA_START_RSP,
    FOT_STA_W4_START,
    FOT_STA_START,
    FOT_STA_PAUSE,
    FOT_STA_W4_TWS_RSP,
    FOT_STA_EXIT,
};

#define FOT_FLASH_SIZE          (FLASH_SIZE/2 - 12*1024)

#define FOT_FLAG_UPDATE_OK      BIT(0)
#define FOT_FLAG_SYS_RESET      BIT(1)
#define FOT_FLAG_CLK_SET        BIT(2)


#define FOT_ADDR_POS            1
#define FOT_DATA_LEN_POS        5
#define FOT_DATA_INFO_LEN       8
#define FOT_DATA_POS            1
#define FOT_SEQ_POS             0

#define FOT_BLOCK_LEN           512         //max 512

typedef struct __attribute__((packed)){
    u8 fot_sta;
    u8 fot_flag;
    u8 fot_recv_ok;
    u8 remote_ver[4];
    u32 hash;
    u32 file_size;
    u32 total_len;
    u32 remain_len;
    u16 data_pos;
    u32 addr;
    u32 tick;
    u8 seq;
#if FOT_SUPPORT_TWS
    u8 tws_recv_ok;
    u8 tws_rsp;
    u8 tws_rem_err;
    u32 tws_rem_addr;
#endif
}fot_s;

static fot_s fot_var AT(.ble_buf.buf);

u8 fot_data[FOT_BLOCK_LEN] AT(.fot_data.buf);

bool wk_ota_is_start(void)
{
    return (fot_var.fot_sta > FOT_STA_STOP);
}


u8 is_fot_update_en(void)
{
    return 1;
}

void wk_ota_init(void)
{
    memset(&fot_var,0,sizeof(fot_var));
}


AT(.text.fot.cache)
static void wk_ota_sta_notify(u8 sta)
{
    u8 data[7];

    memcpy(data,app_cmd_id,4);
    data[4] = WK_CMD_OTA_STA;
    data[5] = 1;
    data[6] = sta;

    ble_wk_app_send_packet(data, 7);
}

void fot_update_pause(void)
{
    if(fot_var.fot_sta == FOT_STA_START){
        fot_var.fot_sta = FOT_STA_PAUSE;
        wk_ota_sta_notify(FOT_UPDATE_PAUSE);
    }
}

void fot_update_continue(void)
{
    if(fot_var.fot_sta == FOT_STA_PAUSE){
        wk_ota_sta_notify(FOT_UPDATE_CONTINUE);
    }
}

void wk_ota_start_req_rsp(u32 addr, u32 block_len, u8 need_update)
{
    u8 data[15];
    u8 offset = 0;

    memcpy(data,app_cmd_id, 4);
    offset += 4;

    data[offset++] = WK_CMD_OTA_START_RSP;
    data[offset++] = 9;
    memcpy(&data[offset], &addr, 4);
    offset += 4;
    memcpy(&data[offset], &block_len, 4);
    offset += 4;
    data[offset++] = need_update;

    ble_wk_app_send_packet(data, 15);
}

//-----------------------------------------------------------
//TWS同时升级功能
#if FOT_SUPPORT_TWS

extern u8 tws_sync_ota[512+16];
extern u16 tws_sync_data_len;

extern void tws_data_sync_do(void);

u8 is_fot_tws_support(void)
{
    return 1;
}

AT(.com_text.fot)
void tws_fot_resp(void)
{
    fot_var.tws_rem_err = FOT_ERR_OK;
    fot_var.tws_rsp = 1;
}

AT(.text.fot.update)
static void wk_tws_ota_err_sync(u8 err)
{
    u8 offset = 0;
    tws_sync_ota[offset++] = TWS_INFO_OTA;
    tws_sync_ota[offset++] = TWS_INFO_OTA_ERR;
    tws_sync_ota[offset++] = err;

    tws_sync_data_len = offset;
    tws_data_sync_do();
}

AT(.text.fot.update)
static void wk_tws_ota_info_sync(void)
{
    tws_sync_ota[0] = TWS_INFO_OTA;
    tws_sync_ota[1] = TWS_INFO_OTA_REQ;
    memcpy(&tws_sync_ota[2], &fot_var.remote_ver, 4);
    memcpy(&tws_sync_ota[6], &fot_var.hash, 4);
    memcpy(&tws_sync_ota[10], &fot_var.addr, 4);

    tws_sync_data_len = 14;
    tws_data_sync_do();
}

AT(.text.fot.update)
static void wk_tws_ota_file_addr_sync(void)
{
    tws_sync_ota[0] = TWS_INFO_OTA;
    tws_sync_ota[1] = TWS_INFO_OTA_FILE_ADDR;
    memcpy(&tws_sync_ota[2],&fot_var.addr,4);

    tws_sync_data_len = 6;
    tws_data_sync_do();
}

AT(.text.fot.update)
static void wk_tws_ota_done_sync(void)
{
    tws_sync_ota[0] = TWS_INFO_OTA;
    tws_sync_ota[1] = TWS_INFO_OTA_UPDATE_DONE;

    tws_sync_data_len = 2;
    tws_data_sync_do();
}

void wk_tws_ota_req_proc(void)
{
    u32 hash;

    param_fot_hash_read((u8*)&hash);

    fot_init();
    if((fot_var.hash != 0xFFFFFFFF) && (hash == fot_var.hash)){
        if((fot_breakpoint_info_read() == true)){
            fot_var.addr = fot_get_curaddr();
        }
    }else{
        fot_var.addr = 0;
        param_fot_addr_write((u8*)&fot_var.addr);
        param_fot_hash_write((u8*)&fot_var.hash);
    }
    if(fot_var.addr > fot_var.tws_rem_addr){
        fot_var.addr = fot_var.tws_rem_addr;
        param_fot_addr_write((u8*)&fot_var.addr);
        fot_breakpoint_info_read();
    }
    wk_tws_ota_file_addr_sync();
    fot_var.fot_sta = FOT_STA_W4_START;
}

AT(.text.fot.update)
static void wk_tws_ota_reply_update_request(void)
{
    u32 block_len = 0;
    u8 need_update = 1;

    if(fot_var.addr > fot_var.tws_rem_addr){
        fot_var.addr = fot_var.tws_rem_addr;
        param_fot_addr_write((u8*)&fot_var.addr);
        fot_breakpoint_info_read();
    }

    if(bt_get_status() >= BT_STA_INCOMING){
        need_update = 0;
        fot_var.fot_sta = FOT_STA_STOP;
        goto fot_req_reply;
    }

    if(ble_is_connect()){
        ble_update_conn_param(12, 0, 400);
    }

    block_len = FOT_BLOCK_LEN;

    fot_var.fot_sta = FOT_STA_W4_START;

fot_req_reply:
    wk_ota_start_req_rsp(fot_var.addr,block_len,need_update);
}
#endif

#if FOT_SUPPORT_TWS
AT(.text.fot.update)
static void wk_ota_req_proc(void)
{
    u32 hash;

    u32 block_len = 0;
    u8 need_update = 1;

    if( (bt_get_status() >= BT_STA_INCOMING) || (fot_var.file_size > FOT_FLASH_SIZE)){
        need_update = 0;
    }

    if(need_update == 0){
        wk_ota_start_req_rsp(fot_var.addr,block_len,need_update);
        fot_var.fot_sta = FOT_STA_STOP;
        return;
    }

    FOT_DEBUG("-->hash_val:0x%x\n",fot_var.hash);

    param_fot_hash_read((u8*)&hash);

    FOT_DEBUG("-->flash hash val:0x%x\n",hash);

    fot_init();

    if((fot_var.hash != 0xFFFFFFFF) && (hash == fot_var.hash)){
        if((fot_breakpoint_info_read() == true)){
            fot_var.addr = fot_get_curaddr();
        }
    }else{
        fot_var.addr = 0;
        param_fot_addr_write((u8*)&fot_var.addr);
        param_fot_hash_write((u8*)&fot_var.hash);
    }

    wk_tws_ota_info_sync();
    fot_var.fot_sta = FOT_STA_W4_TWS_RSP;
}
#else
AT(.text.fot.update)
static void wk_ota_req_proc(void)
{
    u32 hash;
    u32 block_len = FOT_BLOCK_LEN;
    u8 need_update = 1;

    if( (bt_get_status() >= BT_STA_INCOMING) || (fot_var.file_size > FOT_FLASH_SIZE)){
        need_update = 0;
        fot_var.fot_sta = FOT_STA_STOP;
        goto fot_req_reply;
    }

    if(ble_is_connect()){
        ble_update_conn_param(12,0,400);
    }

    fot_init();

    FOT_DEBUG("hash_val:0x%x\n",fot_var.hash);

    param_fot_hash_read((u8*)&hash);

    FOT_DEBUG("flash hash val:0x%x\n",hash);

    if((fot_var.hash != 0xFFFFFFFF) && (hash == fot_var.hash)){
        if(fot_breakpoint_info_read() == true){
            fot_var.addr = fot_get_curaddr();
        }
    }else{
        param_fot_addr_write((u8*)&fot_var.addr);
        param_fot_hash_write((u8*)&fot_var.hash);
    }

    fot_var.fot_sta = FOT_STA_W4_START;

fot_req_reply:
    wk_ota_start_req_rsp(fot_var.addr,block_len,need_update);
}
#endif

void wk_ota_disconnect_callback(void)
{
    if((fot_var.fot_flag & FOT_FLAG_UPDATE_OK) == 0){
        memset(&fot_var,0,sizeof(fot_var));
    }
}

void wk_ota_tws_data_proc(uint8_t *data_ptr, u16 size)
{
#if FOT_SUPPORT_TWS
    u8 cmd = data_ptr[0];
    size = size - 1;
    u8 *buf = &data_ptr[1];


    switch(cmd){
    case TWS_INFO_OTA_DATA:{
        u32 addr;
        u32 recv_data_len;
        if(fot_var.remain_len){
            FOT_DEBUG("--->len err:%d\n",fot_var.remain_len);
            wk_tws_ota_err_sync(FOT_ERR_DATA_LEN);
            return;
        }
        if(fot_var.fot_sta != FOT_STA_START){
            fot_var.fot_sta = FOT_STA_START;
            fot_var.fot_flag |= FOT_FLAG_CLK_SET;
        }
        recv_data_len = size-FOT_DATA_INFO_LEN;
        memcpy(&addr,&buf[FOT_ADDR_POS-1],4);
        memcpy(&fot_var.total_len,&buf[FOT_DATA_LEN_POS-1],4);
        fot_var.remain_len = fot_var.total_len - recv_data_len;
        memcpy(&fot_data[fot_var.data_pos],&buf[FOT_DATA_INFO_LEN],recv_data_len);
        fot_var.data_pos += recv_data_len;
        if(fot_var.remain_len == 0){
            fot_var.tws_recv_ok = 1;
            fot_var.data_pos = 0;
        }
    }   break;
    case TWS_INFO_OTA_REQ:{
        memset(&fot_var,0,sizeof(fot_var));
        memcpy(&fot_var.remote_ver, &buf[0], 4);
        memcpy(&fot_var.hash, &buf[4], 4);
        memcpy(&fot_var.tws_rem_addr,&buf[8],4);
        fot_var.fot_sta = FOT_STA_START_REQ;
    }   break;
    case TWS_INFO_OTA_FILE_ADDR:{
        memcpy(&fot_var.tws_rem_addr,&buf[0],4);
        fot_var.fot_sta = FOT_STA_START_RSP;
    }   break;
    case TWS_INFO_OTA_UPDATE_DONE:
        fot_tws_done();
        fot_var.fot_flag |= (FOT_FLAG_UPDATE_OK | FOT_FLAG_SYS_RESET);
        fot_var.tick = tick_get();
        break;
    case TWS_INFO_OTA_ERR:
        fot_var.tws_rsp = 1;
        fot_var.tws_rem_err = buf[0];
        if(!ble_is_connect()){
            if(is_fot_update_success()){
                fot_tws_done_sync_err_deal();
            }
            fot_var.fot_sta = FOT_STA_EXIT;
        }
        break;
    default:
        break;
    }
#endif
}

void wk_ota_proc(u8 cmd,u8 *payload,u8 payload_len)
{
    u32 addr;
    u32 recv_data_len;

    switch(cmd){
    case WK_CMD_OTA_START_REQ:
        memset(&fot_var,0,sizeof(fot_var));
        memcpy(&fot_var.remote_ver,&payload[0],4);
        memcpy(&fot_var.hash,&payload[4],4);
        memcpy(&fot_var.file_size,&payload[8],4);
        fot_var.fot_sta = FOT_STA_START_REQ;
        break;

    case WK_CMD_OTA_DATA_START:
        if(fot_var.seq != payload[FOT_SEQ_POS]){
            FOT_DEBUG("--->seq err:%d, %d\n",fot_var.seq, payload[FOT_SEQ_POS]);
            wk_ota_sta_notify(FOT_ERR_SEQ);
            return;
        }
        fot_var.seq++;
        if(fot_var.fot_sta != FOT_STA_START){
            fot_var.fot_sta = FOT_STA_START;
            fot_var.fot_flag |= FOT_FLAG_CLK_SET;
        }
#if FOT_SUPPORT_TWS
        tws_sync_ota[0] = TWS_INFO_OTA;
        tws_sync_ota[1] = TWS_INFO_OTA_DATA;
        memcpy(&tws_sync_ota[2],&payload[FOT_ADDR_POS],FOT_DATA_INFO_LEN);
#endif
        memcpy(&addr,&payload[FOT_ADDR_POS],4);
        memcpy(&fot_var.total_len,&payload[FOT_DATA_LEN_POS],4);
        fot_var.remain_len = fot_var.total_len;
        break;

    case WK_CMD_OTA_DATA_CONTINUE:
        if(fot_var.seq != payload[FOT_SEQ_POS]){
            FOT_DEBUG("--->seq err:%d, %d\n",fot_var.seq, payload[FOT_SEQ_POS]);
            wk_ota_sta_notify(FOT_ERR_SEQ);
            return;
        }
        fot_var.seq++;
        recv_data_len = payload_len-FOT_DATA_POS;
        if(fot_var.remain_len < recv_data_len){
            FOT_DEBUG("--->len err:%d,%d\n",fot_var.remain_len,recv_data_len);
            wk_ota_sta_notify(FOT_ERR_DATA_LEN);
            return;
        }
        fot_var.remain_len -= recv_data_len;
        memcpy(&fot_data[fot_var.data_pos],&payload[FOT_DATA_POS],recv_data_len);
        fot_var.data_pos += recv_data_len;
        if(fot_var.remain_len == 0){
#if FOT_SUPPORT_TWS
            memcpy(&tws_sync_ota[FOT_DATA_INFO_LEN+2],fot_data,fot_var.total_len);
            tws_sync_data_len = fot_var.total_len + FOT_DATA_INFO_LEN + 2;
            tws_data_sync_do();
#endif
            fot_var.fot_recv_ok = 1;
            fot_var.data_pos = 0;
        }
        break;

    default:
        break;
    }
}

void wk_ota_recv_proc(u8 *ptr, u8 len)
{
    FOT_DEBUG("OTA[%d]:",len);
    FOT_DEBUG_R(ptr,len);

    if(memcmp(app_cmd_id, ptr, 4)){
        return;
    }

    u8 cmd = ptr[4];
    u8 payload_len = ptr[5];
    u8 *payload = &ptr[6];

    wk_ota_proc(cmd,payload,payload_len);
}

AT(.text.fot.cache)
void wk_ota_process(void)
{
    switch(fot_var.fot_sta){
    case FOT_STA_START_REQ:
        if(ble_is_connect()){
            wk_ota_req_proc();
        }else{
#if FOT_SUPPORT_TWS
            wk_tws_ota_req_proc();
#endif
        }
        break;

    case FOT_STA_START_RSP:
#if FOT_SUPPORT_TWS
        wk_tws_ota_reply_update_request();
#endif
        break;

    case FOT_STA_W4_TWS_RSP:
    case FOT_STA_STOP:
    case FOT_STA_W4_START:
        break;

    case FOT_STA_EXIT:
        unlock_code_fota();
        fot_var.fot_sta = FOT_STA_STOP;
        break;

    case FOT_STA_START:
        reset_sleep_delay();
        reset_pwroff_delay();
#if FOT_SUPPORT_TWS
        if(fot_var.tws_recv_ok){
            FOT_DEBUG("--->tws_recv_ok\n");
            fot_var.tws_recv_ok = 0;
            fot_write(fot_data, fot_get_curaddr(), fot_var.total_len);
            if(fot_get_err()){
                wk_tws_ota_err_sync(fot_get_err());
                fot_var.fot_sta = FOT_STA_EXIT;
            }else{
                bt_fot_tws_resp();
            }
        }
        if(fot_var.fot_recv_ok == 0x01){
            fot_var.fot_recv_ok = 0x02;
            fot_write(fot_data, fot_get_curaddr(), fot_var.total_len);
        }else if(fot_var.tws_rsp && (fot_var.fot_recv_ok == 0x02)){
            FOT_DEBUG("--->fot_recv_ok\n");
            fot_var.tws_rsp = 0;
            fot_var.fot_recv_ok = 0;
            if(fot_var.tws_rem_err != FOT_ERR_OK){
                FOT_DEBUG("fot_tws_remote_err:0x%x\n",fot_var.tws_rem_err);
                wk_ota_sta_notify(fot_var.tws_rem_err);
                if(is_fot_update_success()){
                    fot_tws_done_sync_err_deal();
                }
                fot_var.fot_sta = FOT_STA_EXIT;
            }else{
                if(fot_get_err()){
                    FOT_DEBUG("fot_err:0x%x\n",fot_get_err());
                    wk_tws_ota_err_sync(fot_get_err());
                    fot_var.fot_sta = FOT_STA_EXIT;
                }
                if(is_fot_update_success()){
                    FOT_DEBUG("--->fot update success\n");
                    wk_tws_ota_done_sync();
                    fot_tws_done();
                    fot_var.fot_flag |= (FOT_FLAG_UPDATE_OK | FOT_FLAG_SYS_RESET);
                    fot_var.tick = tick_get();
                    wk_ota_sta_notify(FOT_UPDATE_DONE);
                }else{
                    wk_ota_sta_notify(fot_get_err());
                }
            }
        }
#else
        if(fot_var.fot_recv_ok){
            //FOT_DEBUG("--->fot_recv_ok\n");
            fot_var.fot_recv_ok = 0;
            fot_write(fot_data, fot_get_curaddr(), fot_var.total_len);

            if(is_fot_update_success()){
                FOT_DEBUG("--->fot update success\n");
                fot_var.fot_flag |= (FOT_FLAG_UPDATE_OK | FOT_FLAG_SYS_RESET);
                fot_var.tick = tick_get();
                wk_ota_sta_notify(FOT_UPDATE_DONE);
            }else{
                wk_ota_sta_notify(fot_get_err());
                if(fot_get_err()){
                    fot_var.fot_sta = FOT_STA_EXIT;
                }
            }
        }
#endif
        break;

    default:
        break;
    }

    if(fot_var.fot_flag & FOT_FLAG_CLK_SET){
        fot_var.fot_flag &= ~FOT_FLAG_CLK_SET;
        set_sys_clk(SYS_120M);
    }

    if(fot_var.fot_flag & FOT_FLAG_SYS_RESET){
        if(tick_check_expire(fot_var.tick,3000)){
            fot_var.fot_flag &= ~FOT_FLAG_SYS_RESET;
            FOT_DEBUG("fota update ok,sys reset\n");
            WDT_RST();
        }
    }

}


#endif

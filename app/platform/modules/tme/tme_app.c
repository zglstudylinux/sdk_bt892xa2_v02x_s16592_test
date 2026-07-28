#include "include.h"
#include "tme_app.h"
#include "tme_opus.h"
#include "tme_ota.h"

#if TME_APP_EN

#define TME_DEBUG_EN       1

#if TME_DEBUG_EN
#define TME_DEBUG(...)                  printf(__VA_ARGS__)
#define TME_DEBUG_R(...)                print_r(__VA_ARGS__)
#else
#define TME_DEBUG(...)
#define TME_DEBUG_R(...)
#endif

extern uint8_t adv_data_buf[31];

extern void aes_ecb_encrypt(uint8_t *plain, uint8_t *key, uint8_t *cipher, uint16_t len);
extern int uECC192r1_compute_public_key(const uint8_t private_key[24],uint8_t public_key[24 * 2]);
extern int uECC192r1_shared_secret(const uint8_t public_key[24*2],const uint8_t private_key[24],uint8_t secret[24]);

tme_app_var_t tme_app_var AT(.tme_buf.app);
tme_cmd_t tme_cmd_rx AT(.tme_buf.app);
tme_cmd_t tme_cmd_tx AT(.tme_buf.app);

static p_soft_timer tme_auth_timer AT(.tme_buf.app);
static p_soft_timer tme_resend_timer AT(.tme_buf.app);

/*
不同的产品有不同的PID和KEY，需要根据实际申请到的PID和KEY修改tme_pid和PubKeyA的值,以下的仅供测试用
*/
/*
pid=100072
E-x=34ed9801246c5915884ed0aed2ad9fc93587d5002a2599f5
E-y=0e1dc814b2740bf2f801b446e3d1f9f3fe838ea7e72fabfd
*/

static const u32 tme_pid = 0x000186E8;
//PubKeyA = E-x(24Byte) + E-y(24Byte)
static const u8 PubKeyA[48] = { 0x34,0xed,0x98,0x01,0x24,0x6c,0x59,0x15,0x88,0x4e,0xd0,0xae,
                                0xd2,0xad,0x9f,0xc9,0x35,0x87,0xd5,0x00,0x2a,0x25,0x99,0xf5,
                                0x0e,0x1d,0xc8,0x14,0xb2,0x74,0x0b,0xf2,0xf8,0x01,0xb4,0x46,
                                0xe3,0xd1,0xf9,0xf3,0xfe,0x83,0x8e,0xa7,0xe7,0x2f,0xab,0xfd };

static const u8 firmware_ver[2] = {0x01,0x00};     //版本号00.00.00.01,如果是1.3.2.1，表示为0x13,0x21

const u8 *tme_firmware_version_get(void)
{
    return firmware_ver;
}

const u32 tme_pid_get(void)
{
    return tme_pid;
}

u8 tme_is_need_wakeup(void)
{
    return (tme_app_var.rx_proc == 1);
}

void tme_enter_sleep(void)
{
    if(ble_get_status() == LE_STA_ADVERTISING){
        ble_set_adv_interval(640);
    }else if(ble_get_status() == LE_STA_CONNECTION){
        ble_update_conn_param(320, 0, 400);
    }
}

void tme_exit_sleep(void)
{
    if(ble_get_status() == LE_STA_ADVERTISING){
        ble_set_adv_interval(96);
    }else if(ble_get_status() == LE_STA_CONNECTION){
        ble_update_conn_param(24, 0, 400);
    }
}

void tme_cm_write(void *buf, u16 addr, u16 size)
{
    cm_write(buf, APP_CM_PAGE(addr), size);
    cm_sync();

}

void tme_cm_read(void *buf, u16 addr, u16 size)
{
    cm_read(buf, APP_CM_PAGE(addr), size);
}

AT(.com_text.tme)
void tme_send_proc(void)
{
    const u32 cmd_head = TME_CMD_HEAD;
    memcpy(tme_cmd_tx.cmd_head, &cmd_head, 3);

    tme_cmd_tx.data[tme_cmd_tx.data_len] = TME_CMD_TAIL;

    //TME_DEBUG("Tme Send: ");
    //TME_DEBUG_R(tme_cmd_tx,tme_cmd_tx.data_len + 9);

    if(ble_is_connect()){
        ble_send_packet((u8*)&tme_cmd_tx, tme_cmd_tx.data_len + 9);
    }else{
        bt_spp_custom1_tx((u8*)&tme_cmd_tx, tme_cmd_tx.data_len + 9);
    }
}

void tme_gap_disconnect(void)
{
    TME_DEBUG("-->tme_gap_disconnect\n");
    if(tme_app_var.tme_connect_sta){
        if(ble_is_connect()){
            ble_disconnect();
        }else{
            spp_disconnect();
        }
    }
}

void tme_auth_timeout_do(p_soft_timer timer)
{
    if((tme_app_var.auth_start == 0) && tme_app_var.tme_connect_sta){
        tme_gap_disconnect();
    }
}

void tme_auth_timer_creat(void)
{
    soft_timer_create(&tme_auth_timer, 4500, TIMER_SINGLE_SHOT, tme_auth_timeout_do);
}

void tme_request_resend_do(p_soft_timer timer)
{
    if(tme_app_var.resend_cnt){
        tme_app_var.resend_cnt--;
        tme_send_proc();
    }else{
        tme_gap_disconnect();
        soft_timer_stop(timer);
    }
}

void tme_request_resend_timer_creat(void)
{
    soft_timer_create(&tme_resend_timer, 2000, TIMER_REPEATED, tme_request_resend_do);
}

static bool tme_auth_cmd_check(u8 cmd)
{
    bool result = false;

    switch(cmd){
        case CMD_QUERY_MAC:
        case CMD_HANDSHAKE_AUTH:
        case CMD_REVERSE_AUTH:
        case CMD_AUTH_FAILED:
        case CMD_CACHE_AUTH:
            result = true;
            break;
        default:
            break;
    }

    return result;
}

static bool tme_resend_cmd_check(u8 cmd)
{
    switch(cmd){
        case CMD_NOTIFY_STOP_RECORD:
            return true;
        default:
            return false;
    }
}

static u8 tme_seq_num_get(u8 inc)
{
    if(inc){
      tme_app_var.seq_num++;
    }
    return tme_app_var.seq_num;
}

void tme_init(void)
{
    static u8 poweron_init = 0;

    memset(&tme_app_var, 0, sizeof(tme_app_var));
    memset(&tme_cmd_tx, 0, sizeof(tme_cmd_tx));
    memset(&tme_cmd_rx, 0, sizeof(tme_cmd_rx));

#if TME_OTA_EN
    tme_ota_init();
#endif

    tme_app_var.seq_num = 0xff;

    if(poweron_init == 0){
        poweron_init = 1;
        soft_timer_init();
        tme_auth_timer_creat();
        tme_request_resend_timer_creat();
    }
}

void tme_bredr_disconnect_callback(void)
{
    TME_DEBUG("tme_bredr_disconnect_callback\n");

    if(ble_is_connect()){
        //TME_DEBUG("le_disconnect\n");
        ble_disconnect();
    }
}

void tme_data_send(u8 *data,u16 data_len)
{
    if(tme_app_var.auth_success == 0){
        TME_DEBUG("--->TME_ERR:NOT AUTH\n");
        return;
    }

    tme_cmd_tx.seq = tme_seq_num_get(1);
    tme_cmd_tx.cmd_type = CMD_TYPE_RESPONSE;
    tme_cmd_tx.data_len = data_len;
    tme_cmd_tx.cmd = 0xff;

    if(data_len && (data != tme_cmd_tx.data)){
        memcpy(tme_cmd_tx.data, data, data_len);
    }

    tme_send_proc();
}


void tme_request_send(u8 cmd,u8 *data,u16 data_len)
{
    if((tme_app_var.auth_success == 0) || (tme_app_var.resend_cnt)){
        TME_DEBUG("--->TME_ERR:NOT AUTH\n");
        return;
    }

    tme_cmd_tx.cmd_type = CMD_TYPE_REQUEST;
    tme_cmd_tx.seq = tme_seq_num_get(1);
    tme_cmd_tx.cmd = cmd;
    tme_cmd_tx.data_len = data_len;

    if(data_len){
        memcpy(tme_cmd_tx.data, data, data_len);
    }

    tme_send_proc();

    if(tme_resend_cmd_check(cmd)){
        tme_app_var.resend_cnt = 3;
        soft_timer_restart(tme_resend_timer, 2000);
    }
}

void tme_response_send(void)
{
    tme_cmd_tx.cmd_type = CMD_TYPE_RESPONSE;

    tme_send_proc();
}

void tme_request_non_response_send(u8 cmd,u8 *data,u16 data_len)
{
    if(tme_app_var.auth_success == 0){
        TME_DEBUG("--->TME_ERR:NOT AUTH\n");
        return;
    }

    tme_cmd_tx.seq = tme_seq_num_get(1);
    tme_cmd_tx.cmd = cmd;
    tme_cmd_tx.cmd_type = CMD_TYPE_REQUEST_NON_RESPONSE;
    tme_cmd_tx.data_len = data_len;

    if(data_len){
        memcpy(tme_cmd_tx.data, data, data_len);
    }

    tme_send_proc();
}

void tme_error_response(tme_rsp_err_t err_type)
{
    tme_cmd_tx.data[0] = err_type;
    tme_cmd_tx.data_len = 1;
    tme_response_send();
}

void tme_handshake_auth_response(void)
{
    u8 PubKeyB[48];
    u8 DHKey[24];
    u8 ClearKey_M[32];
    u8 Secret_P[32];
    u8 *PriKeyB = ClearKey_M;  //u8 PriKeyB[24];
    u8 i = 0;

    for(i=0;i<24;i++){
        PriKeyB[i] = get_random(0xff);
    }
    //TME_DEBUG("PriKeyB:\n");
    //TME_DEBUG_R(PriKeyB,24);
    if (!uECC192r1_compute_public_key(PriKeyB,PubKeyB)) {
        TME_DEBUG("uECC_make_key() failed\n");
    }
    //TME_DEBUG("PubKeyB:\n");
    //TME_DEBUG_R(PubKeyB,48);
    if (!uECC192r1_shared_secret(PubKeyA, PriKeyB, DHKey)) {
        TME_DEBUG("shared_secret() failed (1)\n");
    }else{
        //TME_DEBUG("DHKey:\n");
        //TME_DEBUG_R(DHKey,24);
    }
    for(i=0;i<8;i++){
        tme_app_var.random[i] = get_random(0xff);
    }
    //TME_DEBUG("tme_app_var.random:\n");
    //TME_DEBUG_R(tme_app_var.random,8);
    memset(ClearKey_M, 0, 32);
    memcpy(ClearKey_M, &tme_pid, 4);
    memcpy(&ClearKey_M[4], tme_app_var.auth_time, 4);
    memcpy(&ClearKey_M[8], tme_app_var.random, 8);
    for(i=0;i<6;i++){
        ClearKey_M[16+i] = xcfg_cb.bt_addr[5-i];
    }
    //TME_DEBUG("ClearKey_M:\n");
    //TME_DEBUG_R(ClearKey_M,32);
    aes_ecb_encrypt(ClearKey_M, DHKey, Secret_P, 32);
    //TME_DEBUG("ClearKey_M:\n");
    //TME_DEBUG_R(ClearKey_M,32);
    //TME_DEBUG("Secret_P\n");
    //TME_DEBUG_R(Secret_P,32);
    tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
    memcpy(&tme_cmd_tx.data[1], &tme_pid, 4);
    memcpy(&tme_cmd_tx.data[5], &PubKeyB, 48);
    memcpy(&tme_cmd_tx.data[53], &Secret_P, 32);
    tme_cmd_tx.data_len = 85;
    tme_response_send();

    tme_app_var.resend_cnt = 5;
    soft_timer_restart(tme_resend_timer, 2000);
}

void tme_reverse_auth_response(u8* remote_random)
{
    if(memcmp(tme_app_var.random,remote_random,8) == 0){
        tme_app_var.auth_success = 1;
        tme_cm_write(tme_app_var.random, TME_CM_AUTH_RANDOM, 8);
        tme_error_response(RESPONSE_ERR_NONE);
        //memset(tme_app_var.random,0,8);
        //param_random_R_read(tme_app_var.random);
        //TME_DEBUG("tme_app_var.random:\n");
        //TME_DEBUG_R(tme_app_var.random,8);
        TME_DEBUG("-->Reverse Auth Success\n");
    }else{
        tme_cmd_tx.data[0] = RESPONSE_ERR_CRC_RANDOM;
        tme_app_var.auth_success = 0;
        tme_app_var.auth_start = 0;
        tme_error_response(RESPONSE_ERR_CRC_RANDOM);
        TME_DEBUG("-->Reverse Auth Fail\n");
        delay_5ms(4);
        tme_gap_disconnect();
    }
}

void tme_cache_auth_response(u8* remote_random)
{
    tme_app_var.auth_start = 1;
    tme_cm_read(tme_app_var.random, TME_CM_AUTH_RANDOM, 8);

    if(memcmp(tme_app_var.random, remote_random, 8) == 0){
        tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
        tme_app_var.auth_success = 1;
        TME_DEBUG("--->Cache Auth Success\n");
    }else{
        tme_cmd_tx.data[0] = RESPONSE_ERR_CRC_RANDOM;
        tme_app_var.auth_success = 0;
        TME_DEBUG("--->Cache Auth Fail\n");
        TME_DEBUG("Random:\n");
        TME_DEBUG_R(tme_app_var.random,8);
        TME_DEBUG("App_Random\n");
        TME_DEBUG_R(remote_random,8);
    }
    tme_cmd_tx.data_len = 1;
    tme_response_send();
}

void tme_query_mac_response(void)
{
    u8 edr_addr[6];

    bt_get_local_bd_addr(edr_addr);
    tme_cmd_tx.data[0] = RESPONSE_ERR_NONE;
    memcpy(&tme_cmd_tx.data[1], &tme_pid, 4);
    for(u8 i=0;i<6;i++){
        tme_cmd_tx.data[5+i] = edr_addr[5-i];
    }
    tme_cmd_tx.data_len = 11;
    tme_response_send();
}

void tme_cmd_request_receive_proc(u8 cmd,u8 seq_num,u8 *buf,u16 len)
{
    TME_DEBUG("tme_req:%d\n",cmd);

    tme_cmd_tx.cmd = cmd;
    tme_cmd_tx.seq = seq_num;

    if(tme_app_var.auth_success == 0){
        if(tme_auth_cmd_check(cmd) == false){
            TME_DEBUG("--->TME_ERR:NOT AUTH\n");
            return;
        }
    }

    switch(cmd){
        case CMD_QUERY_MAC:   //APP查询设备：PID和MAC地址
            tme_query_mac_response();
            break;

        case CMD_HANDSHAKE_AUTH:   //APP查询设备：握手鉴权，下发时间戳
            memcpy(tme_app_var.auth_time, buf, 4);
            tme_app_var.auth_start = 1;           //握手鉴权开始
            tme_app_var.do_flag |= TME_FLAG_AUTH_RSP;
            break;

        case CMD_REVERSE_AUTH:   //反向鉴权，下发随机数R
            tme_app_var.resend_cnt = 0;
            soft_timer_stop(tme_resend_timer);
            tme_reverse_auth_response(buf);
            break;

        case CMD_CACHE_AUTH:   //缓存鉴权，下发随机数R
            tme_cache_auth_response(buf);
            break;

        case CMD_AUTH_FAILED:   //鉴权失败
            TME_DEBUG("--->CMD_AUTH_FAILED:%d\n",buf[0]);
            tme_app_var.resend_cnt = 0;
            soft_timer_stop(tme_resend_timer);
            tme_app_var.auth_success = 0;
            tme_app_var.auth_start = 0;
            tme_gap_disconnect();
            break;

        case CMD_STOP_RECORD:   //APP设置设备，停止录音
            TME_DEBUG("--->CMD_STOP_RECORD\n");
            bsp_opus_encode_stop();
            tme_app_var.record_start = 0;
            tme_error_response(RESPONSE_ERR_NONE);
            break;

        case CMD_START_RECORD:   //APP设置设备，开始录音
            TME_DEBUG("--->CMD_START_RECORD\n");
            bsp_opus_encode_start();
            tme_app_var.record_start = 1;
            tme_error_response(RESPONSE_ERR_NONE);
            break;

#if TME_OTA_EN
        case CMD_OTA_QUERY_VERSION:   //查询当前固件版本号
        case CMD_OTA_CHECK_CRC:   //固件完整校验命令
        case CMD_OTA_CANCAL:   //取消OTA升级
        case CMD_OTA_QUERY_INFO:   //OTA配置信息获取命令
        case CMD_OTA_MODIFY_NAME_MAC:   //更改固件蓝牙信息
        case CMD_OTA_TRANSPORT_SEGMENT:   //OTA数据传输命令
            tme_cmd_ota_proc(cmd,buf,len);
            break;
#endif

        default:
            break;
    }
}

bool tme_cmd_response_receive_proc(u8 rsp_cmd,u8 rsp_seq,u8 *buf,u16 len)
{
    u16 rsp_data_len = len;
    u8 *rsp_data = buf;

    if((rsp_cmd != tme_cmd_tx.cmd) || (rsp_seq != tme_cmd_tx.seq)){
        return false;
    }

    if(tme_resend_cmd_check(rsp_cmd)){
        tme_app_var.resend_cnt = 0;
        soft_timer_stop(tme_resend_timer);
    }

    switch(rsp_cmd){
        case CMD_NOTIFY_START_RECORD:   //可以开始AI语音
            if((rsp_data_len ==1) && (rsp_data[0] == RESPONSE_ERR_NONE)){
                TME_DEBUG("-->CMD_NOTIFY_START_RECORD_RSP\n");
                tme_app_var.record_start = 1;
                bsp_opus_encode_start();
            }
            break;

        case CMD_NOTIFY_STOP_RECORD:   //结束AI语音
            if((rsp_data_len ==1) && (rsp_data[0] == RESPONSE_ERR_NONE)){
                TME_DEBUG("-->CMD_NOTIFY_STOP_RECORD_RSP\n");
                tme_app_var.record_start = 0;
                bsp_opus_encode_stop();
            }
            break;

        case CMD_NOTIFY_ONESHOT:
            TME_DEBUG("-->CMD_NOTIFY_ONESHOT_RSP:%d\n",rsp_data[0]);
            break;

        default:
            break;
    }

    return true;
}

void tme_cmd_request_non_response_receive_proc(u8 cmd,u8 *buf,u16 len)
{
    switch(cmd){

        default:
            break;
    }
}

void tme_cmd_data_receive_proc(u8 seq_num,u8 *buf,u16 len)
{

}

AT(.text.tme.process)
void tme_recv_proc_do(void)
{
    if(tme_app_var.rx_proc){
        switch(tme_cmd_rx.cmd_type){
            case CMD_TYPE_REQUEST:
                tme_cmd_request_receive_proc(tme_cmd_rx.cmd, tme_cmd_rx.seq, tme_cmd_rx.data, tme_cmd_rx.data_len);
                break;

            case CMD_TYPE_RESPONSE:
                if(tme_cmd_rx.cmd == 0xff){
                    tme_cmd_data_receive_proc(tme_cmd_rx.seq, tme_cmd_rx.data, tme_cmd_rx.data_len);
                }else{
                    tme_cmd_response_receive_proc(tme_cmd_rx.cmd, tme_cmd_rx.seq, tme_cmd_rx.data, tme_cmd_rx.data_len);
                }
                break;

            case CMD_TYPE_REQUEST_NON_RESPONSE:
                tme_cmd_request_non_response_receive_proc(tme_cmd_rx.cmd, tme_cmd_rx.data, tme_cmd_rx.data_len);
                break;

            default:
                break;
        }
        tme_app_var.rx_proc = 0;
    }
}

bool tme_recv_proc(u8 *buf,u16 len)
{
    TME_DEBUG("tme rx: ");
    TME_DEBUG_R(buf,len);

    u32 cmd_head = TME_CMD_HEAD;

    while(tme_app_var.rx_proc){
        delay_5ms(1);
    }

    memcpy(&tme_cmd_rx,buf,len);

    if(memcmp(tme_cmd_rx.cmd_head, &cmd_head, 3)){
        TME_DEBUG("tme head err!!!!\n");
        return false;
    }

    if(tme_cmd_rx.data[tme_cmd_rx.data_len] != TME_CMD_TAIL){
        TME_DEBUG("tme tail err!!!!\n");
        return false;
    }

    tme_app_var.rx_proc = 1;

    if(tme_app_var.auth_success == 0){
        tme_recv_proc_do();
    }


    return true;
}

void tme_record_start(void)
{
    TME_DEBUG("tme_record_start_request\n");

    if(tme_app_var.auth_success){
        tme_request_send(CMD_NOTIFY_START_RECORD, NULL, 0);
    }else{
        bt_tws_user_key(CMD_NOTIFY_START_RECORD);
    }
}

void tme_record_stop(void)
{
    TME_DEBUG("tme_record_stop_request\n");

    bsp_opus_encode_stop();
    tme_app_var.record_stop = 1;
}

void tme_oneshot_request(tme_oneshot_code_t oneshot_code)
{
    tme_request_send(CMD_NOTIFY_ONESHOT, (u8*)&oneshot_code, 2);
}

AT(.text.tme.process)
void tme_opus_data_proc(void)
{
    u8 test_crc[4] = {0x00, 0x00, 0x00, 0x00};
    u8 *p_data = &tme_cmd_tx.data[1];

    if(tme_app_var.tme_connect_sta == 0){
        if(bsp_opus_is_encode()){
            bsp_opus_encode_stop();
        }
    }

    if(tme_app_var.record_start && (opus_enc_data_len_get() >= (40*4)) && tme_app_var.auth_success){
        tme_cmd_tx.data[0] = 4;   //一次传输4帧
        for(u8 i=0;i<4;i++){
            bsp_opus_get_enc_frame(&p_data[i*46+6],40);
            //TME_DEBUG("frame_energy  %d\n",tme_app_var.frame_energy[i]);
            memcpy(&p_data[i*46], &tme_app_var.frame_energy[i], 2);
            memcpy(&p_data[i*46+2], test_crc, 4);
        }
        tme_data_send(tme_cmd_tx.data,185);
    }else if(tme_app_var.record_stop){

        TME_DEBUG("-->CMD_NOTIFY_STOP_RECORD\n");
        tme_request_send(CMD_NOTIFY_STOP_RECORD,NULL,0);
        tme_app_var.record_stop = 0;
    }

}


AT(.text.tme.process)
void tme_ble_adv_data_update_proc(void)
{
    static u8 bt_edr_connect = 0;

    if(bt_nor_is_connected()){
        bt_edr_connect = 1;
        if(tme_app_var.tme_connect_sta == 0){
            if(adv_data_buf[17] != 0x10){
               adv_data_buf[17] = 0x10;
               ble_set_adv_data(adv_data_buf, sizeof(adv_data_buf));
            }
        }
    }else{
        if(bt_edr_connect){
            if(adv_data_buf[17] != 0x00){
               adv_data_buf[17] = 0x00;
               ble_set_adv_data(adv_data_buf, sizeof(adv_data_buf));
            }
        }
    }
}

AT(.text.tme.process)
void tme_do_flag_proc(void)
{
    if(tme_app_var.do_flag){
        if(tme_app_var.do_flag & TME_FLAG_AUDIO_BYPASS){
            tme_app_var.do_flag &= ~TME_FLAG_AUDIO_BYPASS;
            bt_audio_bypass();
            tme_app_var.bt_bypass = 1;
        }
        if(tme_app_var.do_flag & TME_FLAG_AUDIO_ENABLE){
            tme_app_var.do_flag &= ~TME_FLAG_AUDIO_ENABLE;
            bt_audio_enable();
            tme_app_var.bt_bypass = 0;
        }
        if(tme_app_var.do_flag & TME_FLAG_AUTH_RSP){
            tme_app_var.do_flag &= ~TME_FLAG_AUTH_RSP;
            tme_handshake_auth_response();
        }
    }
}

AT(.text.tme.process)
void tme_process(void)
{
    soft_timer_run();

    tme_do_flag_proc();

    tme_recv_proc_do();

    tme_ble_adv_data_update_proc();

    tme_opus_data_proc();
}

void tme_tws_user_key_process(uint32_t *opcode)
{
    u8 *packet = (u8*)opcode;

    switch(packet[0]){

    case CMD_NOTIFY_START_RECORD:
        if(tme_app_var.auth_success){
            tme_request_send(CMD_NOTIFY_START_RECORD, NULL, 0);
        }
        break;

    case OPUS_ENC_START:
        tme_app_var.do_flag |= TME_FLAG_AUDIO_BYPASS;
        break;

    case OPUS_ENC_STOP:
        if(tme_app_var.bt_bypass){
            tme_app_var.do_flag |= TME_FLAG_AUDIO_ENABLE;
        }
        break;

    default:
        break;
    }
}

void tme_disconnect_proc(void)
{
    tme_app_var.tme_connect_sta = 0;

    if(tme_app_var.auth_success){
        tme_init();
    }
    soft_timer_stop(tme_resend_timer);
}

void tme_connect_proc(void)
{
    soft_timer_restart(tme_auth_timer,4500);
}

void tme_ble_disconnect_callback(void)
{
    printf("--->tme_ble_disconnect_callback\n");

    tme_disconnect_proc();
}

void tme_ble_connect_callback(void)
{
    printf("--->tme_ble_connect_callback\n");

    tme_app_var.tme_connect_sta = TME_CONNECT_BLE;

    tme_connect_proc();
}

void tme_spp_connect_callback(void)
{
    printf("--->tme_spp_connect_callback\n");

    tme_app_var.tme_connect_sta = TME_CONNECT_SPP;

    tme_connect_proc();

    ble_disable_adv();
}

void tme_spp_disconnect_callback(void)
{
    printf("--->tme_spp_disconnect_callback\n");

    tme_disconnect_proc();

    if((ble_is_connect() == 0) && (bt_tws_is_slave() == 0)){
        ble_enable_adv();
    }
}

void tme_tws_disconnect_callback(void)
{
    if(tme_app_var.auth_success == 0){
        if(tme_app_var.bt_bypass){
            tme_app_var.do_flag |= TME_FLAG_AUDIO_ENABLE;
        }
    }

    if(!tme_app_var.tme_connect_sta){
        ble_adv_en();
    }
}

uint16_t role_switch_get_user_data(uint8_t *data_ptr)
{
    u8 offset = 0;

    if(tme_app_var.tme_connect_sta){
        data_ptr[offset++] = tme_app_var.tme_connect_sta;
        data_ptr[offset++] = tme_app_var.auth_start;
        data_ptr[offset++] = tme_app_var.auth_success;
		data_ptr[offset++] = tme_app_var.seq_num;

        TME_DEBUG_R(data_ptr,offset);
    }

	tme_disconnect_proc();

    return offset;
}

uint16_t role_switch_set_user_data(uint8_t *data_ptr, uint16_t len)
{
    u8 offset = 0;

    if(!tme_app_var.tme_connect_sta && len){
        tme_app_var.tme_connect_sta = data_ptr[offset++];
        tme_app_var.auth_start = data_ptr[offset++];
        tme_app_var.auth_success = data_ptr[offset++];
        tme_app_var.seq_num = data_ptr[offset++];

        if(!tme_app_var.auth_start){
            soft_timer_restart(tme_auth_timer,4500);
        }

        TME_DEBUG_R(data_ptr,offset);
    }

    return offset;
}

void tme_bt_evt_notice(uint evt, void *params)
{

    switch(evt){
        case BT_NOTICE_DISCONNECT:
            tme_bredr_disconnect_callback();
            break;

        case BT_NOTICE_CONNECTED:
            break;

        case BT_NOTICE_TWS_DISCONNECT:
            tme_tws_disconnect_callback();
            break;

        case BT_NOTICE_TWS_CONNECTED:
            break;

        case BT_NOTICE_TWS_ROLE_CHANGE:
            break;
    }
}

#endif

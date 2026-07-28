#ifndef __TME_APP_H
#define __TME_APP_H

#include "tme_timer.h"
#include "tme_opus.h"

#if TME_APP_EN

// Function config
#define TME_OTA_EN          0

//Const
#define TME_CMD_HEAD        0xfdfcfa
#define TME_CMD_TAIL        0xfb

//Cmd
#define CMD_NOTIFY_START_RECORD         101 //设备通知APP：开始AI语音，常用于按键触发AI语音
#define CMD_STOP_RECORD                 102	//APP设置设备：停止录音，常用于APP VAD
#define CMD_QUERY_MAC                   103 //APP查询设备：PID、蓝牙MAC地址。 Response：PID、MAC
#define CMD_START_RECORD                104 //APP设置设备：开始录音
#define CMD_AUDIO_CONFIG                105	//APP查询设备：支持录音格式。 Response：录音配置参数
#define CMD_NOTIFY_STOP_RECORD          108	//设备通知APP：停止AI语音，常用于说完松开按键时

#define CMD_HANDSHAKE_AUTH              120 //APP查询设备：握手鉴权。 Request：时间戳 Response：设备鉴权信息
#define CMD_REVERSE_AUTH                121	//APP查询设备：反向鉴权。 Request：APP鉴权信息
#define CMD_AUTH_FAILED                 122	//APP设置设备：鉴权失败，即将断开连接。 Request：失败原因
#define CMD_WRITE_COOKIE                123	//APP设置设备：写Cookie。  Request：6字节Cookie值
#define CMD_CACHE_AUTH                  124	//APP查询设备：缓存鉴权	Request：最近一次鉴权成功后生成的随机数

#define CMD_NOTIFY_ONESHOT              130 //设备通知APP：一键直达。 Request：功能码

//248	APP查询设备: 获取OTA配置信息		Request：新固件信息   Response：设备配置信息
//247	APP设置设备: 更改固件蓝牙信息		Request：是否更改蓝牙名称 MAC地址
//246	APP查询设备: 检查断点续传			Request：当前固件版本md5信息   Response：是否需要断点续传及相关信息
//245	APP设置设备: 传输ota文件分片数据	Request：每次传输一个Segment，回复成功才传输下一段
//243	APP设置设备: 校验完整固件CRC		Request：固件完整的CRC数据
//242	APP设置设备: 取消OTA
//240	设备通知APP: 设备上报OTA结果
#define CMD_OTA_QUERY_INFO          248
#define CMD_OTA_MODIFY_NAME_MAC     247
#define CMD_OTA_CHECK_BREAK_POINT   246
#define CMD_OTA_TRANSPORT_SEGMENT   245
#define CMD_OTA_QUERY_VERSION       244
#define CMD_OTA_CHECK_CRC           243
#define CMD_OTA_CANCAL              242

//CM
#define TME_CM_AUTH_RANDOM          0                               //8Byte
#define TME_CM_OTA_CRC32            (TME_CM_AUTH_RANDOM + 8)        //4Byte

//Flag
#define TME_FLAG_AUDIO_BYPASS       BIT(0)
#define TME_FLAG_AUDIO_ENABLE       BIT(1)
#define TME_FLAG_AUTH_RSP           BIT(2)

typedef enum{
    CMD_TYPE_REQUEST = 1,
    CMD_TYPE_RESPONSE,
    CMD_TYPE_REQUEST_NON_RESPONSE,
}tme_cmd_type_t;


typedef enum{
    RESPONSE_ERR_NONE = 0,                  //成功
    RESPONSE_ERR_CMD,                       //非法命令
    RESPONSE_ERR_PARA_LONG,                 //参数超长
    RESPONSE_ERR_PARA_SHORT,                //参数太短
    RESPONSE_ERR_CMD_EXEC,                  //命令执行出差
    RESPONSE_ERR_TIMEOUT,                   //等待回复超时
    RESPONSE_ERR_TRANSFER_STARTING,         //数据流传输已开始
    RESPONSE_ERR_TRANSFER_NOT_START,        //数据流传输未开始
    RESPONSE_ERR_CRC_PART,                  //数据块CRC校验失败
    RESPONSE_ERR_CRC_ALL,                   //全部数据CRC校验失败
    RESPONSE_ERR_DATA_LEN,                  //数据接收长度出错
    RESPONSE_ERR_RECORD_OPEN,               //无法打开录音通道
    RESPONSE_ERR_SPEECH_INIT,               //语音服务初始化失败
    RESPONSE_ERR_CRC_RANDOM,                //随机数校验失败

    RESPONSE_ERR_OTA_LOW_POWER = 61,                 //电池电量小于50%
    RESPONSE_ERR_OTA_CONFIG,                         //无法解析OTA配置信息
    RESPONSE_ERR_OTA_CRC_FIRMWARE_DATA,              //固件段数据CRC校验失败
    RESPONSE_ERR_OTA_CRC_FIRMWARE_DATA_WHOLE,        //固件完整校验失败
    RESPONSE_ERR_OTA_DATA_LEN,                       //接收到的数据长度出错
    RESPONSE_ERR_OTA_DATA_OFFSET,                    //数据偏移值错误
    RESPONSE_ERR_OTA_FIRMWARE_LEN,                   //固件大小错误
    RESPONSE_ERR_OTA_CRC_UPDATE_DATA,                //升级数据校验出错
    RESPONSE_ERR_OTA_UPDATE_FAIL,                    //升级失败
    RESPONSE_ERR_OTA_KEY,                            //加密key不匹配
    RESPONSE_ERR_OTA_UPDATE_FILE,                    //升级文件出错
    RESPONSE_ERR_OTA_UBOOT,                          //uboot不匹配
}tme_rsp_err_t;

typedef enum{
    TME_TWS_OTA_QUERY_INFO = 1,
    TME_TWS_OTA_TRANSPORT_START,
    TME_TWS_OTA_TRANSPORT_SEGMENT,
    TME_TWS_OTA_UPDATE_DONE,
    TME_TWS_OTA_ERR,
}tme_tws_info_sync_t;

typedef enum{
    TME_ONESHOT_COLLECTION_CTL = 0,     //收藏/取消收藏
    TME_ONESHOT_COLLECTION_PLAY,        //播放收藏（"我喜欢"）歌单
    TME_ONESHOT_RECOMMEND_PLAY,         //播放个性化推荐歌曲
    TME_ONESHOT_SOUNDHOUND_START,       //启动听歌识曲
    TME_ONESHOT_EQ_CTL,                 //开/关音效
    TME_ONESHOT_RUNNING_RADIO,          //跑步电台
}tme_oneshot_code_t;

enum{
    TME_CONNECT_NONE = 0,
    TME_CONNECT_SPP,
    TME_CONNECT_BLE,
};


typedef struct __attribute__((packed)) {
    u8 cmd_head[3];
    u8 cmd;
    u8 cmd_type;
    u8 seq;
    u16 data_len;
    u8 data[200];
}tme_cmd_t;


typedef struct{
    volatile u8 seq_num;
    u8 auth_start;
    u8 auth_success;       //握手鉴权成功标志
    u8 record_start;
    u8 record_stop;
    u8 tme_connect_sta;
    u8 rx_proc;
    u8 resend_cnt;
    u8 random[8];
    u8 bt_bypass;
    u16 frame_energy[4];
    u8 auth_time[4];
    u16 do_flag;
}tme_app_var_t;

extern tme_cmd_t tme_cmd_tx;
extern tme_app_var_t tme_app_var;

const u8 *tme_firmware_version_get(void);
const u32 tme_pid_get(void);
void tme_init(void);
void tme_process(void);
bool tme_recv_proc(u8 *buf,u16 len);
void tme_error_response(tme_rsp_err_t err_type);
void tme_record_start(void);
void tme_record_stop(void);
void tme_oneshot_request(tme_oneshot_code_t oneshot_code);
void tme_response_send(void);
void tme_cm_write(void *buf, u16 addr, u16 size);
void tme_cm_read(void *buf, u16 addr, u16 size);
void tme_ble_disconnect_callback(void);
void tme_ble_connect_callback(void);
void tme_spp_connect_callback(void);
void tme_spp_disconnect_callback(void);
void tme_tws_user_key_process(uint32_t *opcode);
u8 tme_is_need_wakeup(void);
void tme_enter_sleep(void);
void tme_exit_sleep(void);
void tme_bt_evt_notice(uint evt, void *params);

#endif

#endif

#ifndef _BSP_TBOX_QTEST_H
#define _BSP_TBOX_QTEST_H


enum{
    QTEST_STA_INBOX,
    QTEST_STA_OUTBOX,
    QTEST_STA_DCIN,
};

#define DC_IN()                  ((RTCCON >> 20) & 0x01)            //VUSB Online state:    0->not online, 1->online

#define QTEST_MODE_DUT           '3'
#define QTEST_MODE_CLR_INFO      '4'
#define QTEST_MODE_PWROFF        '5'
#define QTEST_MODE_PWROFF_LP     '6'              //船运模式

void bsp_qtest_init(void);
void qtest_tws_message_init(void);
void qtest_set_pickup_sta(u8 sta);
void qtest_exit(void);
void qtest_beforehand_process(void);
void bsp_tbox_qest_packet_recv(u8 data);

u8 qtest_get_mode(void);
u8 qtest_get_pickup_sta(void);
u8 qtest_init(u8 rst_source);
void qtest_process(void);
void qtest_create_env(void);
bool qtest_is_send_btmsg(void);
void qtest_send_msg2tbox(u8 *buf,u16 len);
void qtest_other_usage_process(void);
void sys_set_qtest_flag(u8 flag);
void qtest_dec_heart_pkt_delay(void);
bool qtest_set_pdn_boat_flag(u8 flag);
void param_bt_qpdn_boat_flag_write(u8 flag);
void param_bt_qpdn_boat_flag_read(void);

u8 qtest_get_heart_pkt_delay(void);
u8* qtest_get_txbuf(void);
uint32_t bt_tws_get_pair_id(void);
extern u32 get_spiloader_offset(void);

typedef struct {
    volatile u8 flag;
    u8 sta;
    u8 ack;
    u8 inbox_cnt;
    u8 other_usage_txbuf[12];
    u8 pdn_boat_flag;
}qtest_cb_t;

extern qtest_cb_t qtest_cb;
#endif // _BSP_TBOX_QTEST_H

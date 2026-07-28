#include "include.h"

#if VUSB_SMART_VBAT_HOUSE_EN

#define TRACE_EN                0

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#if VUSB_HUART_DMA_EN
extern u8 *huart_rx_buf;
#endif

void bt_get_local_bd_addr(u8 *addr);
void bt_tws_put_link_info_addr(uint8_t *bd_addr,uint8_t* data);
void bt_tws_put_link_info_feature(uint8_t *bd_addr, uint8_t feature);
u8 qtest_get_mode(void);

AT(.com_rodata.vhouse_tbl)
const char vhouse_header[]={0x55,0xAA,0xFF};

bool vhouse_bat_is_ready(void)
{
    return vhouse_cb.rem_bat_ok;
}

u8 bsp_vhouse_get_bat_for_channel(bool left_channel)
{
    if (!bt_tws_is_connected() && !vhouse_bat_is_ready()) { //未连接时且智能充电仓未准备好，通过本地获取
        return sys_cb.loc_bat & 0x7f;
    } else {
        if(sys_cb.tws_left_channel == left_channel) {
            return sys_cb.loc_bat & 0x7f;
        } else {
            return sys_cb.rem_bat & 0x7f;
        }
    }
}

u8 bsp_vhouse_get_house_bat_level(void)
{
    if(bt_tws_is_connected()){
        if((sys_cb.loc_bat & BIT(7)) == 0 && (sys_cb.rem_bat & BIT(7)) != 0) {
            return sys_cb.rem_house_bat & 0x7f;
        }
    }

    return sys_cb.loc_house_bat & 0x7f;
}

#if !VUSB_HUART_DMA_EN
//uart中断解析并匹配命令包
AT(.com_text.bsp.uart.vusb)
void bsp_vhouse_packet_recv(u8 data)
{
#if VUSB_TBOX_QTEST_EN
    if(qtest_get_mode()){
        return ;
    }
#endif
    vh_packet_t *p = &vh_packet;

    u32 parse_done = bsp_vhouse_packet_parse(p, data);

    if (parse_done) {                               //心跳包数据放在中断实时更新
        vhouse_cb.ticks = tick_get();
        if (p->cmd == VHOUSE_CMD_OPEN_WINDOW) {
            vhouse_cb.win_ticks = vhouse_cb.ticks;
            vhouse_cb.open_win_flag = 1;
        }

        if((!vhouse_cb.ack_dat_confirm) || (vhouse_cb.ack_dat == p->buf[0])){
            vhouse_cb.ack_dat = p->buf[0];
            if((p->cmd != VHOUSE_CMD_PAIR) && (p->cmd != VHOUSE_CMD_GET_TWS_BTADDR)){
                vhouse_cb.need_ack = 1;
            }
        }
    }
}

#else


//uart中断解析并匹配命令包
AT(.com_text.bsp.uart.vusb)
void bsp_vhouse_packet_dma_recv(u8 *buf)
{
    if(qtest_get_mode()){
        return ;
    }
    if(!memcmp(buf,vhouse_header,3)){
        if(buf[4] < VH_DATA_LEN){
            vhouse_cb.ticks = tick_get();
            if (buf[3] == VHOUSE_CMD_OPEN_WINDOW) {
                vhouse_cb.win_ticks = vhouse_cb.ticks;
                vhouse_cb.open_win_flag = 1;
            }
            if((!vhouse_cb.ack_dat_confirm) || (vhouse_cb.ack_dat == buf[5])){
                vhouse_cb.ack_dat = buf[5];
                if((buf[3] != VHOUSE_CMD_PAIR) && (buf[3] != VHOUSE_CMD_GET_TWS_BTADDR)){
                    vhouse_cb.need_ack = 1;
                }
            }
            vhouse_cb.rx_flag=1;
        }
    }
}

#endif // VUSB_HUART_DMA_EN
//接收心跳包后需要5~10ms回应1字节
AT(.com_text.timer)
void bsp_vhouse_cmd_ack(void)
{
    if (vhouse_cb.need_ack == 1) {
        if (tick_check_expire(vhouse_cb.ticks, 6)) {
#if VUSB_HUART_DMA_EN
            huart_putcs((u8*)&vhouse_cb.ack_dat,1);
#else
            UART1DATA = vhouse_cb.ack_dat;
#endif
            vhouse_cb.need_ack = 2;
        }
    }
}

AT(.com_text.vhouse)
void bsp_vhouse_inbox_sta(u8 sta)
{
    if (sta) {
        bool new_sta = vhouse_cb.inbox_sta;
        if (sta & BIT(1)) {
            new_sta = true;
        } else if (sta & BIT(0)) {
            new_sta = false;
        }

        if(new_sta != vhouse_cb.inbox_sta) {
            vhouse_cb.inbox_sta = new_sta;
            vhouse_cb.update_ear_flag = true;
        }
    }
}

AT(.text.vhouse)
void bsp_vhouse_update_sta(void)
{
    vhouse_cb.update_ear_flag = true;
}

AT(.text.vhouse)
void vhouse_send_data(u8 *buf,u8 len)
{
    //超过5ms没有接收到VBUS程序才发送数据
    while (!tick_check_expire(vhouse_cb.ticks, 5)) {
         WDT_CLR();
    }
    if(vhouse_cb.need_ack == 2) {
        while (!(UART1CON & BIT(8)));
    }
    vhouse_cb.need_ack = 0;

    if(tick_check_expire(vhouse_cb.ticks, 80)){
        return;
    }

#if VUSB_HUART_DMA_EN
    huart_putcs(buf,len);
#else
    for(u8 i = 0; i < len; i++){
        vusb_uart_putchar(buf[i]);
    }
#endif

}

AT(.text.vhouse)
u8 vhouse_cmd_ack(vh_packet_t *packet)
{
    u8 *buf = (u8*)packet;

    u8 length = 5 + packet->length;
    buf[length] = crc8_maxim(buf, length);
    vhouse_send_data(buf, length + 1);

    return true;
}

//AT(.text.vhouse)
//void vhouse_send_message_err(void)
//{
//    TRACE("left_right_channel_err\n");
//    vh_packet_t *packet = &vhouse_cb.packet;
//    packet->header = 0xAA55;
//    packet->distinguish = VHOUSE_DISTINGUISH;
//    packet->cmd = VHOUSE_CMD_ERR;
//    packet->length = 0;
//    vhouse_cmd_ack(packet);
//}
//

AT(.text.vhouse)
void vhouse_send_message_suceess(void)
{
    vh_packet_t *packet = &vhouse_cb.packet;
    packet->header = 0xAA55;
    packet->distinguish = VHOUSE_DISTINGUISH;
    packet->cmd = VHOUSE_CMD_SUCCESS;
    packet->length = 0;
    vhouse_cmd_ack(packet);
}


AT(.text.vhouse)
void bsp_vhouse_update_bat_adv(void)
{
    if(vhouse_cb.update_ear_flag) {
        vhouse_cb.update_ear_flag = false;

        bool old_sta = (bool)(sys_cb.loc_bat & BIT(7));
        bool new_sta = vhouse_cb.inbox_sta;
        if(old_sta != new_sta) {
            if(new_sta) {
                sys_cb.loc_bat |= BIT(7);
            } else {
                sys_cb.loc_bat &= ~BIT(7);
            }
        }
    }
}

AT(.text.vhouse)
static void bsp_vhouse_update_bat_value(vh_packet_t *packet)
{
    bool update_flag = false;
    if ((packet->cmd == VHOUSE_CMD_OPEN_WINDOW) || (packet->cmd == VHOUSE_CMD_GET_VBAT)||packet->cmd == VHOUSE_CMD_CLOSE_WIN_GET_VBAT) {
        if(sys_cb.loc_house_bat != packet->buf[1]) {
            sys_cb.loc_house_bat = packet->buf[1];
            sys_cb.rem_house_bat = packet->buf[1];
            update_flag = true;
        }
        if(packet->cmd == VHOUSE_CMD_CLOSE_WIN_GET_VBAT) {
            return;
        }
        if(packet->buf[2] & BIT(7)) {
            if(sys_cb.rem_bat != packet->buf[2]) {              //对方在仓内，且电量不为0时，用仓互传
                if((packet->buf[2]&0x7f) == 0) {
                    sys_cb.rem_bat |= BIT(7);
                } else {
                    sys_cb.rem_bat = packet->buf[2];
                    vhouse_cb.rem_bat_ok = true;
                }
                update_flag = true;
            }
        } else {
            if(vhouse_cb.rem_bat_ok) {
                update_flag = true;
            }
            vhouse_cb.rem_bat_ok = false;                       //对方出仓后，电量通过TWS互传
        }

        if(!vhouse_cb.inbox_sta) {
            vhouse_cb.inbox_sta = true;
            update_flag = true;
        }
    }

    vhouse_cb.update_ear_flag = update_flag;
}

//分析电量接收包并回应相关电量数据
AT(.text.vhouse)
void bsp_vhouse_vbat_ack(vh_packet_t *packet)
{
    vhouse_cb.cmd3_rx_flag = 0;
    u8 channel = packet->buf[0];
    if (!bsp_vusb_channel_check(channel)) {
//        vhouse_send_message_err();
        return;
    }
    bsp_vhouse_update_bat_value(packet);

    //发送电量响应包
    packet->header = 0xAA55;
    packet->distinguish = VHOUSE_DISTINGUISH;
    packet->length = 0x04;
    packet->buf[0] = channel;
    packet->buf[2] = sys_cb.loc_bat;
    packet->buf[3] = sys_cb.charge_once_full ? 2 : sys_cb.charge_sta;
    vhouse_cmd_ack(packet);
}

#if CHARGE_BOX_LK_CRC_EN
AT(.text.charge_box)
static u16 charge_box_get_tws_link_key_crc(u8* tws_addr)
{
    u8 link_key_info[16];
    u16 crc = get_random(0xffff);
    bool ret = bt_tws_get_link_info_key(tws_addr,link_key_info);
    if (ret) {
        crc = calc_crc(link_key_info, 16, 0xffff);
    }
    return crc;
}
#endif

AT(.text.vhouse)
void bsp_vhouse_pair_ack(vh_packet_t *packet)
{
    u8 channel = packet->buf[0];
    if (!bsp_vusb_channel_check(channel)) {
        return;
    }
    if((channel != VHOUSE_LEFT_CHANNEL)||vhouse_cb.cmd3_rx_flag){
        //发送蓝牙地址包
        u8 bt_tws_addr[6];
        u8 feature = bt_tws_get_link_info(bt_tws_addr);
        memset(packet, 0, sizeof(vh_packet_t));
        packet->header = 0xAA55;
        packet->distinguish = VHOUSE_DISTINGUISH;
        packet->cmd = VHOUSE_CMD_GET_TWS_BTADDR;
        packet->length = 14;
        packet->buf[0] = (channel == VHOUSE_RIGHT_CHANNEL) ? VHOUSE_LEFT_CHANNEL : VHOUSE_RIGHT_CHANNEL; //发送对方声道
        memcpy(packet->buf+1, bt_tws_addr, 6);              //TWS地址
        memcpy(packet->buf+7, xcfg_cb.bt_addr, 6);          //本地的地址
        packet->buf[13] = feature;                          //TWS主从Feature

#if CHARGE_BOX_LK_CRC_EN
        u16 linkkey_crc = charge_box_get_tws_link_key_crc(bt_tws_addr);
        packet->length += 2;
        packet->buf[14] = linkkey_crc;
        packet->buf[15] = linkkey_crc >> 8;
#endif

        vhouse_cmd_ack(packet);
        if (!bt_tws_is_slave()) {
            bt_nor_disconnect();
            ble_adv0_idx_update();
        }
    }else{
        vhouse_cb.cmd3_rx_flag = 1;
    }

}

AT(.text.vhouse)
void bsp_vhouse_get_tws_btaddr_ack(vh_packet_t *packet)
{
    vhouse_cb.cmd3_rx_flag = 1;
    u8 tws_role = 0;
    u8 bt_tws_addr[6], new_addr[6];
    u8 channel = packet->buf[0];
    u8 pkt_feature = packet->buf[13];
    u8 feature = bt_tws_get_link_info(bt_tws_addr);     //获取TWS地址及feature

    if (!bsp_vusb_channel_check(channel)) {
        return;
    }
#if CHARGE_BOX_LK_CRC_EN
    u16 loc_linkkey_crc = charge_box_get_tws_link_key_crc(bt_tws_addr);
	u16 rem_linkkey_crc = packet->buf[14] | (packet->buf[15] << 8);
#endif
    //TWS地址不匹配或Feature不对需要清除配对信息
    if ((memcmp(packet->buf+1, bt_tws_addr, 6) != 0) ||
#if CHARGE_BOX_LK_CRC_EN
        (loc_linkkey_crc != rem_linkkey_crc) ||
#endif
        (feature == pkt_feature) || (feature == 0) || (pkt_feature == 0)) {

		if(bt_tws_is_connected()){
			bt_tws_disconnect();
		}

        tws_role = vusb_get_tws_role();
        TRACE("BT_ADDDR_IS_DIFF: %d\n", tws_role);
        bt_tws_clr_link_info();
        if (tws_role) {
            memcpy(new_addr, packet->buf + 7, 6);     //保存master
        } else {
            memcpy(new_addr, xcfg_cb.bt_addr, 6);
        }
		tws_update_local_addr(new_addr);
        u8 data[4];
        data[0] = new_addr[2];
        data[1] = new_addr[3];
        data[2] = new_addr[4];
        data[3] = new_addr[5];
        bt_tws_put_link_info_addr(new_addr,data);
        bt_tws_put_link_info_feature(new_addr, tws_role);
#if 0
        func_cb.sta = FUNC_NULL;
#endif
    }
    if (sys_cb.tws_force_channel == 1) {
         vhouse_send_message_suceess();
    }

	if(tws_role){
		bt_tws_connect();
	}
}

AT(.text.vhouse)
void bsp_vhouse_get_ear_addr(vh_packet_t *packet)
{
    u8 channel = packet->buf[0];
    if (!bsp_vusb_channel_check(channel)) {
        return;
    }

    //发送蓝牙地址包
    u8 bt_tws_addr[6];
    u8 feature = bt_tws_get_link_info(bt_tws_addr);
    memset(packet, 0, sizeof(vh_packet_t));
    packet->header = 0xAA55;
    packet->distinguish = VHOUSE_DISTINGUISH;
    packet->cmd = VHOUSE_CMD_GET_EAR_ADDR;
    packet->length = 16;
    packet->buf[0] = channel;
    memcpy(packet->buf+1, bt_tws_addr, 6);              //TWS地址
    memcpy(packet->buf+7, xcfg_cb.bt_addr, 6);          //本地的地址
    packet->buf[13] = feature;                          //TWS主从Feature

    u16 link_key_sum = 0;
    u8 link_key[16];
    memset(link_key, 0, 16);
    btstack_get_link_key(bt_tws_addr, link_key, NULL, 1);
    for (u8 i = 0; i < 16; i++) {
        link_key_sum += link_key[i];
    }
    memcpy(packet->buf+14, (u8 *)&link_key_sum, 2);

    vhouse_cmd_ack(packet);
}

AT(.text.vhouse)
void bsp_vhouse_open_windows(void)
{
    vhouse_cb.open_win_flag = 0;
    vhouse_cb.status = 1;                           //开窗，停止充电
}

u8 bsp_vhouse_popup_is_enable(void)
{
    return sys_cb.popup_en;
}

AT(.text.vhouse)
void bsp_vhouse_popup_ctrl(vh_packet_t *packet)
{
    u8 channel = packet->buf[0];
    u8 data = packet->buf[1];
    if (!bsp_vusb_channel_check(channel)) {
        return;
    }

    if ((data == 1) || (data == 2)) {                   //开或关广播功能
        sys_cb.popup_en = data - 1;
        ble_adv0_set_ctrl(sys_cb.popup_en);
    } else {                                            //翻转广播功能
        if (sys_cb.popup_en) {
            sys_cb.popup_en = 0;
            ble_adv0_set_ctrl(0);
        } else {
            sys_cb.popup_en = 1;
            ble_adv0_set_ctrl(2);
        }
    }

    param_vuart_popup_flag_write(sys_cb.popup_en + 1);
}

void bsp_vhouse_cmd_notice(u8 cmd)
{
    le_popup_vhouse_cmd_notice(cmd);
}

AT(.text.vhouse)
void bsp_vhouse_analysis_packet(vh_packet_t *packet)
{
    u8 cmd = packet->cmd;

    switch (packet->cmd) {
        case VHOUSE_CMD_GET_VBAT:
            TRACE("VHOUSE_CMD_GET_VBAT\n");
            bsp_vhouse_vbat_ack(packet);
            break;

        case VHOUSE_CMD_PAIR:
            TRACE("VHOUSE_CMD_PAIR\n");
            bsp_vhouse_pair_ack(packet);
            break;

        case VHOUSE_CMD_GET_TWS_BTADDR:
            TRACE("VHOUSE_CMD_GET_TWS_BTADDR\n");
            bsp_vhouse_get_tws_btaddr_ack(packet);
            break;

        case  VHOUSE_CMD_CLEAR_PAIR:
            TRACE("VHOUSE_CMD_CLEAR_PAIR\n");
            bt_clr_all_link_info();                         //删除所有配对信息
            break;

        case  VHOUSE_CMD_PWROFF:
            TRACE("VHOUSE_CMD_PWROFF\n");
            if (bsp_vusb_channel_check(packet->buf[0])) {
                vhouse_cb.status = 2;                           //充满电
                func_cb.sta = FUNC_PWROFF;
            }
            break;

        case VHOUSE_CMD_CLOSE_WINDOW:
            TRACE("VHOUSE_CMD_CLOSE_WINDOW\n");
            vhouse_cb.open_win_flag = 0;
            vhouse_cb.status = 0;                           //关盖, 充电
            bsp_vhouse_vbat_ack(packet);
            break;

        case VHOUSE_CMD_OPEN_WINDOW:
            TRACE("VHOUSE_CMD_OPEN_WINDOW\n");
            bsp_vhouse_open_windows();
            bsp_vhouse_vbat_ack(packet);
            break;

        case VHOUSE_CMD_ENABLE_POPUP:
            bsp_vhouse_popup_ctrl(packet);
            break;

        case VHOUSE_CMD_SYS_RST:
            sw_reset_kick(SW_RST_FLAG);
            break;

        case VHOUSE_CMD_GET_EAR_ADDR:
            TRACE("VHOUSE_CMD_GET_EAR_ADDR\n");
            bsp_vhouse_get_ear_addr(packet);
            break;

        default:
#if IODM_TEST_MODE
            iodm_reveice_data_deal(packet);
#endif // IODM_TEST_MODE
            break;
    }

    bsp_vhouse_cmd_notice(cmd);
}

AT(.text.vhouse)
void bsp_vhouse_analysis_packet_for_charge(vh_packet_t *packet)
{
    switch (packet->cmd) {
        //仓关盖后获取电量
        case VHOUSE_CMD_CLOSE_WIN_GET_VBAT:
            TRACE("VHOUSE_DISP_VBAT\n");
            bsp_vhouse_vbat_ack(packet);
            vhouse_cb.open_win_flag = 0;
			vhouse_cb.status = 0;                           //关盖, 充电
            break;

        //仓开盖后获取电量
        case VHOUSE_CMD_OPEN_WINDOW:
            TRACE("VHOUSE_CMD_OPEN_WINDOW\n");
        case VHOUSE_CMD_GET_VBAT:
            TRACE("VHOUSE_CMD_GET_VBAT\n");
            bsp_vhouse_vbat_ack(packet);
            vhouse_cb.status = 1;                           //开窗，停止充电
            break;

        case VHOUSE_CMD_PWROFF:
            TRACE("VHOUSE_CMD_PWROFF\n");
            if (bsp_vusb_channel_check(packet->buf[0])) {
                vhouse_cb.status = 2;                           //充满电
            }
            break;

        default:
            break;
    }
}

AT(.text.vhouse)
u32 bsp_smart_vhouse_process(u32 charge_sta)
{
#if VUSB_TBOX_QTEST_EN
    if (qtest_get_mode()){
        return 0;
    }
#endif

    vh_packet_t *packet = &vhouse_cb.packet;

    if (vhouse_cb.open_win_flag && tick_check_expire(vhouse_cb.win_ticks, 180)) {
        bsp_vhouse_open_windows();                          //防止开盖命令被堵而丢失的问题
    }

    if (tick_check_expire(vhouse_cb.loc_ticks, 500)) {      //没必要一直检测，容易跳来跳去
        u8 loc_bat = bsp_get_bat_level();
        //修正会充不满电的问题
        if(charge_sta&&(loc_bat==100)){
            loc_bat=99;
        }
        if((sys_cb.loc_bat & 0x7f) != loc_bat) {
            sys_cb.loc_bat = (sys_cb.loc_bat & 0x80) | loc_bat;
            vhouse_cb.update_ear_flag = true;
        }
        vhouse_cb.loc_ticks = tick_get();
    }
#if VUSB_HUART_DMA_EN

    if(vhouse_cb.rx_flag){

        if(huart_rx_buf[4]<VH_DATA_LEN){
            memcpy(packet,huart_rx_buf,huart_rx_buf[4]+5);
	        packet->checksum=huart_rx_buf[packet->length+5];
	        if(packet->checksum == crc8_maxim((u8 *)packet, 5 + packet->length)){
	            TRACE("cmd [%d], %d\n", packet->cmd, charge_sta);
	            if (charge_sta) {
	                bsp_vhouse_analysis_packet_for_charge(packet);
	            } else {
	                reset_sleep_delay();                    //耳机在仓内不进休眠, 需要接收电量心跳包数据
	                bsp_vhouse_analysis_packet(packet);
	                bsp_vhouse_update_bat_adv();
	            }
	        }
		}
        vhouse_cb.rx_flag=0;
    }
#else
    u32 parse_done;
    u8 ch;
    while (bsp_vusb_uart_get(&ch))
    {
        WDT_CLR();
        parse_done = bsp_vhouse_packet_parse(packet, ch);
        if ((parse_done) && (packet->checksum == crc8_maxim((u8 *)packet, 5 + packet->length))) {
            TRACE("cmd [%d], %d\n", packet->cmd, charge_sta);
            if (charge_sta) {
                bsp_vhouse_analysis_packet_for_charge(packet);
            } else {
                reset_sleep_delay();                    //耳机在仓内不进休眠, 需要接收电量心跳包数据
                bsp_vhouse_analysis_packet(packet);
                bsp_vhouse_update_bat_adv();
            }
        }
    }
#endif
    if (charge_sta == 0) {
        bsp_vhouse_update_bat_adv();
    }
    return vhouse_cb.status;
}
AT(.text.vhouse)
void bsp_smart_house_channel_confirm(void)
{
    u8 channel = get_tws_channel();
    if(!channel){
        return ;
    }
    vhouse_cb.ack_dat_confirm = 1;
    if (channel == LEFT_CHANNEL) {
        vhouse_cb.ack_dat = VHOUSE_LEFT_CHANNEL;
    }else if(channel == RIGHT_CHANNEL){
         vhouse_cb.ack_dat = VHOUSE_RIGHT_CHANNEL;
    }
}
AT(.text.vhouse)
void bsp_smart_vhouse_init(void)
{
    printf("bsp_smart_vhouse_init\n");

    memset(&vh_packet, 0, sizeof(vh_packet));
    memset(&vhouse_cb, 0, sizeof(vhouse_cb));

	tws_lr_xcfg_sel();

    bsp_smart_house_channel_confirm();
    bsp_vusb_channel_read();
#if !VUSB_HUART_DMA_EN
    bsp_vusb_uart_init();
#endif
}
#endif

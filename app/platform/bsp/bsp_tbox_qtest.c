#include "include.h"

#if VUSB_TBOX_QTEST_EN
extern u8 *huart_rx_buf;

qtest_cb_t qtest_cb;

//
u8* qtest_get_txbuf_new(void)
{
    return qtest_cb.other_usage_txbuf;
}

//恢复5v复位
AT(.text.qtest)
void qtest_exit(void)
{
    xcfg_cb.charge_en = 1;
    RTCCON |= BIT(6);
}

AT(.text.qtest)
bool qtest_pwroff_check(u8 mode)
{

    if(mode == 0) {
        return false;
    }
    static u32 inbox_tick = 0;
    static u8 ear_sta = 0;
    u8 pre_ear_sta = ear_sta;

    if (!tick_check_expire(inbox_tick, 10)) {
        return false;
    }
    inbox_tick = tick_get();

    if(qtest_get_heart_pkt_delay()) {                           //发完心跳包后延时段时间再检测状态
        qtest_dec_heart_pkt_delay();
        if (qtest_cb.inbox_cnt) {
            qtest_cb.inbox_cnt--;
        }
        return false;
    }

    if(DC_IN()){
        ear_sta = QTEST_STA_DCIN;

    }else if(CHARGE_INBOX()){
        ear_sta = QTEST_STA_INBOX;
    }else {
        ear_sta = QTEST_STA_OUTBOX;
    }

    if((ear_sta == QTEST_STA_INBOX)||( pre_ear_sta != ear_sta)){
        qtest_cb.inbox_cnt = 150;
    }

//     my_printf("*%x_%d",ear_sta,qtest_cb.inbox_cnt));
    if (!qtest_cb.inbox_cnt) {
        qtest_exit();
        //拿起复位
        if ((qtest_get_pickup_sta() == 2)||(ear_sta == QTEST_STA_DCIN)){
             if(bt_nor_is_connected()){
                 bt_nor_disconnect();
             }
            sw_reset_kick(SW_RST_FLAG);
        }else if(qtest_get_pickup_sta() == 1){
        //拿起关机
            if(mode == SW_RST_QTEST_NORMAL){
                msg_enqueue(EVT_QTEST_PICKUP_PWROFF);
            }else if(mode == SW_RST_QTEST_ONLY_PAIR){
                  saradc_exit(adc_cb.channel);
                  sfunc_pwrdown(1);
            }
        }
        else if (mode == SW_RST_QTEST_ONLY_PAIR) {
             saradc_exit(adc_cb.channel);
             sfunc_pwrdown(1);
        }
        return true;
    }else{
        qtest_cb.inbox_cnt--;
    }
   return false;
}

AT(.text.qtest)
void qtest_xcfg_init(void)
{
    //开快速测试关闭入仓关机
    xcfg_cb.chg_inbox_pwrdwn_en=0;
    xcfg_cb.bt_tswi_charge_rst_en=0;
    if (qtest_get_mode()) {
         xcfg_cb.bt_tws_en=0;
         xcfg_cb.bt_spp_en=0;
         xcfg_cb.bt_rf_page_pwrdec=0;
         xcfg_cb.charge_dc_reset = 0; //因为RTCCON11 BIT(6)置上去会有概率导致DC 5V检测不到
         xcfg_cb.lpwr_off_vbat=0;//低电不关机
		 xcfg_cb.charge_en=0;
		 xcfg_cb.ble_en=0;
         xcfg_cb.huart_sel = 10;
         RTCCON8 |= BIT(4);    //仓的最低维持电压为1.7V
         bsp_charge_leakage_set(1, 0);                                   //enable leakage
//         param_vusb_channel_write(0);
//         qtest_tws_message_init();
    }
}

AT(.text.qtest)
void bsp_qtest_init(void)
{
//    TRACE("bsp_qtest_init\n");
    memset(&qtest_cb,0x00,sizeof(qtest_cb_t));
    qtest_cb.inbox_cnt = 150;
    param_bt_qpdn_boat_flag_read();
    param_bt_qpdn_boat_flag_write(0);
    u8 res = qtest_init(sys_cb.sw_rst_flag);
    if (res) {
        //进入快测或者打开vusb dma的方式的话，都用dma的方式进行串口通讯
        if((res == 1)||(VUSB_HUART_DMA_EN)){
            set_vusb_uart_flag(0);
            xcfg_cb.huart_en = 1;
            xcfg_cb.huart_sel = 10;
            bsp_huart_init();
        }else{
            bsp_vusb_uart_init();
        }
        bsp_vusb_channel_read();
        //先确定系统左右耳
        tws_lr_xcfg_sel();
        qtest_xcfg_init();
        //先发送蓝牙地址给到测试盒，加速快测
        if(res == 1){
        	qtest_beforehand_process();
        }
    }
}

//尽快结束战斗
void qtest_tbox_set_custom_param(u8 param)
{
    //printf("param %d\n",param);
}

//传给测试盒数据用于显示，测试盒需要打开“显示耳机传入信息”的功能，可自定义显示内容
//第0byte为：字符串数据
//第1byte为：数值
void qtest_show_user_info(u8* param)
{
    param[0] = 'V';
    param[1] = (u8)bsp_get_bat_level();
}


void qtest_custom_pair_id(u8* param)
{
    uint pair_id = bt_tws_get_pair_id();
    memcpy(param,(u8*)&pair_id,4) ;
}

WEAK u32 xcfg_get_flash_addr(void)
{
    return 0;
}

//修正1拖24的校频会影响版本对比的问题
AT(.text.qtest)
u32 qtest_get_osci_cap_addr(void)
{
    u32 offest_addr =  xcfg_get_flash_addr();
    if(offest_addr != 0) {
        offest_addr += (u8*)(&xcfg_cb.osci_cap) - (u8*)&xcfg_cb;
    }
    return offest_addr;
}

AT(.com_text.qtest)
void sys_set_qtest_flag(u8 flag)
{
    sys_cb.qtest_flag = flag;
    sys_cb.qtest_sta = 1;
}

bool qtest_set_pdn_boat_flag(u8 flag)
{
    sys_cb.pdn_boat_flag = flag;
    return true;
}

bool qtest_vusb_dma_en(void)
{
    if(VUSB_HUART_DMA_EN||qtest_get_mode()){
        return true;
    }
    return false;
}

void qtest_other_usage_process_do(void)
{
    u8 *tx_buf = qtest_get_txbuf_new();

    if(sys_cb.dc_rst_flag == 2){
        sys_cb.dc_rst_flag = 1;
    }

    if (sys_cb.qtest_ack == QTEST_MODE_DUT) {
        sys_cb.discon_reason = 0;//不同步关耳机
        func_cb.sta = FUNC_BT_DUT;
    } else if (sys_cb.qtest_ack == QTEST_MODE_CLR_INFO) {
        bt_clr_all_link_info();
        func_cb.sta = FUNC_NULL;
    } else if (sys_cb.qtest_ack == QTEST_MODE_PWROFF || sys_cb.qtest_ack == QTEST_MODE_PWROFF_LP) {
        if(sys_cb.qtest_ack == QTEST_MODE_PWROFF_LP) {
            sys_cb.pdn_boat_flag = 1;
        }
        sys_cb.discon_reason = 0;//不同步关耳机
        func_cb.sta = FUNC_PWROFF;
    }
    tx_buf[0] = 'S';
    tx_buf[1] = 'I';
    tx_buf[2] = 'G';
    tx_buf[3] = sys_cb.qtest_ack;

    qtest_send_msg2tbox(tx_buf,4);
    sys_cb.qtest_ack = 0;
}

void qtest_other_usage_process(void)
{
    if(qtest_get_mode()){
        return;
    }

    if (sys_cb.qtest_ack != sys_cb.qtest_flag) {
        sys_cb.qtest_ack = sys_cb.qtest_flag;
        sys_cb.qtest_flag = 0;
        qtest_other_usage_process_do();
    }
}

#endif

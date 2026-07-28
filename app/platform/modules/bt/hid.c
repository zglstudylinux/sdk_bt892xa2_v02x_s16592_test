#include "include.h"
#include "api.h"
#include "func.h"

#if BT_HID_EN

uint8_t cfg_hid_type = BT_HID_TYPE;

#if BT_HID_MANU_EN
void bsp_bt_hid_tog_conn(void)
{
    if (xcfg_cb.bt_hid_manu_en) {
        func_bt_chkclr_warning(BT_WARN_HID_CON|BT_WARN_HID_DISCON);
        f_bt.hid_menu_flag = 2;     //按键连接/断开HID，置上标志，用于播放提示音
        if (bt_hid_is_connected()) {
            bt_hid_disconnect();
        } else {
            bt_hid_connect();
        }
    }
}
#endif

bool bsp_bt_hid_photo(u16 keycode)
{
    if ((xcfg_cb.bt_hid_en) && (bt_hid_is_connected())) {
        bt_hid_key(HID_KEY_ENTER);              //enter key, android 4.0以上
        delay_5ms(10);
        bt_hid_consumer(keycode);               //consumer key vol_up or vol_down, ios
        return true;
    }
    return false;
}

bool bsp_bt_hid_vol_change(u16 keycode)
{
#if BT_A2DP_VOL_HID_CTRL_EN
    if ((xcfg_cb.bt_hid_en) && (bt_hid_is_connected())) {
        bt_hid_consumer(keycode);               //consumer key vol_up or vol_down, ios
        return true;
    }
#endif
    return false;
}

//点击左边屏幕中心位置
void bsp_bt_hid_screen_left(void)
{
    bt_hid_touch_screen(0);
    delay_5ms(10);
    bt_hid_touch_screen(1);
}

//点击右边屏幕中心位置
void bsp_bt_hid_screen_right(void)
{
    bt_hid_touch_screen(0x11);
    delay_5ms(10);
    bt_hid_touch_screen(0x10);
}

#if BT_HID_DOUYIN_EN

extern u8  hid_douyin_wptr;
typedef struct
{
   u8 key;
   u8 cnt;
   const u8* ptr;
}dy_key_t;

extern dy_key_t douyin_key[6];

void bsp_bt_hid_douyin(u8 msg)
{
    if ((xcfg_cb.bt_hid_en) && (bt_hid_is_connected()) == 0){
        return;
    }
    u8 cnt = 0;
    for (u8 i = 0; i < sizeof(douyin_key)/sizeof(dy_key_t); i++) {
        if (msg == douyin_key[i].key) {
            cnt = douyin_key[i].cnt;
            break;
        }
    }

    hid_douyin_wptr = 0;
    for (int i = 0; i < cnt; i++) {
        bt_hid_douyin(msg);
        delay_5ms(5);
    }

}


/**
 *    向下滑
 */
void user_finger_down(void)
{
    if(bt_is_ios_device()){
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
        bt_hid_point_pos(0, 60, 60);       	//移动到上中间
        bt_hid_point_pos(1, 0, 10);       	//按住按键，移动往下移动
        bt_hid_point_pos(1, 0, 40);       	//按住按键，移动往下移动
        bt_hid_point_pos(1, 0, 100);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
    }else{
        bt_hid_point_pos(1, 2000, 1000);
        bt_hid_point_pos(1, 2000, 1100);
        bt_hid_point_pos(1, 2000, 1400);
        bt_hid_point_pos(1, 2000, 2000);
        bt_hid_point_pos(1, 2000, 2800);
        bt_hid_point_pos(0, 2000, 3000);
    }
}


/**
 *    向上滑
 */
void user_finger_up(void)
{
    if(bt_is_ios_device()){
        bt_hid_point_pos(0, -2047, 2047);  	//这两步是把指针回到左下角去
        bt_hid_point_pos(0, 60, -60);       //移动到下中间
        bt_hid_point_pos(1, 0, -10);       	//按住按键，移动往下移动
        bt_hid_point_pos(1, 0, -40);       	//按住按键，移动往下移动
        bt_hid_point_pos(1, 0, -100);       //按住按键，移动往下移动
        bt_hid_point_pos(0, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, -2047, 2047);  	//这两步是把指针回到左下角去
    }else{
        bt_hid_point_pos(1, 2000, 3000);
        bt_hid_point_pos(1, 2000, 2900);
        bt_hid_point_pos(1, 2000, 2700);
        bt_hid_point_pos(1, 2000, 2300);
        bt_hid_point_pos(1, 2000, 1000);
        bt_hid_point_pos(0, 2000, 1000);
    }
}


/**
 *    单击
 */
void user_finger_p(void)
{
    if(bt_is_ios_device()){
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
        bt_hid_point_pos(0, 60, 100);       //移动到上中间
        bt_hid_point_pos(1, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
    }else{
        bt_hid_point_pos(1, 2000, 2000);
        bt_hid_point_pos(0, 2000, 2000);
    }
}


/**
 *    双击
 */
void user_finger_pp(void)
{
    if(bt_is_ios_device()){
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
        bt_hid_point_pos(0, 60, 100);       //移动到上中间
        bt_hid_point_pos(1, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(1, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, 0, 0);       	//按住按键，移动往下移动
        bt_hid_point_pos(0, -2047, -2047);  //这两步是把指针回到左上角去
    }else{
        bt_hid_point_pos(1, 2000, 2000);
        bt_hid_point_pos(0, 2000, 2000);
        bt_hid_point_pos(1, 2000, 2000);
        bt_hid_point_pos(0, 2000, 2000);
        bt_hid_point_pos(1, 2000, 2000);
        bt_hid_point_pos(0, 2000, 2000);
    }
}
#endif
#endif

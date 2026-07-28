#include "include.h"

#define XTP_EFFECT_FRAME_HEAD_TAG           12
#define XTP_EFFECT_NAME_TAG                 8
#define CAR_TOOL_SEED                       0xffff

#if BT_MUSIC_EFFECT_DBG_EN || BT_MUSIC_EFFECT_ABT_EN

//跟数组 bsp_effect_name 一一对应，请同步修改，顺序也不要弄乱
enum BSP_EFFECT_CH {
    BSP_EFFECT_DYEQ,
    BSP_EFFECT_DYEQ_DRC,
    BSP_EFFECT_NUM,
};

//name最长8个字符，跟CAR_Tool里模块的名字一一对应
static const char bsp_effect_name[BSP_EFFECT_NUM][9] = {
    [BSP_EFFECT_DYEQ]           =   "DYNEQ",
    [BSP_EFFECT_DYEQ_DRC]       =   "DYEQ_DRC",
};

u8 effect_tx_ack_buf[14];

uint calc_crc(void *buf, uint len, uint seed);
void tx_ack(uint8_t *packet, uint16_t len);
u8 check_sum(u8 *buf, u16 size);
void drclphp_coef_update_all(u8 *buf);

void bsp_set_effect(u8 effect_id, u8* buf)
{
    print_r(buf, 20);

    switch (effect_id) {
#if BT_DYNAMIC_EQ_EN
        case BSP_EFFECT_DYEQ:
            printf("gain       : %x\n", little_endian_read_32(buf, 2));
            printf("threshold1 : %x\n", little_endian_read_32(buf, 6));
            printf("threshold2 : %x\n", little_endian_read_32(buf, 10));
            printf("slop       : %x\n", little_endian_read_32(buf, 14));
            music_dyeq_set_param(&buf[2]);
            printf("BSP_EFFECT_DYEQ\n");
            break;

        case BSP_EFFECT_DYEQ_DRC:
            music_dyeq_drc_set_param(&buf[8]);
            printf("BSP_EFFECT_DYEQ_DRC\n");
            break;
#endif // BT_MUSIC_EFFECT_DYEQ_EN

        default:
            break;
    }
#if BT_TWS_EN
    if ((bt_tws_is_connected() && eq_dbg_cb.rx_type) || (!eq_dbg_cb.rx_type && !bt_tws_is_slave())) {
        bt_tws_sync_eq_param();
    }
#endif // BT_TWS_EN
}
#endif

#if BT_MUSIC_EFFECT_DBG_EN
void bsp_effect_tx_ack(u8 *head_tag, u8 ack)
{
    head_tag[12] = ack;
    head_tag[13] = check_sum(head_tag, 13);
    tx_ack(head_tag, 14);
}

void bsp_effect_parse_cmd(void)
{
//    printf("-->%s\n", __func__);
//    print_r(eq_rx_buf, 20);
    int i;
    u8 name_len = 8;
    u8 name_8byte[8];
    u8* name = &eq_rx_buf[4];                   //把“CAR_”截掉，只获取后面的模块名字
    u16 size = little_endian_read_16(eq_rx_buf, 12);
    u32 crc = calc_crc(eq_rx_buf, size+XTP_EFFECT_FRAME_HEAD_TAG, CAR_TOOL_SEED);

    for(i = 0; i < XTP_EFFECT_NAME_TAG; i++) {     //获取name有效字符长度
        if(name[i]=='#') {
            name_len = i;
            break;
        }
    }
    memcpy(effect_tx_ack_buf, eq_rx_buf, 12);

    if (crc != little_endian_read_16(eq_rx_buf, size+XTP_EFFECT_FRAME_HEAD_TAG)) {
        bsp_effect_tx_ack(effect_tx_ack_buf, 1);
        memset(eq_rx_buf, 0, EQ_BUFFER_LEN);
        printf("-->CRC_ERROR %x %x %d\n", crc, little_endian_read_16(eq_rx_buf, size+XTP_EFFECT_FRAME_HEAD_TAG), size+XTP_EFFECT_FRAME_HEAD_TAG);
        return;
    }

    memcpy(name_8byte, name, name_len);
    name_8byte[name_len] = 0;
    printf("CAR_RESEVE:%s, len:%d\n", name_8byte, name_len);

    for (i = 0; i < BSP_EFFECT_NUM; i++) {
        if (0 == memcmp((char *)name, &bsp_effect_name[i], name_len)) {
//            printf("res___%d\n", i);
            bsp_set_effect(i, &eq_rx_buf[14]);
            bsp_effect_tx_ack(effect_tx_ack_buf, 0);
            break;
        }
    }

    memset(eq_rx_buf, 0, EQ_BUFFER_LEN);
}
#endif // BT_MUSIC_EFFECT_DBG_EN

#if BT_MUSIC_EFFECT_ABT_EN

#define     ABT_HEAD_TAG            (31+4)      //head + "CAR_"
#define     MODULE_INDEX_TAG        19          //module_index_head
#define     MODULE_HEAD_TAG         24          //module_head
#define     MODULE_DATA_TAG         14

void bsp_set_effect_by_abt(void)
{
    u8 *ptr = (u8*)RES_BUF_EFFECT_ABT;
    int i,j;
    u16 num;
    u16 crc;
    u32 len;
    u8* module_addr;
    u8 name_8byte[9];
    u8 name_len;

    if ((u32)RES_LEN_EFFECT_ABT == 0) {
        printf("EFFECT_ABT file is NULL!\n");
        return;
    }

    if ((ptr[0] != 'C') || (ptr[1] != 'A') || (ptr[2] != 'R')) {    //check head
        printf("EFFECT_ABT file error!\n");
        return;
    }

    len = little_endian_read_16(ptr, 3);
    num = little_endian_read_16(ptr, 13);
    crc = little_endian_read_16(ptr, len+3);
    if (crc != calc_crc(ptr, len+3, CAR_TOOL_SEED)) {   //check abt file crc
        printf("EFFECT_ABT crc error!\n");
        return;
    }
    ptr += ABT_HEAD_TAG;

    for (i = 0; i < num; i++) {
        memcpy(name_8byte, ptr+4, 8);
        name_len = 8;
        for(j = 0; j < 9; j++) {     //获取name有效字符长度
            if(name_8byte[j]=='#') {
                memset(&name_8byte[j], 0, 9-j);
                name_len = j;
                break;
            }
        }
        for (j = 0; j < BSP_EFFECT_NUM; j++) {
            if (0 == memcmp((char *)name_8byte, &bsp_effect_name[j], name_len)) {
                module_addr = (u8*)little_endian_read_32(ptr, 15)+RES_BUF_EFFECT_ABT+MODULE_HEAD_TAG+2;
                bsp_set_effect(j, module_addr+MODULE_DATA_TAG);
                break;
            }
        }
        ptr += MODULE_INDEX_TAG;
    }
}

void effect_abt_param_do(u16 effect_id)
{
    u8 *ptr = (u8*)RES_BUF_EFFECT_ABT;
    int i,j;
    u16 num;
    u16 crc;
    u32 len;
    u8* module_addr;
    u8 name_8byte[9];
    u8 name_len;

    if ((ptr[0] != 'C') || (ptr[1] != 'A') || (ptr[2] != 'R')) {
        printf("EFFECT_ABT file error!\n");
        return;
    }

    len = little_endian_read_16(ptr, 3);
    num = little_endian_read_16(ptr, 13);
    crc = little_endian_read_16(ptr, len+3);
    if (crc != calc_crc(ptr, len+3, CAR_TOOL_SEED)) {
        printf("EFFECT_ABT crc error!\n");
        return;
    }

    ptr += ABT_HEAD_TAG;

    for (i = 0; i < num; i++) {
        memcpy(name_8byte, ptr+4, 8);
        name_len = 8;
        for(j = 0; j < 9; j++) {     //获取name有效字符长度
            if(name_8byte[j]=='#') {
                memset(&name_8byte[j], 0, 9-j);
                name_len = j;
                break;
            }
        }
        if (0 == memcmp((char *)name_8byte, &bsp_effect_name[effect_id], name_len)) {
            module_addr = (u8*)little_endian_read_32(ptr, 15)+RES_BUF_EFFECT_ABT+MODULE_HEAD_TAG+2+4;
            bsp_set_effect(effect_id, module_addr+MODULE_DATA_TAG);
            break;
        }
        ptr += MODULE_INDEX_TAG;
    }
}
#endif // BT_MUSIC_EFFECT_ABT_EN


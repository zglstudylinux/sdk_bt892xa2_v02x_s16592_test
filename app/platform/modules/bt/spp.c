#include "include.h"
#include "api.h"

#if BT_SPP_EN
#if LE_TUYA_EN
#define SPP_TX_BUF_INX      3
#define SPP_TX_BUF_LEN      64     //max=512
#elif LE_DUEROS_DMA_EN
#define SPP_TX_BUF_INX      3
#define SPP_TX_BUF_LEN      DUEROS_DMA_MTU_MAX_LEN    //max=512
#else
#define SPP_TX_BUF_INX      4
#define SPP_TX_BUF_LEN      256     //max=512
#endif
#define SPP_POOL_SIZE       (SPP_TX_BUF_LEN + sizeof(struct txbuf_tag)) * SPP_TX_BUF_INX


AT(.ble_buf.stack.spp)
uint8_t spp_tx_pool[SPP_POOL_SIZE];

void spp_txpkt_init(void)
{
    txpkt_init(&spp_tx, spp_tx_pool, SPP_TX_BUF_INX, SPP_TX_BUF_LEN);
    spp_tx.send_kick = spp_send_kick;
}

u16 get_spp_mtu_size(void)
{
    return SPP_TX_BUF_LEN;
}
#endif


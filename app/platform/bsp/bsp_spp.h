#ifndef _BSP_SPP_H
#define _BSP_SPP_H

#if AB_MATE_APP_EN
#include "ab_mate_app.h"
    #define app_spp1_disconnect_callback()
    #define app_spp1_connect_callback()
    #define app_spp1_rx_callback(a,b)

#ifdef AB_MATE_SPP_UUID
    #define app_spp_disconnect_callback()
    #define app_spp_connect_callback()
    #define app_spp_rx_callback(a,b)
    #define app_spp2_disconnect_callback()          ab_mate_spp_disconnect_callback()
    #define app_spp2_connect_callback()             ab_mate_spp_connect_callback()
    #define app_spp2_rx_callback(a,b)               ab_mate_receive_proc(a, b, AB_MATE_CON_SPP)
#else
    #define app_spp_disconnect_callback()           ab_mate_spp_disconnect_callback()
    #define app_spp_connect_callback()              ab_mate_spp_connect_callback()
    #define app_spp_rx_callback(a,b)                ab_mate_receive_proc(a, b, AB_MATE_CON_SPP)
    #define app_spp2_disconnect_callback()
    #define app_spp2_connect_callback()
    #define app_spp2_rx_callback(a,b)
#endif

#elif TME_APP_EN
#include "tme_app.h"
    #define app_spp_disconnect_callback()
    #define app_spp_connect_callback()
    #define app_spp_rx_callback(a,b)

    #define app_spp1_disconnect_callback()          tme_spp_disconnect_callback()
    #define app_spp1_connect_callback()             tme_spp_connect_callback()
    #define app_spp1_rx_callback(a,b)               tme_recv_proc(a,b)

    #define app_spp2_disconnect_callback()
    #define app_spp2_connect_callback()
    #define app_spp2_rx_callback(a,b)
#elif LE_DUEROS_DMA_EN
#include "dueros_dma_app.h"
    #define app_spp_disconnect_callback()
    #define app_spp_connect_callback()
    #define app_spp_rx_callback(a,b)

    #define app_spp1_disconnect_callback()         dueros_dma_spp_disconnect_callback()
    #define app_spp1_connect_callback()            dueros_dma_spp_connect_callback()
    #define app_spp1_rx_callback(a,b)              dueros_dma_recv_proc(a,b,DUEROS_DMA_CON_SPP)

    #define app_spp2_disconnect_callback()         
    #define app_spp2_connect_callback()            
    #define app_spp2_rx_callback(a,b)              
#else
    #define app_spp_disconnect_callback()
    #define app_spp_connect_callback()
    #define app_spp_rx_callback(a,b)

    #define app_spp1_disconnect_callback()
    #define app_spp1_connect_callback()
    #define app_spp1_rx_callback(a,b)

    #define app_spp2_disconnect_callback()
    #define app_spp2_connect_callback()
    #define app_spp2_rx_callback(a,b)
#endif

extern u16 get_spp_mtu_size(void);

#endif

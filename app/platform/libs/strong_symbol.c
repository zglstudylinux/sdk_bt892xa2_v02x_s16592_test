/**********************************************************************
*
*   strong_symbol.c
*   定义库里面部分WEAK函数的Strong函数，动态关闭库代码
***********************************************************************/
#include "include.h"

uint32_t cfg_sdk_version = SDK_VERSION;

#if !FUNC_USBDEV_EN
void usb_dev_isr(void){}
void ude_ep_reset(void){}
void ude_control_flow(void){}
void ude_isoc_tx_process(void){}
void ude_isoc_rx_process(void){}
void lock_code_usbdev(void){}
#endif //FUNC_USBDEV_EN

#if (REC_TYPE_SEL != REC_MP3)
int mpa_encode_frame(void) {return 0;}
#endif //(REC_TYPE_SEL != REC_MP3)

#if (REC_TYPE_SEL != REC_SBC)
#if (!BT_HFP_MSBC_EN)
bool msbc_encode_init(void){return false;}
#endif
bool sbc_encode_init(u8 spr, u8 nch){return false;}
void sbc_encode_process(void){}
#endif

#if (REC_TYPE_SEL != REC_ADPCM && !BT_HFP_REC_EN)
void adpcm_encode_process(void){}
#endif //(REC_TYPE_SEL != REC_ADPCM)

#if (!(MUSIC_WMA_SUPPORT | MUSIC_APE_SUPPORT | MUSIC_FLAC_SUPPORT))
void msc_stream_start(u8 *ptr) {}
void msc_stream_end(void) {}
int msc_stream_read(void *buf, unsigned int size) {return 0;}
bool msc_stream_seek(unsigned int ofs, int whence) {return false;}
void os_stream_fill(void) {}
void os_stream_read(void) {}
void os_stream_seek(void) {}
void os_stream_end(void) {}
AT(.com_text.stream)
void msc_stream_fill(void) {}
#endif


#if !MUSIC_WAV_SUPPORT
int wav_dec_init(void){return 0;}
bool wav_dec_frame(void){return false;}
void lock_code_wavdec(void){}
int wav_decode_init(void){return 0;}
#endif // MUSIC_WAV_SUPPORT

#if !MUSIC_WMA_SUPPORT
int wma_dec_init(void){return 0;}
bool wma_dec_frame(void){return false;}
void lock_code_wmadec(void){}
int wma_decode_init(void){return 0;}
#endif // MUSIC_WMA_SUPPORT

#if !MUSIC_APE_SUPPORT
int ape_dec_init(void){return 0;}
bool ape_dec_frame(void){return false;}
void lock_code_apedec(void){}
int ape_decode_init(void){return 0;}
#endif // MUSIC_APE_SUPPORT

#if !MUSIC_FLAC_SUPPORT
int flac_dec_init(void){return 0;}
bool flac_dec_frame(void){return false;}
void lock_code_flacdec(void){}
int flac_decode_init(void){return 0;}
#endif // MUSIC_FLAC_SUPPORT

#if !MUSIC_SBC_SUPPORT
int sbcio_dec_init(void){return 0;}
bool sbcio_dec_frame(void){return false;}
int sbcio_decode_init(void){return 0;}
#endif // MUSIC_SBC_SUPPORT

#if !MUSIC_AAC_SUPPORT
int aacio_dec_init(void) {return 0;}
bool aacio_dec_frame(void) {return false;}
int aacio_decode_init(void) {return 0;}
#endif

#if !FUNC_AUX_EN
void sdadc_analog_aux_start(u8 channel, u8 gain){}
void sdadc_analog_aux_exit(u8 channel){}
#endif

#if !FUNC_MUSIC_EN
int mp3_decode_init(void){return 0;}
int music_decode_init(void){return 0;}
void mp3_get_total_time(void){}
void update_codec_playtime_callback(void *s){}
#endif

#if BT_A2DP_VOL_CTRL_WITHOUT_KEY
void btstack_volume_change_do(void)
{
}
#endif

#if !BT_A2DP_RECORD_DEVICE_VOL
void a2dp_save_dev_vol(void) {}
void a2dp_set_cur_dev_vol(uint8_t vol) {}
uint8_t a2dp_get_cur_dev_vol(void) {return 0xff;}
#endif

#if !BT_HID_EN || !BT_TWS_MS_SWITCH_EN
uint8_t tws_get_hid_service_report(uint8_t *data_ptr) {return 0;}
uint8_t tws_set_hid_service_report(uint8_t *data_ptr, uint8_t len) {return 0;}
uint8_t tws_get_hid_service(uint8_t *data_ptr){return 0;}
uint8_t tws_set_hid_service(uint8_t *data_ptr, uint16_t conhdl, u8 address[], uint8_t len){return 0;}
#endif

//是否支持BT AAC音频
#if !BT_A2DP_AAC_AUDIO_EN
void aac_dec_init(void) {}
void aac_decode_init(void) {}
bool aac_dec_frame(void) {return false;}
bool aac_nor_dec_frame(void) {return false;};
bool aac_tws_dec_frame(void) {return false;};
void aac_cache_free_do(void) {}
size_t aac_cache_read_do(uint8_t *buf, uint max_size) {return 0;}
uint16_t tws_pack_aac(uint8_t *buf) {
    return 0;
}
AT(.com_text.aac.obuf)
void aac_fill_tws_obuf(void) {}
AT(.aacdec.text)
void aac_obuf_tws_cpy(void) {}
AT(.aacdec.text)
bool aac_decode(void) { return false; }
#else
void aac_decode_init_do(void);
bool aac_dec_frame_do(void);
void aac_cache_free_do(void);
size_t aac_cache_read_do(uint8_t *buf, uint max_size);
AT(.text.music.init.aac)
void aac_decode_init(void) {
    aac_decode_init_do();
}
AT(.aacdec.text)
bool aac_dec_frame(void) {
    return aac_dec_frame_do();
}
AT(.aacdec.text)
void aac_cache_free(void) {
#if BT_TWS_EN
    aac_cache_free_do();
#endif
}
AT(.aacdec.text)
size_t aac_cache_read(uint8_t *buf, uint max_size) {
#if BT_TWS_EN
    return aac_cache_read_do(buf, max_size);
#else
    return 0;
#endif
}
#endif

#if !FMRX_REC_EN
void fmrx_rec_start(void){}
void fmrx_rec_stop(void){}
#endif // FMRX_REC_EN

#if !BT_REC_EN
void bt_music_rec_start(void) {}
void bt_music_rec_stop(void) {}
#endif

#if !USB_SUPPORT_EN
void usb_isr(void){}
void usb_init(void){}
#endif

#if ((!SD_SUPPORT_EN) && (!FUNC_USBDEV_EN))
void sd_disk_init(void){}
void sdctl_isr(void){}
void sd_disk_switch(u8 index){}
bool sd0_stop(bool type){return false;}

bool sd0_init(void){return false;}
bool sd0_read(void *buf, u32 lba){return false;}
bool sd0_write(void* buf, u32 lba){return false;}

#endif

#if !FUNC_MUSIC_EN
u32 fs_get_file_size(void){return 0;}
void fs_save_file_info(unsigned char *buf){}
void fs_load_file_info(unsigned char *buf){}
#endif // FUNC_MUSIC_EN

#if !BT_TWS_EN
void btstack_tws_init(void){}
void a2dp_play_init(void){}
AT(.bcom_text.sbc.play)
void a2dp_play_reset(void){}
AT(.bcom_text.sbc.play)
void a2dp_play_reset_do(void){}

AT(.bcom_text.sbc_cache)
bool a2dp_cache_fill(uint8_t *packet, uint16_t size) {
    return true;
}

AT(.bcom_text.sbc_cache)
uint8_t avdtp_fill_tws_buffer(u8 *ptr, uint len) {
    return 0;
}

AT(.bcom_text.sbc_cache)
uint8_t a2dp_cache_before_rx(uint8_t *data_ptr, uint16_t data_len) {
    return 0;
}

AT(.sbcdec.code)
void sbc_cache_free(void) {
}

AT(.sbcdec.code)
size_t sbc_cache_read(uint8_t **buf) {
    return 0;
}

AT(.com_text.bb.tws)
void bt_tws_ticks_isr(void) {
}

AT(.bcom_text.bb.btisr)
void bt_tws_ticks_instant(void) {
}

AT(.bcom_text.bb.btisr)
uint8_t tws_get_ticks_status(void) {
    return 0x80;
}

uint8_t bt_tws_set_spr(uint8_t index, uint spridx, uint32_t instant) {
    return 0;
}

AT(.bcom_text.sbc.play)
void tws_ticks_trigger(uint32_t ticks) {
}

AT(.bcom_text.sbc.play)
uint32_t tws_get_play_ticks(uint16_t seq_num, uint32_t duration) {
    return 0;
}

AT(.com_text.sbc.play)
void tws_trigger_isr(void) {
}

AT(.bcom_text.sbc.play)
bool tws_cache_is_empty(void) {
    return false;
}

AT(.bcom_text.sbc.play)
void sbc_cache_env_reset(void)
{
}

AT(.bcom_text.sbc.send)
void tws_send_pkt(void)
{
}

AT(.bcom_text.bb.btisr)
void bt_tws_ticks_next_instant(void) {}

AT(.bcom_text.lc.tws)
uint8_t tws_get_snoop_status(void) {
    return 0;
}
AT(.bcom_text.sbc.play)
void a2dp_set_play_ticks(uint32_t e_ticks, uint16_t seqn) {}
AT(.bcom_text.sbc.play)
bool a2dp_is_rx_stop(uint8_t index) { return false; }
AT(.bcom_text.a2dp.lost)
uint32_t a2dp_lost_get_cache(void) { return 0; }
AT(.bcom_text.sbc)
void tws_ticks_set(void) {}

AT(.bcom_text.bb.tws_switch)
bool bt_tws_need_switch(bool calling) { return false; }
AT(.bcom_text.bb.bttx)
bool tws_evs_rx_remote(u8 *buf) { return false; }
AT(.bcom_text.bb.bttx)
void tws_seqn_update_local(u32 e_ticks, uint16_t seqn, bool first) {}
AT(.bcom_text.sbc.play)
uint8_t avdtp_check_play_reset(void) { return 0; }
AT(.bcom_text.sbc.play)
size_t a2dp_play_read_do(uint8_t **buf) { return 0; }
AT(.bcom_text.sbc.play)
void a2dp_play_free_do(void) {}
AT(.bcom_text.sbc.play)
bool a2dp_fix_slv_ticks(u32 m_ticks, u16 m_seqn) { return false; }
AT(.bcom_text.sbc.play)
bool tws_is_first_pkt(void) { return false; }
AT(.bcom_text.sbc.play)
bool tws_next_pkt_is_exist(void) { return true; }
uint16_t tws_set_snoop_ticks(uint8_t *data_ptr, uint16_t size) { return false; }
AT(.bcom_text.bb.btisr)
void bt_set_sync_tick(u32 clock) {}
AT(.bcom_text.bb.btisr)
bool bt_sync_tick(void) { return false; }
AT(.bcom_text.bb.btisr)
void bt_set_sync_info(u8 *buf) {}
AT(.com_text.irq)
void bt_tws_sync_timer_run(void) {}
AT(.com_text.irq)
void music_res_sync_kick_run() {}
#else
#endif


#if !BT_FCC_TEST_EN || !LE_EN
uint8_t vs_ble_test(void const *cmd) {
    return 0x11;
}
uint8_t ble_test_start(void* params) {
    return 0x0c;
}
uint8_t ble_test_stop(void) {
    return 0x0c;
}
#endif

#if BT_FCC_TEST_EN || FUNC_BT_FCC_EN
#if LE_EN
uint8_t vs_ble_test_do(void const *cmd);
uint8_t vs_ble_test(void const *cmd) {
    return vs_ble_test_do(cmd);
}
#endif
#else
AT(.com_text.bt21.isr.test.fcc)
bool bt_acl_test_rx_end(uint8_t index, void *par) {
    return false;
}
void hci_fcc_init(void) {}
#if !IODM_TEST_MODE
uint8_t vs_fcc_test_cmd(void const *param) {
    return 0x11;
}
#endif
void huart_init(void)
{
}
AT(.bcom_text.stack.uart_isr)
bool bt_uart_isr(void) {
    return false;
}
#endif

#if !BT_HFP_REC_EN
AT(.com_text.bt_rec)
void bt_sco_rec_mix_do(u8 *buf, u32 samples) {}
void bt_sco_fill_remote_buf(u16 *buf, u16 samples) {}
#endif

#if !SYS_MAGIC_VOICE_EN
void magic_voice_process(void) {}
void mav_kick_start(void) {}
#endif

#if !FUNC_SPDIF_EN
void spdif_pcm_process(void){}
bool spdif_smprate_detect(void) {    return false;}
void spdif_isr(void){}
#endif


#if ((!MUSIC_UDISK_EN)&&(!MUSIC_SDCARD_EN)&&(!MUSIC_SDCARD1_EN))
FRESULT fs_open(const char *path, u8 mode){return 0;}
FRESULT fs_read (void* buff, UINT btr, UINT* br){return 0;}
FRESULT fs_lseek (DWORD ofs, u8 whence){return 0;}
#endif

#if !KARAOK_REC_EN
void karaok_rec_process(u8 *ptr) {}
AT(.com_text.karaok.rec)
bool karaok_rec_fill_buf(u8 *buf, u16 len) {return false;}
#endif

#if !I2S_DMA_EN
void i2s_isr(void) {}
void i2s_process(void) {}
#endif

#if !LE_EN && !BT_ATT_EN
void ble_txpkt_init(void) {}
void btstack_gatt_init(void) {}
#endif

#if !BT_ATT_EN
void latt_establish_service_level_connection(uint8_t * bd_addr) {}
void latt_release_service_level_connection(uint8_t * bd_addr) {}
void latt_send_kick(void) {}
int latt_tx_notify(uint8_t index, const uint8_t *value, uint16_t value_len) {return -1;}
void * provide_latt_connection_context_for_bd_addr(void * bd_addr) {return NULL;}
void remove_latt_connection_context(void * latt_con) {}
int latt_send_notify_packet(void *context, struct txbuf_tag *buf) {return -1;}
void latt_event_server_send(uint16_t cid) {}
void btstack_latt_send(void) {}
void btstack_latt_connect(void) {}
void btstack_latt_disconnect(void) {}
void latt_init_do(void) {}
#endif

#if !LE_EN
u16 ble_get_gatt_mtu(void) { return 0; }
void btstack_ble_send_req(void) {}
int ble_send_notify_packet(void *context, void *buf) {return -1;}
void att_event_server_send(void) {}
//void hci_run_le_connection(void) {}
bool ble_event_cmd_complete(uint8_t *packet, int size) { return false; }
void ble_event_meta(uint8_t *packet, int size) {}
void btstack_ble_init(void) {}
void btstack_ble_update_conn_param(void) {}
void btstack_ble_set_adv_interval(void) {}

uint16_t tws_get_ble_service(uint8_t *data_ptr){return 0;}
void tws_set_ble_service(uint8_t *data_ptr, uint16_t size){}
uint16_t tws_ble_get_adv_info(uint8_t *data_ptr){return 0;}
void tws_ble_set_adv_info(uint8_t *data_ptr, uint16_t size){}
void tws_send_ble_service_cfm(void) {}
void tws_send_ble_service_continue_cfm(void) {}
uint16_t tws_get_ble_link(uint8_t *data_ptr) { return 0; }
uint16_t tws_set_ble_link(uint8_t *data_ptr, uint16_t size) { return 0; }
void tws_set_ble_service_continue(uint8_t *data_ptr, uint16_t size) {}
bool tws_ble_switch_save_pend(uint8_t type, uint8_t flag, uint32_t buf_ptr, uint16_t len) { return false; }
uint16_t tws_get_ble_service_continue(uint8_t *data_ptr) { return 0; }

typedef uint8_t (*bb_msg_func_t)(uint16_t index, void const *param);
typedef uint8_t (*bb_cmd_func_t)(uint16_t cmd, void const *param);
typedef uint8_t (*ll_cntl_func_t)(uint8_t opcode);

struct ll_cntl_pdu_info
{
    ll_cntl_func_t  func;
    const char*     desc;
    uint16_t        length;
    uint8_t         flag;
};

struct bb_msg_info
{
    uint16_t index;
    bb_msg_func_t func;
};

struct bb_cmd_info
{
    uint16_t index;
    bb_cmd_func_t func;
};

AT(.rodata.le.ll_proc)
const struct bb_cmd_info ll_hci_cmd_tbl[1] = {0};
AT(.rodata.le.ll_proc)
const struct bb_msg_info ll_msg_tbl[1] = {0};
AT(.rodata.le.ll_cntl)
const struct ll_cntl_pdu_info ll_cntl_pdu_tbl[1] = {0};
AT(.rodata.le.ll_proc)
const struct bb_msg_info mgr_hci_cmd_tbl[1] = {0};
void ll_init(uint8_t init_type) {}
uint8_t ll_start(uint8_t index, void *param) {return -1;}
void ll_stop(uint8_t index) {}
void ll_cntl_state_set(uint8_t index, uint8_t txrx, uint8_t state) {}
void ll_proc_timer_set(uint8_t index, uint8_t type, bool enable) {}
void ll_proc_timer_set_state(uint8_t index, uint8_t type, bool enable) {}
void ll_cntl_send(uint8_t index, void *pdu, ll_cntl_func_t tx_func) {}
void ll_cntl_tx_check(uint8_t index) {}
bool ble_adv_end_con_ind(void const *param) { return false; }
AT(.com_text.bb.ble.chs)
void ble_channel_assess(uint8_t channel, bool rx_ok, uint32_t ts) {}
AT(.com_text.bb.ble.end)
void ble_con_rx_end(uint8_t index, bool rx_ok, uint16_t rxchass) {}
AT(.com_text.bb.ble.prio)
void ble_con_prio_check(void *con) {}
void aes_init(uint8_t init_type) {};
void aes_result_handler(uint8_t status, uint8_t* result) {};
void ble_ecpy(uint8_t *key, uint8_t *enc_data) {}
#endif

#if !LE_PAIR_EN
#if (LE_ADDRESS_TYPE == GAP_RANDOM_ADDRESS_TYPE_OFF)
void le_sm_init(void) {}
void gap_random_address_set_mode(int type) {}
void le_device_db_init(void){}
void gap_set_address_type(int address_type){}
#endif
void sm_just_works_confirm(void) {}
void sm_request_pairing(void) {}
void sm_send_security_request(void) {}
int sm_authorization_state(void) { return 0; }
int sm_authenticated(void) { return 0; }
int sm_encryption_key_size(void) { return 0; }
void sm_add_event_handler(void) {}
void sm_cmac_signed_write_start(void){ }
int sm_cmac_ready(void) { return 0; }
uint16_t sm_set_setup_context(uint8_t *ptr){ return 0;}
uint16_t sm_get_setup_context(uint8_t *ptr){ return 0;}
uint16_t tws_get_sm_db_info(uint8_t *ptr){ return 0;}
uint16_t tws_set_sm_db_info(uint8_t *ptr, uint16_t len){ return 0;}
#endif

#if !BT_PBAP_EN
void pbap_client_init(void) {}
void btstack_pbap(u8 param) {}
u32 pbap_get_sta(void){ return false; }
#endif

#if !BT_MAP_EN
void map_client_init(void) {}
void btstack_map(uint param) {}
void bt_get_time(char *ptr){}
int bt_map_is_connected(void) { return 0; }
void map_client_remove_connection(void) {}
uint8_t tws_get_map_service(uint8_t *data_ptr) { return 0; }
uint8_t tws_set_map_service(uint8_t *data_ptr, uint16_t conhdl, u8 * address, uint8_t len) {return 0; }
#endif
#if !(BT_PBAP_EN || BT_MAP_EN)
void goep_client_init(uint8_t rfcomm_channel_nr) {}
void goep_client_remove_connection(void) {}
uint8_t tws_get_goep_service(uint8_t *data_ptr) {return 0; }
uint8_t tws_set_goep_service(uint8_t *data_ptr, uint16_t conhdl, u8 * address, uint8_t len) {return 0; }
#endif

#if !DAC_DRC_EN
AT(.com_text.bt_dac_drc.process)
void alg_drc_process(void) {}
AT(.com_text.dac_drc.process)
void dac_drc_set_param(const void* param){}
AT(.com_text.func.aux)
void aux_pcm_drc_process(u8 *ptr, u32 samples, int ch_mode){}
#else
AT(.com_text.drc_v3.dnr)
void drc_v3_dnr(s32 *input, int slope){}
#endif

#if !SDADC_SOFT_GAIN_EN
AT(.com_text.sdadc.soft_gain)
void sdadc_drc_soft_gain_proc(s16 *ptr, u32 samples, int ch_mode) {}
AT(.bt_voice.sdadc.soft_gain.proc)
bool sdadc_soft_gain_proc(s16 *ptr, u32 samples, int ch_mode){return false;}
bool sdadc_drc_init(u8 *drc_addr, int drc_len){return false;}
#endif

#if !SDADC_EQ_EN
AT(.bt_voice.soft_eq)
void sdadc_soft_eq_proc(s16 *ptr, u32 samples, int ch_mode) {}
bool sdadcl_set_soft_eq_by_res(u32 addr, u32 len){return false;}
bool sdadcr_set_soft_eq_by_res(u32 addr, u32 len){return false;}
s16 soft_eq_proc(s16 sample, soft_eq_t *soft_eq){return 0;}
bool soft_eq_set_by_res(u32 addr, u32 len, soft_eq_t *soft_eq){return false;}
#endif

#if !SDADC_DRC_EN
bool sdadc_drc_v3_init(u8 *drc_addr, int drc_len){return false;}
AT(.bt_voice.sdadc.drc_v3)
bool sdadc_drc_v3_calc(s16 *ptr, u32 samples, int ch_mode){return false;}
#endif

#if !SDADC_SINGLE_DRC_EN
bool sdadc_drc_single_init(u8 *drc_addr, int drc_len){return false;}
AT(.bt_voice.sdadc.drc_single)
bool sdadc_drc_single_calc(s16 *ptr, u32 samples, int ch_mode){return false;}
#endif

#if !BT_MIC_DRC_EN
bool mic_drc_init(u8 *drc_addr, int drc_len){return false;}
s16 mic_drc_calc(s32 sample){return 0;}
void bt_sco_drc_process(u32 *in_ptr, u16 *out_ptr, int samples){}
#endif

#if !BT_SCO_DAC_DRC_EN
bool bt_sco_dac_drc_init(u8 *drc_addr, int drc_len) {return false;}
AT(.bt_voice.dac.drc)
s16 bt_sco_dac_drc_calc(s32 data) {return 0;}
AT(.bt_voice.dac.drc)
void bt_sco_dac_drc_proc(s16 *ptr, u32 samples) {}
#endif

#if !BT_SCO_DAC_DRC_DNR_EN && !DAC_DRC_EN
AT(.com_text.drc_v3.dnr)
void drc_v3_dnr(s32 *input, int slope) {}
#endif

#if !BT_SCO_FAR_NR_EN
void nr_process(s16 *data) {}
void nr_init(u32 nt) {}
void nr_far_process(s16 *data) {}
void nr_far_init(u16 noise_thr, u16 nr_level) {}
void far_ains2_process(s16 *data){}
void far_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min){}
void bt_sco_nr_process(u16 *ptr, u8 len) {}
#else
#if (BT_SCO_FAR_NR_SELECT == 0)
void far_ains2_process(s16 *data){}
void far_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min){}
void nr_far_process(s16 *data) {}
void nr_far_init(u16 noise_thr, u16 nr_level) {}
#elif (BT_SCO_FAR_NR_SELECT == 1)
void nr_process(s16 *data) {}
void nr_init(u32 nt) {}
void nr_far_process(s16 *data) {}
void nr_far_init(u16 noise_thr, u16 nr_level) {}
#elif (BT_SCO_FAR_NR_SELECT == 2)
void far_ains2_process(s16 *data){}
void far_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min){}
void nr_process(s16 *data) {}
void nr_init(u32 nt) {}
#endif
#endif

#if !BT_SNDP_DUMP_EN && !BT_AEC_DUMP_EN && !BT_SCO_FAR_DUMP_EN && !BT_BCNS_DUMP_EN && !BT_DMNS_DUMP_EN && !BT_AINS2_DUMP_EN && !BT_DNN_DUMP_EN && !BT_AEC_FRE_DUMP_EN && !BT_SCO_DUMP_EN && !BT_EQ_DUMP_EN
void sco_huart_init(void){}
bool sco_huart_putcs(u8 type, u8 frame_num, const void *buf, uint len){return 0;}
AT(.bt_voice.sco.dump)
void bt_sco_dump_cb(uint type, void *ptr, uint size) {}
AT(.nr_buf.dump)
s16 bt_sco_dump_buf[0][0];
#endif

//#if !BT_AEC_DUMP_EN && !BT_SCO_FAR_DUMP_EN && !BT_EQ_DUMP_EN && !BT_AEC_FRE_DUMP_EN
//void aec_audio_dump_init(void){}
//void aec_audio_dump(u8 type, s16* ptr, u16 len, u8 ch){}
//#endif

#if !BT_SCO_CALLING_NR_EN
AT(.bt_voice.sco)
u16 dnr_voice_maxpow_calling(u32 *ptr, u16 len){return 0;};
#endif

#if !BT_NLMS_AEC_EN
void nlms_aec_init(aec_cb_t *aec){}
void nlms_aec_process(void){}
void nlp_aec_process_plus(short *farend, short *nearend_nr, short *nearend_mic, short *output){}
void nlms_far_buf_count_clear(u8 far_frame_len){}
#endif

#if !BT_NLMS_FRE_EN
void bt_fdnlms_init(aec_cb_t *aec) {}
AT(.bt_voice.fdnlms.proc)
void bt_fdnlms_fill_far_buf(void) {}
AT(.bt_voice.fdnlms.proc)
void bt_fdnlms_block_proc(s32 *tmic_fft, s32 *fmic_fft) {}
AT(.bt_voice.fdnlms.proc)
void nlp_gain_dnn_mul(s32 *nearend_fft) {}
AT(.bt_voice.fdnlms.proc)
void bt_fdnlms_nlp_gain(s32 *nearend_fft) {}
#else
    #if BT_NLMS_FRE_REF_SET == 0        //XD & XE
    void fdnlms_nlinear_xe_proc(s32 *fft_out, s32 *nearend) {}
    void fdnlms_nlinear_xd_proc(s32 *fft_out, s32 *nearend) {}
    #elif BT_NLMS_FRE_REF_SET == 1      //XD
    void fdnlms_xe_init(void) {}
    void fdnlms_nlinear_xd_xe_proc(s32 *fft_out, s32 *nearend) {}
    void fdnlms_nlinear_xe_proc(s32 *fft_out, s32 *nearend) {}
    int  fdnlms_xe_smooth(s32 ps_e_tmp, int tmpval) {return 0;}
    #elif BT_NLMS_FRE_REF_SET == 2      //XE
    void fdnlms_xd_init(void) {}
    void fdnlms_nlinear_xd_xe_proc(s32 *fft_out, s32 *nearend) {}
    void fdnlms_nlinear_xd_proc(s32 *fft_out, s32 *nearend) {}
    void fdnlms_block_xd_proc(s32 *far_fft, s32 *tmic_fft, s32 *fmic_fft) {}
    #endif
#endif

#if !BT_AEC_EN
void aec_init(aec_cb_t *aec) {}
void aec_process(void) {}
AT(.bt_voice.aec)
bool aec_isr(void) { return false; }
#else
    #if BT_SCO_DNN_EN && !BT_SCO_DNN_MODE
    s16 far_temp[0][0] AT(.nr_cache.far_buf);
    #endif
#endif

#if BT_NLMS_AEC_EN || BT_AEC_EN || BT_NLMS_FRE_EN || BT_ALC_EN
void aec_init_dft(aec_cb_t *aec) {}
#endif

#if BT_NLMS_AEC_EN || BT_AEC_EN
void bt_noaec_process(u8 *ptr, u32 samples, int ch_mode) {}
#endif

#if !BT_AEC_EN || ((!BT_SNDP_EN || !BT_SNDP_MODE) && (!BT_SCO_AINS2_EN || !BT_SCO_AINS2_MODE) && (!BT_SCO_AINS3_EN || !BT_SCO_AINS3_MODE) && (!BT_SCO_DNN_EN || !BT_SCO_DNN_MODE) && !BT_SCO_AIAEC_DNN_EN && !BT_SNDP_DMIC_EN && !BT_SNDP_DMIC_DNN_EN && !BT_SCO_DMNS_EN && !BT_SCO_DMDNN_EN && !BT_SCO_LDMDNN_EN)
AT(.bt_voice.aec)
void far_buf_count_clear(u8 far_frame_len){}
AT(.bt_voice.aec)
void mode1_far_buf_cov(s16* buf){}
#endif

#if !BT_ALC_EN
void alc_init(void) {}
void alc_process(void) {}
void alc_fade_in(s16 *buf) {}
void alc_fade_out(s16 *buf) {}
AT(.bt_voice.alc)
bool alc_isr(void) { return false; }
AT(.bt_voice.alc)
void bt_alc_process(u8 *ptr, u32 samples, int ch_mode) {};
#else
void alc_init_dft(alc_cb_t *alc) {}
#endif

#if !BT_SNDP_EN
void sndp_process(void) {}
void bt_sndp_process(s16 *buf) {}
void bt_sndp_init(void) {}
void bt_sndp_exit(void) {}
#else
    #if BT_SNDP_TYPE
    void sndp_rnn_process(void) {}
    void bt_sndp_rnn_process(s16 *buf) {}
    void bt_sndp_rnn_init(void) {}
    void bt_sndp_rnn_exit(void) {}
    #else
    void sndp_dnn_process(void) {}
    void bt_sndp_dnn_process(s16 *buf) {}
    void bt_sndp_dnn_init(void) {}
    void bt_sndp_dnn_exit(void) {}
    #endif
#endif

#if !BT_SCO_AINS2_EN
void ains2_process(s16 *data){}
void ains2_init(u16 nt, u16 noise, u8 trumpet) {}
void ains2_plus_process(void) {}
void bt_ains2_plus_init(void) {}
void bt_ains2_plus_process(s16 *buf) {}
#endif

#if !BT_SCO_AINS3_EN
void bt_ains3_init(ains3_cb_t *ains3_cb) {}
void ains3_plus_process(void) {}
void bt_ains3_plus_process(s16 *buf) {}
#endif

#if !BT_SCO_BCNS_EN
int bcns_process(s16 *data,s16 *vdata){return 0;}
void bcns_init(u32 nt, u32 vad_threshold, u8 enlarge){}
void bcns_plus_process(void){}
void bt_bcns_plus_process(s16 *buf){}
#endif

#if !BT_SCO_DMNS_EN && !BT_SCO_DMDNN_EN
void dmns_plus_process(void) {}
void bt_dmns_plus_process(s16 *buf) {}
void bt_dmns_init(dmns_cb_t *dmns_cb) {}
#endif

#if !BT_SCO_DMNS_EN
AT(.bt_voice.dmns.proc)
void dmns_fre_process(s32 *fft_in0, s32 *fft_in1) {}
void dmns_init(dmns_cb_t *dmns_cb) {}
#else
    #if !BT_SCO_DMNS_BF_TYPE
    AT(.dmns_text.bf1)
    void dmns_proc_bf_type1(s32 *f,s32 *f2) {}
    #else
    AT(.dmns_text.bf0)
    void dmns_proc_bf_type0(s32 *f,s32 *f2) {}
    #endif
#endif

#if !BT_SCO_DMDNN_EN
AT(.bt_voice.dmdnn.proc)
void dmdnn_fre_process(s32 *fft_in0, s32 *fft_in1) {}
void dmdnn_init(dmns_cb_t *dmdnn_cb) {}
void dmdnn_proc_bf_type1(s32 *f,s32 *f2) {}
//#else
//    #if !BT_SCO_DMDNN_MODEL_SET
//    AT(.bt_voice.dmdnn.m1_type0)
//    void dmdnn_model1_bf_type0(void *st_tmp, s32 *f,s32 *f2) {}
//    AT(.dmdnn_text.m1_type1)
//    void dmdnn_model1_proc(void *st_tmp, s32 *f,s32 *f2) {}
//    void dmdnn_m1_init(void) {};
//    #else
//    AT(.dmdnn_text.model0)
//    void dmdnn_model0_proc(void *st_tmp, s32 *f,s32 *f2) {}
//    void dmdnn_m0_init(void) {};
//        #if !BT_SCO_DMDNN_BF_TYPE
//        AT(.dmdnn_text.m1_type1)
//        void dmdnn_model1_proc(void *st_tmp, s32 *f,s32 *f2) {}
//        #else
//        AT(.bt_voice.dmdnn.m1_type0)
//        void dmdnn_model1_bf_type0(void *st_tmp, s32 *f,s32 *f2) {}
//        #endif
//    #endif
#endif

#if !BT_SCO_LDMDNN_EN
void ldmdnn_plus_process(void) {}
void bt_ldmdnn_plus_process(s16 *buf) {}
void bt_ldmdnn_init(ldmdnn_cb_t *ldmdnn_cb) {}
AT(.bt_voice.ldmdnn.proc)
void ldmdnn_fre_process(s32 *fft_in0, s32 *fft_in1) {}
void ldmdnn_init(ldmdnn_cb_t *ldmdnn_cb) {}
void ldmdnn_proc_bf_type1(s32 *f,s32 *f2) {}
#endif

#if !BT_SCO_COMFORT_NOISE_EN
AT(.bt_voice.comfort_noise.proc)
void ComfortNoiseProcessing(s32 *f, s32 *noise_spectrum, u32 *seed, u8 comforN_level, u16 comforN_floor) {}
#if BT_SCO_DMDNN_EN || BT_SCO_DMNS_EN || BT_SCO_LDMDNN_EN
s32 dmns_noise_spectrum[0] AT(.bt_dmns.cng);
#elif BT_SCO_DNN_EN || BT_SCO_AIAEC_DNN_EN || BT_SCO_DNN_PLUS_EN
s32 dnn_noise_spectrum[0] AT(.bt_dmns.cng);
#endif
#endif

#if !BT_SCO_AGC_EN
void bt_sco_agc_proc(s16 *buf) {}
void bt_agc_init(void) {}
void agc_init(s32 sampleHzIn, s32 bit, s32 agcDb, s32 agcDbfs) {}
void agc_proc(s16 *input, s16 *output, s32 pcmLen) {}
void bt_sco_agc_proc_do(s16 *ptr, int samples) {}
s16 agc_ibuf[0] = {};
s16 agc_obuf[0] = {};
#endif

AT(.com_text.qtest)
bool tbox_qtest_en(void)
{
#if VUSB_TBOX_QTEST_EN
    return xcfg_cb.qtest_en;
#else
    return 0;
#endif
}

#if !BT_SPP_EN
void spp_txpkt_init(void) {}
AT(.com_text.spp.send_req)
void btstack_spp_send_req(void) {}
AT(.com_text.pp.send_pkt)
int spp_send_packet(void *context, void *buf) {return -1;}
AT(.com_text.spp.event_send)
void spp_event_send(void) {}
#endif

#if !BT_SNDP_DMIC_EN && !BT_SNDP_DMIC_DNN_EN
void sndp_dm_v02_init(void *cfg) {}
void sndp_dm_v03_init(void) {}
void sndp_dm_v03_config(sndp3_dm_cb_t *dm_cb) {}
void sndp_dm_process(void) {}
void bt_sndp_dm_process(s16 *buf) {}
void bt_sndp_dmic_init(void) {}
void bt_sndp_dmic_exit(void) {}
AT(.com_text.dma.sndp)
void dma_sndp_dm_process(s16 *buf){}
void dmic_init_dft(dmic_cb_t *dmic){}
#else
    #if BT_SNDP_DMIC_DNN_EN
    AT(.bt_voice.sndp2_dm)
    void sndp_dm_v02_proc(s32* outY, s32* inX0, s32* inX1) {}
    void sndp_dm_v02_init(void *cfg) {}
    void dmic_init_dft(dmic_cb_t *dmic) {}
        #if !BT_SNDP_DMIC_DNN_NLMS_EN
        AT(.sndp3_dm_text.bf)
        void Sndp_FdBNlms_Process_Fixed_Dmv3(int* pErr, int* pEst, int* pRef, int* pDes, int* pRotFactor, void* p_sFdN, int* BinStepCtrlFactor, int inputQ) {}
        AT(.sndp3_dm_text.bf)
        int Sndp_GCC_EstNormlizedDelay_Fixed_Dmv3(int* pCohBuff, int* CX0X1, int* CX0X0, int* CX1X1, int CX0X0_Q, const int* weighFactor, int MaxDOA_Samp, int startBin, int EndBin, int FFTLen, int* tmpBuff) {return 0;}
        char g_NLMS_BUF[0]              AT(.sndp3_dm.sta);
        char g_shareBuf_2mV3[4104]      AT(.sndp3_dm.share);
        int  p_SeGain_Fixed_2mV3[257]   AT(.sndp3_dm.SeGain);
        int  BF_out_2mV3[512]           AT(.sndp3_dm.BF);
        #else   //BT_SNDP_DMIC_DNN_NLMS_EN模式空间紧张，需要调整一下
            #if BT_AEC_EN           //开硬件AEC需要挪一下远端缓存BUF空间才够
            s16 far_temp[14][64] AT(.bt_sndp.sndp3_dm);
            #elif BT_NLMS_FRE_EN    //开频域NLMS AEC要调整一下空间才够用
            int  p_SeGain_Fixed_2mV3[257] AT(.fdnlms.sndp3_dm.SeGain);
            #endif
        #endif
    #else
    AT(.bt_voice.sndp3_dm)
    void sndp_dm_v03_proc(s32* outY, s32* inX) {}
    void sndp_dm_v03_init(void) {}
    void sndp_dm_v03_config(sndp3_dm_cb_t *dm_cb) {}
    #endif
#endif

#if !VUSB_TBOX_QTEST_EN
void qtest_parm_init(void) {}
void qtest_only_pair(void) {}
void qtest_only_pair_process(void) {}
AT(.com_text.qtest)
u8 qtest_get_mode(void) {return 0;}
bool qtest_is_send_btmsg(void) {return false;}
bool update_set_qcheck_code(void) { return false; }
void qtest_set_check_code(u32 check_code){};
void qtest_show_user_info(u8* param){};
void qtest_custom_pair_id(u8* param){};
void qtest_generate_linkey(u8* random_key,u8 *linkey){};
void qtest_send_packet(u8* txbuf,u8 sub_cmd){};
void qtest_set_local_bt_info_do(u8* buf,u8* txbuf){};
void qtest_set_tx_param(void){};
void qtest_deal_uart_message(void){};
void qtest_set_tws_bt_info(u8* buf){};
void qtest_set_local_bt_info(u8 *buf){};
void qtest_beforehand_process(void){};
void qtest_set_tws_bt_info_do(u8 *buf){};
void qtest_process(void){};
void qtest_get_bt_link_info(u8* buf){};
u8 qtest_check_ear_feature(u8* tx_ptr){return 0;};
u32 qtest_check_tbox_feature(u8* rx_ptr){return 0;};
u32 qtest_get_osci_cap_addr(void) {return 0;}
void param_bt_qpdn_boat_flag_write(u8 flag){};
void param_bt_qpdn_boat_flag_read(void){};
void qtest_dec_heart_pkt_delay(void){};
u8 qtest_get_pickup_sta(void);
void qtest_create_env(void){};
u8 qtest_get_tws_channel(void){ return 0;};
void qtest_send_msg2tbox(u8 *buf, u16 len){};
void qtest_deal_uart_message_do(u8* buf){};
void qtest_process_do(void){};
void qtest_var_init(void){};
u8 qtest_init(u8 rst_source){ return 0;};
bool qtest_set_pdn_boat_flag(u8 flag) {return 0;};
#endif

#if !BT_NEAR_AINS2_EN
void near_ains2_process(s16 *data, u8 alg_count) {}
void near_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min) {}
#endif

#if !BT_TRUMPET_DENOISE_EN
void trumpet_denoise_init(u8 level) {}
void trumpet_denoise(int *fft_in) {}
AT(.bt_voice.sco.trumpet)
void bt_sco_trumpet_nr(s32 *fft_in) {}
#endif

#if !SMIC_DBG_EN && !DMIC_DBG_EN
AT(.bt_voice.test)
bool bt_call_test_process(u8 *ptr, u32 samples) { return false;}
#endif

#if !BT_HID_EN
void hid_device_init(void) {}
int bt_hid_is_connected(void) { return 0;}
void hid_establish_service_level_connection(void* bd_addr) {}
void hid_release_service_level_connection(void* bd_addr) {}
void btstack_hid_api(uint param) {}
const void *btstack_hid_tbl[0];
void btstack_hid_send(void) {}
bool bt_hid_send(void *buf, uint len, bool auto_release) { return false;}
bool bt_hid_is_send_complete(void) { return true;}
void hid_report_set(void *buf, uint len, bool auto_release) {}
bool bsp_bt_hid_vol_change(u16 keycode) {return 0;}
bool bsp_bt_hid_photo(u16 keycode) {return 0;}
void bsp_bt_hid_tog_conn(void) {}
#endif

#if !BT_HID_DOUYIN_EN
void btstack_hid_douyin(uint keycode){}
#endif

#if !BT_V3D_AUDIO_EN
void v3d_process(s16 *data_l, s16 *data_r, u8 flag) {}
void v3d_init(void) {}
const char* get_v3d_version(void) {return NULL;}
void v3d_set_param(int mode, u16 wet, u16 dry) {}
#endif

#if !BT_VBASS_AUDIO_EN
void vbass_process_do(s16 *data_l, s16 *data_r) {}
void vbass_init(void) {}
#endif

#if !BT_HARMONIC_AUDIO_EN
void hd_process(s16 *ldata_buf, s16 *rdata_buf) {}
void hd_init(void) {}
#endif

#if !BT_AUTOWAH_AUDIO_EN
void aw_process(s16 *ldata_buf, s16 *rdata_buf) {}
void aw_init(void) {}
#endif

#if !BT_MUSIC_EFFECT_SOFT_VOL_EN
void soft_vol_process_mono_one_sample(soft_vol_t* p, s16* input) {}
void soft_vol_process_stereo_one_sample(soft_vol_t* p, s16* inputl, s16* inputr) {}
void soft_vol_set_vol_param(soft_vol_t* p, u16 vol, u8 vol_direct_set) {}
void soft_vol_set_fade_step(soft_vol_t* p, u16 step) {}
void soft_vol_init(soft_vol_t* p) {}
#endif

#if !BT_DYNAMIC_BASS_BOOST_EN
void dynamic_bass_process(s16* buf_l, s16* buf_r, u32 samples, u8 sample_rate) {}
void dynamic_bass_clear_cache(void) {}
void dynamic_bass_set_param(dbb_param_cb_t* p) {}
void dynamic_bass_set_coef_param(const u8* param, u32 len) {}
void dynamic_bass_exit(void) {}
void dynamic_bass_init(void) {}
#endif

#if !BT_DYNAMIC_EQ_EN
void dynamic_eq_frame_process(s16* buf_l, s16* buf_r) {}
void dynamic_eq_clear_cache(void) {}
void dynamic_eq_process(void *cb, s32 *samples) {}
void dyeq_init(void *cb, void *p) {}
void dyeq_coef_update(void *cb, u8 *buf) {}
void dyeq_clear_cache(void *cb) {}
#endif

#if !BT_MUSIC_AUDIO_EN
void music_audio_process(void) {}
void bt_music_audio_init(u8 audio_type) {}
bool bt_music_audio_set_state(u8 music_audio) {return false;}
u8 bt_music_audio_get_state(void) {return 0;}
#endif

#if !BT_AVDTP_DYN_LATENCY_EN
void a2dp_set_latency_ms(uint32_t a2dp_latency_ms) {}
void a2dp_latency_check(){}
uint16_t a2dp_calc_latency(int8_t rssi, uint16_t rxpkt_ok) {return 0;}
#endif

#if !BT_A2DP_EXCEPT_RESTORE_PLAY_EN
void noload_set_play_state(uint8_t play_state){}
uint16_t noload_get_play_state(uint8_t play_state){return 0;}
void noload_clear_play_state(uint8_t play_state){}
void noload_reset_play_state() {}
#endif

#if !WARNING_ESBC_PLAY
AT(.com_text.esbc)
void esbc_sync_kick(void) {}
int esbc_init(void){return 0;}
int spi_esbc_decode_init(void){return 0;}
bool esbc_decode_frame(void){return false;}
bool esbc_res_play_kick(u32 addr, u32 len, bool kick){return true;}
void esbc_res_play_exit(void) {}
#else
#if !FUNC_MUSIC_EN
AT(.mp3dec.dec)
bool mp3_dec_frame(void) { return false; }
#endif
#endif

#if !BT_SCO_DNN_EN
void dnn_fre_process(s32 *f) {}
void dnn_init(dnn_cb_t *dnn_cb) {}
void bt_dnn_init(dnn_cb_t *dnn_cb) {}
void bt_dnn_process(s16 *buf) {}
void dnn_sm_process(void) {}
//#else
//    #if !BT_SCO_DNN_MODEL_SET
//    AT(.dnn_text.model1)
//    void dnn_model1_proc(void *st_tmp, s32 *f) {}
//    void dnn_m1_init(void) {};
//    #else
//    AT(.dnn_text.model0)
//    void dnn_model0_proc(void *st_tmp, s32 *f) {}
//    void dnn_m0_init(void) {};
//    #endif
//    #if !BT_SCO_DNN_WIND_EN
//    AT(.bt_voice.dnn.wind)
//    void dnn_wind_detect(void *st_tmp, s32 *f) {}
//    #endif
#endif

#if !BT_SCO_DNN_PLUS_EN
void dnn_plus_fre_process(s32 *f) {}
void dnn_plus_init(dnn_cb_t *dnn_cb) {}
void bt_dnn_plus_init(dnn_cb_t *dnn_cb) {}
void bt_dnn_plus_process(s16 *buf) {}
void dnn_plus_sm_process(void) {}
#endif

#if !BT_SCO_AIAEC_DNN_EN
void aiaec_nlms_fill_far_buf(void) {}
void aiaec_nlms_fft_proc(s32 *tmic_fft, s32 *fmic_fft) {}
s32 dnn_aec_ns_process(s32 *xfw, s32 *dfw) {return 0;}
void aiaec_dnn_init(dnn_aec_ns_cb_t *aiaec_dnn_cb) {}
void bt_aiaec_dnn_init(dnn_aec_ns_cb_t *aiaec_dnn_cb) {}
void bt_aiaec_dnn_process(s16 *buf) {}
void aiaec_dnn_sm_process(void) {}
void pink_noise_proc(s16 *pink_buf, s16 *mic_in) {}
void aiaec_pink_generation_init(s32 plfsr_val) {}
s16 pink_gen_buf[0] AT(.aiaec_dnn_buf.sta);
#endif


#if !BT_SPP_EN
void spp_establish_service_level_connection(void *bd_addr) {}
void spp_release_service_level_connection(void *bd_addr) {}
void remove_spp_connection_context(void * spp_connection) {}
void * provide_spp_connection_context_for_bd_addr(void *bd_addr) { return NULL; }
void spp_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {}
void spp_service_flush(void) {}
void * get_spp_connection_context_for_bd_addr(void *bd_addr) { return NULL; }
void spp_init_var(void) {}
#endif

#if !BT_SCO_FADE_EN
void bt_voice_fade_init(u32 timeout) {}
AT(.bt_voice.sco.fade)
void bt_voice_fade_do(s16 *ptr, u32 samples, int ch_mode) {}
#else
AT(.bt_voice.sco.skip)
void bt_voice_skip_frame_do(s16 *ptr, u32 samples, int ch_mode) {}
#endif

#if !BT_SCO_DNN_EN && !BT_SCO_DMDNN_EN && !BT_SCO_AIAEC_DNN_EN
AT(.bt_voice.sco)
bool bt_sco_dnn_en(void) {return 0;}
AT(.bt_voice.sco.dnn)
void dnn_far_upsample(s16 *out, s16 *in, u32 samples, u8 step) {}
#endif

#if (BT_SNDP_DMIC_DNN_DISTANCE < 15) || (BT_SNDP_DMIC_DNN_DISTANCE>30)
#error "BT_SNDP_DMIC_DNN_DISTANCE is out of range [15~30]\n"
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (15)
int gc_FixedBeamFilter_Q19_15mm[0] = {};    //15mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (16)
int gc_FixedBeamFilter_Q19_16mm[0] = {};    //16mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (17)
int gc_FixedBeamFilter_Q19_17mm[0] = {};    //17mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (18)
int gc_FixedBeamFilter_Q19_18mm[0] = {};    //18mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (19)
int gc_FixedBeamFilter_Q19_19mm[0] = {};    //19mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (20)
int gc_FixedBeamFilter_Q19_20mm[0] = {};    //20mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (21)
int gc_FixedBeamFilter_Q19_21mm[0] = {};    //21mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (22)
int gc_FixedBeamFilter_Q19_22mm[0] = {};    //22mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (23)
int gc_FixedBeamFilter_Q19_23mm[0] = {};    //23mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (24)
int gc_FixedBeamFilter_Q19_24mm[0] = {};    //24mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (25)
int gc_FixedBeamFilter_Q19_25mm[0] = {};    //25mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (26)
int gc_FixedBeamFilter_Q19_26mm[0] = {};    //26mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (27)
int gc_FixedBeamFilter_Q19_27mm[0] = {};    //27mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (28)
int gc_FixedBeamFilter_Q19_28mm[0] = {};    //28mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (29)
int gc_FixedBeamFilter_Q19_29mm[0] = {};    //29mm
#endif
#if BT_SNDP_DMIC_DNN_DISTANCE != (30)
int gc_FixedBeamFilter_Q19_30mm[0] = {};    //30mm
#endif

#if !USER_TKEY_SLIDE
AT(.com_text.tkey.isr)
void tkey_slide_param_reint(tk_cb_t *s) {}
AT(.com_text.tkey.isr)
void tkey_slide_up_down_check(tk_cb_t *s, tk_cb_t *p) {}
AT(.com_text.tkey.isr)
void tkey_slide_process(tk_cb_t *s, tk_cb_t *p, u16 tkcnt, u32 variance) {}
#endif

#if !ASR_VAD_EN
AT(.com_text.process)
int vad_process(int16_t *ptr) {return 0;}
AT(.com_text.sleep)
bool is_vad_wake(void) {return 0;}
AT(.com_text.sleep)
void vad_enter_sleep(u8 sleep);
AT(.com_text.vad)
u32 vad_get_ste_value(void) {return 0;}
int vad_init(u32 start_threshold_value, u32 stop_threshold_value) {return 0;}
#endif

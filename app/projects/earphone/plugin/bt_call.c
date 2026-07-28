#include "include.h"

const uint8_t cfg_bt_call_enc_support = BT_SCO_DMNS_EN | BT_SCO_DMDNN_EN | BT_SCO_LDMDNN_EN;

#if BT_SNDP_DMIC_EN
    static dmic_cb_t dmic_cb AT(.sndp_dm.init);
    u8 cfg_micr_phase_en;
#elif BT_SNDP_DMIC_DNN_EN
    static sndp3_dm_cb_t dmic_cb AT(.sndp3_dm.init);
    u8 cfg_micr_phase_en;
#elif BT_SCO_DMNS_EN || BT_SCO_DMDNN_EN
    static dmns_cb_t dmns_cb AT(.nr_buf.dmdnn.sta);
    u8 cfg_micr_phase_en;
#elif BT_SCO_LDMDNN_EN
    static ldmdnn_cb_t ldmdnn_cb AT(.ldmdnn_data.sta);
    u8 cfg_micr_phase_en;
#elif BT_SCO_AINS3_EN
    static ains3_cb_t ains3_cb AT(.ains3_buf.sta);
#elif BT_SCO_DNN_EN
    static dnn_cb_t dnn_cb AT(.dnn_data.sta);
#elif BT_SCO_AIAEC_DNN_EN
    static dnn_aec_ns_cb_t aiaec_dnn_cb AT(.aiaec_dnn_buf.sta);
#endif

AT(.bt_voice.gain.tbl)
const int mic_gain_tbl[16] = {
    SOFT_DIG_P0DB,
    SOFT_DIG_P1DB,
    SOFT_DIG_P2DB,
    SOFT_DIG_P3DB,
    SOFT_DIG_P4DB,
    SOFT_DIG_P5DB,
    SOFT_DIG_P6DB,
    SOFT_DIG_P7DB,
    SOFT_DIG_P8DB,
    SOFT_DIG_P9DB,
    SOFT_DIG_P10DB,
    SOFT_DIG_P11DB,
    SOFT_DIG_P12DB,
    SOFT_DIG_P13DB,
    SOFT_DIG_P14DB,
    SOFT_DIG_P15DB,
};

////库调用，设置MIC的增益（算法之后）
AT(.bt_voice.aec)
int sco_set_mic_gain_after_aec(void)
{
    if (xcfg_cb.mic_post_gain) {
        return mic_gain_tbl[xcfg_cb.mic_post_gain];
    }
    return 0;
}

//------------------------------------------------------------------------------------------
#if BT_SCO_DUMP_EN || BT_AEC_DUMP_EN || BT_AEC_FRE_DUMP_EN || BT_SCO_FAR_DUMP_EN || BT_EQ_DUMP_EN
static struct {
    u8 frame_cnt[DUMP_SCO_MAX_NB];
} sco_dump;

WEAK s16 bt_sco_dump_buf[4][256] AT(.nr_buf.dump);

AT(.bt_voice.sco.dump)
void bt_sco_dump_cb(uint type, void *ptr, uint size)
{
    int idx = -1;

    switch(type) {
        //idx的值按照算法链路处理的顺序由小到大排列，最后处理的放最前面，idx值越大
#if BT_SCO_DUMP_EN || BT_AEC_DUMP_EN || BT_AEC_FRE_DUMP_EN
        case DUMP_MIC_NR:           idx++;
        #if BT_AEC_DUMP_EN || BT_AEC_FRE_DUMP_EN
        case DUMP_AEC_FAR:          idx++;
        #endif
        #if BT_SCO_DMIC_EN
        case DUMP_MIC_FF:           idx++;
        #endif
        case DUMP_MIC_TALK:         idx++;

#elif BT_EQ_DUMP_EN
        case DUMP_EQ_OUTPUT:        idx++;
        case DUMP_MIC_NR:           idx++;
        #if BT_SCO_DMIC_EN
        case DUMP_MIC_FF:           idx++;
        #endif
        case DUMP_MIC_TALK:         idx++;

#elif BT_SCO_FAR_DUMP_EN
        case DUMP_FAR_NR_OUTPUT:    idx++;
        case DUMP_FAR_NR_INPUT:     idx++;
#endif
            //需要打印的case
            if ((0 <= idx) && (idx < DUMP_SCO_MAX_NB)) {
                void *buf = bt_sco_dump_buf[idx];
                memcpy(buf, ptr, size);
                sco_huart_putcs(idx, sco_dump.frame_cnt[idx]++, buf, size);
            }
            break;

        default :
            //不需要的走default
            break;
    }
}

    #define bt_sco_dump(a, b, c)            bt_sco_dump_cb(a, b, c)
#else
    #define bt_sco_dump(a, b, c)
#endif

void bt_sco_aec_init(u8 *sysclk, aec_cb_t *aec, alc_cb_t *alc)
{
#if BT_AEC_EN || BT_ALC_EN || BT_NLMS_AEC_EN || BT_NLMS_FRE_EN
    if (xcfg_cb.bt_aec_en) {
        aec_init_dft(aec);
    #if BT_AEC_EN
        aec->aec_en = 1;
        aec->rfu[0] = 0;       //0:自动模式 1:手动模式
        aec->echo_level = BT_ECHO_LEVEL;
        aec->far_pass_nr = xcfg_cb.bt_aec_far_nr_en;
    #elif BT_NLMS_AEC_EN
        aec->nlms_aec_en    = 1;
        aec->echo_level     = BT_ECHO_LEVEL;
        aec->nlms_nlp_mode  = BT_NLMS_NLP_MODE;
        *sysclk = *sysclk < SYS_80M ? SYS_80M : *sysclk;
    #elif BT_NLMS_FRE_EN
        aec->nlms_aec_en    = 2;
        aec->echo_level     = BT_ECHO_LEVEL;
        aec->ref_set        = BT_NLMS_FRE_REF_SET;
        aec->UpBin			= 63;
        aec->LowBin		    = 7;
        aec->qIdx			= 0.55f * (1 << 15);

        aec->echo_th        = 666;
        aec->gamma_sm		= 0.83f * (1 << 15);
        aec->arrgFact		= 0.8f * (1 << 15);
        aec->xd_avg_th		= 16384;
        *sysclk = *sysclk < SYS_80M ? SYS_80M : *sysclk;
    #endif
        aec->aec_dig_gain   = mic_gain_tbl[xcfg_cb.aec_dig_gain];
    } else if (xcfg_cb.bt_alc_en) {
    #if BT_ALC_EN
        alc_init_dft(alc);
        alc->alc_en = 1;
        alc->fade_in_delay = BT_ALC_FADE_IN_DELAY;
        alc->fade_in_step = BT_ALC_FADE_IN_STEP;
        alc->fade_out_delay = BT_ALC_FADE_OUT_DELAY;
        alc->fade_out_step = BT_ALC_FADE_OUT_STEP;
        alc->far_voice_thr = BT_ALC_VOICE_THR;
    #else
        printf("ALC init err\n");
    #endif
    }
#endif
    aec->far_offset     = BT_FAR_OFFSET;
}

void bt_sco_far_nr_init(u8 *sysclk, nr_cb_t *nr)
{
#if BT_SCO_FAR_NR_EN
    nr->type |= BIT(4 + BT_SCO_FAR_NR_SELECT);
    *sysclk = *sysclk < SYS_120M ? SYS_120M : *sysclk;
    #if (BT_SCO_FAR_NR_SELECT == 1)
    far_ains2_init(BT_SCO_FAR_NOISE_LEVEL, 0, 0);
    *sysclk = BT_NLMS_AEC_EN ? SYS_160M : *sysclk;
    #elif (BT_SCO_FAR_NR_SELECT == 2)
    nr_far_init(BT_SCO_FAR_NOISE_THRESHOID, BT_SCO_FAR_NOISE_LEVEL);
    *sysclk = BT_NLMS_AEC_EN ? SYS_160M : *sysclk;
    #else
    nr_init(((u32)BT_SCO_FAR_NOISE_LEVEL << 16) | BT_SCO_FAR_NOISE_THRESHOID);
    *sysclk = BT_NLMS_AEC_EN? SYS_147M : *sysclk;
    #endif
#endif

#if BT_SCO_DAC_DRC_EN
    nr->nr_cfg_en |= NR_CFG_DAC_DRC_EN;
    bt_sco_dac_drc_init((u8 *)RES_BUF_EQ_CALL_DAC_DRC, RES_LEN_EQ_CALL_DAC_DRC);
#endif

#if BT_SCO_CALLING_NR_EN
    nr->calling_voice_cnt = BT_SCO_CALLING_VOICE_CNT;
    nr->calling_voice_pow = BT_SCO_CALLING_VOICE_POW;
#endif
}

#if BT_SNDP_DMIC_EN || BT_SNDP_DMIC_DNN_EN
void bt_sco_sndp_dm_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->type = NR_CFG_TYPE_DMIC;
#if BT_SNDP_DMIC_DNN_EN
    nr->dmic = &dmic_cb;
	nr->sndp_dmic_type = BT_SNDP_DMIC_DNN_EN;
    memset(&dmic_cb, 0, sizeof(sndp3_dm_cb_t));
    dmic_cb.mic_cfg         = (xcfg_cb.micl_cfg == 1)? 0 : BIT(0);
    dmic_cb.nlms_en         = BT_SNDP_DMIC_DNN_NLMS_EN;             //自适应BF使能
    dmic_cb.distance        = BT_SNDP_DMIC_DNN_DISTANCE;            //双麦间距配置
    dmic_cb.degree          = xcfg_cb.bt_sndp_dmdnn_degree;         //默认30度
    dmic_cb.max_degree      = xcfg_cb.bt_sndp_dmdnn_max_degree;     //默认89度
    dmic_cb.level           = xcfg_cb.bt_sndp_dmdnn_level;          //传统后处理降噪量，默认5
    dmic_cb.dnn_level       = xcfg_cb.bt_sndp_dmdnn_dnn_level;      //DNN最大降噪量，默认13
    dmic_cb.wind_level      = xcfg_cb.bt_sndp_dmdnn_wind_level;     //风噪增益设置，默认3
    dmic_cb.bf_upper        = 128;                                  //固定波束频率上限, 默认128
    dmic_cb.filter_coef     = NULL;
    #ifdef RES_BUF_DMIC_SPAI_BLOCK_BIN
    if(0 != RES_LEN_DMIC_SPAI_BLOCK_BIN){
        dmic_cb.filter_coef = (int*)RES_BUF_DMIC_SPAI_BLOCK_BIN;
    }
    #endif
//    dmic_cb.param_printf    = 1;								    //是否打印双麦参数
    sndp_dm_v03_config(&dmic_cb);
    #if BT_SNDP_DMIC_DNN_NLMS_EN
    *sysclk = SYS_160M;
    #else
    *sysclk = BT_NLMS_AEC_EN ? SYS_160M : SYS_120M;
    #endif
#else
    nr->dmic = &dmic_cb;
    memset(&dmic_cb, 0, sizeof(dmic_cb_t));
    dmic_init_dft(&dmic_cb);
    dmic_cb.mic_cfg      = (xcfg_cb.micl_cfg == 1)? 0 : BIT(0);
    dmic_cb.distance     = xcfg_cb.bt_sndp_dm_distance;         //23mm(默认)
    dmic_cb.degree       = xcfg_cb.bt_sndp_dm_degree;           //30度(默认)
    dmic_cb.max_degree   = xcfg_cb.bt_sndp_dm_max_degree;       //90度
    dmic_cb.level        = xcfg_cb.bt_sndp_dm_level;            //30dB(默认)
    dmic_cb.wind_level   = xcfg_cb.bt_sndp_dm_wind_level;       //风噪降噪量（默认20dB）
    dmic_cb.snr_x0_level = xcfg_cb.bt_sndp_dm_snr_x0_level;     //如果在一般信噪比下，降噪不足，就适当调大，但不要大于8
    dmic_cb.snr_x1_level = xcfg_cb.bt_sndp_dm_snr_x1_level;     //如果低信噪比下，语音断断续续，就适当调小，但不要低于10
    dmic_cb.windEQ_en    = 1;
    //默认关闭以下参数，需要可手动打开
//    dmic_cb.param_printf = 1;									//是否开启双麦参数打印
//    dmic_cb.maxIR        = 131072;
//    dmic_cb.maxIR2       = 91750;
//    dmic_cb.windnoise_conditioned_eq_x0 = 80;                   //80(2500Hz@16kHz, 默认),  风噪抑制起始频段(Hz)
//    dmic_cb.windnoise_conditioned_eq_x1 = 208;                  //208(6500Hz@16kH, 默认),  风噪抑制终止频段(Hz)
//    dmic_cb.windnoise_conditioned_eq_y1 = 1677722;              //1677722(-20dB, 1<<24, 默认), 风噪抑制的终止大小(db), 风噪抑制的起始大小为0db
//    dmic_cb.gc_NoiseRMSdB_TH0           = (80) << (24 - 8);     //set param,set talk mic' noise level
    #ifdef RES_BUF_DMIC_SNDP_BLOCK_BIN
    if(0 != RES_LEN_DMIC_SNDP_BLOCK_BIN){
        dmic_cb.filter_coef = (int*)RES_BUF_DMIC_SNDP_BLOCK_BIN;
        dmic_cb.maxIR       = xcfg_cb.bt_sndp_dm_maxir;
    }
    #endif
    *sysclk = BT_NLMS_AEC_EN ? SYS_160M : SYS_120M;
#endif

    cfg_micr_phase_en = xcfg_cb.micr_phase_en;      //右麦反相

    trumpet_denoise_init(xcfg_cb.bt_trumpet_level);
    nr->trumpet_denoise_en = BT_TRUMPET_DENOISE_EN;
}
#endif

#if BT_SCO_BCNS_EN
void bt_sco_bcns_init(u8 *sysclk, nr_cb_t *nr)
{
	nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_BCNS;
	nr->mode = BT_SCO_BCNS_MODE;
	bcns_init(BT_SCO_BCNS_LEVEL, BT_BCNS_VAD_VALUE, BT_BCNS_ENLARGE);
	*sysclk = *sysclk < SYS_120M ? SYS_120M : *sysclk;
}
#endif

#if BT_SCO_DMNS_EN || BT_SCO_DMDNN_EN
void bt_sco_dmns_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->threshoid = 0;
	nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_DMNS;
	nr->mode = 0;
    nr->dmic = &dmns_cb;
	memset(&dmns_cb, 0, sizeof(dmns_cb_t));
    dmns_cb.mic_cfg         = (xcfg_cb.micl_cfg == 1) ? 0 : BIT(0);
    #if BT_SCO_DMNS_EN
	dmns_cb.bf_type         = BT_SCO_DMNS_BF_TYPE;
	#else
	dmns_cb.dm_dnn_en       = BT_SCO_DMDNN_EN;
	#endif
    dmns_cb.distance        = xcfg_cb.bt_dmns_distance;
	dmns_cb.degree          = xcfg_cb.bt_dmns_degree;
	dmns_cb.nt              = xcfg_cb.bt_dmns_level;
    dmns_cb.nt2             = xcfg_cb.bt_dmns_bf_level;
	dmns_cb.param_printf    = 0;
	if(dmns_cb.dm_dnn_en) {
        //dmdnn自研ai双麦参数
    	dmns_cb.noise_ps_rate           = 1;
        dmns_cb.low_fre_ns0_range		= 1;//range:1-36
        dmns_cb.comp_mic_fre_L          = 0;
        dmns_cb.comp_mic_fre_H          = 255;
        dmns_cb.prior_opt_idx			= 10;
        dmns_cb.prior_opt_ada_en		= 1;
        dmns_cb.tf_en					= 1;
        dmns_cb.phase_comp_en 		    = 0;
        dmns_cb.phase_comp_en_B         = 0;
        dmns_cb.low_bf_lim              = 8;
        dmns_cb.tf_len				    = 140; //smart: range:1-140   zoom:1:256 默认设能设的最大
        dmns_cb.tf_norm_en			    = 1;
        dmns_cb.tf_norm_only_en		    = 0;
        dmns_cb.opt_limit				= 32767;
        dmns_cb.bf_choose				= 1;
        dmns_cb.BaseGain                = 2024;
        dmns_cb.white_approach_shape	= 1;
        dmns_cb.wind_sig_choose		    = 1;//0时代表副mic受风小  1代表主mic受风小
        dmns_cb.wind_state_lim          = 25000;
        dmns_cb.wind_sup_floor          = 23000;
        dmns_cb.wind_sup_open           = 1;
        dmns_cb.wind_a_pow_en			= 0;

        dmns_cb.exp_range_H			    = 0;
        dmns_cb.exp_range_L			    = 0;
        dmns_cb.music_lev			    = 11;
        dmns_cb.gain_expand			    = 1024;
        dmns_cb.nn_only				    = 0;
        dmns_cb.nn_only_len			    = 16;
        dmns_cb.hi_fre_en 			    = 1;
        dmns_cb.gain_assign			    = 16666;
        dmns_cb.sin_gain_post_en		= 0;
        dmns_cb.sin_gain_post_len_f     = 140;
        dmns_cb.b_frein2_floor		    = 19666;
        dmns_cb.enlarge_v				= 1;//1 or 2 or 3

        dmns_cb.Gcoh_thr				= 13666;
        dmns_cb.Gcoh_sum_thr			= 9666;
        dmns_cb.wind_or_en			    = 1;
        dmns_cb.v_white_en			    = 1;
        dmns_cb.while_scale             = 1000;
        dmns_cb.mask_floor_hi			= 140;
        dmns_cb.mask_floor			    = 1000;
        dmns_cb.bfrein_en				= 0;
        dmns_cb.gu_value				= 1000;
        dmns_cb.bfrein3_en              = 0;
        dmns_cb.bfrein3_intensity       = 524;  //噪声估计强度，q10越大去噪越强最大不超过u16
        dmns_cb.bfrein3_ns		        = 4;
        dmns_cb.sp_thres				= 6000;
        dmns_cb.mic1_gain_idx           = 10;
        dmns_cb.mic2_gain_idx           = 10;
        dmns_cb.white_type_len		    = 0;
        dmns_cb.rtf_post_thres		    = 2048;
	} else {
	    //dmns自研传统双麦参数
//      dmns_cb.lowside_corr            = 0;
//      dmns_cb.fast_convergence_en     = 0;
        dmns_cb.filter_constraint       = 6;
//      dmns_cb.cmp_tbl_dis             = 0;
        dmns_cb.noise_ps_rate           = 8;
//      dmns_cb.comp_mic_fre_L          = 0;
//      dmns_cb.comp_mic_fre_H          = 0;
        dmns_cb.prior_opt_idx			= 6;
//      dmns_cb.wind_sig_choose         = 0;
	}

    nr->trumpet_denoise_en  = BT_TRUMPET_DENOISE_EN;

    trumpet_denoise_init(xcfg_cb.bt_trumpet_level);

    cfg_micr_phase_en = xcfg_cb.micr_phase_en;      //右麦反相
#if BT_SCO_DMDNN_EN && BT_AEC_EN
    *sysclk = SYS_120M;
#elif (BT_SCO_DMDNN_EN && BT_NLMS_FRE_EN)
    *sysclk = SYS_160M;
#else
    *sysclk = (BT_NLMS_FRE_EN & BT_SCO_FAR_NR_EN) ? SYS_160M : SYS_120M;
#endif
}
#endif

#if BT_SCO_LDMDNN_EN
void bt_sco_ldmdnn_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->threshoid = 0;
	nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_LDMDNN;
	nr->mode = 0;
    nr->dmic = &ldmdnn_cb;
	memset(&ldmdnn_cb, 0, sizeof(ldmdnn_cb_t));
    ldmdnn_cb.mic_cfg         = (xcfg_cb.micl_cfg == 1) ? 0 : BIT(0);
    ldmdnn_cb.distance        = xcfg_cb.bt_dmns_distance;
	ldmdnn_cb.degree          = xcfg_cb.bt_dmns_degree;
	ldmdnn_cb.nt              = xcfg_cb.bt_dmns_level;
    ldmdnn_cb.param_printf           = 0;

	ldmdnn_cb.noise_ps_rate          = 1;
	ldmdnn_cb.low_fre_ns0_range		 = 16;//range:1-36
	ldmdnn_cb.comp_mic_fre_L         = 0;
	ldmdnn_cb.comp_mic_fre_H         = 256;
	ldmdnn_cb.prior_opt_idx			 = 3;
	ldmdnn_cb.tf_en					 = 1;

	ldmdnn_cb.tf_len				 = 256; //smart: range:1-140   zoom:1:256 默认设能设的最大
	ldmdnn_cb.tf_norm_en			 = 0;
	ldmdnn_cb.tf_norm_only_en		 = 0;
	ldmdnn_cb.bf_choose				 = 1;

	ldmdnn_cb.wind_sig_choose		 = 1;//0时代表副mic受风小  1代表主mic受风小
	ldmdnn_cb.wind_state_lim         = 25000;
	ldmdnn_cb.wind_sup_floor         = 23000;
	ldmdnn_cb.wind_sup_open          = 1;
	ldmdnn_cb.wind_a_pow_en			 = 0;
	ldmdnn_cb.wind_len_used          = 36;  //要小于 WIND_LEN 36
	ldmdnn_cb.Ftrust                 = 64;   //用前多少个点来确定入射角 不能超过FTRUST
	ldmdnn_cb.exp_range_H			 = 0;
	ldmdnn_cb.exp_range_L			 = 0;
	ldmdnn_cb.music_lev			     = 11;
	ldmdnn_cb.gain_expand			 = 1024;
	ldmdnn_cb.nn_only				 = 0;
	ldmdnn_cb.nn_only_len			 = 16;
	ldmdnn_cb.hi_fre_en 			 = 1;
	ldmdnn_cb.gain_assign			 = 16666;
	ldmdnn_cb.sin_gain_post_en		 = 1;
	ldmdnn_cb.sin_gain_post_len_f    = 160;
	ldmdnn_cb.enlarge_v				 = 1;//1 or 2 or 3
	ldmdnn_cb.mask_floor			 = 1600;
	ldmdnn_cb.mask_floor_hi		     = 160;

	ldmdnn_cb.Gcoh_thr				 = 13666;
	ldmdnn_cb.Gcoh_sum_thr			 = 9666;
	ldmdnn_cb.wind_or_en			 = 1;
	ldmdnn_cb.gu_value				 = 0;  //0是完全不泄露抑制，如果两个麦克风一致性一般的时候泄露一致一般效果也不好

	nr->trumpet_denoise_en  = BT_TRUMPET_NR_EN;
    trumpet_denoise_init(BT_TRUMPET_NR_LEVEL);

    *sysclk = SYS_160M;
    cfg_micr_phase_en = xcfg_cb.micr_phase_en;
}
#endif

#if BT_SCO_AINS2_EN
void bt_sco_ains2_init(u8 *sysclk, nr_cb_t *nr)
{
    u8 trumpet = (xcfg_cb.bt_trumpet_level << 4) | BT_TRUMPET_DENOISE_EN;
	nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_AINS2;
	nr->mode = BT_SCO_AINS2_MODE;
	ains2_init(BT_SCO_AINS2_LEVEL, 0, trumpet);
	*sysclk = *sysclk < SYS_80M ? SYS_80M : *sysclk;
}
#endif

#if BT_SCO_AINS3_EN
void bt_sco_ains3_init(u8 *sysclk, nr_cb_t *nr)
{
	nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_AINS3;
	nr->mode = BT_SCO_AINS3_MODE;
    nr->dmic = &ains3_cb;
	memset(&ains3_cb, 0, sizeof(ains3_cb_t));
	ains3_cb.nt = BT_SCO_AINS3_LEVEL;
	ains3_cb.prior_opt_idx =BT_SCO_AINS3_PRIOR_OPT;
	*sysclk = SYS_120M;
}
#endif

#if BT_SNDP_EN
void bt_sco_sndp_sm_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_SNDP;
	nr->mode = BT_SNDP_MODE;
    #if BT_SNDP_TYPE
    nr->dnn_level = 5;                                      //10dB(默认)，DNN稳态降噪量
    nr->max_level = xcfg_cb.bt_sndp_dnn_max_level;          //24dB(默认)，DNN最大降噪量
    nr->min_level = xcfg_cb.bt_sndp_dnn_min_level;          //12dB(默认)，DNN最小降噪量
    #else
    nr->level = xcfg_cb.bt_sndp_level;                      //20dB(默认)，RNN稳态降噪量
    #endif
    nr->sndp_smic_type = BT_SNDP_TYPE;
#if BT_NEAR_AINS2_EN
    nr->near_ains2_en = 1;
    near_ains2_init(BT_NEAR_AINS2_NOISE_LEVEL,0,0);
    *sysclk =  BT_NLMS_AEC_EN ? SYS_147M : SYS_120M;
#else
    if (nr->sndp_smic_type) {
        *sysclk = SYS_120M;
    } else {
        *sysclk =  BT_NLMS_AEC_EN ? SYS_120M : SYS_60M;
    }
#endif

#if BT_TRUMPET_DENOISE_EN
    trumpet_denoise_init(xcfg_cb.bt_trumpet_level);
    nr->trumpet_denoise_en = 1;
    *sysclk = *sysclk < SYS_80M ? SYS_80M : *sysclk;
#endif
}
#endif

void bt_sco_near_nr_dft_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->type = NR_CFG_TYPE_DEFUALT;
    nr->mode = NR_CFG_MODE0;
    nr->level = 18;
    nr->threshoid = BT_NOISE_THRESHOID;
#if BT_SCO_ANS_EN
    nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_ANS;
#endif
}


void bt_sco_comfort_noise_init(u8 *sysclk, nr_cb_t *nr)
{
#if BT_SCO_COMFORT_NOISE_EN
    nr->comforN_en      = BT_SCO_COMFORT_NOISE_EN;
    nr->seed            = 666;
    nr->comforN_level   = 4;
    nr->comforN_floor   = 900;
#endif
}

void bt_sco_agc_init(u8 *sysclk, nr_cb_t *nr)
{
#if BT_SCO_AGC_EN
    nr->agc_cb.agc_en   = BT_SCO_AGC_EN;

    if (bt_sco_is_msbc()) {
        nr->agc_cb.sampleHzIn = 16000;
    } else {
        nr->agc_cb.sampleHzIn = 8000;
    }

    nr->agc_cb.bit      		= 16;
    nr->agc_cb.compress_agcDb   = 12;
    nr->agc_cb.target_agcDbfs   = 3;
#endif
}

#if BT_SCO_DNN_EN
void bt_sco_dnn_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_DNN;
    nr->dmic = &dnn_cb;
    memset(&dnn_cb, 0, sizeof(dnn_cb_t));
//    dnn_cb.nt                   = BT_SCO_DNN_LEVEL;
//    dnn_cb.exp_range_H          = 1;
//	  dnn_cb.exp_range_L          = 0;
//    dnn_cb.param_printf         = 1;
//    dnn_cb.noise_ps_rate        = 1;
//    dnn_cb.prior_opt_idx        = 3;
    dnn_cb.prior_opt_ada_en       = 1;
//    dnn_cb.wind_level           = 0;						//0：不打开风噪
//	  dnn_cb.wind_range           = 0;
	dnn_cb.low_fre_range          = 16;                     //范围0~16
	dnn_cb.low_fre_range0         = 0;
//  dnn_cb.pitch_filter_en        = 1;
    dnn_cb.mask_floor             = 1000;
	dnn_cb.mask_floor_r           = 0;
	dnn_cb.music_lev              = 11;
	dnn_cb.gain_expand            = 1024;
	dnn_cb.nn_only                = 0;
	dnn_cb.nn_only_len            = 16;
	dnn_cb.gain_assign            = 26666;
    dnn_cb.sin_gain_post_en	      = 0;
	dnn_cb.sin_gain_post_len	  = 128;
	dnn_cb.sin_gain_post_len_f    = 256;
	dnn_cb.spp_thr                = 8000;

    *sysclk = *sysclk < SYS_120M ? SYS_120M : *sysclk;
}
#endif

#if BT_SCO_AIAEC_DNN_EN
void bt_sco_aiaec_dnn_init(u8 *sysclk, nr_cb_t *nr)
{
    nr->type = (nr->type & ~NR_CFG_TYPE_MASK) | NR_CFG_TYPE_AIAEC;
    nr->dmic = &aiaec_dnn_cb;
    memset(&aiaec_dnn_cb, 0, sizeof(dnn_aec_ns_cb_t));
//	aiaec_dnn_cb.param_printf           	= 1;
	aiaec_dnn_cb.nt                     	= BT_SCO_AIAEC_DNN_LEVEL;
//	aiaec_dnn_cb.exp_range_H			   	= 1;
//	aiaec_dnn_cb.exp_range_L			   	= 0;
	aiaec_dnn_cb.noise_ps_rate          	= 1;
	aiaec_dnn_cb.prior_opt_idx	       	    = 10;
	aiaec_dnn_cb.prior_opt_ada_en           = 1;
	aiaec_dnn_cb.wind_level			   	    = 0;
	aiaec_dnn_cb.wind_range			    	= 0;
	aiaec_dnn_cb.low_fre_range          	= 15;
	aiaec_dnn_cb.mask_floor			   	    = 600;
	aiaec_dnn_cb.music_lev			   	    = 11;
	aiaec_dnn_cb.nn_only				   	= 0;
	aiaec_dnn_cb.nn_only_len			   	= 16;
	aiaec_dnn_cb.gain_assign                = 16666;
	aiaec_dnn_cb.sin_gain_post_en			= 0;
	aiaec_dnn_cb.sin_gain_post_len		    = 128;
	aiaec_dnn_cb.sin_gain_post_len_f		= 0;

    aiaec_dnn_cb.gamma                      = 27852;
	aiaec_dnn_cb.nlp_en                     = 1;
	aiaec_dnn_cb.nlp_level                  = 3;
    aiaec_dnn_cb.smooth_en				    = 1;
    aiaec_dnn_cb.far_post_thr				= 1024;//Q8
    aiaec_dnn_cb.dtd_post_thr				= 80000000;//Q15
    aiaec_dnn_cb.echo_gain_floor			= 100;
    aiaec_dnn_cb.dtd_smooth				    = 29491;
	aiaec_dnn_cb.single_floor				= 0;
	aiaec_dnn_cb.gain_assign_nlp			= 6000;
	aiaec_dnn_cb.gain_st_thr				= 12000;
	aiaec_dnn_cb.intensity					= 1;
	aiaec_dnn_cb.aec_gain_db                = 6;
	aiaec_dnn_cb.nlms_en                    = 1;
//    nr->aiaec_en                            = 1;

#if BT_TRUMPET_NR_EN
    nr->trumpet_denoise_en        = BT_TRUMPET_NR_EN;
    trumpet_denoise_init(BT_TRUMPET_NR_LEVEL);
#endif

    *sysclk = *sysclk < SYS_120M ? SYS_120M : *sysclk;
}
#endif

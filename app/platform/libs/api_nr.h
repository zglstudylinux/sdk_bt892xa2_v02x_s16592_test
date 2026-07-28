#ifndef _API_NR_H
#define _API_NR_H

#define _MAX_GAIN                            32767
#define SOFT_DIG_P0DB                       (_MAX_GAIN * 1.000000)
#define SOFT_DIG_P1DB                     	(_MAX_GAIN * 1.122018)
#define SOFT_DIG_P2DB                     	(_MAX_GAIN * 1.258925)
#define SOFT_DIG_P3DB                     	(_MAX_GAIN * 1.412538)
#define SOFT_DIG_P4DB                     	(_MAX_GAIN * 1.584893)
#define SOFT_DIG_P5DB                     	(_MAX_GAIN * 1.778279)
#define SOFT_DIG_P6DB                     	(_MAX_GAIN * 1.995262)
#define SOFT_DIG_P7DB                     	(_MAX_GAIN * 2.238721)
#define SOFT_DIG_P8DB                     	(_MAX_GAIN * 2.511886)
#define SOFT_DIG_P9DB                     	(_MAX_GAIN * 2.818383)
#define SOFT_DIG_P10DB                    	(_MAX_GAIN * 3.162278)
#define SOFT_DIG_P11DB                    	(_MAX_GAIN * 3.548134)
#define SOFT_DIG_P12DB                    	(_MAX_GAIN * 3.981072)
#define SOFT_DIG_P13DB                    	(_MAX_GAIN * 4.466836)
#define SOFT_DIG_P14DB                    	(_MAX_GAIN * 5.011872)
#define SOFT_DIG_P15DB                    	(_MAX_GAIN * 5.623413)

#define DUMP_SNDP_MASK                      0x000f
#define DUMP_AEC_MASK                       0x00f0

#define NR_CFG_TYPE_MASK                    0x0f

enum {
    NR_CFG_TYPE_DEFUALT = 0,                    //默认硬件
    NR_CFG_TYPE_ANS,
    NR_CFG_TYPE_AINS2,
    NR_CFG_TYPE_AINS3,
    NR_CFG_TYPE_SNDP,
    NR_CFG_TYPE_BCNS,
    NR_CFG_TYPE_DMNS,
    NR_CFG_TYPE_DNN,

    NR_CFG_TYPE_AIAEC,
    NR_CFG_TYPE_LDMDNN,
    NR_CFG_TYPE_DMIC,

    NR_CFG_TYPE_FAR         = BIT(4),           //远端降噪(ANS)
    NR_CFG_TYPE_FAR_AINS2   = BIT(5),           //远端降噪(AINS2)
    NR_CFG_TYPE_NR_FAR      = BIT(6),           //远端降噪(NR_FAR)
};

enum {
    NR_CFG_MODE0        = 0,                    //defualt
    NR_CFG_MODE1,
    NR_CFG_MODE2,
};

enum {
    NR_CFG_DAC_DNR_EN       = BIT(4),           //使能通话下行————动态降噪
    NR_CFG_DAC_DRC_EN       = BIT(5),           //使能通话下行————DRC
};

enum {
    DUMP_MIC_TALK           = 0,                //主MIC数据
    DUMP_MIC_FF,                                //副MIC数据
    DUMP_MIC_NR,                                //降噪后数据
    DUMP_AEC_INPUT,                             //AEC输入数据
    DUMP_AEC_FAR,                               //AEC远端数据
    DUMP_AEC_OUTPUT,                            //AEC输出数据
    DUMP_FAR_NR_INPUT,                          //远端降噪输入数据
    DUMP_FAR_NR_OUTPUT,                         //远端降噪输出数据
    DUMP_EQ_OUTPUT,                             //MIC EQ输出数据
    DUMP_SCO_MAX_NB,
};

typedef struct {
    u8 mic_cfg;
    u8 distance;                                //双mic间距：mm（步进1mm）
    u8 degree;                                  //语音角度（小于90度）:10+5*N (5度步进，N<17)
    u8 max_degree;                              //最大保护角度（小于90度）:10+5*N (5度步进，N<17)
    u8 level;                                   //降噪强度：2*N（2dB步进，N<30）
    u8 wind_level;
    u8 param_printf;                            //使能参数打印
    u8 rfu[1];
    const int *filter_coef;                     //int coef[512]
    int maxIR;                                  //Q18
    int maxIR2;                                 //Q18
    u16 windnoise_conditioned_eq_x0;            //风噪抑制起始频段(Hz)
    u16 windnoise_conditioned_eq_x1;            //风噪抑制终止频段(Hz)
    int windnoise_conditioned_eq_y1;            //风噪抑制的终止大小(db), 最小为0db
    u8 snr_x0_level;                            //如果在一般信噪比下，降噪不足，就适当调大（0~0.4 步进0.05），默认0.1
    u8 snr_x1_level;                            //如果低信噪比下，语音断断续续，就适当调小（0.5~1 步进0.05），默认0.8
    int gc_NoiseRMSdB_TH0;                      //降噪量控制参数配置，在噪声水平高时，降低双麦的实际降噪量
    u8 windEQ_en;                               //风噪EQ的控制接口
} dmic_cb_t;

typedef struct {
    u8  param_printf;                           //使能参数打印
    u8  nlms_en;                                //自适应BF使能
    u8  mic_cfg;                                //主副麦选择
	u8  distance;                               //双mic间距表
    u8  degree;                                 //语音角度（小于90度）:10+5*N (5度步进，N<17)
    u8  max_degree;                             //最大保护角度（小于90度）:10+5*N (5度步进，N<17)
    u8  level;                                  //降噪量：2*N（2dB步进，N<30）
    u8  dnn_level;                              //DNN最大降噪量
    u8  wind_level;                             //风噪增益设置
    int bf_upper;                               //固定波束频率上限, 默认128
    const int *filter_coef;                     //初始化滤波器系数，int coef[16]
} sndp3_dm_cb_t;

typedef struct {
    u32 nt;
    u8  prior_opt_idx;
} ains3_cb_t;

typedef struct {
    u8  param_printf;                           //使能参数打印
    u8  mic_cfg;                                //主副麦选择
    u8  bf_type;                                //beamforming类型
	s16 distance;				                //双麦间距
	u16 degree;					                //拾音角度方向
	s16 nt;						                //普通降噪量
	s8  nt2;					                //beamforming抑制量
	u8  fast_convergence_en;                    //快速收敛功能，对人声有影响，默认不开
	u16 lowside_corr;                           //下支路相关系数
	u8  filter_constraint;                      //滤波器约束范围，基于差分的beamforming对该参数比较敏感
	u8  trumpet_en;                             //喇叭声抑制
	u8  cmp_tbl_dis;
	u8  dm_dnn_en;                              //是否使能双麦AI算法
	u8  noise_ps_rate;
	u8  comp_mic_fre;                           //双麦+AI的参数
	u16 comp_mic_fre_L;                         //传统双麦的参数
	u16 comp_mic_fre_H;                         //传统双麦的参数
	u8  prior_opt_idx;
	u8  prior_opt_ada_en;
	u16 low_fre_ns0_range;
	u8  wind_sig_choose;
	u8  bf_choose;
	u8  white_approach_shape;
	u8  tf_en;
	u16 tf_len;
	u8  tf_norm_en;
	u8  tf_norm_only_en;
	s32 opt_limit;
	s16 exp_range_H;
	s16 exp_range_L;
	u8	music_lev;
	u16 gain_expand;
	u8  nn_only;
	u16 nn_only_len;
	u8  hi_fre_en;
	u16 gain_assign;
	u8  sin_gain_post_en;
    u16 Gcoh_thr;
	u16 Gcoh_sum_thr;
	u8  v_white_en;
	s32 while_scale;
	u8  wind_or_en;
	u8  phase_comp_en;
	u8  phase_comp_en_B;
	u8  low_bf_lim;
	u16 sin_gain_post_len_f;
	u16 b_frein2_floor;
	u16 mask_floor_hi;
	s16 mask_floor;
	s16 wind_state_lim;
	s16 wind_sup_floor;
	u8  wind_sup_open;
	u8  wind_a_pow_en;
	u8  enlarge_v;
	u8  bfrein_en;
	s32 gu_value;
	u8 bfrein3_en;
	u16 bfrein3_intensity;
	u8  bfrein3_ns;
	s16 sp_thres;
	u8  mic1_gain_idx;
	u8  mic2_gain_idx;
	s16 white_type_len;
	s16 rtf_post_thres;
    s16 BaseGain;
	//s32 high_freq_floor;
} dmns_cb_t;    //dmns和dmdnn共用一套结构体，改结构体需要一起更新，不能随便删除变量

typedef struct {
    u8  param_printf;                           //使能参数打印
    u8  mic_cfg;                                //主副麦选择
    u8  bf_type;                                //beamforming类型
    u16 degree;					                //拾音角度方向
	s16 distance;				                //双麦间距
	s16 nt;						                //普通降噪量
	u8  noise_ps_rate;
	u16 comp_mic_fre_L;                         //传统双麦的参数
	u16 comp_mic_fre_H;                         //传统双麦的参数
	u8  prior_opt_idx;
	u16 low_fre_ns0_range;
	u8  wind_sig_choose;
	s16 wind_state_lim;
	s16 wind_sup_floor;
	u8  wind_sup_open;
	u8  wind_a_pow_en;
	u8  bf_choose;
	u8  tf_en;
	u16 tf_len;
	u8  tf_norm_en;
	u8  tf_norm_only_en;
	s16 exp_range_H;
	s16 exp_range_L;
	u8	music_lev;
	u16 gain_expand;
	u8  nn_only;
	u16 nn_only_len;
	u8  hi_fre_en;
	u16 gain_assign;
	u8  sin_gain_post_en;
    u16 Gcoh_thr;
	u16 Gcoh_sum_thr;
	u8  wind_or_en;
	s16 sin_gain_post_len_f;
	u8  enlarge_v;
	s16 mask_floor;
    u16 mask_floor_hi;
	s32 gu_value;
	u16 wind_len_used;
	u16 Ftrust;
} ldmdnn_cb_t;

typedef struct {
	u16 nt;
	s16 exp_range_H;
	s16 exp_range_L;
	u8  model_select;
	u16 min_value;
	u16 nostation_floor;
	u8  wind_thr;
	u8  wind_en;
	u8  noise_ps_rate;
	u8  prior_opt_idx;
	u8  prior_opt_ada_en;
    u8  param_printf;                           //使能参数打印
    u8  wind_level;
	u16 wind_range;
	u16 low_fre_range;
	u16 low_fre_range0;
	u8  pitch_filter_en;
	u16 mask_floor;
	u16 noise_ceil;
	//u16 ps_lowlimt;
	u8  mask_floor_r;
	u8  music_lev;
	u16 gain_expand;
	u8  nn_only;
	u16 nn_only_len;
	u16 gain_assign;
	u8  sin_gain_post_en;
	u16 sin_gain_post_len;
	u16 sin_gain_post_len_f;
	s16 spp_thr;
	//u16 pitch_filter_range;
} dnn_cb_t;

typedef struct {
	s16	gamma;
	u8  nlp_en;
	u8  nlp_level;

	s16 nt;
	u8 param_printf;
	u8  music_lev;
	//s16 exp_range_H;
	//s16 exp_range_L;
	u8  noise_ps_rate;
	u8  prior_opt_idx;
	u8  prior_opt_ada_en;
	u8  wind_level;
	u16 wind_range;
	u16 mask_floor;
	u16 low_fre_range;
	u8  nn_only;
	u16 nn_only_len;
	u8  sin_gain_post_en;
	u16 sin_gain_post_len;
	u16 sin_gain_post_len_f;
	u8  smooth_en;
	u16 far_post_thr;
	u32 dtd_post_thr;
	u16 gain_assign;
	u16 echo_gain_floor;
	s16 dtd_smooth;
	s16 single_floor;
	s16 gain_assign_nlp;
	u16 gain_st_thr;
	s32 intensity;
	s16 aec_gain_db;
	u8  nlms_en;
} dnn_aec_ns_cb_t;

typedef struct {
    u8  agc_en;
    u16 sampleHzIn;
    u8  bit;
    u8  compress_agcDb;
    u8  target_agcDbfs;
} agc_cb_t;

typedef struct {
    u8 type;
    u8 mode;
    u16 level;                                  //降噪强度
    u16 threshoid;                              //降噪阈值
    u16 min_level;                              //DNN最小降噪量
    u16 max_level;                              //DNN最大降噪量
    void *dmic;                                 //双麦结构体指针统一改为降噪算法的初始化结构体指针
    u16 dump_en;
    u8 rfu[2];
    u8 calling_voice_pow;                       //用于呼出电话，响铃之前动态降噪
    u8 calling_voice_cnt;
    u8 calling_voice_temp_cnt;
    u8 near_ains2_en;
    u8 trumpet_denoise_en;
    u8 sndp_smic_type;
    u8 sco_fade_en;
    u8 sndp_dmic_type;
	u8 model_set;
	u8 comforN_en;
	u32 seed;
    u8 comforN_level;
    u16 comforN_floor;
    u8 dnn_level;                               //DNN稳态降噪量
    u8 nr_cfg_en;
    agc_cb_t agc_cb;
} nr_cb_t;

typedef struct {
    u8 aec_en;
    u8 rfu[1];
    u8 echo_level;
    u8 far_offset;
    u8 far_pass_nr;
    u8 nlms_aec_en;
    u8 nlms_nlp_mode;
    u8 single_en;
    u32 threshold;
	u8 ref_set;
	s16 UpBin;
	s16 LowBin;
	int qIdx;
	s16 echo_th;
	int gamma_sm;
	int arrgFact;
	int xd_avg_th;
    u32 aec_dig_gain;
} aec_cb_t;

typedef struct {
    u8 alc_en;
    u8 rfu[1];
    u8 fade_in_step;
    u8 fade_out_step;
    u8 fade_in_delay;
    u8 fade_out_delay;
    s32 far_voice_thr;
} alc_cb_t;

typedef struct{
	u8  outsat_en;
	s32 coef_ptr[8][5];
	s32 zx[8+1][2];
} soft_eq_t;

typedef struct {
    aec_cb_t aec;
    alc_cb_t alc;
    nr_cb_t nr;

    u8 rfu[3];
    u8 mic_eq_en        : 1;
    u8 mic_drc_en       : 1;
} bt_voice_cfg_t;

typedef struct {
    s16 *input;
    s16 *output;
    u8 iptr;
    u8 optr;
} agc_rb_t;

typedef s32 (*aiaec_kick_func_t)(s32 *xfw, s32 *dfw, s32 *efw);

void aec_init_dft(aec_cb_t *aec);
void alc_init_dft(alc_cb_t *alc);
void dmic_init_dft(dmic_cb_t *dmic);

void sco_huart_init(void);
bool sco_huart_putcs(u8 type, u8 frame_num, const void *buf, uint len);

void bt_voice_init(bt_voice_cfg_t *p);
void bt_voice_exit(void);
void far_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min);
void nr_far_init(u16 noise_thr, u16 nr_level);
void nr_init(u32 nt);
void ains2_init(u16 nt, u16 noise, u8 trumpet);
void ains2_process(s16 *data);
void bt_ains2_plus_process(s16 *buf);
void bt_ains3_init(ains3_cb_t *ains3_cb);
void ains3_process(s16 *data);
void bt_ains3_plus_process(s16 *buf);
void bcns_init(u32 nt, u32 vad_threshold, u8 enlarge);
void bt_bcns_plus_process(s16 *buf);
void bt_dmns_init(dmns_cb_t *dmns_cb);
void bt_dmns_plus_process(s16 *buf);
void bt_ldmdnn_init(ldmdnn_cb_t *ldmdnn_cb);
void bt_ldmdnn_plus_process(s16 *buf);
void bt_sndp_process(s16 *buf);
void bt_sndp_dm_process(s16 *buf);
bool mic_drc_init(u8 *drc_addr, int drc_len);
void bt_dnn_process(s16 *buf);
void bt_aiaec_dnn_process(s16 *buf);
void sndp_dm_v03_config(sndp3_dm_cb_t *dm_cb);

void bt_agc_init(void);
void bt_sco_agc_proc(s16 *buf);

void near_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min);
void trumpet_denoise_init(u8 level);

bool sdadcl_set_soft_eq_by_res(u32 addr, u32 len);
bool sdadcr_set_soft_eq_by_res(u32 addr, u32 len);

bool bt_sco_dac_drc_init(u8 *drc_addr, int drc_len);

#endif

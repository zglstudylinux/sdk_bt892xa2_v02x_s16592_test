#ifndef _API_MUSIC_AUDIO_H
#define _API_MUSIC_AUDIO_H

/*
空间音频接口
注意：
1、开启空间音频，系统时钟需要切到至少48M
2、目前的空间音频只支持TWS模式的
3、spatial_audio_update_angle更新角度接口的说明：
    （1）更新角度的频率建议不要超过50Hz
    （2）参数说明：in_azimuth：方位角（范围：[0, 360)），in_elevation：俯仰角（范围：[0]）
    （3）方位角in_azimuth的0度定义为正前方，按照逆时针方向递增，即90度为正左，180度为正后方，270度为正右
*/
void spatial_audio_init(void);                                          //空间音频初始化
void spatial_audio_update_angle(short in_azimuth, short in_elevation);  //更新角度，角度制表示，详见上面的注意事项
void spatial_audio_set_state(u8 enable);                                //设置空间音频状态 0：关闭空间音频，1：开启空间音频
u8 spatial_audio_get_state(void);                                       //获取空间音频状态


/*
音乐音效接口
注意：
1、开启虚拟3D音效，系统时钟需要切到至少60M
2、支持TWS和非TWS模式，注意TWS模式需要使用TWS_USER_KEY去同步音效的状态
3、需要在config.h文件开启对应音效的宏
4、初始化接口的BIT定义：
    BIT(0) -> V3D
    BIT(1) -> VBASS
    BIT(4) -> DBB
    BIT(5) -> DYEQ
*/
enum music_audio_state_t {
    MUSIC_AUDIO_NONE,           //无
    MUSIC_AUDIO_V3D = 2,        //虚拟3D音效模式
    MUSIC_AUDIO_VBASS,          //虚拟低音音效
    MUSIC_AUDIO_DBB = 6,        //动态低音音效
    MUSIC_AUDIO_DYEQ,           //动态EQ音效
};

typedef struct {
    u16 fade_step;
    volatile s8 fade_dir;
    u16 gain;
    volatile u16 target_gain;
} soft_vol_t;

typedef struct {
    u8 vol_level;
    u8 bass_level;
    u16 vol;
    u8 vol_direct_set;
    u8 sample_rate;
} dbb_param_cb_t;

void bt_music_audio_init(u8 audio_type);                                //音乐音效初始化，参数详见上面的注意事项
bool bt_music_audio_set_state(u8 music_audio);                          //设置当前的音效，参数详见music_audio_state_t，返回设置是否成功
u8 bt_music_audio_get_state(void);                                      //获取当前的音效，参数详见music_audio_state_t

///软件音量
void soft_vol_process_mono_one_sample(soft_vol_t* p, s16* input);
void soft_vol_process_stereo_one_sample(soft_vol_t* p, s16* inputl, s16* inputr);
void soft_vol_set_vol_param(soft_vol_t* p, u16 vol, u8 vol_direct_set);
void soft_vol_init(soft_vol_t* p);

///动态EQ
void dynamic_eq_process(void *cb, s32 *samples);
void dyeq_coef_update(void *cb, u8 *buf);
void dyeq_clear_cache(void *cb);
s16 dyeq_drc_v3_calc(s32 sample, void *drc_cb);
bool dyeq_drc_v3_init(const void *bin, int bin_size, void *drc_cb);
bool dyeq_drc_v3_set_param(void *buf, void *drc_cb);

#endif // _API_MUSIC_AUDIO_H

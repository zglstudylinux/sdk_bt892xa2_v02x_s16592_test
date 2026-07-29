/*****************************************************************************
 * Module    : Config
 * File      : config.h
 * Function  : SDK配置文件
 *****************************************************************************/

#ifndef USER_CONFIG_H
#define USER_CONFIG_H
#include "config_define.h"

/*****************************************************************************
 * Module    : Function选择相关配置
 *****************************************************************************/
#define FUNC_MUSIC_EN                   1           //是否打开MUSIC功能
#define FUNC_CLOCK_EN                   1           //是否打开时钟功能
#define FUNC_BT_EN                      1           //是否打开蓝牙功能
#define FUNC_BTHID_EN                   0           //是否打开独立自拍器模式
#define FUNC_BT_DUT_EN                  0   //是否打开蓝牙的独立DUT测试模式
#define FUNC_BT_FCC_EN                  0   //是否打开蓝牙的独立FCC测试模式
#define FUNC_AUX_EN                     1           //是否打开AUX功能
#define FUNC_USBDEV_EN                  1           //是否打开USB DEVICE功能
#define FUNC_SPEAKER_EN                 1           //是否打开Speaker模式
#define FUNC_SPDIF_EN                   0           //是否打开SPDIF功能
#define FUNC_FMAM_FREQ_EN               0           //是否打开FMAM读频率显示功能
#define FUNC_IDLE_EN                    0           //是否打开IDLE功能
#define FUNC_I2S_EN                     0           //是否打开I2S功能

/*****************************************************************************
 * Module    : 系统功能选择配置
 *****************************************************************************/
#define SYS_CLK_SEL                     SYS_24M                 //选择系统时钟
#define BUCK_MODE_EN                    xcfg_cb.buck_mode_en    //是否BUCK MODE
#define POWKEY_10S_RESET                xcfg_cb.powkey_10s_reset
#define SOFT_POWER_ON_OFF               1                       //是否使用软开关机功能
#define PWRKEY_2_HW_PWRON               0                       //用PWRKEY模拟硬开关
#define USB_SD_UPDATE_EN                1                       //是否支持UDISK/SD的离线升级
#define SYS_ADJ_DIGVOL_EN               0                       //系统是否调数字音量
#define GUI_SELECT                      GUI_NO                  //GUI Display Select
#define UART0_PRINTF_SEL                PRINTF_PB3              //选择UART打印信息输出IO，或关闭打印信息输出

/*****************************************************************************
 * Module    : FLASH配置
 *****************************************************************************/
#define FLASH_SIZE                      FSIZE_1M                //LQFP48芯片内置1MB，其它封装芯片内置512KB(实际导出prd文件要小于492K)
#define FLASH_CODE_SIZE                 768K                    //程序使用空间大小
#define FLASH_ERASE_4K                  1                       //是否支持4K擦除
#define FLASH_DUAL_READ                 1                       //是否支持2线模式
#define FLASH_QUAD_READ                 0                       //是否支持4线模式
#define SPIFLASH_SPEED_UP_EN            1                       //SPI FLASH提速。

/*****************************************************************************
 * Module    : 音乐功能配置
 *****************************************************************************/
#define MUSIC_UDISK_EN                  1   //是否支持播放UDISK
#define MUSIC_SDCARD_EN                 1   //是否支持播放SDCARD  [TF_TEST] 启用以编入 sd_gpio_init()
#define MUSIC_SDCARD1_EN                0   //是否支持播放双卡

#define MUSIC_WAV_SUPPORT               1   //是否支持WAV格式解码
#define MUSIC_WMA_SUPPORT               0   //是否支持WMA格式解码
#define MUSIC_APE_SUPPORT               0   //是否支持APE格式解码
#define MUSIC_FLAC_SUPPORT              0   //是否支持FLAC格式解码
#define MUSIC_SBC_SUPPORT               0   //是否支持SBC格式解码(SD/UDISK的SBC歌曲, 此宏不影响蓝牙音乐)
#define MUSIC_AAC_SUPPORT               0   //仅用于AAC解码测试

#define MUSIC_FOLDER_SELECT_EN          1   //文件夹选择功能
#define MUSIC_AUTO_SWITCH_DEVICE        1   //双设备循环播放
#define MUSIC_BREAKPOINT_EN             0   //音乐断点记忆播放
#define MUSIC_QSKIP_EN                  1   //快进快退功能
#define MUSIC_PLAYMODE_NUM              4   //音乐播放模式总数
#define MUSIC_MODE_RETURN               0   //退出音乐模式之后是否返回原来的模式
#define MUSIC_PLAYDEV_BOX_EN            1   //是否显示“USB”, "SD"界面
#define MUSIC_ID3_TAG_EN                0   //是否获取MP3 ID3信息
#define MUSIC_REC_FILE_FILTER           0   //是否区分录音文件与非录音文件分别播放
#define MUSIC_LRC_EN                    0   //是否支持歌词显示
#define MUSIC_NAVIGATION_EN             0   //音乐文件导航功能(LCD点阵屏功能)
#define MUSIC_ENCRYPT_EN                0   //是否支持加密MP3文件播放(使用MusicEncrypt.exe工具进行MP3加密)
#define MUSIC_SD_LOOPBACK_EN            0   //SD卡音乐循环功能

#define MUSIC_ENCRYPT_KEY               12345   //MusicEncrypt.exe工具上填的加密KEY

#define IPHONE_POWER_VAL                50  //苹果充电电流设置
#define IPHONE_POWER_INDEX              190 //苹果充电电流设置

/*****************************************************************************
 * Module    : 蓝牙功能配置
 *****************************************************************************/
#define BT_BACKSTAGE_EN                 0   //蓝牙后台管理（全模式使用蓝牙，暂不支持BLE后台）
#define BT_BACKSTAGE_PLAY_DETECT_EN     0   //非蓝牙模式下检测到手机蓝牙播放音乐，则切换到蓝牙模式
#define BT_NAME_DEFAULT                 "BT-BOX"     //默认蓝牙名称（不超过31个字符）
#define BT_NAME_WITH_ADDR_EN            0   //蓝牙名称是否附加地址信息（调试用，例如：btbox-***）
#define BT_LINK_INFO_PAGE1_EN           0   //是否使用PAGE1回连信息（打开后可以最多保存8个回连信息）
#define BT_POWER_UP_RECONNECT_TIMES     3   //上电回连次数
#define BT_TIME_OUT_RECONNECT_TIMES     20  //掉线回连次数
#define BT_SIMPLE_PAIR_EN               1   //是否打开蓝牙简易配对功能（关闭时需要手机端输入PIN码）
#define BT_DISCOVER_CTRL_EN             0   //是否使用按键打开可被发现（需自行添加配对键处理才能被连接配对）
#define BT_PWRKEY_5S_DISCOVER_EN        1   //是否使用长按5S开机进入可被发现(耳机长按开机功能)
#define BT_DISCOVER_TIMEOUT             100 //按键打开可被发现后，多久后仍无连接自动关闭，0不自动关闭，单位100ms
#define BT_ANTI_LOST_EN                 0   //是否打开蓝牙防丢报警
#define BT_DUT_MODE_EN                  0   //正常连接模式，是否使能DUT测试
#define BT_BQB_RF_EN                    0   //BR/EDR DUT测试模式，为方便测试不自动回连
#define BT_FCC_TEST_EN                  0   //蓝牙FCC测试使能   //默认PB3 波特率1500000通信
#define BT_LOCAL_ADDR                   0   //蓝牙是否使用本地地址，0使用配置工具地址
#define BT_LOW_LATENCY_EN               1   //是否打开蓝牙低延时切换功能

#define BT_2ACL_EN                      1   //是否支持连接两部手机
#define BT_2ACL_AUTO_SWITCH             0   //连接两部手机时是否支持点击播放切换到对应的手机
#define BT_2ACL_FORCE_SWITCH            0   //连接两部手机时是否支持点击播放切换到对应的手机（强制切换，需先开启BT_2ACL_AUTO_SWITCH）
#define BT_2ACL_PAIR_ONLY_ODEV          0   //支持连接两部手机配对时仅支持连接一部手机
#define BT_A2DP_EN                      1   //是否打开蓝牙音乐服务
#define BT_HFP_EN                       1   //是否打开蓝牙通话服务
#define BT_HSP_EN                       0   //是否打开蓝牙HSP通话服务
#define BT_PBAP_EN                      0   //是否打开蓝牙PBAP电话本服务
#define BT_MAP_EN                       0   //是否打开蓝牙短信服务(用于获取设备时间，支持IOS/Android)
#define BT_HFP_TIME_EN                  0   //是否使用HFP服务获取设备时间(仅支持IOS，可与BT_MAP_EN同时使用)
#define BT_SPP_EN                       1   //是否打开蓝牙串口服务
#define BT_HCI_DIS_ONLY_SPP_EN 			0	//是否支持当只有spp协议时断开hci连接，兼容鸿蒙断开蓝牙时没有完全断开问题
#define BT_HID_EN                       0   //是否打开蓝牙HID服务
#define BT_ATT_EN                       0   //是否打开GATT_OVER_BREDR
#define BT_HID_DOUYIN_EN                0   //是否打开刷抖音功能(对应BT_HID_TYPE = 4,配置文件要开启拍照功能)
#define BT_HID_TYPE                     0   //选择HID服务类型: 0=自拍器(VOL+, 部分Android不能拍照), 1=自拍器(VOL+和ENTER, 影响IOS键盘使用), 2=游戏手柄, 3=custom defines(keyboard, mouse, custom) 4=抖音
#define BT_HID_MANU_EN                  1   //蓝牙HID是否需要手动连接/断开
#define BT_HID_DISCON_DEFAULT_EN        0   //蓝牙HID服务默认不连接，需要手动进行连接。
#define BT_HFP_CALL_PRIVATE_SWITCH_EN   1   //是否打开按键切换私密接听与蓝牙接听功能
#define BT_HFP_CALL_PRIVATE_FORCE_EN    0   //是否强制使用私密接听（仅在手机接听，不通过蓝牙外放）
#define BT_HFP_RING_NUMBER_EN           1   //是否支持来电报号
#define BT_HFP_INBAND_RING_EN           0   //是否支持手机来电铃声（部分android不支持，默认用本地RING提示音）
#define BT_HFP_BAT_REPORT_EN            1   //是否支持电量显示
#define BT_HFP_MSBC_EN                  1   //是否打开宽带语音功能
#define BT_HFP_1T2_CALL_HOLD_EN         0   //是否打开一拖二通话hold功能
#define BT_A2DP_AAC_AUDIO_EN            1   //是否支持蓝牙AAC音频格式
#define BT_A2DP_SUPTO_RESTORE_PLAY_EN   1   //是否支持蓝牙超距回连恢复播放
#define BT_A2DP_EXCEPT_RESTORE_PLAY_EN  0   //是否支持异常复位后回连恢复播放
#define BT_A2DP_VOL_CTRL_EN             1   //是否支持A2DP音量与手机同步
#define BT_A2DP_VOL_HID_CTRL_EN         0   //是否打开HID调A2DP音量与手机同步功能，同时需打开HID拍照功能，音量同步功能
#define BT_A2DP_VOL_CTRL_WITHOUT_KEY    0   //打开音量同步功能并且耳机没有音量键，可打开该功能，用于兼容不支持音量同步的手机
#define BT_A2DP_FIRST_CON_RESTORE_VOL   0   //是否支持首次连接不支持同步音量的手机恢复到默认音量
#define BT_A2DP_RECORD_DEVICE_VOL       0   //是否支持分别记录不同连接设备的音量，使用设备时恢复当前设备音量
#define BT_A2DP_AVRCP_PLAY_STATUS_EN    0   //是否支持手机播放状态同步，可加快播放暂停响应速度
#define BT_A2DP_RECON_EN                0   //是否支持A2DP控制键（播放/暂停、上下曲键）回连
#define BT_AVDTP_DELAY_REPORT_EN        1   //是否支持AVDTP1.3 delay report，上报耳机当前延迟给手机进行音视频同步
#define BT_AVDTP_DYN_LATENCY_EN         0   //是否支持根据信号环境动态调整延迟
#define BT_SCO_DBG_EN                   1   //是否打开无线调试通话参数功能
#define BT_ID3_TAG_EN                   0   //是否获取蓝牙ID3信息

#define BT_TWS_EN                       1   //是否支持TWS
#define BT_TWS_SCO_EN                   1   //是否支持TWS双路通话
#define BT_TWS_PAIR_MODE                0   //0=通过蓝牙名字配对，1=通过ID配对
#define BT_TWS_PAIR_ID                  0x38393230
#define BT_TWS_PAIR_BONDING_EN          0   //是否支持TWS组队绑定，调用bt_tws_delete_link_info()删除配对信息可解除绑定
#define BT_TWS_MS_SWITCH_EN             0   //是否支TWS主从切换
#define BT_TWS_NOT_AUTO_CONNECT_EN      0   //手机端断开后，TWS单耳出入仓是否主动回连
#define BT_TWS_DBG_EN                   0   //是否支持BT-Assistant工具分析信号质量，需要打开BT_SPP_EN
#define BT_TWS_PUBLIC_ADDR_EN           0   //是否支持TWS配对后生成新地址连接手机（0：使用主耳地址，1：使用新地址，注意0、1程序不兼容不能相互配对）
#define BT_TWS_SLEEP_LED_SYNC_EN        0   //是否打开休眠模式下led灯同步闪烁功能

//蓝牙音乐音效
#define BT_MUSIC_EFFECT_DBG_EN          0   //蓝牙音乐音效在线调试使能
#define BT_MUSIC_EFFECT_ABT_EN          0   //是否使用abt文件获取音效参数，abt文件由在线调试工具生成，需要自行替换abt文件或在setting中选择对应算法的资源文件
#define BT_V3D_AUDIO_EN                 0   //是否打开虚拟3D音效
#define BT_VBASS_AUDIO_EN               0   //是否打开虚拟低音音效
#define BT_DYNAMIC_BASS_BOOST_EN        0   //是否打开动态低音音效
#define BT_DYNAMIC_EQ_EN                0   //是否打开动态EQ音效

//通话参数
#define BT_PLC_EN                       1
#define BT_NOISE_THRESHOID              xcfg_cb.bt_noise_threshoid  //环境噪声阈值（低于此阈值便认为是噪声）(0~65535)
#define BT_ANL_GAIN                     bt_mic_anl_gain        		//MIC模拟增益(0~23)
#define BT_DIG_GAIN                     xcfg_cb.bt_dig_gain         //MIC数字增益(0-31),且于0~3DB范围细调,步进3/32DB
#define BT_CALL_MAX_GAIN                xcfg_cb.bt_call_max_gain    //配置通话时DAC最大模拟增益
#define BT_CALL_SWITCH_WAV_CACHE_EN     0                           //通话是否切换WAV提示音的缓存（修复通话播WAV提示音有杂音）

#define BT_ALC_EN                       0                           //是否使能ALC
#define BT_ALC_FADE_IN_DELAY            xcfg_cb.bt_alc_in_delay     //近端淡入延时
#define BT_ALC_FADE_IN_STEP             xcfg_cb.bt_alc_in_step      //近端淡入速度
#define BT_ALC_FADE_OUT_DELAY           xcfg_cb.bt_alc_out_delay    //远端淡入延时
#define BT_ALC_FADE_OUT_STEP            xcfg_cb.bt_alc_out_step     //远端淡入速度
#define BT_ALC_VOICE_THR                0x50000

//通话回声消除算法
#define BT_ECHO_LEVEL                   xcfg_cb.bt_echo_level       //回声消除级别（级别越高，回声衰减越明显，但通话效果越差）(0~15)
#define BT_FAR_OFFSET                   xcfg_cb.bt_far_offset       //远端补偿值(0~255)
#define BT_AEC_DUMP_EN                  0                           //是否打开AEC (BT_AEC_EN, BT_NLMS_AEC_EN) 数据打印,打印优先级1
#define BT_AEC_FRE_DUMP_EN              0                           //是否打开频域NLMS AEC（BT_NLMS_FRE_EN）数据打印

#define BT_AEC_EN                       1                           //是否打开硬件频域AEC算法

#define BT_NLMS_AEC_EN                  0                           //是否打开NLMS_AEC算法,此功能打开原始AEC失效
#define BT_NLMS_NLP_MODE                0                           //1：改进回声抑制和双讲效果（BT_NLMS_AEC_EN置1有效）

#define BT_NLMS_FRE_EN                  0                           //是否打开频域NLMS AEC，需要与降噪算法同时打开，注：BT_SCO_ANS_EN, BT_SCO_AINS2_EN 算法不支持该AEC
#define BT_NLMS_FRE_REF_SET             1                           //0:NLP参考AEC前(DF)+线性后(EF)，1:NLP只参考AEC前(DF)，2:NLP只参考AEC后(EF)

//通话MIC端算法（近端）
#define SMIC_DBG_EN                     0       //是否打开单麦算法开关指令（需要打开任意1种单mic算法）
#define DMIC_DBG_EN                     0       //是否打开双麦功能产测（需要打开任意1种双mic算法）
#define BT_SCO_DUMP_EN                  0       //是否打开近端降噪算法数据打印，打印优先级0
#define BT_EQ_DUMP_EN                   0       //是否dump近端EQ的数据

#define BT_MIC_DRC_EN                   0       //DRC参数调试在 bt_mic_8k.drc //(msbc)bt_mic_16k.drc
#define BT_TRUMPET_DENOISE_EN           0       //是否打开近端汽车喇叭声降噪
#define BT_SCO_FADE_EN                  0       //是否打开通话前500ms淡入，默认mute掉

#define BT_SNDP_EN                      0       //是否打开声加单麦降噪算法
#define BT_SNDP_TYPE                    1 	    //声加单mic算法类型: 0(RNN), 1(DNN)
#define BT_SNDP_MODE                    1       //单麦模式模式0，模式1
#define BT_NEAR_AINS2_EN                0       //是否打开声加单麦前的AINS2
#define BT_NEAR_AINS2_NOISE_LEVEL       5       //ains2(0~30,默认20)
#define BT_SNDP_DUMP_EN                 0       //是否打开声加算法数据打印，打印优先级0

#define BT_SNDP_DMIC_EN                 0       //是否打开声加双麦降噪算法

#define BT_SNDP_DMIC_DNN_EN             0       //是否打开声加双麦+(AI)降噪算法;
#define BT_SNDP_DMIC_DNN_NLMS_EN        0       //自适应BF使能，0：密封性要求低，占用空间小；1：使能自适应BF，占用空间大，不兼容远端降噪；
#define BT_SNDP_DMIC_DNN_DISTANCE       (22)    //设置双麦间距，支持范围(15~30mm)；

#define BT_SCO_ANS_EN                   0       //是否打开近端ANS算法

#define BT_SCO_AINS2_EN					0	    //是否打开AINS2降噪
#define	BT_SCO_AINS2_MODE				1	    //AINS2模式，模式0，模式1（建议）
#define BT_SCO_AINS2_LEVEL				(0)	    //0-15级（默认0级）
#define BT_AINS2_DUMP_EN                0       //是否打开AI2算法数据打印，打印优先级0

#define BT_SCO_AINS3_EN					0	    //是否打开AINS3降噪
#define	BT_SCO_AINS3_MODE				1 	    //AINS3模式，模式0，模式1（建议）
#define BT_SCO_AINS3_LEVEL				(0)	    //0-15级（默认0级）
#define BT_SCO_AINS3_PRIOR_OPT			(6)	    //噪声谱过估计参数（默认6）

#define BT_SCO_DNN_EN                   1       //是否打开自研单麦DNN降噪算法
#define BT_SCO_DNN_LEVEL                6       //降噪量：0-30级

#define BT_SCO_AIAEC_DNN_EN             0       //是否打开自研AIAEC+单麦DNN降噪算法
#define BT_SCO_AIAEC_DNN_LEVEL          6       //降噪量：0-30级

#define BT_SCO_DMNS_EN                  0       //是否打开自研双麦降噪算法
#define BT_SCO_DMNS_BF_TYPE             1       //自研双麦beamforming方法选择，建议TYPE 1

#define BT_SCO_DMDNN_EN                 0       //是否打开自研双麦+AI降噪算法

#define BT_SCO_LDMDNN_EN                0       //是否打开长麦距自研双麦+AI降噪算法

#define BT_SCO_COMFORT_NOISE_EN         0       //是否打开舒适噪声算法

#define BT_SCO_AGC_EN                   0       //是否打开AGC算法

//通话喇叭端算法（远端）
#define BT_SCO_FAR_NR_EN                0       //是否打开远端降噪算法
#define BT_SCO_FAR_NR_SELECT            2       //0：ans 1：ains2 2: nr_far
#define BT_SCO_FAR_NOISE_LEVEL          5       //ans:强度（0~30dB，越大降噪效果越好，音质越差） ains2(0~15,默认0)
#define BT_SCO_FAR_NOISE_THRESHOID      1       //远端环境噪声阈值（低于此阈值便认为是噪声）(0~65535); ains2此参数无效
#define BT_SCO_FAR_DUMP_EN              0       //是否打开远端降噪算法数据,打印优先级2

#define BT_SCO_DAC_DRC_EN               0       //是否打开通话下行————DRC
#define BT_SCO_DAC_DRC_DNR_EN           0       //是否打开通话下行————动态降噪，注：需要打开下行DRC

#define BT_SCO_CALLING_NR_EN            0       //是否打开去电后响铃前喇叭的降噪算法
#define BT_SCO_CALLING_VOICE_POW        100     // 设置降噪阈值
#define BT_SCO_CALLING_VOICE_CNT        5       //设置降噪次数

//BLE功能配置
#define LE_BQB_RF_EN                    0   //BLE DUT测试模式，使用串口通信
#define LE_PAIR_EN                      0   //是否使能BLE的加密配对
#define LE_SM_SC_EN                     0   //是否使能BLE的加密连接，需同时打开 LE_PAIR_EN
#define LE_ADV_POWERON_EN               1   //上电是否默认打开BLE广播
#define LE_ADV0_EN                      0   //是否打开无连接广播功能

//APP功能选择（只能打开其中一个）
#define LE_APP_EN                       0   //是否打开BLE APP控制功能,BLE Demo程序，可供参考与二次开发
#define AB_MATE_APP_EN                  0   //是否打开AB_Mate控制功能，AB_Mate的FOTA功能在ab_mate_app.h中通过AB_MATE_OTA_EN控制，与独立FOTA不同且互斥
#define LE_TUYA_EN                      0   //是否打开涂鸦功能
#define LE_WK_APP_EN                    0   //是否打开Wearfit Pro APP控制功能,需添加拓展库lib_wearfit.a
#define LE_QCY_APP_EN                   0   //是否打开QCY APP控制功能
#define TME_APP_EN                      0   //是否打开TME APP功能
#define LE_DUEROS_DMA_EN                0   //是否打开DUEROS DMA APP功能，请查阅dueros_dma_app.h第一行的readme

//快速配对功能选择
#define LE_WIN10_POPUP                  0   //是否打开win10 swift pair快速配对
#define GFPS_EN                         0   //是否打开Google快速配对功能(支持与AB_Mate APP同时打开，暂不支持和其他APP一起打开)


//独立FOTA(双备份升级)功能配置
//注意需要使用2M flash的话，需要把FLASH_SIZE的宏改成2M, 程序大小需限制在（FLASH_SIZE/2 - 16）K以内
//记得settings里也有设置BLE和SPP的开关
//APP请使用AB OTA Demo进行OTA功能测试，该APP只有FOTA功能，只需要FOTA功能的场景可使用这个，需要耳机多功能APP的请使用AB_Mate APP
#define BT_SPP_FOT_EN                   0   //是否打开SPP FOTA升级功能,需和 BT_SPP_EN 同时打开
#define LE_APP_FOT_EN                   0   //是否打开BLE FOTA升级功能,需和 LE_APP_EN 同时打开
#define AB_FOT_TYPE                     AB_FOT_TYPE_PACK     //独立FOTA升级方式选择
#define FOT_SUPPORT_TWS                 BT_TWS_EN   //是否打开FOTA tws同步升级功能，打开后只能左右耳一起升级

/*****************************************************************************
* Module    : 音频压缩算法配置
******************************************************************************/
#define OPUS_ENC_EN                     0   //是否打开opus压缩算法

/*****************************************************************************
* Module    : NTC预警关机功能配置
******************************************************************************/
#define USER_NTC                        0
#define ADCCH_NTC                       ADCCH_PF5

/*****************************************************************************
* Module    : AUX功能配置
******************************************************************************/
#define AUX_CHANNEL_CFG                 (CH_AUXL_PB1 | CH_AUXR_PB2) //选择LINEIN通路
#define MIC_CHANNEL_CFG                 CH_MICL0                    //选择MIC的通路
#define AUX_2_SDADC_EN                  1                           //AUX是否进SDADC, 否则直通DAC。进SDADC可以调AUX EQ, DAC DRC及AUX录音等功能
#define AUX_SNR_EN                      0                           //AUX模式动态降噪(AUX模拟直通也能用)
#define LINEIN_DETECT_EN                1                           //是否打开LINEIN检测
#define SDCMD_MUX_DETECT_LINEIN         0                           //是否复用SDCMD检测LINEIN插入
#define MICL_MUX_DETECT_LINEIN          0                           //是否复用MICL/PF2检测LINEIN插入
#define LINEIN_2_PWRDOWN_EN             0                           //是否插入Linein后直接软关机（大耳包功能）

///通过配置工具选择检测GPIO
#define LINEIN_DETECT_INIT()            linein_detect_init()
#define LINEIN_IS_ONLINE()              linein_is_online()
#define IS_DET_LINEIN_BUSY()            is_detect_linein_busy()

/*****************************************************************************
* Module    : ADC配置控制
******************************************************************************/
#define SDADC_SOFT_GAIN_EN              1                           //ADC 软件增益使能
#define SDADC_EQ_EN                     0                           //是否打开通话ADC EQ，可用于mic频响补偿
#define SDADC_DRC_EN                    0                           //是否打开ADC DRC v3
#define SDADC_SINGLE_DRC_EN             0                           //是否打开单端mic sdadc DRC

/*****************************************************************************
* Module    : DAC配置控制
******************************************************************************/
#define DAC_CH_SEL                      xcfg_cb.dac_sel             //DAC_MONO ~ DAC_VCMBUF_DUAL
#define DAC_FAST_SETUP_EN               0                           //DAC快速上电，有噪声需要外部功放MUTE
#define DAC_MAX_GAIN                    xcfg_cb.dac_max_gain        //配置DAC最大模拟增益，默认设置为dac_vol_table[VOL_MAX]
#define DAC_OUT_SPR                     DAC_OUT_44K1                //dac out sample rate
#define DAC_LDOH_SEL                    xcfg_cb.dac_ldoh_sel
#define DAC_VCM_CAPLESS_EN              xcfg_cb.dac_vcm_less_en     //DAC VCM省电容方案,使用内部VCM
#define DACVDD_BYPASS_EN                xcfg_cb.dacvdd_bypass_en    //DACVDD Bypass
#define DAC_PULL_DOWN_DELAY             80                          //控制DAC隔直电容的放电时间, 无电容时可设为0，减少开机时间。
#define DAC_DNR_EN                      1                           //是否使能动态降噪
#define DAC_DRC_EN                      0                           //是否使能DRC功能（目前只支持蓝牙音乐AAC、SBC和aux），需要把时钟调到至少48M，DRC参数在dac.drc
#define DAC_OFF_FOR_BT_CONN_EN          xcfg_cb.dac_off_for_conn
#define DACRP_CH_VUSB_EN                xcfg_cb.dacrp_vusb_en       //是否使能DACRP通过VUSB输出

/*****************************************************************************
* Module    : ANC配置控制
******************************************************************************/
#define ANC_EN                          0                           //是否使能ANC
#define ANC_HYBRID_EN                   0                           //是否使能ANC Hybrid
#define PDM_MIC_EN                      0                           //是否使能PDM MIC
#define PDM_MIC_MAPPING                 PDM_MAP_PE6PE7              //选择PDM MIC mapping
#define TINY_TRANSPARENCY_EN            0					 		//是否使能简单的通透模式（不能与ANC_EN同时打开）
#define ANC_SOFT_FADE_EN                0                           //是否使能ANC软件淡入淡出功能，ANC_FADE_IN(OUT)_STEP可以设置步进单位
#define ANC_EXP_DC_ADJ_EN               0                           //是否使能在ANC链路上抬高dc量

/*****************************************************************************
* Module    : ASR配置控制
******************************************************************************/
#define ASR_EN                          0                           //是否使能语音识别功能
#define ASR_VAD_EN                      0                           //是否使能VAD功能
#define ASR_BASE_ADDR                   0XBE000                     //模型存放地址
#define ASR_BASE_LEN                    0X37000                     //模型长度

/*****************************************************************************
 * Module    : 录音功能配置
 *****************************************************************************/
#define FUNC_REC_EN                     0   //录音功能总开关
#define FMRX_REC_EN                     0   //是否打开FM录音功能
#define AUX_REC_EN                      1   //是否打开AUX录音功能
#define MIC_REC_EN                      1   //是否打开MIC录音功能
#define BT_REC_EN                       0   //是否打开蓝牙音乐录音功能
#define BT_HFP_REC_EN                   0   //是否打开蓝牙通话录音功能（不支持karaok）
#define KARAOK_REC_EN                   0   //是否打开KARAOK录音功能，需同步打开KARAOK功能
#define REC_ONE_FOLDER_EN               0   //是否各模式录音放到同一目录下
#define REC_DIR_LFN                     1   //是否打开长文件名目录
#define REC_AUTO_PLAY                   0   //录音结束是否回放当前录音内容
#define REC_FAST_PLAY                   1   //播卡播U下快速播放最新的录音文件(双击REC)
#define REC_STOP_MUTE_1S                0   //录音停止时, MUTE 1S功放. //提醒客户录音结束.
#define REC_TYPE_SEL                    REC_SBC     //注意REC_ADPCM格式只支持通话录音
#define REC_DIG_GAIN_EN                 0    //录音是否需要加大数字增益


/*****************************************************************************
 * Module    :外接SPIFLASH配置, 外接SPIFLASH可以播放MP3音乐文件, 及录音
 *****************************************************************************/
#define EX_SPIFLASH_SUPPORT              0           //可以配置为 EXSPI_NOT_SUPPORT(0) 或 EXSPI_MUSIC 或 EXSPI_REC 或 (EXSPI_MUSIC | EXSPI_REC)

#define SPIFLASH_MP3_LOOPBACK_EN         0           //外部flash音乐循环
#define SPIFLASH_ID                      0x40170000  //通过读ID判断FLASH是否在线, 需要改成SPIFLASH对应的ID
#define SPIFALSH_BAUD                    (500000)    //SPI波特率500K

#if (EX_SPIFLASH_SUPPORT & EXSPI_MUSIC)
//FLASH_MUSIC.BIN 镜像文件占用区域(BYTE)
#define SPIFLASH_MUSIC_BEGIN_ADDR        0           //FLASH_MUSIC.BIN镜像文件默认从0地址开始存放,此宏暂不支持修改.
#define SPIFLASH_MUSIC_END_ADDR         (1024*36)    //FLASH_MUSIC.BIN镜像文件结束地址. 测试DEMO的镜像文件是36K大小.
#define SPIFALSH_MUSIC_BIN_WRITE_TEST    0           //默认的FLASH_MUSIC.BIN写入SPIFLASH, 可以在func_exspifalsh_music中测试外接SPIFALSH播放MP3.
#endif

#if (EX_SPIFLASH_SUPPORT & EXSPI_REC)
//录音占用区域(BYTE)  //注意SPIFALSH的录音区域不要覆盖 FLASH_MUSIC.BIN区域
#define SPIFLASH_REC_BEGIN_ADDR        (1024*37)    //录音起始地址
#define SPIFLASH_REC_END_ADDR          (1024*1024)  //录音结束地址
#endif

#define SPIFALSH_MUSIC_PLAY_REC        0     // 1 FUNC_EXSPIFLASH_MUSIC 模式下播放录音示例： 支持录音上下曲，及删除当前播放的录音  //0 播放镜像音乐示例

#if EX_SPIFLASH_SUPPORT                 //TEST CONFIG
#undef GUI_SELECT
#undef FLASH_SIZE
#undef FUNC_REC_EN
#undef MIC_REC_EN
#undef REC_AUTO_PLAY

#define GUI_SELECT                      GUI_NO
#define FLASH_SIZE                      FSIZE_1M
#define FUNC_REC_EN                     1
#define MIC_REC_EN                      1
#define REC_AUTO_PLAY                   1
#endif


/*****************************************************************************
 * Module    : K歌功能配置
 *****************************************************************************/
#define SYS_KARAOK_EN                   0   //是否打开K歌功能（暂不支持与录音、WMA、FLAC同时打开）
#define BT_HFP_CALL_KARAOK_EN           0   //通话是否支持KARAOK.
#define SYS_ECHO_EN                     1   //是否使能混响功能
#define SYS_ECHO_DELAY                  16  //混响间隔级数
#define SYS_BASS_TREBLE_EN              0   //是否使能高低音调节
#define SYS_MAGIC_VOICE_EN              0   //是否使能魔音功能
#define SYS_HOWLING_EN                  0   //是否使能防啸叫功能

#define MIC_DETECT_EN                   0   //是否使能MIC检测
#define MIC_DNR_EN                      0   //是否使能MIC动态降噪

///通过配置工具选择检测GPIO
#define MIC_DETECT_INIT()               mic_detect_init()
#define MIC_IS_ONLINE()                 mic_is_online()
#define IS_DET_MIC_BUSY()               is_detect_mic_busy()


/*****************************************************************************
 * Module    : User按键配置 (可以同时选择多组按键)
 *****************************************************************************/
#define USER_ADKEY                      0           //ADKEY的使用， 0为不使用  [TF_TEST] 释放 PE7 给 SD0_DAT0
#define USER_ADKEY2                     0           //ADKEY2的使用，0为不使用
#define USER_PWRKEY                     1           //PWRKEY的使用，0为不使用
#define USER_IOKEY                      0           //IOKEY的使用， 0为不使用
#define USER_ADKEY_MUX_SDCLK            0           //是否使用复用SDCLK的ADKEY, 共用USER_ADKEY的按键table
#define USER_ADKEY_MUX_LED              0           //是否使用ADKEY与LED复用, 共用USER_ADKEY的流程(ADKEY与BLED配置同一IO)
#define ADKEY_PU10K_EN                  0           //ADKEY是否使用内部10K上拉, 按键数量及阻值见port_key.c

#define USER_TKEY                       0           //TouchKEY的使用，0为不使用
#define USER_TKEY_SOFT_PWR_EN           0           //是否使用TouchKey进行软开关机
#define USER_TKEY_INEAR                 0           //是否使用TouchKey的入耳检测功能
#define USER_TKEY_DEBUG_EN              0           //仅调试使用，用于确认TKEY的参数
#define IS_TKEY_PRESS()			        (tkey_is_pressed() && USER_TKEY_SOFT_PWR_EN)

#define USER_KEY_KNOB_EN                0           //旋钮的使用，0为不使用
#define USER_KEY_KNOB_LEVEL             16          //旋钮的级数

#define USER_MULTI_PRESS_EN              1           //按键N击检测使能
#define USER_KEY_DOUBLE                 {KU_PLAY_PWR_USER_DEF, KU_PLAY_USER_DEF}        //支持双击/三击/四击/五击的按键
#define ADKEY_CH                        xcfg_cb.adkey_ch    //ADCCH_PE7
#define ADKEY2_CH                       xcfg_cb.adkey2_ch   //ADCCH_PE6

#define IS_PWRKEY_PRESS()			    ((sys_cb.wko_pwrkey_en) && (0 == (RTCCON & BIT(19))))
#define DOUBLE_KEY_TIME                 (xcfg_cb.double_key_time)                       //按键双击响应时间（单位50ms）
#define PWRON_PRESS_TIME                (500*xcfg_cb.pwron_press_time)                  //长按PWRKEY多长时间开机？
#define PWROFF_PRESS_TIME               (3+3*xcfg_cb.pwroff_press_time)                 //长按PWRKEY多长时间关机？

/*****************************************************************************
 * Module    : SD0配置
 *****************************************************************************/
#define SD_SOFT_DETECT_EN               0           //是否使用软件检测 (SD发命令检测)
#define SDCLK_MUX_DETECT_SD             1           //是否复用SDCLK检测SD卡
#define SD0_MAPPING                     SD0MAP_G3   //选择SD0 mapping  [TF_TEST] G3=PE5(SDCMD)/PE6(SDCLK)/PE7(SDDAT0)
#define SD1_MAPPING                     SD0MAP_G3   //选择SD1 mapping

///通过配置工具选择检测GPIO
#define SD_DETECT_INIT()                sdcard_detect_init()
#define SD_IS_ONLINE()                  sdcard_is_online()
#define IS_DET_SD_BUSY()                is_det_sdcard_busy()
#define SD_IS_SOFT_DETECT()             (xcfg_cb.sddet_iosel == 30)  //配置工具中选则30是软件检测.

#define SD1_DETECT_INIT()               sdcard1_detect_init()
#define SD1_IS_ONLINE()                 sdcard1_is_online()
#define IS_DET_SD1_BUSY()               is_det_sdcard1_busy()

/*****************************************************************************
 * Module    : I2C配置
 *****************************************************************************/
#define I2C_HW_EN                       0           //是否使能硬件I2C功能
#define I2C_SW_EN                       0           //是否使能软件I2C功能
#define I2C_MAPPING                     I2CMAP_PB3PB4 //选择I2C mapping

#define I2C_MUX_SD_EN                   1           //是否I2C复用SD卡的IO

#define I2C_SCL_IN()                    SD_CMD_DIR_IN()
#define I2C_SCL_OUT()                   SD_CMD_DIR_OUT()
#define I2C_SCL_H()                     SD_CMD_OUT_H()
#define I2C_SCL_L()                     SD_CMD_OUT_L()

#define I2C_SDA_IN()                    SD_DAT_DIR_IN()
#define I2C_SDA_OUT()                   SD_DAT_DIR_OUT()
#define I2C_SDA_H()                     SD_DAT_OUT_H()
#define I2C_SDA_L()                     SD_DAT_OUT_L()
#define I2C_SDA_IS_H()                  SD_DAT_STA()

#define I2C_SDA_SCL_OUT()               {I2C_SDA_OUT(); I2C_SCL_OUT();}
#define I2C_SDA_SCL_H()                 {I2C_SDA_H(); I2C_SCL_H();}

/*****************************************************************************
 * Module    : I2S配置
 *****************************************************************************/
#define I2S_EN                          0           //是否使能I2S功能
#define I2S_DEVICE                      I2S_DEV_NO //I2S设备选择
#define I2S_MAPPING_SEL                 I2S_GPIOA   //I2S IO口选择
#define I2S_MODE_SEL                    0           //I2S主从模式选择 0: master; 1:slave
#define I2S_BIT_MODE                    0           //I2S数据位宽选择 0:16bit; 1:32bit
#define I2S_DATA_MODE                   1           //I2S数据格式选择 0:left-justified mode; 1:normal mode
#define I2S_DMA_EN                      0           //I2S数据源选择 0:src; 1:dma
#define I2S_MCLK_EN                     1           //I2S是否打开MCLK
#define I2S_MCLK_SEL                    2           //I2S MCLK选择 0:64fs 1:128fs 2:256fs
#define I2S_PCM_MODE                    0           //I2S是否打开PCM mode
#define I2S_DAC_OUT_SET                 SPR_44100   //做从机时，配置dac采样率，做主机时可忽略

#define I2S_EXT_EN                      0
#define I2S_2_BT_SCO_EN                 0
/*****************************************************************************
 * Module    : SPDIF配置
 *****************************************************************************/
 #define SPDIF_CH                       SPF_PA0_CH0   //[TF_TEST] 释放 PE6 给 SD0_CLK

/*****************************************************************************
 * Module    : IRRX配置
 *****************************************************************************/
#define IRRX_HW_EN                      0           //是否打IRRX硬件模块  [TF_TEST] 释放 PE6 给 SD0_CLK
#define IRRX_SW_EN                      0           //是否打开timer capture ir
#define IR_NUMKEY_EN                    1           //是否打开数字键输入
#define IR_INPUT_NUM_MAX                999         //最大输入数字9999

//可以打开一个或多个
#define IR_ADDR_FF00_EN                 1
#define IR_ADDR_BF00_EN                 0
#define IR_ADDR_FD02_EN                 0
#define IR_ADDR_FE01_EN                 0
#define IR_ADDR_7F80_EN                 0

#define IR_CAPTURE_PORT()               {GPIOEDE |= BIT(6); GPIOEPU  |= BIT(6); GPIOEDIR |= BIT(6);}
#define IRRX_MAPPING                    IRMAP_PB0   //[TF_TEST] 释放 PE6
#define TMR3CAP_MAPPING                 TMR3MAP_PB1 //[TF_TEST] 释放 PE6


/*****************************************************************************
 * Module    : usb device 功能选择
 *****************************************************************************/
#define UDE_STORAGE_EN                 1
#define UDE_SPEAKER_EN                 1
#define UDE_HID_EN                     1
#define UDE_MIC_EN                     1


/*****************************************************************************
 * Module    : 系统细节配置
 *****************************************************************************/
#define RGB_SERIAL_EN                   0           //串行RGB推灯功能
#define PWM_RGB_EN                      0           //PWM RGB三色灯功能
#define ENERGY_LED_EN                   0           //能量灯软件PWM显示,声音越大,点亮的灯越多.
#define SYS_PARAM_RTCRAM                0           //是否系统参数保存到RTCRAM
#define PWRON_ENTER_BTMODE_EN           0           //是否上电默认进蓝牙模式
#define VBAT_DETECT_EN                  1           //电池电量检测功能
#define VBAT2_ADCCH                     ADCCH_VBAT  //ADCCH_VBAT为内部1/2电压通路，带升压应用需要外部ADC通路检测1/2电池电压
#define VBAT_FILTER_USE_PEAK            0           //电池检测滤波选则://0 取平均值.//1 取峰值(适用于播放音乐时,电池波动比较大的音箱方案).
#define EQ_MODE_EN                      1           //是否调节EQ MODE (POP, Rock, Jazz, Classic, Country)
#define EQ_DBG_IN_UART                  1           //是否使能UART在线调节EQ
#define EQ_DBG_IN_SPP                   1           //是否使能SPP在线调节EQ
#define EQ_DBG_IN_UART_VUSB_EN          0           //打开vusb在线调EQ的功能，配置需要打开，波特率9600
#define SYS_EQ_FOR_IDX_EN               0           //是否使能10条EQ独立调节(包括高低音)
#define SLEEP_DAC_OFF_EN                (is_sleep_dac_off_enable()) //sfunc_sleep是否关闭DAC， 复用MICL检测方案不能关DAC。
#define SYS_INIT_VOLUME                 xcfg_cb.sys_init_vol        //系统默认音量
#define SYS_LIMIT_VOLUME                5                           //开机最小音量，避免开机时音量太小误认为没开机
#define LPWR_WARNING_VBAT               xcfg_cb.lpwr_warning_vbat   //低电提醒电压
#define LPWR_OFF_VBAT                   xcfg_cb.lpwr_off_vbat       //低电关机电压
#define LOWPWR_REDUCE_VOL_EN            1                           //低电是否降低音量
#define LPWR_WARING_TIMES               0xff                        //报低电次数

/*****************************************************************************
 * Module    : LED指示灯配置
 *****************************************************************************/
#define LED_DISP_EN                     1           //是否使用LED指示灯(蓝灯)
#define LED_PWR_EN                      1           //充电及电源指示灯(红灯)
#define LED_LOWBAT_EN                   0                           //电池低电是否闪红灯
#define BLED_CHARGE_FULL                xcfg_cb.charge_full_bled    //充电满是否亮蓝灯
#define BT_RECONN_LED_EN                0//xcfg_cb.bt_reconn_led_en    //蓝牙回连状态是否不同的闪灯方式

#define LED_INIT()                      bled_func.port_init(&bled_gpio)
#define LED_SET_ON()                    bled_func.set_on(&bled_gpio)
#define LED_SET_OFF()                   bled_func.set_off(&bled_gpio)

#define LED_PWR_INIT()                  rled_func.port_init(&rled_gpio)
#define LED_PWR_SET_ON()                rled_func.set_on(&rled_gpio)
#define LED_PWR_SET_OFF()               rled_func.set_off(&rled_gpio)

/*****************************************************************************
 * Module    : Loudspeaker mute及耳机检测配置
 *****************************************************************************/
#define EARPHONE_DETECT_EN              1           //是否打开耳机检测
#define SDCMD_MUX_DETECT_EARPHONE       1           //是否复用SDCMD检测耳机插入

///通过配置工具选择检测GPIO
#define EARPHONE_DETECT_INIT()          earphone_detect_init()
#define EARPHONE_IS_ONLINE()            earphone_is_online()
#define IS_DET_EAR_BUSY()               is_detect_earphone_busy()

#define LOUDSPEAKER_MUTE_EN             1           //是否使能功放MUTE
#define LOUDSPEAKER_MUTE_INIT()         loudspeaker_mute_init()
#define LOUDSPEAKER_MUTE_DIS()          loudspeaker_disable()
#define LOUDSPEAKER_MUTE()              loudspeaker_mute()
#define LOUDSPEAKER_UNMUTE()            loudspeaker_unmute()
#define LOUDSPEAKER_UNMUTE_DELAY        6           //UNMUTE延时配置，单位为5ms

#define AMPLIFIER_SEL_INIT()            amp_sel_cfg_init(xcfg_cb.ampabd_io_sel)
#define AMPLIFIER_SEL_D()               amp_sel_cfg_d()
#define AMPLIFIER_SEL_AB()              amp_sel_cfg_ab()


/*****************************************************************************
 * Module    : 充电功能选择
 *****************************************************************************/
#define CHARGE_EN                       1           //是否打开充电功能
#define CHARGE_TRICK_EN                 xcfg_cb.charge_trick_en     //是否打开涓流充电功能
#define CHARGE_DC_RESET                 xcfg_cb.charge_dc_reset     //是否打开DC插入复位功能
#define CHARGE_DC_NOT_PWRON             xcfg_cb.charge_dc_not_pwron //DC插入，是否软开机。 1: DC IN时不能开机
#define CHARGE_DC_IN()                  ((RTCCON >> 20) & 0x01)
#define CHARGE_INBOX()                  ((RTCCON >> 22) & 0x01)

//充电截止电流：0:2.5mA; 1:5mA; 2:10mA; 3:15mA; 4:20mA; 5:25mA; 6:30mA; 7:35mA
#define CHARGE_STOP_CURR                xcfg_cb.charge_stop_curr    //0~7
//充电截止电压：0:4.15v 1:4.3v
#define CHARGE_STOP_VOLT                0    //0~1
//恒流充电（电池电压大于2.9v）电流：0:10mA, 1:20mA, 2:30mA, 3:40mA, 4:50mA, 5:60mA, 6:70mA, 7:80mA, 8:90mA, 9:100mA, 10:110mA, 11:120mA, 12:140mA, 13:160mA, 14:180mA, 15:200mA
#define CHARGE_CONSTANT_CURR            (xcfg_cb.charge_constant_curr)
//涓流截止电压：0:2.9v; 1:3v
#define CHARGE_TRICK_STOP_VOLT          1
//涓流充电（电池电压小于2.9v）电流：0:10mA; 1:20mA; 2:30mA;
#define CHARGE_TRICKLE_CURR             (xcfg_cb.charge_trickle_curr)


/*****************************************************************************
 * Module    : VUSB UART功能选择
 *****************************************************************************/
#define VUSB_HUART_DMA_EN               0            //VUSB使用高速串口
#define VUSB_TBOX_QTEST_EN              0            //VUSB快速测试盒测试
#define VUSB_TBOX_QTEST_CUSTOM_EN       0            //VUSB快速测试盒自定义测试
#define VUSB_SMART_VBAT_HOUSE_EN        0            //昇生微智能充电仓
#define VUSB_SMART_VBAT_DELAY_DISC      3500         //智能充电仓时，延迟断线进入充电（0=不延迟, n=延迟n毫秒）
#define IODM_TEST_MODE                  0            // IODM 测试 功能


/*****************************************************************************
 * Module    : 提示音 功能选择
 *****************************************************************************/
#define WARNING_TONE_EN                 1            //是否打开提示音功能, 总开关
#define WARNING_PIANO_EN                1            //是否打开Piano音功能
#define WARNING_FIXED_VOLUME            1            //提示音是否固定音量，不跟随系统音量调节
#define WARING_MAXVOL_MP3               0            //最大音量提示音WAV或MP3选择， 播放WAV可以与MUSIC叠加播放。
#define WARNING_VOLUME                  xcfg_cb.warning_volume   //播放提示音的音量级数
#define LANG_SELECT                     LANG_EN      //提示音语言选择

#define WARNING_POWER_ON                1
#define WARNING_POWER_OFF               1
#define WARNING_FUNC_MUSIC              0
#define WARNING_FUNC_BT                 1
#define WARNING_FUNC_CLOCK              1
#define WARNING_FUNC_FMRX               1
#define WARNING_FUNC_AUX                1
#define WARNING_FUNC_USBDEV             1
#define WARNING_FUNC_SPEAKER            1
#define WARNING_LOW_BATTERY             1
#define WARNING_BT_WAIT_CONNECT         1
#define WARNING_BT_CONNECT              1
#define WARNING_BT_DISCONNECT           1
#define WARNING_BT_INCALL               1
#define WARNING_USB_SD                  1
#define WARNING_MAX_VOLUME              1
#define WARNING_MIN_VOLUME              0
#define WARNING_BT_HID_MENU             1            //BT HID MENU手动连接/断开HID Profile提示音
#define WARNING_BTHID_CONN              0            //BTHID模式是否有独立的连接/断开提示音
#define WARNING_BT_PAIR                 1            //BT PAIRING提示音


#define SW_VERSION		"V0.0.1"		//只能使用数字0-9,ota需要转码
#define HW_VERSION		"V0.0.1"		//只能使用数字0-9,ota需要转码
#include "config_extra.h"

#endif // USER_CONFIG_H

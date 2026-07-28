#include "include.h"
#include "func_exspiflash_music.h"

#if MUSIC_SD_LOOPBACK_EN || SPIFLASH_MP3_LOOPBACK_EN
void music_qskip_frame(bool direct, u32 frame);
u32 fs_get_file_pos(void);
void music_qskip(bool direct, u8 second);
u16 music_get_total_time(void);

uint32_t spiflash_music_start_offset = 0;
uint32_t spiflash_music_end_offset  = 0;
bool spiflash_music_loopback_en = false;

typedef struct  {
    u32 frame_count;                //current frame count
    u32 file_ptr;                   //file ptr of current frame
    u16 samples;                    //sample pionts of frame
    u8  spr;                        //sample rate of enum number
    u32 sample_rate;                //sample rate
    u32 bitrate;                    //bit rate
} codec_info_t;
extern codec_info_t *codec_info;

static u32 loopback_frame = 0;
static u32 music_loopback_end = 0;

static u32 music_get_mp3_frame_length(void)
{
    u32 frame_len =(codec_info->samples * codec_info->bitrate / 8) / codec_info->sample_rate;           //帧长度（单位字节）
    return frame_len;
}

static u32 music_get_mp3_frame_ms(void)
{
    u32 frame_ms =(codec_info->samples * 1000) / codec_info->sample_rate;                               //帧时间
    return frame_ms;
}

static u32 get_exflash_mp3_size(void)
{
    u32 size = 0;
    #if SPIFLASH_MP3_LOOPBACK_EN && EX_SPIFLASH_SUPPORT == EXSPI_MUSIC
    spiflash1_read(&size, 0x24 + ((exspi_msc.cur_num - 1) * 8), 4);
    #endif
    return size;
}

//SD音乐循环，字节转换为帧
static u32 music_byte_to_frame(u32 byte)
{
    u32 frame = 0, frame_len;
    frame_len = music_get_mp3_frame_length();
    frame = byte / frame_len;
    return frame;
}

//SD音乐循环,首尾地址偏移
AT(.com_text.music_loopback)
bool music_mp3_loopback_size(u32 start_byte_offset, u32 end_byte_offset, u8 type)
{
    u32 start_frame, music_size;

    if (type == SD_MP3_MUSIC) {
        music_size = fs_get_file_size();
    } else if (type == EXFLASH_MP3_MUSIC){
        music_size = get_exflash_mp3_size();
    } else if (type == INNER_MP3_MUSIC) {       													//芯片内置音乐资源
        spiflash_music_start_offset = start_byte_offset;
        spiflash_music_end_offset = end_byte_offset;
        return true;
    }
    if (music_size > (start_byte_offset + end_byte_offset)) {
        start_frame = music_byte_to_frame(start_byte_offset);
        loopback_frame = music_byte_to_frame(music_size - start_byte_offset - end_byte_offset);      //循环的帧数
        music_loopback_end = music_size - end_byte_offset;
        music_qskip_frame(0,start_frame);                                                            //快进到起始帧，准备循环
//        printf("start_byte_offset:0X%X\n",start_byte_offset);
//        printf("end_byte_offset:0X%X\n",end_byte_offset);
//        printf("music_size:0X%X\n",music_size);
//        printf("loopback_frame:%d\n",loopback_frame);
//        printf("music_loopback_end:0X%X\n",music_loopback_end);
        return true;
    } else {                                                                                          //总偏移字节大于文件体积
        return false;
    }
}

//SD音乐循环，设置播放起点
AT(.com_text.music_loopback)
bool music_mp3_loopback_set_play(u32 second, u8 type)
{
//    printf("music_get_total_time:%d\n",music_get_total_time());
    if (music_get_total_time() <= second || type == INNER_MP3_MUSIC){                                                             //设置的时间大于歌曲长度
        return false;
    } else {
        u32 frame_len, frame_ms, offset_byte;
        frame_len = music_get_mp3_frame_length();
        frame_ms = music_get_mp3_frame_ms();
        if (type == SD_MP3_MUSIC) {
            music_loopback_end = fs_get_file_size();
        } else {
            music_loopback_end = get_exflash_mp3_size();
        }
        offset_byte = frame_len * second * 1000 / frame_ms;                                             //偏移地址
        loopback_frame = music_byte_to_frame(music_loopback_end - offset_byte);                         //循环的帧数
        music_qskip(0,second);                                                                          //快进到起始帧，准备循环
        return true;
    }
}

//SD音乐循环，尾部ms不播放
AT(.com_text.music_loopback)
bool music_mp3_loopback_reduce_end(u32 ms, u8 type)
{
    if (1000 * music_get_total_time() <= ms || type == INNER_MP3_MUSIC){                                                           //设置的时间大于歌曲长度
        return false;
    } else {
        u32 frame_len, frame_ms, offset_byte;
        frame_len = music_get_mp3_frame_length();
        frame_ms = music_get_mp3_frame_ms();
        offset_byte = frame_len * ms / frame_ms;                                                        //尾部偏移字节
        if (type == SD_MP3_MUSIC) {
            music_loopback_end = fs_get_file_size() - offset_byte;
        } else {
            music_loopback_end = get_exflash_mp3_size() - offset_byte;
        }
        loopback_frame = music_byte_to_frame(music_loopback_end);                                       //循环的帧数
        return true;
    }
}

//SD音乐循环，回到循环播放的起点
AT(.text.music_loopback)
void music_mp3_loopback_replay(void)
{
    static bool sd_skip_init = true;
    if (fs_get_file_pos() >= music_loopback_end && music_loopback_end != 0 ) {
        if (sd_skip_init) {
            music_qskip_frame(1, loopback_frame);                                                        //往后退loopback_frame帧
            sd_skip_init = false;
        } else {
            music_qskip_frame(1, 0);                                                                     //除了第一次需要loopback_frame外，后面再退就用0
        }
    }
}

AT(.text.music_loopback)
void exflash_mp3_loopback_replay(u32 pos)
{
    static bool exflash_skip_init = true;
    if (pos >= music_loopback_end && music_loopback_end != 0 ) {
        if (exflash_skip_init) {
            music_qskip_frame(1, loopback_frame);                                                         //往后退loopback_frame帧
            exflash_skip_init = false;
        } else {
            music_qskip_frame(1, 0);                                                                      //除了第一次需要loopback_frame外，后面再退就用0
        }
    }
}

AT(.text.music_loopback)
void inner_flash_music_loopback_en(bool en)
{
    spiflash_music_loopback_en = en;
}
#endif

#if FUNC_MUSIC_EN
extern u8 fname_buf[100];

//#define MUSIC_EXT_NUM 6                         //一共搜索3个扩展名
//const char tbl_music_ext[MUSIC_EXT_NUM][4] = {"mp3", "wav", "wma", "ape", "fla", "sbc"};

//转换为小写字符
AT(.text.fs.scan)
unsigned char char_tolower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z') {
		c -= 'A'-'a';
	}
	return c;
}

//将字符串转换为小写
AT(.text.fs.scan)
void str_tolower(char *str, uint len)
{
    uint i;
    for (i = 0; i < len; i++) {
        str[i] = char_tolower(str[i]);
    }
}

AT(.text.fs.scan)
static bool file_is_music(char *ext_name)
{
//    u8 i;
//    for (i=0; i != MUSIC_EXT_NUM; i++) {
//        if (0 == memcmp(ext_name, (char *)(tbl_music_ext[i]), 3)) {
//            return true;
//        }
//    }
//    return false;

    if (0 == memcmp(ext_name, "mp3", 3)) {
        return true;
    }
#if MUSIC_WAV_SUPPORT
    if (xcfg_cb.music_wav_support) {
        if (0 == memcmp(ext_name, "wav", 3)) {
            return true;
        }
    }
#endif // MUSIC_WAV_SUPPORT

#if MUSIC_WMA_SUPPORT
    if (xcfg_cb.music_wma_support) {
        if (0 == memcmp(ext_name, "wma", 3)) {
            return true;
        }
    }
#endif // MUSIC_WMA_SUPPORT

#if MUSIC_APE_SUPPORT
    if (xcfg_cb.music_ape_support) {
        if (0 == memcmp(ext_name, "ape", 3)) {
            return true;
        }
    }
#endif // MUSIC_APE_SUPPORT

#if MUSIC_FLAC_SUPPORT
    if (xcfg_cb.music_flac_support) {
        if (0 == memcmp(ext_name, "fla", 3)) {
            return true;
        }
    }
#endif // MUSIC_FLAC_SUPPORT

#if MUSIC_SBC_SUPPORT
    if (0 == memcmp(ext_name, "sbc", 3)) {
        return true;
    }
#endif // MUSIC_SBC_SUPPORT

#if MUSIC_AAC_SUPPORT
    if (0 == memcmp(ext_name, "aac", 3)) {
        return true;
    }
#endif // MUSIC_AAC_SUPPORT
    return false;
}

///返回值： 0->不匹配的文件类型， 1->匹配的文件类型
AT(.text.fs.scan)
u8 music_file_filter(void)
{
    char extension[3];
    fs_get_fname_extension(extension);
    str_tolower(extension, 3);
    if (file_is_music(extension)) {
#if REC_FAST_PLAY
        bsp_update_final_rec_file_num();
#endif // REC_FAST_PLAY
        return 1;
    }
    return 0;
}

///返回值： 0->不匹配的目录类型， 1->匹配的目录类型
AT(.text.fs.scan)
u8 music_dir_filter(void)
{
#if REC_FAST_PLAY
    char sfn[13];
    fs_get_short_fname(sfn, 1);

    //录音目录
    if (is_record_dir(sfn)) {
        if (f_msc.rec_scan & BIT(0)) {
            f_msc.rec_scan |= BIT(1);
        }
    } else {
        f_msc.rec_scan &= ~BIT(1);
    }
#endif // REC_FAST_PLAY

    return 1;
}

#if MUSIC_REC_FILE_FILTER
///只播放录音文件歌曲
u8 music_only_record_dir_filter(void)
{
    char sfn[13];
    fs_get_short_fname(sfn, 1);

    //录音目录
    if (is_record_dir(sfn)) {
#if REC_FAST_PLAY
        if (f_msc.rec_scan & BIT(0)) {
            f_msc.rec_scan |= BIT(1);
        }
#endif // REC_FAST_PLAY
        return 1;
    }
    return 0;
}

///只播放录音文件歌曲
u8 music_only_record_file_filter(void)
{
    char extension[3];
    fs_get_fname_extension(extension);
    str_tolower(extension, 3);
    if (file_is_music(extension) && fs_get_dir_depth()) {   //去掉根目录正常歌曲
#if REC_FAST_PLAY
        bsp_update_final_rec_file_num();
#endif // REC_FAST_PLAY
        return 1;
    }
    return 0;
}

///去掉录音文件
u8 music_rm_record_dir_filter(void)
{
    char sfn[13];
    fs_get_short_fname(sfn, 1);

    //录音目录
    if (is_record_dir(sfn)) {
        return 0;
    }

    return 1;
}
#endif // MUSIC_REC_FILE_FILTER

#if REC_FAST_PLAY
void bsp_update_final_rec_file_num(void)
{
    if (fs_get_dir_depth() && (f_msc.rec_scan & BIT(1))) {
        if (fs_get_ftime() > sys_cb.ftime) {
            sys_cb.ftime = fs_get_ftime();
            sys_cb.rec_num = fs_get_file_count();
        }
    }
}
#endif // REC_FAST_PLAY

AT(.text.stream)
int stream_read(void *buf, unsigned int size)
{
    UINT len;
    u8 res = fs_read(buf, size, &len);
#if MUSIC_SD_LOOPBACK_EN
    music_mp3_loopback_replay();
#endif
    if (res == FR_OK) {
#if MUSIC_ENCRYPT_EN
        if (f_msc.encrypt) {
            music_stream_decrypt(buf, len);
        }
#endif
        return len;
    } else {
        return -1;
    }
}

AT(.text.stream)
bool stream_seek(unsigned int ofs, int whence)
{
#if MUSIC_ENCRYPT_EN
    if (f_msc.encrypt) {
        if (whence == SEEK_SET) {
            ofs += 1;
        }
    }
#endif
    u8 res = fs_lseek(ofs, whence);
    if (res == FR_OK) {
        return true;
    }
    return false;
}
#endif // FUNC_MUSIC_EN

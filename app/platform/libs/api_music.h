#ifndef _API_MUSIC_H
#define _API_MUSIC_H

enum {
    MUSIC_STOP = 0,
    MUSIC_PAUSE,
    MUSIC_PLAYING,
};

enum {
    NONE_TYPE = 0,
    WAV_TYPE,
    MP3_TYPE,
    WMA_TYPE,
    APE_TYPE,
    FLAC_TYPE,
    SBCIO_TYPE,
    SBC_TYPE,
    AACIO_TYPE,
    AAC_TYPE,
    ESBC_TYPE,
};

enum {
    //decode msg
    MUSIC_MSG_STOP = 0,
    MUSIC_MSG_PAUSE,
    MUSIC_MSG_PLAY,

    //encode msg
    ENC_MSG_MP3 = 32,
    ENC_MSG_SCO,
    ENC_MSG_SBC,
    ENC_MSG_WAV,
    ENC_MSG_ADPCM,
};

enum {
    QSKIP_BACKWARD,
    QSKIP_FORWARD,
};

extern unsigned char avio_buf[556];
extern u32 rec_lba_num;

void music_control(u8 msg);
u8 get_music_dec_sta(void);
int music_decode_init(void);
int wav_decode_init(void);
int mp3_decode_init(void);
int wma_decode_init(void);
int ape_decode_init(void);
int flac_decode_init(void);
int sbcio_decode_init(void);
void sbc_decode_init(void);
int aacio_decode_init(void);
u16 music_get_total_time(void);
u16 music_get_cur_time(void);
void music_set_cur_time(u32 cur_time);
void music_qskip(bool direct, u8 second);      //快进快退控制 direct = 1为快退，0为快进
void music_qskip_end(void);
void music_set_jump(void *brkpt);
void music_get_breakpiont(void *brkpt);
u32 get_music_bitrate(void);
void music_stream_var_init(void);
int music_is_encrypt(u16 key);                 //返回0表示为加密音乐
void music_stream_decrypt(void *buf, unsigned int len);
void music_sync_kick(void);
void wav_sync_kick(void);
void wav_set_sync_kick(uint16_t sync_time);
void music_mp3_loopback_replay(void);
bool music_mp3_loopback_size(u32 start_byte_offset, u32 end_byte_offset, u8 type);   //音乐循环,首尾地址偏移，返回true表示设置成功
bool music_mp3_loopback_set_play(u32 second, u8 type);                               //设置循环播放的起点，返回true表示设置成功
bool music_mp3_loopback_reduce_end(u32 ms, u8 type);                                 //循环播放，尾部ms不播放，返回true表示设置成功
void inner_flash_music_loopback_en(bool en);										 //内部音乐资源循环使能
void esbc_sync_kick(void);
#endif // _API_MUSIC_H

#ifndef _API_SYSCLK_H_
#define _API_SYSCLK_H_

enum {
    SYS_2M,
    SYS_24M,
    SYS_48M,
    SYS_60M,
    SYS_80M,
    SYS_120M,
    SYS_147M,
    SYS_160M,
};

enum {
    INDEX_VOICE     = 0,
    INDEX_DECODE,
    INDEX_STACK,
    INDEX_KARAOK,
    INDEX_RES_PLAY,
    INDEX_RSVD1,
    INDEX_GFPS,
    INDEX_MAX_NB    = 7,
};

u8 get_cur_sysclk(void);
void set_sys_clk(u32 clk_sel);
bool sys_clk_free_all(void);
bool sys_clk_free(uint8_t index);
bool sys_clk_req(uint8_t index, uint8_t sys_clk);

#endif

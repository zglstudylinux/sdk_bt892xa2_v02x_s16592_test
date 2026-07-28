#include "include.h"

#if TME_APP_EN

AT(.com_text.opus)
void tme_opus_frame_energy_cal(u8 *buf, u32 samples)
{
    static u32 total = 0;
    static u16 cnt = 0;
    static u8 frame_cnt = 0;

    u8 i = 0;
    s16 *p_data = (s16*)buf;

    for(i=0;i<samples;i++){
        total += s_abs(p_data[i]);
        cnt++;
        if(cnt >= 320){     //20ms * 16K
            tme_app_var.frame_energy[frame_cnt++] = total/320;
            total = 0;
            cnt = 0;
            if(frame_cnt >= 4){
                frame_cnt = 0;
            }
        }
    }
}

#endif

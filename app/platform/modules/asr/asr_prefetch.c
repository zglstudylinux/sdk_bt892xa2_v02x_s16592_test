#include "include.h"
#include "asr.h"


#if ASR_PREFETCH_EN && ASR_EN

int8_t matrix_x0[512] AT(.matrix.x0);
int32_t matrix_sum AT(.ws_asr.sum);

#define BASE_ADDR   0xBE000
#define BASE_LEN    0x37000

typedef struct {
    u16 all_frame;
    u16 frame_len;
    u8 load_frame;
} tdnn_prenum_t;

AT(.com_rodata.tdnn)
const tdnn_prenum_t tdnn_prenum[7] = {
    {128, 120, 68},
    {128, 256, 32},
    {128, 256, 32},
    {128, 256, 32},
    {128, 256, 32},
    {128, 128, 64},
    {488, 128, 64},
};

typedef struct {
    u8 load_frame;
    u8 *buf;
} tdnn_cache_t;

typedef struct {
    u16 all_frame;
    u8 index;
    u8 toggle;
    u32 load_addr;
    tdnn_cache_t cache[2];
} tdnn_pretetch_t;

extern const int16_t tdnn_mean_1[128];
extern const int16_t tdnn_bias_1[128];
extern const float tdnn_var_1[128];
extern const int16_t tdnn_mean_2[128];
extern const int16_t tdnn_bias_2[128];
extern const float tdnn_var_2[128];
extern const int16_t tdnn_mean_3[128];
extern const int16_t tdnn_bias_3[128];
extern const float tdnn_var_3[128];
extern const int16_t tdnn_mean_4[128];
extern const int16_t tdnn_bias_4[128];
extern const float tdnn_var_4[128];
extern const int16_t tdnn_mean_5[128];
extern const int16_t tdnn_bias_5[128];
extern const float tdnn_var_5[128];
extern const int16_t tdnn_mean_6[128];
extern const int16_t tdnn_bias_6[128];
extern const float tdnn_var_6[128];
extern const int16_t tdnn_bias_7[488];
extern const float tdnn_scale_7[488];

int16_t tdnn_ram_mean1[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias1[128] AT(.ws_asr.test);
float tdnn_ram_var1[128] AT(.ws_asr.test);
int16_t tdnn_ram_mean2[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias2[128] AT(.ws_asr.test);
float tdnn_ram_var2[128] AT(.ws_asr.test);
int16_t tdnn_ram_mean3[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias3[128] AT(.ws_asr.test);
float tdnn_ram_var3[128] AT(.ws_asr.test);
int16_t tdnn_ram_mean4[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias4[128] AT(.ws_asr.test);
float tdnn_ram_var4[128] AT(.ws_asr.test);
int16_t tdnn_ram_mean5[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias5[128] AT(.ws_asr.test);
float tdnn_ram_var5[128] AT(.ws_asr.test);
int16_t tdnn_ram_mean6[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias6[128] AT(.ws_asr.test);
float tdnn_ram_var6[128] AT(.ws_asr.test);
int16_t tdnn_ram_bias7[488] AT(.ws_asr.test);
float tdnn_ram_scale7[488] AT(.ws_asr.test);

u8 tdnn_buffer[2][0x2000] AT(.ws_asr.test);
tdnn_pretetch_t tdnn_pretetch AT(.ws_asr.test);

u8 asr_prefetch_kisck;
int sum_buffer[0x1e8] AT(.vad.sum);

AT(.com_text.tdnn)
void tdnn_compute(int8_t *in_buf, float in_scale, int in_dim, int out_dim,
                  const int8_t *tdnn_weight, const int16_t *tdnn_bias,
                  const int16_t *tdnn_mean, const float *tdnn_var, const float *tdnn_scale,
                  float *out_buf, int last_layer) {
    int i;
    if (asr_prefetch_kisck) {
        return;
    }

    tdnn_pretetch_t *t = &tdnn_pretetch;
    const tdnn_prenum_t *p = &tdnn_prenum[t->index];
    u32 frame_len = p->frame_len * p->load_frame;
//    printf("tdnn_weight:%x out_dim:%d in_dim:%d index:%d frame_len:%d\n", tdnn_weight, out_dim, in_dim, t->index, frame_len);

    if (tdnn_mean == tdnn_mean_1) {
        tdnn_mean = tdnn_ram_mean1;
        tdnn_bias = tdnn_ram_bias1;
        tdnn_var = tdnn_ram_var1;
    } else if (tdnn_mean == tdnn_mean_2) {
        tdnn_mean = tdnn_ram_mean2;
        tdnn_bias = tdnn_ram_bias2;
        tdnn_var = tdnn_ram_var2;
    } else if (tdnn_mean == tdnn_mean_3) {
        tdnn_mean = tdnn_ram_mean3;
        tdnn_bias = tdnn_ram_bias3;
        tdnn_var = tdnn_ram_var3;
    } else if (tdnn_mean == tdnn_mean_4) {
        tdnn_mean = tdnn_ram_mean4;
        tdnn_bias = tdnn_ram_bias4;
        tdnn_var = tdnn_ram_var4;
    } else if (tdnn_mean == tdnn_mean_5) {
        tdnn_mean = tdnn_ram_mean5;
        tdnn_bias = tdnn_ram_bias5;
        tdnn_var = tdnn_ram_var5;
    } else if (tdnn_mean == tdnn_mean_6) {
        tdnn_mean = tdnn_ram_mean6;
        tdnn_bias = tdnn_ram_bias6;
        tdnn_var = tdnn_ram_var6;
    } else {
        tdnn_scale = tdnn_ram_scale7;
        tdnn_bias = tdnn_ram_bias7;
    }

    spiflash_lock();
//    GPIOBSET = BIT(1);///
    for (i = 0; i < out_dim; i++) {
        u16 offset = (i * in_dim) % frame_len;
        //load下一帧
        if (offset == 0) {
            u16 load_frame = 0;
            t->toggle ^= 1;
            if (t->all_frame == 0) {
                t->index++;
                if (t->index >= 7) {
                    t->index = 0;
                }
                p = &tdnn_prenum[t->index];
                t->all_frame = p->all_frame;
                load_frame = p->load_frame;
            } else {
                if (t->all_frame >= p->load_frame) {
                    load_frame = p->load_frame;
                } else {
                    load_frame = t->all_frame;
                }
            }
            u32 load_len = load_frame*p->frame_len;
//            GPIOBSET = BIT(2);///
            spiflash_read_wait();
            spiflash_read_kick(t->cache[t->toggle ^ 1].buf, t->load_addr, load_len);
//            GPIOBCLR = BIT(2);///
            t->all_frame -= load_frame;
            t->load_addr += load_len;
            if (t->load_addr >= (BASE_ADDR + BASE_LEN)) {
                t->load_addr = BASE_ADDR;
            }
        }

        const int8_t *weight = (const int8_t *)t->cache[t->toggle].buf + offset;
        matrix_sum = 0;

//        GPIOESET = BIT(4);///
        memcpy(matrix_x0, weight, in_dim);
        matrix_hw(&matrix_sum, matrix_x0, in_buf, in_dim);
//        GPIOECLR = BIT(4);///
        sum_buffer[i] = matrix_sum;
    }

    spiflash_read_wait();
    spiflash_unlock();

    for (i = 0; i < out_dim; i++) {
        matrix_sum = sum_buffer[i];
        int32_t z = matrix_sum * in_scale + tdnn_bias[i];
        if (!last_layer) {
            if (z <= 0) {
                z = 0;
            }
            out_buf[i] = (z - tdnn_mean[i]) * tdnn_var[i];
        } else {
            out_buf[i] = z * tdnn_scale[i];
        }
    }
}

ALIGNED(512)
void asr_prefetch_init_do(void)
{
    tdnn_pretetch_t *t = &tdnn_pretetch;
    memset(t, 0, sizeof(tdnn_pretetch_t));
    const tdnn_prenum_t *p = &tdnn_prenum[t->index];
    t->load_addr = BASE_ADDR;
    t->cache[0].buf = tdnn_buffer[0];
    t->cache[1].buf = tdnn_buffer[1];

    //先读
    u32 load_len = p->load_frame*p->frame_len;
    spiflash_lock();
    spiflash_read_kick(t->cache[0].buf, t->load_addr, load_len);
    spiflash_read_wait();
    t->cache[0].load_frame = p->load_frame;
    t->all_frame = p->all_frame - p->load_frame;        //已经load了一帧，减掉
    t->load_addr += load_len;

    spiflash_unlock();

    memcpy(tdnn_ram_mean1,     tdnn_mean_1,     sizeof(tdnn_mean_1));
    memcpy(tdnn_ram_bias1,     tdnn_bias_1,     sizeof(tdnn_bias_1));
    memcpy(tdnn_ram_var1,      tdnn_var_1,      sizeof(tdnn_var_1));
    memcpy(tdnn_ram_mean2,     tdnn_mean_2,     sizeof(tdnn_mean_2));
    memcpy(tdnn_ram_bias2,     tdnn_bias_2,     sizeof(tdnn_bias_2));
    memcpy(tdnn_ram_var2,      tdnn_var_2,      sizeof(tdnn_var_2));
    memcpy(tdnn_ram_mean3,     tdnn_mean_3,     sizeof(tdnn_mean_3));
    memcpy(tdnn_ram_bias3,     tdnn_bias_3,     sizeof(tdnn_bias_3));
    memcpy(tdnn_ram_var3,      tdnn_var_3,      sizeof(tdnn_var_3));
    memcpy(tdnn_ram_mean4,     tdnn_mean_4,     sizeof(tdnn_mean_4));
    memcpy(tdnn_ram_bias4,     tdnn_bias_4,     sizeof(tdnn_bias_4));
    memcpy(tdnn_ram_var4,      tdnn_var_4,      sizeof(tdnn_var_4));
    memcpy(tdnn_ram_mean5,     tdnn_mean_5,     sizeof(tdnn_mean_5));
    memcpy(tdnn_ram_bias5,     tdnn_bias_5,     sizeof(tdnn_bias_5));
    memcpy(tdnn_ram_var5,      tdnn_var_5,      sizeof(tdnn_var_5));
    memcpy(tdnn_ram_mean6,     tdnn_mean_6,     sizeof(tdnn_mean_6));
    memcpy(tdnn_ram_bias6,     tdnn_bias_6,     sizeof(tdnn_bias_6));
    memcpy(tdnn_ram_var6,      tdnn_var_6,      sizeof(tdnn_var_6));
    memcpy(tdnn_ram_bias7,     tdnn_bias_7,     sizeof(tdnn_bias_7));
    memcpy(tdnn_ram_scale7,    tdnn_scale_7,    sizeof(tdnn_scale_7));
}

void asr_prefetch_init(void)
{
    asr_prefetch_init_do();
    asr_prefetch_kisck = 5;
}

#endif

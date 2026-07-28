#include "config.h"
depend(0x01010200);
setflash(1, FLASH_SIZE, FLASH_ERASE_4K, FLASH_DUAL_READ, FLASH_QUAD_READ);
setdataseg(0x1000);
#if ASR_EN
setuserbin(ASR_BASE_ADDR, ASR_BASE_LEN, weight.bin);
#endif
setspace(FLASH_RESERVE_SIZE);
#if ((FLASH_SIZE == FSIZE_2M) && (AB_FOT_TYPE == AB_FOT_TYPE_PACK))
setunpack(unpack.bin);
setpkgarea(0x12C000, 0xC3000);
#endif
setkey(FUNCKEY_TWS);
addkey(FUNCKEY_ANC);
addkey(FUNCKEY_ANC_HYBRID);
addkey(FUNCKEY_SNDP_NS);
addkey(FUNCKEY_SNDP_DMIC);
addkey(FUNCKEY_LOW_LATENCY);
make(dcf_buf, header.bin, app.bin, res.bin, xcfg.bin, updater.bin);
save(dcf_buf, app.dcf);

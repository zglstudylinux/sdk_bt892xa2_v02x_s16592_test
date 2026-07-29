/*---------------------------------------------------------------------------/
/  Petit FatFs - BT892XA2 disk I/O bridge
/----------------------------------------------------------------------------/
/ This file replaces the skeleton `diskio.c` shipped with Petit FatFs.
/
/ It bridges the generic Petit FatFs layer to the BT892XA2 SDK's SDIO
/ driver (libdrivers.a, see app/platform/libs/api_sd.h). The SDK exposes
/ only full-sector read/write (sd0_read / sd0_write, 512 bytes each), but
/ Petit FatFs issues partial-sector accesses via disk_readp / disk_writep,
/ so we maintain a single 512-byte read cache and a single 512-byte write
/ staging buffer here.
/---------------------------------------------------------------------------*/

/* IMPORTANT: include include.h FIRST so api_fs.h (via api.h) gets a chance
 * to define FRESULT / DRESULT / RES_OK etc. before diskio.h / pff.h are
 * processed.  Otherwise both headers would try to define the same enums
 * and the compiler would emit "redeclaration of enumerator" errors.
 *
 * NB: the "#ifndef FRESULT" guards inside pff.h / diskio.h do NOT help
 * here -- #ifndef only checks for *preprocessor macros*, not for typedef
 * names.  Once api_fs.h has done `typedef enum { ... } FRESULT`, the
 * guard still sees FRESULT as undefined and re-enters the block.  The
 * real fix is to remove the enum definitions from our headers entirely
 * and rely on api_fs.h as the single source of truth. */
#include "include.h"          /* pulls in api_sd.h, port_sd.h via include.h */
#include "api_sd.h"           /* sd0_init / sd0_read / sd0_write / ...    */
#include "port_sd.h"          /* sd_gpio_init (GPIO mux + FUNCMCON0)        */

#include "diskio.h"           /* prototypes assume api_fs.h types are set */

/*--------------------------------------------------------------------------*/
/* Module-private state                                                     */
/*--------------------------------------------------------------------------*/

/* Single sector buffer, reused for both read-cache and write-staging.
 *
 * MEMORY PLACEMENT: we put this 512-byte buffer in the SDK's reserved
 * .diskio_buf section inside aram (see app/projects/standard/ram.ld,
 * .aram_music around 0x3800).  The .data (BSS) region only has 13 KB of
 * RAM and is already full from SDK code; even 1 KB of cache overflows
 * BSS by ~752 bytes.  aram has space at the .diskio_buf slot.
 *
 * Why ONE buffer and not two (read + write): the SDK's reserved
 * .aram_music slot has `. = 0x4000` as a hard upper bound, and the
 * SDK's own `.pff.buf` section inside the same slot consumes ~404 B
 * after our `.diskio_buf`.  With two 512-byte buffers (1024 B total)
 * we would push the location counter past 0x4000, causing the linker
 * to fail with "cannot move location counter backwards".  One 512-byte
 * buffer keeps us comfortably under 0x4000.
 *
 * Read-modify-write scheme: when Petit FatFs starts a write, we read
 * the target sector into s_cache (no-op if s_lba already matches), then
 * copy the new bytes in.  When finalize is called, we sd0_write the
 * whole 512 B back to disk.  4-byte alignment satisfies the SDIO DMA
 * baseline. */
static u8 s_cache[512] __attribute__((aligned(4), section(".diskio_buf")));
static u32 s_lba = 0xFFFFFFFFu;     /* LBA currently in s_cache */
static u16 s_wr_pos = 0;            /* 0..512: bytes of pending write */
static bool s_wr_in_progress = false;

/* Module-level error sticky bit (used to gate subsequent reads) */
static bool s_disk_ok = false;

/*--------------------------------------------------------------------------*/
/* Helpers                                                                  */
/*--------------------------------------------------------------------------*/

/* (Re)initialise the SD card. Called by pff_disk_initialize() and at every
 * mount retry from pff_mount(). Idempotent. */
static bool sd_reinit(void)
{
    sd_gpio_init(0);          /* configure PE5/6/7 + activate SD0 controller */
    delay_5ms(10);            /* let power / pull-ups settle */
    if (!sd0_init()) {
        return false;
    }
    s_lba = 0xFFFFFFFFu;
    s_wr_in_progress = false;
    s_wr_pos = 0;
    return true;
}

/* Pull a full 512 B sector into s_cache. Returns true on success.
 * If s_lba already matches and we are not in the middle of a write,
 * the function is a no-op (cache hit). */
static bool read_full_sector(u32 lba)
{
    if (s_lba == lba && !s_wr_in_progress) {
        return true;            /* already cached */
    }
    if (!sd0_read(s_cache, lba)) {
        s_lba = 0xFFFFFFFFu;
        return false;
    }
    s_lba = lba;
    return true;
}


/*--------------------------------------------------------------------------*/
/* Public Petit FatFs hooks                                                */
/*--------------------------------------------------------------------------*/

DSTATUS pff_disk_initialize (void)
{
    /* pff_disk_initialize is called every pff_mount(). We treat any failure as
     * "drive not ready" (STA_NOINIT) so pff_mount returns FR_NOT_READY. */
    if (sd_reinit()) {
        s_disk_ok = true;
        return 0;              /* OK */
    }
    s_disk_ok = false;
    return STA_NOINIT;
}


DRESULT pff_disk_readp (
    BYTE* buff,     /* Pointer to the destination object                */
    DWORD sector,   /* Sector number (LBA)                              */
    UINT offset,    /* Offset in the sector (0..511)                    */
    UINT count      /* Byte count (must be 1..512)                      */
)
{
    if (!s_disk_ok) {
        return RES_NOTRDY;
    }
    if (offset + count > 512) {
        return RES_PARERR;     /* out-of-sector span */
    }
    if (!buff) {
        return RES_PARERR;     /* Petit FatFs always supplies a dest */
    }
    if (!read_full_sector(sector)) {
        return RES_ERROR;
    }
    /* Copy the requested slice from cache to the caller's buffer */
    {
        u8 *src = s_cache + offset;
        while (count--) {
            *buff++ = *src++;
        }
    }
    return RES_OK;
}


DRESULT pff_disk_writep (
    const BYTE* buff,  /* NULL: control op; else: pointer to bytes to copy   */
    DWORD sc           /* LBA (init mode) or 0 (finalize mode) or byte count */
)
{
    if (!s_disk_ok) {
        return RES_NOTRDY;
    }

    if (buff == NULL) {
        if (sc != 0) {
            /* ---- Initiate: choose the destination sector ------------- */
            s_lba = sc;
            s_wr_pos = 0;
            s_wr_in_progress = true;
            /* Read the existing sector into s_cache so partial writes
             * don't leave stale bytes behind.  read_full_sector is a
             * no-op if s_lba already matches. */
            if (!read_full_sector(sc)) {
                s_wr_in_progress = false;
                return RES_ERROR;
            }
            return RES_OK;
        } else {
            /* ---- Finalize: commit s_cache to disk -------------------- */
            if (!s_wr_in_progress) {
                return RES_OK;   /* nothing to flush */
            }
            if (!sd0_write(s_cache, s_lba)) {
                s_wr_in_progress = false;
                return RES_ERROR;
            }
            s_wr_in_progress = false;
            s_wr_pos = 0;
            /* s_cache already holds the up-to-date data; no need to
             * invalidate. */
            return RES_OK;
        }
    } else {
        /* ---- Send data: copy `sc` bytes from buff into s_cache ------ */
        if (!s_wr_in_progress) {
            /* pff_write must have called the initiate op first; if not,
             * we accept data and treat it as an overwrite of the current
             * (cached) sector. This guards against Petit FatFs bugs. */
            return RES_ERROR;
        }
        if (s_wr_pos + sc > 512) {
            return RES_PARERR;
        }
        {
            u8       *dst  = s_cache + s_wr_pos;
            const u8 *src  = (const u8 *)buff;
            UINT      left = sc;
            while (left--) {
                *dst++ = *src++;
            }
            s_wr_pos += sc;
        }
        return RES_OK;
    }
}
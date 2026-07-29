/*-----------------------------------------------------------------------
/  PFF - Low level disk interface modlue include file    (C)ChaN, 2014
/----------------------------------------------------------------------------
/ BT892XA2 customisation:
/   The SDK's libplatform.a / libdrivers.a already define bare
/   disk_initialize / disk_readp / disk_writep symbols (used by
/   sfunc_record.c and the music-mode pipeline). To avoid linker duplicate
/   symbol errors, our Petit FatFs port uses the pff_disk_* prefixed names
/   and the macros at the bottom of this file rewrite ChaN's
/   `disk_initialize` etc. calls inside pff.c to our prefixed versions.
/   This way pff.c stays unmodified but links to our diskio.c.
/---------------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "pff.h"


/* Status of Disk Functions */
typedef BYTE	DSTATUS;


/* Results of Disk Functions
 *
 * NOTE: DRESULT is NOT defined here -- it comes from the SDK's
 * api_fs.h (app/platform/libs/api_fs.h).  We used to have a guarded
 * local copy with `#ifndef DRESULT`, but that only works for
 * preprocessor macros, not typedef names.  Once api_fs.h has done
 * `typedef enum { ... } DRESULT`, our guard would still try to
 * redefine it, causing "redeclaration of enumerator" errors.
 *
 * Callers of diskio.h MUST include api_fs.h (via include.h) first.
 */


/*---------------------------------------*/
/* Prototypes for our prefixed disk I/O functions.
 *
 * Note: only pff.c (compiled with this header included) sees the macros
 * that redirect `disk_*` -> `pff_disk_*`. SDK callers of the bare names
 * continue to bind to libplatform.a's implementations. */

DSTATUS pff_disk_initialize (void);
DRESULT pff_disk_readp (BYTE* buff, DWORD sector, UINT offser, UINT count);
DRESULT pff_disk_writep (const BYTE* buff, DWORD sc);

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */

/* The bare-name -> prefixed-name shim (disk_initialize -> pff_disk_initialize
 * etc.) lives in pff_compat.h and is included ONLY by pff.c.  Defining the
 * shim here would leak it into every translation unit that includes
 * diskio.h, including those that later include api_fs.h -- that would
 * rewrite the SDK's disk_readp/disk_writep prototypes to ours and create
 * signature conflicts.  Keep this header side-effect free. */

#ifdef __cplusplus
}
#endif

#endif	/* _DISKIO_DEFINED */

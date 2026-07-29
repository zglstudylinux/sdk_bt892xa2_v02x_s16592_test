/*---------------------------------------------------------------------------/
/  Petit FatFs - Configuration file  (BT892XA2 customisation)
/---------------------------------------------------------------------------*/
/*
 * The layout below follows ChaN's R0.03a pffconf.h; only the enabled features
 * have been flipped to suit Phase 4 of the BT892XA2 SDK learning path:
 *   - PF_USE_READ  = 1  (we need pf_read for mount validation + read tests)
 *   - PF_USE_WRITE = 1  (Phase 4.4 explicitly tests file write)
 *   - PF_USE_DIR   = 0  (directory listing is out of scope; saves code space)
 *   - PF_USE_LSEEK = 1  (we need to seek to verify overwrite)
 *   - FAT12/16/32  = all enabled so the same binary works on any SD card
 *
 * The code page table is unused (PF_USE_LCC == 0) so the ~256 byte lookup
 * does not get linked into the final image, saving precious .bss.
 */

#ifndef PFCONF_DEF
#define PFCONF_DEF 8088	/* Revision ID */

/*---------------------------------------------------------------------------/
/ Function Configurations (0:Disable, 1:Enable)
/---------------------------------------------------------------------------*/

#define	PF_USE_READ		1	/* pf_read() function */
#define	PF_USE_DIR		0	/* pf_opendir() and pf_readdir() function */
#define	PF_USE_LSEEK	1	/* pf_lseek() function */
#define	PF_USE_WRITE	1	/* pf_write() function */

#define PF_FS_FAT12		1	/* FAT12 */
#define PF_FS_FAT16		1	/* FAT16 */
#define PF_FS_FAT32		1	/* FAT32 */


/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define PF_USE_LCC		0	/* Allow lower case ASCII and non-ASCII chars */

#define	PF_CODE_PAGE	437
/* See upstream file for full PF_CODE_PAGE table. */

#endif /* PF_CONF */
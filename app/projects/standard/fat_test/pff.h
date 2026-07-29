/*---------------------------------------------------------------------------/
/  Petit FatFs - FAT file system module include file  R0.03a
/----------------------------------------------------------------------------/
/ Petit FatFs module is an open source software to implement FAT file system to
/ small embedded systems. This is a free software and is opened for education,
/ research and commercial developments under license policy of following trems.
/
/  Copyright (C) 2019, ChaN, all right reserved.
/
/ * The Petit FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial use UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/----------------------------------------------------------------------------*/

#ifndef PF_DEFINED
#define PF_DEFINED	8088	/* Revision ID */

#ifdef __cplusplus
extern "C" {
#endif

#include "pffconf.h"

#if PF_DEFINED != PFCONF_DEF
#error Wrong configuration file (pffconf.h).
#endif


/* Integer types used for FatFs API */

/* BT892XA2 SDK: BYTE/UINT/WORD/DWORD already defined in typedef.h
 * (included via include.h). Each typedef is guarded individually so the
 * upstream ChaN code stays compilable on stock hosts and just no-ops when
 * the SDK has already provided the types. Layout matches ChaN's intent:
 *   BYTE  = unsigned char  (8 bit)
 *   UINT  = unsigned int   (16 or 32 bit)
 *   WORD  = unsigned short (16 bit)
 *   WCHAR = unsigned short (16 bit)
 *   DWORD = unsigned long  (32 bit)
 */
#ifndef PF_INTDEF

#if defined(_WIN32)	/* Main development platform */
#include <windows.h>
#else /* Either C99 or earlier; we don't include stdint.h here to avoid
         clashes with the SDK's typedef.h. */

#ifndef BYTE
typedef unsigned char	BYTE;	/* char must be 8-bit */
#endif

#ifndef UINT
typedef unsigned int	UINT;	/* int must be 16-bit or 32-bit */
#endif

#ifndef WORD
typedef unsigned short	WORD;	/* 16-bit unsigned integer */
#endif

#ifndef WCHAR
typedef unsigned short	WCHAR;	/* 16-bit unsigned integer */
#endif

#ifndef DWORD
typedef unsigned long	DWORD;	/* 32-bit unsigned integer */
#endif

#endif
#define PF_INTDEF 1

#endif /* PF_INTDEF */


#if PF_FS_FAT32
#define	CLUST	DWORD
#else
#define	CLUST	WORD
#endif


/* File system object structure */

typedef struct {
	BYTE	fs_type;	/* FAT sub type */
	BYTE	flag;		/* File status flags */
	BYTE	csize;		/* Number of sectors per cluster */
	BYTE	pad1;
	WORD	n_rootdir;	/* Number of root directory entries (0 on FAT32) */
	CLUST	n_fatent;	/* Number of FAT entries (= number of clusters + 2) */
	DWORD	fatbase;	/* FAT start sector */
	DWORD	dirbase;	/* Root directory start sector (Cluster# on FAT32) */
	DWORD	database;	/* Data start sector */
	DWORD	fptr;		/* File R/W pointer */
	DWORD	fsize;		/* File size */
	CLUST	org_clust;	/* File start cluster */
	CLUST	curr_clust;	/* File current cluster */
	DWORD	dsect;		/* File current data sector */
} FATFS;



/* Directory object structure */

typedef struct {
	WORD	index;		/* Current read/write index number */
	BYTE*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
	CLUST	sclust;		/* Table start cluster (0:Static table) */
	CLUST	clust;		/* Current cluster */
	DWORD	sect;		/* Current sector */
} DIR;



/* File status structure */

typedef struct {
	DWORD	fsize;		/* File size */
	WORD	fdate;		/* Last modified date */
	WORD	ftime;		/* Last modified time */
	BYTE	fattrib;	/* Attribute */
	char	fname[13];	/* File name */
} FILINFO;



/* File function return code (FRESULT)
 *
 * NOTE: FRESULT is NOT defined here -- it comes from the SDK's
 * api_fs.h (app/platform/libs/api_fs.h).  We used to have a guarded
 * local copy with `#ifndef FRESULT`, but `#ifndef` only tests for
 * preprocessor macros, not typedef names.  Since api_fs.h declares
 * FRESULT as `typedef enum {...}`, the guard doesn't help and we
 * end up with "redeclaration of enumerator" errors.
 *
 * Callers of pff.h MUST include api_fs.h (via include.h) first.
 */



/*--------------------------------------------------------------*/
/* Petit FatFs module application interface                     */

FRESULT pff_mount (FATFS* fs);								/* Mount/Unmount a logical drive */
FRESULT pff_open (const char* path);							/* Open a file */
FRESULT pff_read (void* buff, UINT btr, UINT* br);			/* Read data from the open file */
FRESULT pff_write (const void* buff, UINT btw, UINT* bw);	/* Write data to the open file */
FRESULT pff_lseek (DWORD ofs);								/* Move file pointer of the open file */
FRESULT pff_opendir (DIR* dj, const char* path);				/* Open a directory */
FRESULT pff_readdir (DIR* dj, FILINFO* fno);					/* Read a directory item from the open directory */



/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File status flag (FATFS.flag) */
#define	FA_OPENED	0x01
#define	FA_WPRT		0x02
#define	FA__WIP		0x40


/* FAT sub type (FATFS.fs_type) */
#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3


/* File attribute bits for directory entry */

#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_MASK	0x3F	/* Mask of defined bits */


#ifdef __cplusplus
}
#endif

#endif /* _PFATFS */

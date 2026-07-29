/*---------------------------------------------------------------------------/
/  pff_compat.h - Symbol-name shim for Petit FatFs on BT892XA2
/---------------------------------------------------------------------------/
/
/ The SDK's libplatform.a / libdrivers.a already define bare
/   disk_initialize / disk_readp / disk_writep
/ symbols (see Output/bin/map.txt .text.disk_* entries).  Our Petit FatFs
/ port provides prefixed equivalents (pff_disk_*) so we can link both the
/ SDK's music/record pipeline and our FS port into the same binary.
/
/ To make pff.c call our prefixed functions WITHOUT modifying ChaN's
/ upstream source, this header maps the bare names to the prefixed ones at
/ the preprocessor level.  Only pff.c should include this header -- if any
/ other file included it, the SDK's api_fs.h declarations would also get
/ rewritten and clash with our prototypes.
/
/ Note: the macros must be defined BEFORE diskio.h is included (the
/ pff_disk_* prototypes live there).
/---------------------------------------------------------------------------*/

#ifndef _PFF_COMPAT_H
#define _PFF_COMPAT_H

/* Rewrite ChaN's bare symbol names to our prefixed ones. */
#define disk_initialize	pff_disk_initialize
#define disk_readp		pff_disk_readp
#define disk_writep		pff_disk_writep

#endif /* _PFF_COMPAT_H */
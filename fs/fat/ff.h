/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module include file  R0.08b    (C)ChaN, 2011
/----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following trems.
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/----------------------------------------------------------------------------*/

#ifndef _FATFS
#define _FATFS	8237	/* Revision ID */

#include <asm/unaligned.h>
#include <linux/list.h>

#include "integer.h"	/* Basic integer types */
#include "ffconf.h"	/* FatFs configuration options */

#if _FATFS != _FFCONF
#error Wrong configuration file (ffconf.h).
#endif

/* Type of path name strings on FatFs API */

#if _LFN_UNICODE			/* Unicode string */
#ifndef CONFIG_FS_FAT_LFN
#error _LFN_UNICODE must be 0 in non-LFN cfg.
#endif
#ifndef _INC_TCHAR
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#endif

#else						/* ANSI/OEM string */
#ifndef _INC_TCHAR
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif

#endif



/* File system object structure (FATFS) */

typedef struct {
	BYTE	fs_type;	/* FAT sub-type (0:Not mounted) */
	BYTE	drv;		/* Physical drive number */
	BYTE	csize;		/* Sectors per cluster (1,2,4...128) */
	BYTE	n_fats;		/* Number of FAT copies (1,2) */
	BYTE	wflag;		/* win[] dirty flag (1:must be written back) */
	BYTE	fsi_flag;	/* fsinfo dirty flag (1:must be written back) */
	WORD	n_rootdir;	/* Number of root directory entries (FAT12/16) */
#if _MAX_SS != 512
	WORD	ssize;		/* Bytes per sector (512,1024,2048,4096) */
#endif
#ifdef CONFIG_FS_FAT_WRITE
	DWORD	last_clust;	/* Last allocated cluster */
	DWORD	free_clust;	/* Number of free clusters */
	DWORD	fsi_sector;	/* fsinfo sector (FAT32) */
#endif
	DWORD	n_fatent;	/* Number of FAT entries (= number of clusters + 2) */
	DWORD	fsize;		/* Sectors per FAT */
	DWORD	fatbase;	/* FAT start sector */
	DWORD	dirbase;	/* Root directory start sector (FAT32:Cluster#) */
	DWORD	database;	/* Data start sector */
	DWORD	winsect;	/* Current sector appearing in the win[] */
	BYTE	win[_MAX_SS];	/* Disk access window for Directory, FAT (and Data on tiny cfg) */
	void	*userdata;	/* User data, ff core does not touch this */
	struct list_head dirtylist;
} FATFS;



/* File object structure (FIL) */

typedef struct {
	FATFS*	fs;		/* Pointer to the owner file system object */
	BYTE	flag;		/* File status flags */
	BYTE	pad1;
	DWORD	fptr;		/* File read/write pointer (0 on file open) */
	DWORD	fsize;		/* File size */
	DWORD	sclust;		/* File start cluster (0 when fsize==0) */
	DWORD	clust;		/* Current cluster */
	DWORD	dsect;		/* Current data sector */
#ifdef CONFIG_FS_FAT_WRITE
	DWORD	dir_sect;	/* Sector containing the directory entry */
	BYTE*	dir_ptr;	/* Ponter to the directory entry in the window */
#endif
#if _USE_FASTSEEK
	DWORD*	cltbl;		/* Pointer to the cluster link map table (null on file open) */
#endif
#if _FS_SHARE
	UINT	lockid;		/* File lock ID (index of file semaphore table) */
#endif
#if !_FS_TINY
	BYTE	buf[_MAX_SS];	/* File data read/write buffer */
#endif
} FIL;



/* Directory object structure (DIR) */

typedef struct {
	FATFS*	fs;			/* Pointer to the owner file system object */
	WORD	index;			/* Current read/write index number */
	DWORD	sclust;			/* Table start cluster (0:Root dir) */
	DWORD	clust;			/* Current cluster */
	DWORD	sect;			/* Current sector */
	BYTE*	dir;			/* Pointer to the current SFN entry in the win[] */
	BYTE*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
#ifdef CONFIG_FS_FAT_LFN
	WCHAR*	lfn;			/* Pointer to the LFN working buffer */
	WORD	lfn_idx;		/* Last matched LFN index number (0xFFFF:No LFN) */
#endif
} FF_DIR;



/* File status structure (FILINFO) */

typedef struct {
	DWORD	fsize;			/* File size */
	WORD	fdate;			/* Last modified date */
	WORD	ftime;			/* Last modified time */
	BYTE	fattrib;		/* Attribute */
	TCHAR	fname[13];		/* Short file name (8.3 format) */
#ifdef CONFIG_FS_FAT_LFN
	TCHAR*	lfname;			/* Pointer to the LFN buffer */
	UINT 	lfsize;			/* Size of LFN buffer in TCHAR */
#endif
} FILINFO;

/*--------------------------------------------------------------*/
/* FatFs module application interface                           */

int f_mount (FATFS*);					/* Mount/Unmount a logical drive */
int f_open (FATFS*, FIL*, const TCHAR*, BYTE);		/* Open or create a file */
int f_read (FIL*, void*, UINT, UINT*);			/* Read data from a file */
int f_lseek (FIL*, DWORD);				/* Move file pointer of a file object */
int f_close (FIL*);					/* Close an open file object */
int f_opendir (FATFS*, FF_DIR*, const TCHAR*);		/* Open an existing directory */
int f_readdir (FF_DIR*, FILINFO*);			/* Read a directory item */
int f_stat (FATFS*, const TCHAR*, FILINFO*);		/* Get file status */
int f_write (FIL*, const void*, UINT, UINT*);		/* Write data to a file */
int f_getfree (FATFS*, const TCHAR*, DWORD*);		/* Get number of free clusters on the drive */
int f_truncate (FIL*);					/* Truncate file */
int f_sync (FIL*);					/* Flush cached data of a writing file */
int f_unlink (FATFS*, const TCHAR*);			/* Delete an existing file or directory */
int f_mkdir (FATFS*, const TCHAR*);			/* Create a new directory */
int f_chmod (FATFS*, const TCHAR*, BYTE, BYTE);		/* Change attriburte of the file/dir */
int f_utime (FATFS*, const TCHAR*, const FILINFO*);	/* Change timestamp of the file/dir */
int f_rename (FATFS*, const TCHAR*, const TCHAR*);	/* Rename/Move a file or directory */
int f_forward (FIL*, UINT(*)(const BYTE*,UINT), UINT, UINT*);	/* Forward data to the stream */
int f_mkfs (BYTE, BYTE, UINT);				/* Create a file system on the drive */
int f_chdrive (BYTE);					/* Change current drive */
int f_chdir (const TCHAR*);				/* Change current directory */
int f_getcwd (TCHAR*, UINT);				/* Get current directory */
int f_putc (TCHAR, FIL*);				/* Put a character to the file */
int f_puts (const TCHAR*, FIL*);			/* Put a string to the file */
int f_printf (FIL*, const TCHAR*, ...);			/* Put a formatted string to the file */
TCHAR* f_gets (TCHAR*, int, FIL*);			/* Get a string from the file */

#ifndef EOF
#define EOF (-1)
#endif

#define f_eof(fp) (((fp)->fptr == (fp)->fsize) ? 1 : 0)
#define f_error(fp) (((fp)->flag & FA__ERROR) ? 1 : 0)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->fsize)

/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

/* RTC function */
DWORD get_fattime (void);

/* Unicode support functions */
WCHAR ff_convert (WCHAR, UINT);	/* OEM-Unicode bidirectional conversion */
WCHAR ff_wtoupper (WCHAR);	/* Unicode upper-case conversion */

/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access control and file status flags (FIL.flag) */

#define	FA_READ			0x01
#define	FA_OPEN_EXISTING	0x00
#define FA__ERROR		0x80

#define	FA_WRITE		0x02
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define FA__WRITTEN		0x20
#define FA__DIRTY		0x40

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


/* Fast seek function */
#define CREATE_LINKMAP	0xFFFFFFFF

/* Multi-byte word access macros  */

#define	LD_WORD(ptr)		get_unaligned_le16(ptr)
#define	LD_DWORD(ptr)		get_unaligned_le32(ptr)
#define	ST_WORD(ptr,val)	put_unaligned_le16(val, ptr)
#define	ST_DWORD(ptr,val)	put_unaligned_le32(val, ptr)

#endif /* _FATFS */

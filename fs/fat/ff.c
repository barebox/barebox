/*----------------------------------------------------------------------------/
/  FatFs - FAT file system module  R0.08b                 (C)ChaN, 2011
/-----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following terms.
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Feb 26,'06 R0.00  Prototype.
/
/ Apr 29,'06 R0.01  First stable version.
/
/ Jun 01,'06 R0.02  Added FAT12 support.
/                   Removed unbuffered mode.
/                   Fixed a problem on small (<32M) partition.
/ Jun 10,'06 R0.02a Added a configuration option (_FS_MINIMUM).
/
/ Sep 22,'06 R0.03  Added f_rename().
/                   Changed option _FS_MINIMUM to _FS_MINIMIZE.
/ Dec 11,'06 R0.03a Improved cluster scan algorithm to write files fast.
/                   Fixed f_mkdir() creates incorrect directory on FAT32.
/
/ Feb 04,'07 R0.04  Supported multiple drive system.
/                   Changed some interfaces for multiple drive system.
/                   Changed f_mountdrv() to f_mount().
/                   Added f_mkfs().
/ Apr 01,'07 R0.04a Supported multiple partitions on a physical drive.
/                   Added a capability of extending file size to f_lseek().
/                   Added minimization level 3.
/                   Fixed an endian sensitive code in f_mkfs().
/ May 05,'07 R0.04b Added a configuration option _USE_NTFLAG.
/                   Added FSInfo support.
/                   Fixed DBCS name can result FR_INVALID_NAME.
/                   Fixed short seek (<= csize) collapses the file object.
/
/ Aug 25,'07 R0.05  Changed arguments of f_read(), f_write() and f_mkfs().
/                   Fixed f_mkfs() on FAT32 creates incorrect FSInfo.
/                   Fixed f_mkdir() on FAT32 creates incorrect directory.
/ Feb 03,'08 R0.05a Added f_truncate() and f_utime().
/                   Fixed off by one error at FAT sub-type determination.
/                   Fixed btr in f_read() can be mistruncated.
/                   Fixed cached sector is not flushed when create and close without write.
/
/ Apr 01,'08 R0.06  Added fputc(), fputs(), fprintf() and fgets().
/                   Improved performance of f_lseek() on moving to the same or following cluster.
/
/ Apr 01,'09 R0.07  Merged Tiny-FatFs as a configuration option. (_FS_TINY)
/                   Added long file name feature.
/                   Added multiple code page feature.
/                   Added re-entrancy for multitask operation.
/                   Added auto cluster size selection to f_mkfs().
/                   Added rewind option to f_readdir().
/                   Changed result code of critical errors.
/                   Renamed string functions to avoid name collision.
/ Apr 14,'09 R0.07a Separated out OS dependent code on reentrant cfg.
/                   Added multiple sector size feature.
/ Jun 21,'09 R0.07c Fixed f_unlink() can return 0 on error.
/                   Fixed wrong cache control in f_lseek().
/                   Added relative path feature.
/                   Added f_chdir() and f_chdrive().
/                   Added proper case conversion to extended char.
/ Nov 03,'09 R0.07e Separated out configuration options from ff.h to ffconf.h.
/                   Fixed f_unlink() fails to remove a sub-dir on _FS_RPATH.
/                   Fixed name matching error on the 13 char boundary.
/                   Added a configuration option, _LFN_UNICODE.
/                   Changed f_readdir() to return the SFN with always upper case on non-LFN cfg.
/
/ May 15,'10 R0.08  Added a memory configuration option. (_USE_LFN = 3)
/                   Added file lock feature. (_FS_SHARE)
/                   Added fast seek feature. (_USE_FASTSEEK)
/                   Changed some types on the API, XCHAR->TCHAR.
/                   Changed fname member in the FILINFO structure on Unicode cfg.
/                   String functions support UTF-8 encoding files on Unicode cfg.
/ Aug 16,'10 R0.08a Added f_getcwd(). (_FS_RPATH = 2)
/                   Added sector erase feature. (_USE_ERASE)
/                   Moved file lock semaphore table from fs object to the bss.
/                   Fixed a wrong directory entry is created on non-LFN cfg when the given name contains ';'.
/                   Fixed f_mkfs() creates wrong FAT32 volume.
/ Jan 15,'11 R0.08b Fast seek feature is also applied to f_read() and f_write().
/                   f_lseek() reports required table size on creating CLMP.
/                   Extended format syntax of f_printf function.
/                   Ignores duplicated directory separators in given path names.
/---------------------------------------------------------------------------*/

#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <filetype.h>
#include "ff.h"			/* FatFs configurations and declarations */
#include "diskio.h"		/* Declarations of low level disk I/O functions */

#if _FATFS != 8237
#error Wrong include file (ff.h).
#endif

/* Definitions on sector size */
#if _MAX_SS != 512 && _MAX_SS != 1024 && _MAX_SS != 2048 && _MAX_SS != 4096
#error Wrong sector size.
#endif
#if _MAX_SS != 512
#define	SS(fs)	((fs)->ssize)	/* Multiple sector size */
#else
#define	SS(fs)	512U		/* Fixed sector size */
#endif

#define	ABORT(fs, res)		{ fp->flag |= FA__ERROR; return res; }

/* Misc definitions */
#define LD_CLUST(dir)	(((DWORD)LD_WORD(dir+DIR_FstClusHI)<<16) | LD_WORD(dir+DIR_FstClusLO))
#define ST_CLUST(dir,cl) {ST_WORD(dir+DIR_FstClusLO, cl); ST_WORD(dir+DIR_FstClusHI, (DWORD)cl>>16);}


/* DBCS code ranges and SBCS extend char conversion table */

/* Codepage 437 (US OEM) */
#define _DF1S	0
#define _EXCVT { \
	0x80,0x9A,0x90,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F, \
	0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
	0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF, \
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF, \
}

#define IsDBCS1(c)	0
#define IsDBCS2(c)	0

/* Name status flags */
#define NS		11	/* Index of name status byte in fn[] */
#define NS_LOSS		0x01	/* Out of 8.3 format */
#define NS_LFN		0x02	/* Force to create LFN entry */
#define NS_LAST		0x04	/* Last segment */
#define NS_BODY		0x08	/* Lower case flag (body) */
#define NS_EXT		0x10	/* Lower case flag (ext) */
#define NS_DOT		0x20	/* Dot entry */


/* FAT sub-type boundaries */
/* Note that the FAT spec by Microsoft says 4085 but Windows works with 4087! */
#define MIN_FAT16	4086	/* Minimum number of clusters for FAT16 */
#define	MIN_FAT32	65526	/* Minimum number of clusters for FAT32 */


/* FatFs refers the members in the FAT structures as byte array instead of
/ structure member because the structure is not binary compatible between
/ different platforms */

#define BS_jmpBoot		0	/* Jump instruction (3) */
#define BS_OEMName		3	/* OEM name (8) */
#define BPB_BytsPerSec		11	/* Sector size [byte] (2) */
#define BPB_SecPerClus		13	/* Cluster size [sector] (1) */
#define BPB_RsvdSecCnt		14	/* Size of reserved area [sector] (2) */
#define BPB_NumFATs		16	/* Number of FAT copies (1) */
#define BPB_RootEntCnt		17	/* Number of root dir entries for FAT12/16 (2) */
#define BPB_TotSec16		19	/* Volume size [sector] (2) */
#define BPB_Media		21	/* Media descriptor (1) */
#define BPB_FATSz16		22	/* FAT size [sector] (2) */
#define BPB_SecPerTrk		24	/* Track size [sector] (2) */
#define BPB_NumHeads		26	/* Number of heads (2) */
#define BPB_HiddSec		28	/* Number of special hidden sectors (4) */
#define BPB_TotSec32		32	/* Volume size [sector] (4) */
#define BS_DrvNum		36	/* Physical drive number (2) */
#define BS_BootSig		38	/* Extended boot signature (1) */
#define BS_VolID		39	/* Volume serial number (4) */
#define BS_VolLab		43	/* Volume label (8) */
#define BS_FilSysType		54	/* File system type (1) */
#define BPB_FATSz32		36	/* FAT size [sector] (4) */
#define BPB_ExtFlags		40	/* Extended flags (2) */
#define BPB_FSVer		42	/* File system version (2) */
#define BPB_RootClus		44	/* Root dir first cluster (4) */
#define BPB_FSInfo		48	/* Offset of FSInfo sector (2) */
#define BPB_BkBootSec		50	/* Offset of backup boot sectot (2) */
#define BS_DrvNum32		64	/* Physical drive number (2) */
#define BS_BootSig32		66	/* Extended boot signature (1) */
#define BS_VolID32		67	/* Volume serial number (4) */
#define BS_VolLab32		71	/* Volume label (8) */
#define BS_FilSysType32		82	/* File system type (1) */
#define	FSI_LeadSig		0	/* FSI: Leading signature (4) */
#define	FSI_StrucSig		484	/* FSI: Structure signature (4) */
#define	FSI_Free_Count		488	/* FSI: Number of free clusters (4) */
#define	FSI_Nxt_Free		492	/* FSI: Last allocated cluster (4) */
#define MBR_Table		446	/* MBR: Partition table offset (2) */
#define	SZ_PTE			16	/* MBR: Size of a partition table entry */
#define BS_55AA			510	/* Boot sector signature (2) */

#define	DIR_Name		0	/* Short file name (11) */
#define	DIR_Attr		11	/* Attribute (1) */
#define	DIR_NTres		12	/* NT flag (1) */
#define	DIR_CrtTime		14	/* Created time (2) */
#define	DIR_CrtDate		16	/* Created date (2) */
#define	DIR_FstClusHI		20	/* Higher 16-bit of first cluster (2) */
#define	DIR_WrtTime		22	/* Modified time (2) */
#define	DIR_WrtDate		24	/* Modified date (2) */
#define	DIR_FstClusLO		26	/* Lower 16-bit of first cluster (2) */
#define	DIR_FileSize		28	/* File size (4) */
#define	LDIR_Ord		0	/* LFN entry order and LLE flag (1) */
#define	LDIR_Attr		11	/* LFN attribute (1) */
#define	LDIR_Type		12	/* LFN type (1) */
#define	LDIR_Chksum		13	/* Sum of corresponding SFN entry */
#define	LDIR_FstClusLO		26	/* Filled by zero (0) */
#define	SZ_DIR			32	/* Size of a directory entry */
#define	LLE			0x40	/* Last long entry flag in LDIR_Ord */
#define	DDE			0xE5	/* Deleted directory enrty mark in DIR_Name[0] */
#define	NDDE			0x05	/* Replacement of a character collides with DDE */

#ifndef CONFIG_FS_FAT_LFN
#define	DEF_NAMEBUF		BYTE sfn[12]
#define INIT_BUF(dobj)		(dobj).fn = sfn
#define	FREE_BUF()

#else
static WCHAR LfnBuf[_MAX_LFN+1];
#define	DEF_NAMEBUF		BYTE sfn[12]
#define INIT_BUF(dobj)		{ (dobj).fn = sfn; (dobj).lfn = LfnBuf; }
#define	FREE_BUF()
#endif

/*
 * Change window offset
 */
struct fat_sector {
	DWORD sector;
	struct list_head list;
	unsigned char data[0];
};

/*-----------------------------------------------------------------------*/
/* Change window offset                                                  */
/*-----------------------------------------------------------------------*/

static
int move_window (
	FATFS *fs,		/* File system object */
	DWORD sector	/* Sector number to make appearance in the fs->win[] */
)					/* Move to zero only writes back dirty window */
{
	DWORD wsect;


	wsect = fs->winsect;
	if (wsect != sector) {	/* Changed current window */
#ifdef CONFIG_FS_FAT_WRITE
		if (fs->wflag) {	/* Write back dirty window if needed */
			if (disk_write(fs, fs->win, wsect, 1) != RES_OK)
				return -EIO;
			fs->wflag = 0;
			if (wsect < (fs->fatbase + fs->fsize)) {	/* In FAT area */
				BYTE nf;
				for (nf = fs->n_fats; nf > 1; nf--) {	/* Reflect the change to all FAT copies */
					wsect += fs->fsize;
					disk_write(fs, fs->win, wsect, 1);
				}
			}
		}
#endif
		if (sector) {
			if (disk_read(fs, fs->win, sector, 1) != RES_OK)
				return -EIO;
			fs->winsect = sector;
		}
	}

	return 0;
}

/*
 * Clean-up cached data
 */
#ifdef CONFIG_FS_FAT_WRITE
static
int sync (	/* 0: successful, -EIO: failed */
	FATFS *fs	/* File system object */
)
{
	int res;

	res = move_window(fs, 0);
	if (res == 0) {
		/* Update FSInfo sector if needed */
		if (fs->fs_type == FS_FAT32 && fs->fsi_flag) {
			fs->winsect = 0;
			/* Create FSInfo structure */
			memset(fs->win, 0, 512);
			ST_WORD(fs->win+BS_55AA, 0xAA55);
			ST_DWORD(fs->win+FSI_LeadSig, 0x41615252);
			ST_DWORD(fs->win+FSI_StrucSig, 0x61417272);
			ST_DWORD(fs->win+FSI_Free_Count, fs->free_clust);
			ST_DWORD(fs->win+FSI_Nxt_Free, fs->last_clust);
			/* Write it into the FSInfo sector */
			disk_write(fs, fs->win, fs->fsi_sector, 1);
			fs->fsi_flag = 0;
		}
		/* Make sure that no pending write process in the physical drive */
		if (disk_ioctl(fs, CTRL_SYNC, (void*)0) != RES_OK)
			res = -EIO;
	}

	return res;
}
#endif

/*
 * Get sector# from cluster#
 */
static DWORD clust2sect (	/* !=0: Sector number, 0: Failed - invalid cluster# */
	FATFS *fs,	/* File system object */
	DWORD clst	/* Cluster# to be converted */
)
{
	clst -= 2;
	if (clst >= (fs->n_fatent - 2))
		return 0;		/* Invalid cluster */
	return clst * fs->csize + fs->database;
}

/*
 * FAT access - Read value of a FAT entry
 */
static DWORD get_fat (	/* 0xFFFFFFFF:Disk error, 1:Internal error, Else:Cluster status */
	FATFS *fs,	/* File system object */
	DWORD clst	/* Cluster# to get the link information */
)
{
	UINT wc, bc;
	BYTE *p;


	if (clst < 2 || clst >= fs->n_fatent)	/* Chack range */
		return 1;

	switch (fs->fs_type) {
	case FS_FAT12 :
		bc = (UINT)clst; bc += bc / 2;
		if (move_window(fs, fs->fatbase + (bc / SS(fs))))
			break;
		wc = fs->win[bc % SS(fs)]; bc++;
		if (move_window(fs, fs->fatbase + (bc / SS(fs))))
			break;
		wc |= fs->win[bc % SS(fs)] << 8;
		return (clst & 1) ? (wc >> 4) : (wc & 0xFFF);

	case FS_FAT16 :
		if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 2))))
			break;
		p = &fs->win[clst * 2 % SS(fs)];
		return LD_WORD(p);

	case FS_FAT32 :
		if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 4))))
			break;
		p = &fs->win[clst * 4 % SS(fs)];
		return LD_DWORD(p) & 0x0FFFFFFF;
	}

	return 0xFFFFFFFF;	/* An error occurred at the disk I/O layer */
}




/*
 * FAT access - Change value of a FAT entry
 */
#ifdef CONFIG_FS_FAT_WRITE

static int put_fat (
	FATFS *fs,	/* File system object */
	DWORD clst,	/* Cluster# to be changed in range of 2 to fs->n_fatent - 1 */
	DWORD val	/* New value to mark the cluster */
)
{
	UINT bc;
	BYTE *p;
	int res;


	if (clst < 2 || clst >= fs->n_fatent) {	/* Check range */
		res = -ERESTARTSYS;

	} else {
		switch (fs->fs_type) {
		case FS_FAT12 :
			bc = clst; bc += bc / 2;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != 0)
				break;
			p = &fs->win[bc % SS(fs)];
			*p = (clst & 1) ? ((*p & 0x0F) | ((BYTE)val << 4)) : (BYTE)val;
			bc++;
			fs->wflag = 1;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != 0)
				break;
			p = &fs->win[bc % SS(fs)];
			*p = (clst & 1) ? (BYTE)(val >> 4) : ((*p & 0xF0) | ((BYTE)(val >> 8) & 0x0F));
			break;

		case FS_FAT16 :
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 2)));
			if (res != 0)
				break;
			p = &fs->win[clst * 2 % SS(fs)];
			ST_WORD(p, (WORD)val);
			break;

		case FS_FAT32 :
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 4)));
			if (res != 0)
				break;
			p = &fs->win[clst * 4 % SS(fs)];
			val |= LD_DWORD(p) & 0xF0000000;
			ST_DWORD(p, val);
			break;

		default :
			res = -ERESTARTSYS;
		}
		fs->wflag = 1;
	}

	return res;
}
#endif /* CONFIG_FS_FAT_WRITE */




/*
 * FAT handling - Remove a cluster chain
 */
#ifdef CONFIG_FS_FAT_WRITE
static
int remove_chain (
	FATFS *fs,			/* File system object */
	DWORD clst			/* Cluster# to remove a chain from */
)
{
	int res;
	DWORD nxt;
#if _USE_ERASE
	DWORD scl = clst, ecl = clst, resion[2];
#endif

	if (clst < 2 || clst >= fs->n_fatent) {	/* Check range */
		res = -ERESTARTSYS;

	} else {
		res = 0;
		while (clst < fs->n_fatent) { /* Not a last link? */
			nxt = get_fat(fs, clst); /* Get cluster status */
			if (nxt == 0)
				break; /* Empty cluster? */

			if (nxt == 1) {
				res = -ERESTARTSYS;
				break; /* Internal error? */
			}

			if (nxt == 0xFFFFFFFF) {
				res = -EIO;
				break; /* Disk error? */
			}

			/* Mark the cluster "empty" */
			res = put_fat(fs, clst, 0);
			if (res != 0)
				break;

			if (fs->free_clust != 0xFFFFFFFF) {	/* Update FSInfo */
				fs->free_clust++;
				fs->fsi_flag = 1;
			}
#if _USE_ERASE
			if (ecl + 1 == nxt) { /* Next cluster is contiguous */
				ecl = nxt;
			} else {
				/* End of contiguous clusters */
				resion[0] = clust2sect(fs, scl); /* Start sector */
				resion[1] = clust2sect(fs, ecl) + fs->csize - 1; /* End sector */
				disk_ioctl(fs, CTRL_ERASE_SECTOR, resion); /* Erase the block */
				scl = ecl = nxt;
			}
#endif
			clst = nxt;	/* Next cluster */
		}
	}

	return res;
}
#endif




/*
 * FAT handling - Stretch or Create a cluster chain
 */
#ifdef CONFIG_FS_FAT_WRITE
static
DWORD create_chain (	/* 0:No free cluster, 1:Internal error, 0xFFFFFFFF:Disk error, >=2:New cluster# */
	FATFS *fs,	/* File system object */
	DWORD clst	/* Cluster# to stretch. 0 means create a new chain. */
)
{
	DWORD cs, ncl, scl;
	int res;


	if (clst == 0) {
		/* Create a new chain */
		scl = fs->last_clust; /* Get suggested start point */
		if (!scl || scl >= fs->n_fatent) scl = 1;
	} else {
		/* Stretch the current chain */
		cs = get_fat(fs, clst);	/* Check the cluster status */
		if (cs < 2)
			return 1; /* It is an invalid cluster */
		if (cs < fs->n_fatent)
			return cs; /* It is already followed by next cluster */
		scl = clst;
	}

	ncl = scl; /* Start cluster */
	for (;;) {
		ncl++; /* Next cluster */
		if (ncl >= fs->n_fatent) { /* Wrap around */
			ncl = 2;
			if (ncl > scl)
				return 0; /* No free cluster */
		}
		cs = get_fat(fs, ncl);	/* Get the cluster status */
		if (cs == 0)
			break; /* Found a free cluster */
		if (cs == 0xFFFFFFFF || cs == 1)/* An error occurred */
			return cs;
		if (ncl == scl)
			return 0; /* No free cluster */
	}

	/* Mark the new cluster "last link" */
	res = put_fat(fs, ncl, 0x0FFFFFFF);
	if (res == 0 && clst != 0) {
		res = put_fat(fs, clst, ncl); /* Link it to the previous one if needed */
	}
	if (res == 0) {
		/* Update FSINFO */
		fs->last_clust = ncl;
		if (fs->free_clust != 0xFFFFFFFF) {
			fs->free_clust--;
			fs->fsi_flag = 1;
		}
	} else {
		ncl = (res == -EIO) ? 0xFFFFFFFF : 1;
	}

	return ncl; /* Return new cluster number or error code */
}
#endif /* CONFIG_FS_FAT_WRITE */

/*
 * Directory handling - Set directory index
 */
static int dir_sdi (
	FF_DIR *dj,	/* Pointer to directory object */
	WORD idx	/* Directory index number */
)
{
	DWORD clst;
	WORD ic;

	dj->index = idx;
	clst = dj->sclust;

	if (clst == 1 || clst >= dj->fs->n_fatent)	/* Check start cluster range */
		return -ERESTARTSYS;

	if (!clst && dj->fs->fs_type == FS_FAT32)	/* Replace cluster# 0 with root cluster# if in FAT32 */
		clst = dj->fs->dirbase;

	if (clst == 0) {
		/* Static table (root-dir in FAT12/16) */
		dj->clust = clst;

		if (idx >= dj->fs->n_rootdir)
			/* Index is out of range */
			return -ERESTARTSYS;

		dj->sect = dj->fs->dirbase + idx / (SS(dj->fs) / SZ_DIR);	/* Sector# */
	} else {
		/* Dynamic table (sub-dirs or root-dir in FAT32) */
		ic = SS(dj->fs) / SZ_DIR * dj->fs->csize; /* Entries per cluster */
		while (idx >= ic) {	/* Follow cluster chain */
			clst = get_fat(dj->fs, clst); /* Get next cluster */
			if (clst == 0xFFFFFFFF)
				return -EIO;	/* Disk error */
			if (clst < 2 || clst >= dj->fs->n_fatent)
				/* Reached to end of table or int error */
				return -ERESTARTSYS;
			idx -= ic;
		}
		dj->clust = clst;
		dj->sect = clust2sect(dj->fs, clst) + idx / (SS(dj->fs) / SZ_DIR); /* Sector# */
	}

	dj->dir = dj->fs->win + (idx % (SS(dj->fs) / SZ_DIR)) * SZ_DIR;	/* Ptr to the entry in the sector */

	return 0; /* Seek succeeded */
}




/*
 * Directory handling - Move directory index next
 */
static int dir_next (	/* 0:Succeeded, FR_NO_FILE:End of table, FR_DENIED:EOT and could not stretch */
	FF_DIR *dj,	/* Pointer to directory object */
	int stretch	/* 0: Do not stretch table, 1: Stretch table if needed */
)
{
	DWORD clst;
	WORD i;


	i = dj->index + 1;
	if (!i || !dj->sect)	/* Report EOT when index has reached 65535 */
		return -ENOENT;

	if (!(i % (SS(dj->fs) / SZ_DIR))) {	/* Sector changed? */
		dj->sect++;			/* Next sector */

		if (dj->clust == 0) {
			/* Static table */
			if (i >= dj->fs->n_rootdir)	/* Report EOT when end of table */
				return -ENOENT;
		}
		else {
			/* Dynamic table */
			if (((i / (SS(dj->fs) / SZ_DIR)) & (dj->fs->csize - 1)) == 0) {	/* Cluster changed? */
				clst = get_fat(dj->fs, dj->clust);		/* Get next cluster */

				if (clst <= 1)
					return -ERESTARTSYS;

				if (clst == 0xFFFFFFFF)
					return -EIO;

				if (clst >= dj->fs->n_fatent) {	/* When it reached end of dynamic table */
#ifdef CONFIG_FS_FAT_WRITE
					BYTE c;

					if (!stretch)
						return -ENOENT;	/* When do not stretch, report EOT */

					clst = create_chain(dj->fs, dj->clust);	/* Stretch cluster chain */

					if (clst == 0)
						return -ENOSPC; /* No free cluster */

					if (clst == 1)
						return -ERESTARTSYS;

					if (clst == 0xFFFFFFFF)
						return -EIO;

					/* Clean-up stretched table */
					if (move_window(dj->fs, 0))
						return -EIO;	/* Flush active window */

					memset(dj->fs->win, 0, SS(dj->fs)); /* Clear window buffer */
					dj->fs->winsect = clust2sect(dj->fs, clst); /* Cluster start sector */

					for (c = 0; c < dj->fs->csize; c++) {
						/* Fill the new cluster with 0 */
						dj->fs->wflag = 1;
						if (move_window(dj->fs, 0))
							return -EIO;
						dj->fs->winsect++;
					}
					dj->fs->winsect -= c; /* Rewind window address */
#else
					return -ENOENT;	/* Report EOT */
#endif
				}
				dj->clust = clst; /* Initialize data for new cluster */
				dj->sect = clust2sect(dj->fs, clst);
			}
		}
	}

	dj->index = i;
	dj->dir = dj->fs->win + (i % (SS(dj->fs) / SZ_DIR)) * SZ_DIR;

	return 0;
}

/*
 * LFN handling - Test/Pick/Fit an LFN segment from/to directory entry
 */
#ifdef CONFIG_FS_FAT_LFN

/* Offset of LFN chars in the directory entry */
static const BYTE LfnOfs[] = {1,3,5,7,9,14,16,18,20,22,24,28,30};

static int cmp_lfn (	/* 1:Matched, 0:Not matched */
	WCHAR *lfnbuf,	/* Pointer to the LFN to be compared */
	BYTE *dir	/* Pointer to the directory entry containing a part of LFN */
)
{
	UINT i, s;
	WCHAR wc, uc;

	/* Get offset in the LFN buffer */
	i = ((dir[LDIR_Ord] & ~LLE) - 1) * 13;
	s = 0; wc = 1;
	do {
		/* Pick an LFN character from the entry */
		uc = LD_WORD(dir+LfnOfs[s]);
		if (wc) {
			/* Last char has not been processed */
			wc = ff_wtoupper(uc);
			if (i >= _MAX_LFN || wc != ff_wtoupper(lfnbuf[i++])) /* Compare it */
				return 0; /* Not matched */
		} else {
			if (uc != 0xFFFF) return 0;	/* Check filler */
		}
	} while (++s < 13); /* Repeat until all chars in the entry are checked */

	if ((dir[LDIR_Ord] & LLE) && wc && lfnbuf[i])
		/* Last segment matched but different length */
		return 0;

	return 1; /* The part of LFN matched */
}



static
int pick_lfn (		/* 1:Succeeded, 0:Buffer overflow */
	WCHAR *lfnbuf,	/* Pointer to the Unicode-LFN buffer */
	BYTE *dir	/* Pointer to the directory entry */
)
{
	UINT i, s;
	WCHAR wc, uc;

	i = ((dir[LDIR_Ord] & 0x3F) - 1) * 13;	/* Offset in the LFN buffer */

	s = 0;
	wc = 1;
	do {
		/* Pick an LFN character from the entry */
		uc = LD_WORD(dir+LfnOfs[s]);
		if (wc) {
			/* Last char has not been processed */
			if (i >= _MAX_LFN)
				return 0; /* Buffer overflow? */
			lfnbuf[i++] = wc = uc; /* Store it */
		} else {
			if (uc != 0xFFFF)
				return 0; /* Check filler */
		}
	} while (++s < 13); /* Read all character in the entry */

	if (dir[LDIR_Ord] & LLE) {
		/* Put terminator if it is the last LFN part */
		if (i >= _MAX_LFN)
			return 0; /* Buffer overflow? */
		lfnbuf[i] = 0;
	}

	return 1;
}


#ifdef CONFIG_FS_FAT_WRITE
static
void fit_lfn (
	const WCHAR *lfnbuf,	/* Pointer to the LFN buffer */
	BYTE *dir,		/* Pointer to the directory entry */
	BYTE ord,		/* LFN order (1-20) */
	BYTE sum		/* SFN sum */
)
{
	UINT i, s;
	WCHAR wc;


	dir[LDIR_Chksum] = sum;	/* Set check sum */
	dir[LDIR_Attr] = AM_LFN; /* Set attribute. LFN entry */
	dir[LDIR_Type] = 0;
	ST_WORD(dir+LDIR_FstClusLO, 0);

	i = (ord - 1) * 13; /* Get offset in the LFN buffer */
	s = wc = 0;
	do {
		if (wc != 0xFFFF)
			wc = lfnbuf[i++]; /* Get an effective char */
		ST_WORD(dir + LfnOfs[s], wc); /* Put it */
		if (!wc)
			wc = 0xFFFF; /* Padding chars following last char */
	} while (++s < 13);
	if (wc == 0xFFFF || !lfnbuf[i])
		ord |= LLE; /* Bottom LFN part is the start of LFN sequence */
	dir[LDIR_Ord] = ord; /* Set the LFN order */
}

#endif
#endif



/*
 * Create numbered name
 */
#if defined(CONFIG_FS_FAT_LFN) && defined(CONFIG_FS_FAT_WRITE)
static void gen_numname (
	BYTE *dst,		/* Pointer to generated SFN */
	const BYTE *src,	/* Pointer to source SFN to be modified */
	const WCHAR *lfn,	/* Pointer to LFN */
	WORD seq		/* Sequence number */
)
{
	BYTE ns[8], c;
	UINT i, j;


	memcpy(dst, src, 11);

	if (seq > 5) {	/* On many collisions, generate a hash number instead of sequential number */
		do {
			seq = (seq >> 1) + (seq << 15) + (WORD) * lfn++;
		} while (*lfn);
	}

	/* itoa (hexdecimal) */
	i = 7;
	do {
		c = (seq % 16) + '0';
		if (c > '9')
			c += 7;
		ns[i--] = c;
		seq /= 16;
	} while (seq);

	ns[i] = '~';

	/* Append the number */
	for (j = 0; j < i && dst[j] != ' '; j++) {
		if (IsDBCS1(dst[j])) {
			if (j == i - 1)
				break;
			j++;
		}
	}

	do {
		dst[j++] = (i < 8) ? ns[i++] : ' ';
	} while (j < 8);
}
#endif

/*
 * Calculate sum of an SFN
 */
#ifdef CONFIG_FS_FAT_LFN
static BYTE sum_sfn (
	const BYTE *dir		/* Ptr to directory entry */
)
{
	BYTE sum = 0;
	UINT n = 11;

	do sum = (sum >> 1) + (sum << 7) + *dir++; while (--n);
	return sum;
}
#endif

/*
 * Directory handling - Find an object in the directory
 */

static int dir_find (
	FF_DIR *dj			/* Pointer to the directory object linked to the file name */
)
{
	int res;
	BYTE c, *dir;
#ifdef CONFIG_FS_FAT_LFN
	BYTE a, ord, sum;
#endif

	res = dir_sdi(dj, 0); /* Rewind directory object */
	if (res != 0)
		return res;

#ifdef CONFIG_FS_FAT_LFN
	ord = sum = 0xFF;
#endif
	do {
		res = move_window(dj->fs, dj->sect);
		if (res != 0)
			break;
		dir = dj->dir; /* Ptr to the directory entry of current index */
		c = dir[DIR_Name];
		if (c == 0) {
			/* Reached to end of table */
			res = -ENOENT;
			break;
		}
#ifdef CONFIG_FS_FAT_LFN	/* LFN configuration */
		a = dir[DIR_Attr] & AM_MASK;
		if (c == DDE || ((a & AM_VOL) && a != AM_LFN)) {
			/* An entry without valid data */
			ord = 0xFF;
		} else {
			if (a == AM_LFN) {
				/* An LFN entry is found */
				if (dj->lfn) {
					if (c & LLE) { /* Is it start of LFN sequence? */
						sum = dir[LDIR_Chksum];
						c &= ~LLE; ord = c;	/* LFN start order */
						dj->lfn_idx = dj->index;
					}
					/* Check validity of the LFN entry and compare it with given name */
					ord = (c == ord && sum == dir[LDIR_Chksum] && cmp_lfn(dj->lfn, dir)) ? ord - 1 : 0xFF;
				}
			} else {
				/* An SFN entry is found */
				if (!ord && sum == sum_sfn(dir))
					break;	/* LFN matched? */

				/* Reset LFN sequence */
				ord = 0xFF;
				dj->lfn_idx = 0xFFFF;
				if (!(dj->fn[NS] & NS_LOSS) && !memcmp(dir, dj->fn, 11))
					break;	/* SFN matched? */
			}
		}
#else		/* Non LFN configuration */
		if (!(dir[DIR_Attr] & AM_VOL) && !memcmp(dir, dj->fn, 11)) /* Is it a valid entry? */
			break;
#endif
		res = dir_next(dj, 0);		/* Next entry */
	} while (res == 0);

	return res;
}




/*
 * Read an object from the directory
 */
static int dir_read (
	FF_DIR *dj /* Pointer to the directory object that pointing the entry to be read */
)
{
	int res;
	BYTE c, *dir;
#ifdef CONFIG_FS_FAT_LFN
	BYTE a, ord = 0xFF, sum = 0xFF;
#endif

	res = -ENOENT;
	while (dj->sect) {
		res = move_window(dj->fs, dj->sect);
		if (res != 0)
			break;
		dir = dj->dir; /* Ptr to the directory entry of current index */
		c = dir[DIR_Name];
		if (c == 0) {
			/* Reached to end of table */
			res = -ENOENT;
			break;
		}
#ifdef CONFIG_FS_FAT_LFN	/* LFN configuration */
		a = dir[DIR_Attr] & AM_MASK;
		if (c == DDE || c == '.' || ((a & AM_VOL) && a != AM_LFN)) {	/* An entry without valid data */
			ord = 0xFF;
		} else {
			if (a == AM_LFN) {
				/* An LFN entry is found */
				if (c & LLE) { /* Is it start of LFN sequence? */
					sum = dir[LDIR_Chksum];
					c &= ~LLE; ord = c;
					dj->lfn_idx = dj->index;
				}
				/* Check LFN validity and capture it */
				ord = (c == ord && sum == dir[LDIR_Chksum] && pick_lfn(dj->lfn, dir)) ? ord - 1 : 0xFF;
			} else {
				/* An SFN entry is found */
				if (ord || sum != sum_sfn(dir))	/* Is there a valid LFN? */
					dj->lfn_idx = 0xFFFF;		/* It has no LFN. */
				break;
			}
		}
#else		/* Non LFN configuration */
		if (c != DDE && c != '.' && !(dir[DIR_Attr] & AM_VOL))	/* Is it a valid entry? */
			break;
#endif
		res = dir_next(dj, 0);				/* Next entry */
		if (res != 0)
			break;
	}

	if (res != 0)
		dj->sect = 0;

	return res;
}

/*
 * Register an object to the directory
 */
#ifdef CONFIG_FS_FAT_WRITE
static
int dir_register (	/* 0:Successful, FR_DENIED:No free entry or too many SFN collision, -EIO:Disk error */
	FF_DIR *dj	/* Target directory with object name to be created */
)
{
	int res;
	BYTE c, *dir;
#ifdef CONFIG_FS_FAT_LFN	/* LFN configuration */
	WORD n, ne, is;
	BYTE sn[12], *fn, sum;
	WCHAR *lfn;


	fn = dj->fn; lfn = dj->lfn;
	memcpy(sn, fn, 12);

	if (sn[NS] & NS_LOSS) {
		/* When LFN is out of 8.3 format, generate a numbered name */
		fn[NS] = 0; dj->lfn = NULL; /* Find only SFN */
		for (n = 1; n < 100; n++) {
			gen_numname(fn, sn, lfn, n); /* Generate a numbered name */
			res = dir_find(dj); /* Check if the name collides with existing SFN */
			if (res != 0)
				break;
		}
		if (n == 100)
			return -ENOSPC;	/* Abort if too many collisions */
		if (res != -ENOENT)
			return res; /* Abort if the result is other than 'not collided' */
		fn[NS] = sn[NS]; dj->lfn = lfn;
	}

	if (sn[NS] & NS_LFN) {
		/* When LFN is to be created, reserve an SFN + LFN entries. */
		for (ne = 0; lfn[ne]; ne++);
		ne = (ne + 25) / 13;
	} else {
		/* Otherwise reserve only an SFN entry. */
		ne = 1;
	}

	/* Reserve contiguous entries */
	res = dir_sdi(dj, 0);
	if (res != 0)
		return res;
	n = is = 0;
	do {
		res = move_window(dj->fs, dj->sect);
		if (res != 0)
			break;
		c = *dj->dir; /* Check the entry status */
		if (c == DDE || c == 0) { /* Is it a blank entry? */
			if (n == 0)
				is = dj->index; /* First index of the contiguous entry */
			if (++n == ne)
				break; /* A contiguous entry that required count is found */
		} else {
			n = 0; /* Not a blank entry. Restart to search */
		}
		res = dir_next(dj, 1); /* Next entry with table stretch */
	} while (res == 0);

	if (res == 0 && ne > 1) {
		/* Initialize LFN entry if needed */
		res = dir_sdi(dj, is);
		if (res == 0) {
			sum = sum_sfn(dj->fn); /* Sum of the SFN tied to the LFN */
			ne--;
			do {
				/* Store LFN entries in bottom first */
				res = move_window(dj->fs, dj->sect);
				if (res != 0)
					break;
				fit_lfn(dj->lfn, dj->dir, (BYTE)ne, sum);
				dj->fs->wflag = 1;
				res = dir_next(dj, 0); /* Next entry */
			} while (res == 0 && --ne);
		}
	}

#else	/* Non LFN configuration */
	res = dir_sdi(dj, 0);
	if (res == 0) {
		do {	/* Find a blank entry for the SFN */
			res = move_window(dj->fs, dj->sect);
			if (res != 0)
				break;
			c = *dj->dir;
			if (c == DDE || c == 0)
				break;	/* Is it a blank entry? */
			res = dir_next(dj, 1); /* Next entry with table stretch */
		} while (res == 0);
	}
#endif

	if (res == 0) {		/* Initialize the SFN entry */
		res = move_window(dj->fs, dj->sect);
		if (res == 0) {
			dir = dj->dir;
			memset(dir, 0, SZ_DIR);	/* Clean the entry */
			memcpy(dir, dj->fn, 11);	/* Put SFN */
#ifdef CONFIG_FS_FAT_LFN
			dir[DIR_NTres] = *(dj->fn+NS) & (NS_BODY | NS_EXT);	/* Put NT flag */
#endif
			dj->fs->wflag = 1;
		}
	}

	return res;
}
#endif /* CONFIG_FS_FAT_WRITE */

/*
 * Remove an object from the directory
 */
#if defined CONFIG_FS_FAT_WRITE
static int dir_remove (	/* 0: Successful, -EIO: A disk error */
	FF_DIR *dj				/* Directory object pointing the entry to be removed */
)
{
	int res;
#ifdef CONFIG_FS_FAT_LFN	/* LFN configuration */
	WORD i;

	i = dj->index;	/* SFN index */
	/* Goto the SFN or top of the LFN entries */
	res = dir_sdi(dj, (WORD)((dj->lfn_idx == 0xFFFF) ? i : dj->lfn_idx));
	if (res == 0) {
		do {
			res = move_window(dj->fs, dj->sect);
			if (res != 0)
				break;
			*dj->dir = DDE; /* Mark the entry "deleted" */
			dj->fs->wflag = 1;
			if (dj->index >= i)
				break;	/* When reached SFN, all entries of the object has been deleted. */
			res = dir_next(dj, 0); /* Next entry */
		} while (res == 0);
		if (res == -ENOENT)
			res = -ERESTARTSYS;
	}

#else			/* Non LFN configuration */
	res = dir_sdi(dj, dj->index);
	if (res == 0) {
		res = move_window(dj->fs, dj->sect);
		if (res == 0) {
			*dj->dir = DDE; /* Mark the entry "deleted" */
			dj->fs->wflag = 1;
		}
	}
#endif

	return res;
}
#endif /* CONFIG_FS_FAT_WRITE */

/*
 * Pick a segment and create the object name in directory form
 */
static int create_name (
	FF_DIR *dj,		/* Pointer to the directory object */
	const TCHAR **path	/* Pointer to pointer to the segment in the path string */
)
{
#ifdef _EXCVT
	static const BYTE excvt[] = _EXCVT;	/* Upper conversion table for extended chars */
#endif

#ifdef CONFIG_FS_FAT_LFN	/* LFN configuration */
	BYTE b, cf;
	WCHAR w, *lfn;
	UINT i, ni, si, di;
	const TCHAR *p;

	/* Create LFN in Unicode */
	for (p = *path; *p == '/' || *p == '\\'; p++) ;	/* Strip duplicated separator */
	lfn = dj->lfn;
	si = di = 0;
	for (;;) {
		w = p[si++]; /* Get a character */
		if (w < ' ' || w == '/' || w == '\\')
			break;	/* Break on end of segment */
		if (di >= _MAX_LFN) /* Reject too long name */
			return -ENAMETOOLONG;
#if !_LFN_UNICODE
		w &= 0xFF;
		if (IsDBCS1(w)) { /* Check if it is a DBC 1st byte (always false on SBCS cfg) */
			b = (BYTE)p[si++]; /* Get 2nd byte */
			if (!IsDBCS2(b))
				return -ENAMETOOLONG;	/* Reject invalid sequence */
			w = (w << 8) + b; /* Create a DBC */
		}
		w = ff_convert(w, 1);		/* Convert ANSI/OEM to Unicode */
		if (!w) return -ENAMETOOLONG;	/* Reject invalid code */
#endif
		if (w < 0x80 && strchr("\"*:<>\?|\x7F", w)) /* Reject illegal chars for LFN */
			return -ENAMETOOLONG;
		lfn[di++] = w;			/* Store the Unicode char */
	}
	*path = &p[si];				/* Return pointer to the next segment */
	cf = (w < ' ') ? NS_LAST : 0;		/* Set last segment flag if end of path */
	while (di) {				/* Strip trailing spaces and dots */
		w = lfn[di-1];
		if (w != ' ' && w != '.')
			break;
		di--;
	}
	if (!di) return -EINVAL;	/* Reject nul string */

	lfn[di] = 0;			/* LFN is created */

	/* Create SFN in directory form */
	memset(dj->fn, ' ', 11);
	for (si = 0; lfn[si] == ' ' || lfn[si] == '.'; si++) ;	/* Strip leading spaces and dots */
	if (si) cf |= NS_LOSS | NS_LFN;
	while (di && lfn[di - 1] != '.') di--;	/* Find extension (di<=si: no extension) */

	b = i = 0; ni = 8;
	for (;;) {
		w = lfn[si++];		/* Get an LFN char */
		if (!w)
			break;		/* Break on end of the LFN */
		if (w == ' ' || (w == '.' && si != di)) {	/* Remove spaces and dots */
			cf |= NS_LOSS | NS_LFN;
			continue;
		}

		if (i >= ni || si == di) {	/* Extension or end of SFN */
			if (ni == 11) {				/* Long extension */
				cf |= NS_LOSS | NS_LFN;
				break;
			}
			if (si != di)
				cf |= NS_LOSS | NS_LFN;	/* Out of 8.3 format */
			if (si > di)
				break;	/* No extension */
			si = di; i = 8; ni = 11;/* Enter extension section */
			b <<= 2;
			continue;
		}

		if (w >= 0x80) {		/* Non ASCII char */
#ifdef _EXCVT
			w = ff_convert(w, 0);	/* Unicode -> OEM code */
			if (w)
				w = excvt[w - 0x80];	/* Convert extended char to upper (SBCS) */
#else
			w = ff_convert(ff_wtoupper(w), 0);	/* Upper converted Unicode -> OEM code */
#endif
			cf |= NS_LFN;	/* Force create LFN entry */
		}

		if (_DF1S && w >= 0x100) {	/* Double byte char (always false on SBCS cfg) */
			if (i >= ni - 1) {
				cf |= NS_LOSS | NS_LFN; i = ni;
				continue;
			}
			dj->fn[i++] = (BYTE)(w >> 8);
		} else {					/* Single byte char */
			if (!w || strchr("+,;=[]", w)) {	/* Replace illegal chars for SFN */
				w = '_'; cf |= NS_LOSS | NS_LFN;/* Lossy conversion */
			} else {
				if (isupper(w)) {
					b |= 2;
				} else {
					if (islower(w)) {
						b |= 1; w -= 0x20;
					}
				}
			}
		}
		dj->fn[i++] = (BYTE)w;
	}

	if (dj->fn[0] == DDE)
		dj->fn[0] = NDDE; /* If the first char collides with deleted mark, replace it with 0x05 */

	if (ni == 8) b <<= 2;
	if ((b & 0x0C) == 0x0C || (b & 0x03) == 0x03)	/* Create LFN entry when there are composite capitals */
		cf |= NS_LFN;
	if (!(cf & NS_LFN)) { /* When LFN is in 8.3 format without extended char, NT flags are created */
		if ((b & 0x03) == 0x01)
			cf |= NS_EXT;	/* NT flag (Extension has only small capital) */
		if ((b & 0x0C) == 0x04)
			cf |= NS_BODY;	/* NT flag (Filename has only small capital) */
	}

	dj->fn[NS] = cf;	/* SFN is created */

	return 0;

#else	/* Non-LFN configuration */
	BYTE b, c, d, *sfn;
	UINT ni, si, i;
	const char *p;

	/* Create file name in directory form */
	for (p = *path; *p == '/' || *p == '\\'; p++);	/* Strip duplicated separator */
	sfn = dj->fn;
	memset(sfn, ' ', 11);
	si = i = b = 0; ni = 8;
	for (;;) {
		c = (BYTE)p[si++];
		if (c <= ' ' || c == '/' || c == '\\') break;	/* Break on end of segment */
		if (c == '.' || i >= ni) {
			if (ni != 8 || c != '.')
				return -EINVAL;
			i = 8; ni = 11;
			b <<= 2; continue;
		}
		if (c >= 0x80) {	/* Extended char? */
			b |= 3;		/* Eliminate NT flag */
#ifdef _EXCVT
			c = excvt[c-0x80];	/* Upper conversion (SBCS) */
#else
#if !_DF1S	/* ASCII only cfg */
			return -EINVAL;
#endif
#endif
		}
		if (IsDBCS1(c)) {		/* Check if it is a DBC 1st byte (always false on SBCS cfg) */
			d = (BYTE)p[si++];	/* Get 2nd byte */
			if (!IsDBCS2(d) || i >= ni - 1)	/* Reject invalid DBC */
				return -EINVAL;
			sfn[i++] = c;
			sfn[i++] = d;
		} else {			/* Single byte code */
			if (strchr("\"*+,:;<=>\?[]|\x7F", c))	/* Reject illegal chrs for SFN */
				return -EINVAL;
			if (isupper(c)) {
				b |= 2;
			} else {
				if (islower(c)) {
					b |= 1; c -= 0x20;
				}
			}
			sfn[i++] = c;
		}
	}
	*path = &p[si];	/* Return pointer to the next segment */
	c = (c <= ' ') ? NS_LAST : 0;	/* Set last segment flag if end of path */

	if (!i)
		return -EINVAL;		/* Reject nul string */
	if (sfn[0] == DDE)
		sfn[0] = NDDE;	/* When first char collides with DDE, replace it with 0x05 */

	if (ni == 8)
		b <<= 2;
	if ((b & 0x03) == 0x01)
		c |= NS_EXT;	/* NT flag (Name extension has only small capital) */
	if ((b & 0x0C) == 0x04)
		c |= NS_BODY;	/* NT flag (Name body has only small capital) */

	sfn[NS] = c;		/* Store NT flag, File name is created */

	return 0;
#endif
}

/*
 * Get file information from directory entry
 */
static void get_fileinfo (		/* No return code */
	FF_DIR *dj,			/* Pointer to the directory object */
	FILINFO *fno	 	/* Pointer to the file information to be filled */
)
{
	UINT i;
	BYTE nt, *dir;
	TCHAR *p, c;


	p = fno->fname;
	if (dj->sect) {
		dir = dj->dir;
		nt = dir[DIR_NTres];		/* NT flag */
		for (i = 0; i < 8; i++) {	/* Copy name body */
			c = dir[i];
			if (c == ' ')
				break;
			if (c == NDDE)
				c = (TCHAR)DDE;
#ifdef CONFIG_FS_FAT_LFN
			if ((nt & NS_BODY) && isupper(c))
				c += 0x20;
#endif
#if _LFN_UNICODE
			if (IsDBCS1(c) && i < 7 && IsDBCS2(dir[i+1]))
				c = (c << 8) | dir[++i];
			c = ff_convert(c, 1);
			if (!c) c = '?';
#endif
			*p++ = c;
		}
		if (dir[8] != ' ') {		/* Copy name extension */
			*p++ = '.';
			for (i = 8; i < 11; i++) {
				c = dir[i];
				if (c == ' ')
					break;
#ifdef CONFIG_FS_FAT_LFN
				if ((nt & NS_EXT) && isupper(c))
					c += 0x20;
#endif
#if _LFN_UNICODE
				if (IsDBCS1(c) && i < 10 && IsDBCS2(dir[i+1]))
					c = (c << 8) | dir[++i];
				c = ff_convert(c, 1);
				if (!c)
					c = '?';
#endif
				*p++ = c;
			}
		}
		fno->fattrib = dir[DIR_Attr];			/* Attribute */
		fno->fsize = LD_DWORD(dir+DIR_FileSize);	/* Size */
		fno->fdate = LD_WORD(dir+DIR_WrtDate);		/* Date */
		fno->ftime = LD_WORD(dir+DIR_WrtTime);		/* Time */
	}
	*p = 0;		/* Terminate SFN str by a \0 */

#ifdef CONFIG_FS_FAT_LFN
	if (fno->lfname && fno->lfsize) {
		TCHAR *tp = fno->lfname;
		WCHAR w, *lfn;

		i = 0;
		if (dj->sect && dj->lfn_idx != 0xFFFF) {/* Get LFN if available */
			lfn = dj->lfn;
			while ((w = *lfn++) != 0) {	/* Get an LFN char */
#if !_LFN_UNICODE
				w = ff_convert(w, 0);	/* Unicode -> OEM conversion */
				if (!w) {
					/* Could not convert, no LFN */
					i = 0;
					break;
				}
				/* Put 1st byte if it is a DBC (always false on SBCS cfg) */
				if (_DF1S && w >= 0x100)
					tp[i++] = (TCHAR)(w >> 8);
#endif
				if (i >= fno->lfsize - 1) { i = 0; break; }	/* Buffer overflow, no LFN */
				tp[i++] = (TCHAR)w;
			}
		}
		tp[i] = 0;	/* Terminate the LFN str by a \0 */
	}
#endif
}

/*
 * Follow a file path
 */
static
int follow_path (	/* 0(0): successful, !=0: error code */
	FF_DIR *dj,		/* Directory object to return last directory and found object */
	const TCHAR *path	/* Full-path string to find a file or directory */
)
{
	int res;
	BYTE *dir, ns;

	if (*path == '/' || *path == '\\')	/* Strip heading separator if exist */
		path++;
	dj->sclust = 0;		/* Start from the root dir */

	if ((UINT)*path < ' ') {
		/* Nul path means the start directory itself */
		res = dir_sdi(dj, 0);
		dj->dir = NULL;
		return res;
	}

	/* Follow path */
	for (;;) {
		res = create_name(dj, &path);	/* Get a segment */
		if (res != 0)
			break;
		res = dir_find(dj);	/* Find it */
		ns = *(dj->fn+NS);
		if (res != 0) {				/* Failed to find the object */
			if (res != -ENOENT)
				break;	/* Abort if any hard error occured */
			/* Object not found */
			if (!(ns & NS_LAST))
				res = -ENOENT;
			break;
		}
		if (ns & NS_LAST)
			break;	/* Last segment match. Function completed. */
		dir = dj->dir;	/* There is next segment. Follow the sub directory */
		if (!(dir[DIR_Attr] & AM_DIR)) {	/* Cannot follow because it is a file */
			res = -ENOTDIR;
			break;
		}
		dj->sclust = LD_CLUST(dir);
	}

	return res;
}

/*
 * Load boot record and check if it is an FAT boot record
 */
static enum filetype check_fs (	/* 0:The FAT BR, 1:Valid BR but not an FAT, 2:Not a BR, 3:Disk error */
	FATFS *fs,	/* File system object */
	DWORD sect,	/* Sector# (lba) to check if it is an FAT boot record or not */
	DWORD *bootsec
)
{
	enum filetype ret;

	/* Load boot record */
	ret = disk_read(fs, fs->win, sect, 1);
	if (ret)
		return filetype_unknown;

	return is_fat_or_mbr(fs->win, bootsec);
}

/*
 * Check if the file system object is valid or not
 */
static int chk_mounted (	/* 0(0): successful, !=0: any error occurred */
	FATFS *fs,	/* Pointer to pointer to the found file system object */
	BYTE chk_wp	/* !=0: Check media write protection for write access */
)
{
	BYTE fmt, b;
	DWORD first_boot_sect;
	DWORD bsect, fasize, tsect, sysect, nclst, szbfat;
	WORD nrsv;
	enum filetype type;

	INIT_LIST_HEAD(&fs->dirtylist);

	/* The logical drive must be mounted. */
	/* Following code attempts to mount a volume. (analyze BPB and initialize the fs object) */

	fs->fs_type = 0;	/* Clear the file system object */
#if _MAX_SS != 512		/* Get disk sector size (variable sector size cfg only) */
	if (disk_ioctl(fs, GET_SECTOR_SIZE, &fs->ssize) != RES_OK)
		return -EIO;
#endif
	/* Search FAT partition on the drive. Supports only generic partitionings, FDISK and SFD. */
	type = check_fs(fs, bsect = 0, &first_boot_sect);	/* Check sector 0 if it is a VBR */
	if (type == filetype_mbr) {
		/* Sector 0 is an MBR, now check for FAT in the first partition */
		type = check_fs(fs, bsect = first_boot_sect, NULL);
		if (type != filetype_fat)
			return -ENODEV;
	}

	if (type == filetype_unknown)
		return -ENODEV;

	/* Following code initializes the file system object */

	/* (BPB_BytsPerSec must be equal to the physical sector size) */
	if (LD_WORD(fs->win+BPB_BytsPerSec) != SS(fs))
		return -EINVAL;

	/* Number of sectors per FAT */
	fmt = FS_FAT12;
	fasize = LD_WORD(fs->win+BPB_FATSz16);
	if (!fasize) {
		fasize = LD_DWORD(fs->win+BPB_FATSz32);
		if (fasize)
			/* Must be FAT32 */
			fmt = FS_FAT32;
	}
	fs->fsize = fasize;

	/* Number of FAT copies */
	fs->n_fats = b = fs->win[BPB_NumFATs];
	if (b != 1 && b != 2)
		return -EINVAL;	/* (Must be 1 or 2) */

	/* Number of sectors for FAT area */
	fasize *= b;

	/* Number of sectors per cluster */
	fs->csize = b = fs->win[BPB_SecPerClus];
	if (!b || (b & (b - 1)))
		return -EINVAL;	/* (Must be power of 2) */

	/* Number of root directory entries */
	fs->n_rootdir = LD_WORD(fs->win+BPB_RootEntCnt);
	if (fs->n_rootdir % (SS(fs) / SZ_DIR))
		return -EINVAL;	/* (BPB_RootEntCnt must be sector aligned) */

	/* Number of sectors on the volume */
	tsect = LD_WORD(fs->win+BPB_TotSec16);
	if (!tsect) tsect = LD_DWORD(fs->win+BPB_TotSec32);

	/* Number of reserved sectors */
	nrsv = LD_WORD(fs->win+BPB_RsvdSecCnt);
	if (!nrsv)
		return -EINVAL; /* (BPB_RsvdSecCnt must not be 0) */

	/* Determine the FAT sub type */
	/* RSV+FAT+FF_DIR */
	sysect = nrsv + fasize + fs->n_rootdir / (SS(fs) / SZ_DIR);
	if (tsect < sysect)
		return -EINVAL;	/* (Invalid volume size) */

	/* Number of clusters */
	nclst = (tsect - sysect) / fs->csize;
	if (!nclst)
		return -EINVAL;	/* (Invalid volume size) */
	if (fmt == FS_FAT12 && nclst >= MIN_FAT16)
		fmt = FS_FAT16;

	/* Boundaries and Limits */
	/* Number of FAT entries */
	fs->n_fatent = nclst + 2;
	/* Data start sector */
	fs->database = bsect + sysect;
	/* FAT start sector */
	fs->fatbase = bsect + nrsv;
	if (fmt == FS_FAT32) {
		if (fs->n_rootdir)
			return -EINVAL;	/* (BPB_RootEntCnt must be 0) */
		/* Root directory start cluster */
		fs->dirbase = LD_DWORD(fs->win+BPB_RootClus);
		/* (Required FAT size) */
		szbfat = fs->n_fatent * 4;
	} else {
		if (!fs->n_rootdir)
			return -EINVAL;/* (BPB_RootEntCnt must not be 0) */
		/* Root directory start sector */
		fs->dirbase = fs->fatbase + fasize;
		/* (Required FAT size) */
		szbfat = (fmt == FS_FAT16) ?
			fs->n_fatent * 2 : fs->n_fatent * 3 / 2 + (fs->n_fatent & 1);
	}
	/* (BPB_FATSz must not be less than required) */
	if (fs->fsize < (szbfat + (SS(fs) - 1)) / SS(fs))
		return -EINVAL;

#ifdef CONFIG_FS_FAT_WRITE
	/* Initialize cluster allocation information */
	fs->free_clust = 0xFFFFFFFF;
	fs->last_clust = 0;

	/* Get fsinfo if available */
	if (fmt == FS_FAT32) {
		fs->fsi_flag = 0;
		fs->fsi_sector = bsect + LD_WORD(fs->win+BPB_FSInfo);
		if (disk_read(fs, fs->win, fs->fsi_sector, 1) == RES_OK &&
			LD_WORD(fs->win+BS_55AA) == 0xAA55 &&
			LD_DWORD(fs->win+FSI_LeadSig) == 0x41615252 &&
			LD_DWORD(fs->win+FSI_StrucSig) == 0x61417272) {
				fs->last_clust = LD_DWORD(fs->win+FSI_Nxt_Free);
				fs->free_clust = LD_DWORD(fs->win+FSI_Free_Count);
		}
	}
#endif
	fs->fs_type = fmt; /* FAT sub-type */
	fs->winsect = 0; /* Invalidate sector cache */
	fs->wflag = 0;

	return 0;
}

/*
 * Mount/Unmount a Logical Drive
 */
int f_mount (
	FATFS *fs /* Pointer to new file system object (NULL for unmount)*/
)
{
	fs->fs_type = 0; /* Clear new fs object */

	return chk_mounted(fs, 0);
}

/*
 * Open or Create a File
 */
int f_open (
	FATFS *fatfs,
	FIL *fp,		/* Pointer to the blank file object */
	const TCHAR *path,	/* Pointer to the file name */
	BYTE mode		/* Access mode and file open mode flags */
)
{
	int res = 0;
	FF_DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;


	fp->fs = NULL;		/* Clear file object */

#ifdef CONFIG_FS_FAT_WRITE
	mode &= FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW;
	dj.fs = fatfs;
#else
	mode &= FA_READ;
	dj.fs = fatfs;
#endif
	INIT_BUF(dj);
	if (res == 0)
		res = follow_path(&dj, path);	/* Follow the file path */
	dir = dj.dir;

#ifdef CONFIG_FS_FAT_WRITE	/* R/W configuration */
	if (res == 0) {
		if (!dir)	/* Current dir itself */
			res = -EISDIR;
	}
	/* Create or Open a file */
	if (mode & (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW)) {
		DWORD dw, cl;

		if (res != 0) {					/* No file, create new */
			if (res == -ENOENT)			/* There is no file to open, create a new entry */
				res = dir_register(&dj);
			mode |= FA_CREATE_ALWAYS;		/* File is created */
			dir = dj.dir;				/* New entry */
		} else {						/* Any object is already existing */
			if (dir[DIR_Attr] & AM_RDO) {
				res = -EROFS;
			} else if (dir[DIR_Attr] & AM_DIR) {	/* Cannot overwrite it (R/O or FF_DIR) */
				res = -EISDIR;
			} else {
				if (mode & FA_CREATE_NEW)	/* Cannot create as new file */
					res = -EEXIST;
			}
		}
		if (res == 0 && (mode & FA_CREATE_ALWAYS)) {	/* Truncate it if overwrite mode */
			dw = get_fattime();				/* Created time */
			ST_DWORD(dir+DIR_CrtTime, dw);
			dir[DIR_Attr] = 0;			/* Reset attribute */
			ST_DWORD(dir+DIR_FileSize, 0);		/* size = 0 */
			cl = LD_CLUST(dir);			/* Get start cluster */
			ST_CLUST(dir, 0);			/* cluster = 0 */
			dj.fs->wflag = 1;
			if (cl) {				/* Remove the cluster chain if exist */
				dw = dj.fs->winsect;
				res = remove_chain(dj.fs, cl);
				if (res == 0) {
					dj.fs->last_clust = cl - 1;	/* Reuse the cluster hole */
					res = move_window(dj.fs, dw);
				}
			}
		}
	} else {	/* Open an existing file */
		if (res == 0) {				/* Follow succeeded */
			if (dir[DIR_Attr] & AM_DIR) {	/* It is a directory */
				res = -EISDIR;
			} else {
				if ((mode & FA_WRITE) && (dir[DIR_Attr] & AM_RDO)) /* R/O violation */
					res = -EROFS;
			}
		}
	}
	if (res == 0) {
		if (mode & FA_CREATE_ALWAYS)	/* Set file change flag if created or overwritten */
			mode |= FA__WRITTEN;
		fp->dir_sect = dj.fs->winsect;	/* Pointer to the directory entry */
		fp->dir_ptr = dir;
	}

#else				/* R/O configuration */
	if (res == 0) {	/* Follow succeeded */
		if (!dir) {	/* Current dir itself */
			res = -EISDIR;
		} else {
			if (dir[DIR_Attr] & AM_DIR)	/* It is a directory */
				res = -EISDIR;
		}
	}
#endif
	FREE_BUF();

	if (res == 0) {
		fp->flag = mode;		/* File access mode */
		fp->sclust = LD_CLUST(dir);	/* File start cluster */
		fp->fsize = LD_DWORD(dir+DIR_FileSize);	/* File size */
		fp->fptr = 0;			/* File pointer */
		fp->dsect = 0;
		fp->fs = dj.fs;
	}

	return res;
}




/*
 * Read File
 */
int f_read (
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	UINT *br		/* Pointer to number of bytes read */
)
{
	DWORD clst, sect, remain;
	UINT rcnt, cc;
	BYTE csect, *rbuff = buff;

	*br = 0;	/* Initialize byte counter */

	if (fp->flag & FA__ERROR)		/* Aborted file? */
		return -ERESTARTSYS;
	if (!(fp->flag & FA_READ)) 		/* Check access mode */
		return -EROFS;
	remain = fp->fsize - fp->fptr;
	if (btr > remain)
		btr = (UINT)remain;	/* Truncate btr by remaining bytes */

	/* Repeat until all data read */
	for ( ;  btr; rbuff += rcnt, fp->fptr += rcnt, *br += rcnt, btr -= rcnt) {
		if ((fp->fptr % SS(fp->fs)) == 0) {		/* On the sector boundary? */
			csect = (BYTE)(fp->fptr / SS(fp->fs) & (fp->fs->csize - 1));	/* Sector offset in the cluster */
			if (!csect) {				/* On the cluster boundary? */
				if (fp->fptr == 0) {		/* On the top of the file? */
					clst = fp->sclust;	/* Follow from the origin */
				} else {			/* Middle or end of the file */
						clst = get_fat(fp->fs, fp->clust);	/* Follow cluster chain on the FAT */
				}
				if (clst < 2)
					ABORT(fp->fs, -ERESTARTSYS);
				if (clst == 0xFFFFFFFF)
					ABORT(fp->fs, -EIO);
				fp->clust = clst;				/* Update current cluster */
			}
			sect = clust2sect(fp->fs, fp->clust);	/* Get current sector */
			if (!sect)
				ABORT(fp->fs, -ERESTARTSYS);
			sect += csect;
			cc = btr / SS(fp->fs);		/* When remaining bytes >= sector size, */
			if (cc) {			/* Read maximum contiguous sectors directly */
				if (csect + cc > fp->fs->csize)	/* Clip at cluster boundary */
					cc = fp->fs->csize - csect;
				if (disk_read(fp->fs, rbuff, sect, (BYTE)cc) != RES_OK)
					ABORT(fp->fs, -EIO);
#if defined CONFIG_FS_FAT_WRITE
				/* Replace one of the read sectors with cached data if it contains a dirty sector */
				if ((fp->flag & FA__DIRTY) && fp->dsect - sect < cc)
					memcpy(rbuff + ((fp->dsect - sect) * SS(fp->fs)), fp->buf, SS(fp->fs));
#endif
				rcnt = SS(fp->fs) * cc;	/* Number of bytes transferred */
				continue;
			}
			if (fp->dsect != sect) {	/* Load data sector if not in cache */
#ifdef CONFIG_FS_FAT_WRITE
				if (fp->flag & FA__DIRTY) {	/* Write-back dirty sector cache */
					if (disk_write(fp->fs, fp->buf, fp->dsect, 1) != RES_OK)
						ABORT(fp->fs, -EIO);
					fp->flag &= ~FA__DIRTY;
				}
#endif
				if (disk_read(fp->fs, fp->buf, sect, 1) != RES_OK)	/* Fill sector cache */
					ABORT(fp->fs, -EIO);
			}
			fp->dsect = sect;
		}
		rcnt = SS(fp->fs) - (fp->fptr % SS(fp->fs));	/* Get partial sector data from sector buffer */
		if (rcnt > btr)
			rcnt = btr;
		memcpy(rbuff, &fp->buf[fp->fptr % SS(fp->fs)], rcnt);	/* Pick partial sector */
	}

	return 0;
}




#ifdef CONFIG_FS_FAT_WRITE
/*
 * Write File
 */
int f_write (
	FIL *fp,		/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	UINT btw,		/* Number of bytes to write */
	UINT *bw		/* Pointer to number of bytes written */
)
{
	DWORD clst, sect;
	UINT wcnt, cc;
	const BYTE *wbuff = buff;
	BYTE csect;

	*bw = 0;	/* Initialize byte counter */

	if (fp->flag & FA__ERROR)			/* Aborted file? */
		return -ERESTARTSYS;
	if (!(fp->flag & FA_WRITE))			/* Check access mode */
		return -EROFS;
	if ((DWORD)(fp->fsize + btw) < fp->fsize)
		btw = 0;	/* File size cannot reach 4GB */

	/* Repeat until all data written */
	for ( ;  btw; wbuff += wcnt, fp->fptr += wcnt, *bw += wcnt, btw -= wcnt) {
		/* On the sector boundary? */
		if ((fp->fptr % SS(fp->fs)) == 0) {
			/* Sector offset in the cluster */
			csect = (BYTE)(fp->fptr / SS(fp->fs) & (fp->fs->csize - 1));
			/* On the cluster boundary? */
			if (!csect) {
				/* On the top of the file? */
				if (fp->fptr == 0) {
					clst = fp->sclust;		/* Follow from the origin */
					if (clst == 0)			/* When no cluster is allocated, */
						/* Create a new cluster chain */
						fp->sclust = clst = create_chain(fp->fs, 0);
				} else {
					/* Middle or end of the file */
					/* Follow or stretch cluster chain on the FAT */
					clst = create_chain(fp->fs, fp->clust);
				}
				if (clst == 0)
					break;		/* Could not allocate a new cluster (disk full) */
				if (clst == 1)
					ABORT(fp->fs, -ERESTARTSYS);
				if (clst == 0xFFFFFFFF)
					ABORT(fp->fs, -EIO);
				fp->clust = clst;		/* Update current cluster */
			}
			if (fp->flag & FA__DIRTY) {		/* Write-back sector cache */
				printf("wr sector cache\n");
				if (disk_write(fp->fs, fp->buf, fp->dsect, 1) != RES_OK)
					ABORT(fp->fs, -EIO);
				fp->flag &= ~FA__DIRTY;
			}
			sect = clust2sect(fp->fs, fp->clust);	/* Get current sector */
			if (!sect)
				ABORT(fp->fs, -ERESTARTSYS);
			sect += csect;
			cc = btw / SS(fp->fs);	/* When remaining bytes >= sector size, */
			if (cc) {
				/* Write maximum contiguous sectors directly */
				if (csect + cc > fp->fs->csize)	/* Clip at cluster boundary */
					cc = fp->fs->csize - csect;
				if (disk_write(fp->fs, wbuff, sect, (BYTE)cc) != RES_OK)
					ABORT(fp->fs, -EIO);
				if (fp->dsect - sect < cc) {
					/* Refill sector cache if it gets invalidated by the direct write */
					memcpy(fp->buf, wbuff + ((fp->dsect - sect) * SS(fp->fs)), SS(fp->fs));
					fp->flag &= ~FA__DIRTY;
				}
				wcnt = SS(fp->fs) * cc;		/* Number of bytes transferred */
				continue;
			}
			if (fp->dsect != sect) {
				/* Fill sector cache with file data */
				if (fp->fptr < fp->fsize &&
					disk_read(fp->fs, fp->buf, sect, 1) != RES_OK)
						ABORT(fp->fs, -EIO);
			}

			fp->dsect = sect;
		}
		/* Put partial sector into file I/O buffer */
		wcnt = SS(fp->fs) - (fp->fptr % SS(fp->fs));
		if (wcnt > btw)
			wcnt = btw;
		memcpy(&fp->buf[fp->fptr % SS(fp->fs)], wbuff, wcnt);	/* Fit partial sector */
		fp->flag |= FA__DIRTY;
	}

	if (fp->fptr > fp->fsize)
		fp->fsize = fp->fptr;	/* Update file size if needed */

	fp->flag |= FA__WRITTEN;	/* Set file change flag */

	return 0;
}

/*
 * Synchronize the File Object
 */
int f_sync (
	FIL *fp		/* Pointer to the file object */
)
{
	int res;
	DWORD tim;
	BYTE *dir;

	/* Has the file been written? */
	if (!(fp->flag & FA__WRITTEN))
		return 0;

	if (fp->flag & FA__DIRTY) {
		if (disk_write(fp->fs, fp->buf, fp->dsect, 1) != RES_OK)
			return -EIO;
		fp->flag &= ~FA__DIRTY;
	}

	/* Update the directory entry */
	res = move_window(fp->fs, fp->dir_sect);
	if (res)
		return res;

	dir = fp->dir_ptr;
	dir[DIR_Attr] |= AM_ARC;		/* Set archive bit */
	ST_DWORD(dir+DIR_FileSize, fp->fsize);	/* Update file size */
	ST_CLUST(dir, fp->sclust);		/* Update start cluster */
	tim = get_fattime();			/* Update updated time */
	ST_DWORD(dir+DIR_WrtTime, tim);
	fp->flag &= ~FA__WRITTEN;
	fp->fs->wflag = 1;
	res = sync(fp->fs);

	return res;
}

#endif /* CONFIG_FS_FAT_WRITE */

/*
 * Close File
 */
int f_close (
	FIL *fp		/* Pointer to the file object to be closed */
)
{
#ifndef CONFIG_FS_FAT_WRITE
	fp->fs = 0;	/* Discard file object */
	return 0;
#else
	int res;

	/* Flush cached data */
	res = f_sync(fp);
	if (res == 0)
		fp->fs = NULL;	/* Discard file object */
	return res;
#endif
}

/*
 * Seek File R/W Pointer
 */
int f_lseek (
	FIL *fp,		/* Pointer to the file object */
	DWORD ofs		/* File pointer from top of file */
)
{
	DWORD clst, bcs, nsect, ifptr;
	int res = 0;

	if (fp->flag & FA__ERROR)		/* Check abort flag */
		return -ERESTARTSYS;

	if (ofs > fp->fsize	/* In read-only mode, clip offset with the file size */
#ifdef CONFIG_FS_FAT_WRITE
		 && !(fp->flag & FA_WRITE)
#endif
		) ofs = fp->fsize;

	ifptr = fp->fptr;
	fp->fptr = nsect = 0;
	if (ofs) {
		bcs = (DWORD)fp->fs->csize * SS(fp->fs);	/* Cluster size (byte) */
		if (ifptr > 0 &&
			(ofs - 1) / bcs >= (ifptr - 1) / bcs) {	/* When seek to same or following cluster, */
			fp->fptr = (ifptr - 1) & ~(bcs - 1);	/* start from the current cluster */
			ofs -= fp->fptr;
			clst = fp->clust;
		} else {					/* When seek to back cluster, */
			clst = fp->sclust;			/* start from the first cluster */
#ifdef CONFIG_FS_FAT_WRITE
			if (clst == 0) {			/* If no cluster chain, create a new chain */
				clst = create_chain(fp->fs, 0);
				if (clst == 1)
					ABORT(fp->fs, -ERESTARTSYS);
				if (clst == 0xFFFFFFFF)
					ABORT(fp->fs, -EIO);
				fp->sclust = clst;
			}
#endif
			fp->clust = clst;
		}
		if (clst != 0) {
			while (ofs > bcs) {	/* Cluster following loop */
#ifdef CONFIG_FS_FAT_WRITE
				if (fp->flag & FA_WRITE) {	/* Check if in write mode or not */
					/* Force stretch if in write mode */
					clst = create_chain(fp->fs, clst);
					/* When disk gets full, clip file size */
					if (clst == 0) {
						ofs = bcs;
						break;
					}
				} else
#endif
					/* Follow cluster chain if not in write mode */
					clst = get_fat(fp->fs, clst);
				if (clst == 0xFFFFFFFF)
					ABORT(fp->fs, -EIO);
				if (clst <= 1 || clst >= fp->fs->n_fatent)
					ABORT(fp->fs, -ERESTARTSYS);
				fp->clust = clst;
				fp->fptr += bcs;
				ofs -= bcs;
			}
			fp->fptr += ofs;
			if (ofs % SS(fp->fs)) {
				nsect = clust2sect(fp->fs, clst);	/* Current sector */
				if (!nsect)
					ABORT(fp->fs, -ERESTARTSYS);
				nsect += ofs / SS(fp->fs);
			}
		}
	}
	if (fp->fptr % SS(fp->fs) && nsect != fp->dsect) {	/* Fill sector cache if needed */
#ifdef CONFIG_FS_FAT_WRITE
		if (fp->flag & FA__DIRTY) {	/* Write-back dirty sector cache */
			if (disk_write(fp->fs, fp->buf, fp->dsect, 1) != RES_OK)
				ABORT(fp->fs, -EIO);
			fp->flag &= ~FA__DIRTY;
		}
#endif
		if (disk_read(fp->fs, fp->buf, nsect, 1) != RES_OK)	/* Fill sector cache */
			ABORT(fp->fs, -EIO);
		fp->dsect = nsect;
	}
#ifdef CONFIG_FS_FAT_WRITE
	if (fp->fptr > fp->fsize) {	/* Set file change flag if the file size is extended */
		fp->fsize = fp->fptr;
		fp->flag |= FA__WRITTEN;
	}
#endif

	return res;
}

/*
 * Create a Directroy Object
 */
int f_opendir (
	FATFS *fatfs,
	FF_DIR *dj,		/* Pointer to directory object to create */
	const TCHAR *path	/* Pointer to the directory path */
)
{
	int res;
	DEF_NAMEBUF;

	dj->fs = fatfs;

	INIT_BUF(*dj);
	res = follow_path(dj, path);	/* Follow the path to the directory */
	FREE_BUF();

	if (res)
		return res;

	if (dj->dir) {
		/* It is not the root dir */
		if (dj->dir[DIR_Attr] & AM_DIR) {
			/* The object is a directory */
			dj->sclust = LD_CLUST(dj->dir);
		} else {
			/* The object is not a directory */
			return -ENOTDIR;
		}
	}

	res = dir_sdi(dj, 0);	/* Rewind dir */

	return res;
}




/*
 * Read Directory Entry in Sequense
 */
int f_readdir (
	FF_DIR *dj,		/* Pointer to the open directory object */
	FILINFO *fno		/* Pointer to file information to return */
)
{
	int res;
	DEF_NAMEBUF;

	if (!fno) {
		res = dir_sdi(dj, 0);	/* Rewind the directory object */
		return res;
	}

	INIT_BUF(*dj);
	res = dir_read(dj);	/* Read an directory item */
	if (res == -ENOENT) { /* Reached end of dir */
		dj->sect = 0;
		res = 0;
		goto out;
	}

	/* A valid entry is found */
	get_fileinfo(dj, fno);		/* Get the object information */
	res = dir_next(dj, 0);		/* Increment index for next */
	if (res == -ENOENT) {
		dj->sect = 0;
		res = 0;
	}
out:
	FREE_BUF();

	return res;
}

/*
 * Get File Status
 */
int f_stat (
	FATFS *fatfs,
	const TCHAR *path,	/* Pointer to the file path */
	FILINFO *fno		/* Pointer to file information to return */
)
{
	int res;
	FF_DIR dj;
	DEF_NAMEBUF;

	dj.fs = fatfs;

	INIT_BUF(dj);
	res = follow_path(&dj, path);	/* Follow the file path */
	if (res == 0) {		/* Follow completed */
		if (dj.dir)		/* Found an object */
			get_fileinfo(&dj, fno);
		else			/* It is root dir */
			res = -ENOENT;
	}
	FREE_BUF();

	return res;
}

#ifdef CONFIG_FS_FAT_WRITE
/*
 * Get Number of Free Clusters
 */
int f_getfree (
	FATFS *fatfs,
	const TCHAR *path,	/* Pointer to the logical drive number (root dir) */
	DWORD *nclst		/* Pointer to the variable to return number of free clusters */
)
{
	int res = 0;
	DWORD n, clst, sect, stat;
	UINT i;
	BYTE fat, *p;


	/* If free_clust is valid, return it without full cluster scan */
	if (fatfs->free_clust <= fatfs->n_fatent - 2) {
		*nclst = fatfs->free_clust;
		return 0;
	}

	/* Get number of free clusters */
	fat = fatfs->fs_type;
	n = 0;
	if (fat == FS_FAT12) {
		clst = 2;
		do {
			stat = get_fat(fatfs, clst);
			if (stat == 0xFFFFFFFF) {
				res = -EIO;
				break;
			}
			if (stat == 1) {
				res = -ERESTARTSYS;
				break;
			}
			if (stat == 0) n++;
		} while (++clst < fatfs->n_fatent);
	} else {
		clst = fatfs->n_fatent;
		sect = fatfs->fatbase;
		i = 0; p = NULL;
		do {
			if (!i) {
				res = move_window(fatfs, sect++);
				if (res != 0) break;
				p = fatfs->win;
				i = SS(fatfs);
			}
			if (fat == FS_FAT16) {
				if (LD_WORD(p) == 0) n++;
				p += 2; i -= 2;
			} else {
				if ((LD_DWORD(p) & 0x0FFFFFFF) == 0) n++;
				p += 4; i -= 4;
			}
		} while (--clst);
	}
	fatfs->free_clust = n;
	if (fat == FS_FAT32)
		fatfs->fsi_flag = 1;
	*nclst = n;

	return res;
}

/*
 * Truncate File
 */
int f_truncate (
	FIL *fp		/* Pointer to the file object */
)
{
	int res = 0;
	DWORD ncl;

	if (fp->flag & FA__ERROR) {	/* Check abort flag */
		return -ERESTARTSYS;
	} else {
		if (!(fp->flag & FA_WRITE))	/* Check access mode */
			return -EROFS;
	}

	if (fp->fsize <= fp->fptr)
		return 0;

	fp->fsize = fp->fptr;	/* Set file size to current R/W point */
	fp->flag |= FA__WRITTEN;
	if (fp->fptr == 0) {
		/* When set file size to zero, remove entire cluster chain */
		res = remove_chain(fp->fs, fp->sclust);
		fp->sclust = 0;
	} else {
		/* When truncate a part of the file, remove remaining clusters */
		ncl = get_fat(fp->fs, fp->clust);
		if (ncl == 0xFFFFFFFF)
			return -EIO;
		if (ncl == 1)
			return -ERESTARTSYS;
		if (ncl < fp->fs->n_fatent) {
			res = put_fat(fp->fs, fp->clust, 0x0FFFFFFF);
			if (res)
				return res;
			res = remove_chain(fp->fs, ncl);
		}
	}

	return res;
}

/*
 * Delete a File or Directory
 */
int f_unlink (
	FATFS *fatfs,
	const TCHAR *path	/* Pointer to the file or directory path */
)
{
	int res;
	FF_DIR dj, sdj;
	BYTE *dir;
	DWORD dclst;
	DEF_NAMEBUF;

	dj.fs = fatfs;

	INIT_BUF(dj);
	res = follow_path(&dj, path);	/* Follow the file path */
	if (res)
		goto out;

	if (dj.fn[NS] & NS_DOT) {
		res = -EINVAL;		/* Cannot remove dot entry */
		goto out;
	}

	/* The object is accessible */
	dir = dj.dir;
	if (!dir) {
		res = -EINVAL;	/* Cannot remove the start directory */
		goto out;
	}

	if (dir[DIR_Attr] & AM_RDO) {
		res = -EROFS; /* Cannot remove R/O object */
		goto out;
	}

	dclst = LD_CLUST(dir);
	if (dir[DIR_Attr] & AM_DIR) {	/* Is it a sub-dir? */
		if (dclst < 2) {
			res = -ERESTARTSYS;
			goto out;
		}
		memcpy(&sdj, &dj, sizeof(FF_DIR));	/* Check if the sub-dir is empty or not */
		sdj.sclust = dclst;
		res = dir_sdi(&sdj, 2);		/* Exclude dot entries */
		if (res)
			goto out;

		res = dir_read(&sdj);
		if (res == 0) {
			res = -ENOTEMPTY; /* Not empty dir */
			goto out;
		}
		if (res == -ENOENT)
			res = 0;	/* Empty */
		if (res)
			goto out;
	}

	res = dir_remove(&dj);	/* Remove the directory entry */
	if (res)
		goto out;

	if (dclst) {
		/* Remove the cluster chain if exist */
		res = remove_chain(dj.fs, dclst);
		if (res)
			goto out;
	}

	res = sync(dj.fs);

out:
	FREE_BUF();

	return res;
}

/*
 * Create a Directory
 */
int f_mkdir (
	FATFS *fatfs,
	const TCHAR *path	/* Pointer to the directory path */
)
{
	int res;
	FF_DIR dj;
	BYTE *dir, n;
	DWORD dsc, dcl, pcl, tim = get_fattime();
	DEF_NAMEBUF;

	dj.fs = fatfs;

	INIT_BUF(dj);
	res = follow_path(&dj, path); /* Follow the file path */
	if (res == 0) {
		res = -EEXIST;	/* Any object with same name is already existing */
		goto out;
	}

	if (res != -ENOENT)
		goto out;

	dcl = create_chain(dj.fs, 0);	/* Allocate a cluster for the new directory table */
	if (dcl == 0) {
		res = -ENOSPC;	/* No space to allocate a new cluster */
		goto out;
	}
	if (dcl == 1) {
		res = -ERESTARTSYS;
		goto out;
	}

	if (dcl == 0xFFFFFFFF) {
		res = -EIO;
		goto out;
	}

	/* Flush FAT */
	res = move_window(dj.fs, 0);
	if (res)
		goto out;

	/* Initialize the new directory table */
	dsc = clust2sect(dj.fs, dcl);
	dir = dj.fs->win;
	memset(dir, 0, SS(dj.fs));
	memset(dir+DIR_Name, ' ', 8+3);	/* Create "." entry */
	dir[DIR_Name] = '.';
	dir[DIR_Attr] = AM_DIR;
	ST_DWORD(dir+DIR_WrtTime, tim);
	ST_CLUST(dir, dcl);
	memcpy(dir+SZ_DIR, dir, SZ_DIR); 	/* Create ".." entry */
	dir[33] = '.'; pcl = dj.sclust;
	if (dj.fs->fs_type == FS_FAT32 && pcl == dj.fs->dirbase)
		pcl = 0;
	ST_CLUST(dir+SZ_DIR, pcl);
	for (n = dj.fs->csize; n; n--) {	/* Write dot entries and clear following sectors */
		dj.fs->winsect = dsc++;
		dj.fs->wflag = 1;
		res = move_window(dj.fs, 0);
		if (res != 0)
			goto out;
		memset(dir, 0, SS(dj.fs));
	}

	res = dir_register(&dj);	/* Register the object to the directoy */
	if (res) {
		remove_chain(dj.fs, dcl);		/* Could not register, remove cluster chain */
		goto out;
	}

	dir = dj.dir;
	dir[DIR_Attr] = AM_DIR;			/* Attribute */
	ST_DWORD(dir+DIR_WrtTime, tim);		/* Created time */
	ST_CLUST(dir, dcl);			/* Table start cluster */
	dj.fs->wflag = 1;
	res = sync(dj.fs);

out:
	FREE_BUF();

	return res;
}

/*
 * Change Attribute
 */
int f_chmod (
	FATFS *fatfs,
	const TCHAR *path,	/* Pointer to the file path */
	BYTE value,			/* Attribute bits */
	BYTE mask			/* Attribute mask to change */
)
{
	int res;
	FF_DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;

	dj.fs = fatfs;

	INIT_BUF(dj);
	res = follow_path(&dj, path);		/* Follow the file path */
	FREE_BUF();
	if (res == 0 && (dj.fn[NS] & NS_DOT))
		res = -ENOENT;
	if (res == 0) {
		dir = dj.dir;
		if (!dir) {			/* Is it a root directory? */
			res = -EINVAL;
		} else {			/* File or sub directory */
			mask &= AM_RDO|AM_HID|AM_SYS|AM_ARC;	/* Valid attribute mask */
			dir[DIR_Attr] = (value & mask) | (dir[DIR_Attr] & (BYTE)~mask);	/* Apply attribute change */
			dj.fs->wflag = 1;
			res = sync(dj.fs);
		}
	}

	return res;
}

/*
 * Change Timestamp
 */
int f_utime (
	FATFS *fatfs,
	const TCHAR *path,	/* Pointer to the file/directory name */
	const FILINFO *fno	/* Pointer to the time stamp to be set */
)
{
	int res;
	FF_DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;

	dj.fs = fatfs;

	INIT_BUF(dj);
	res = follow_path(&dj, path);	/* Follow the file path */
	FREE_BUF();
	if (res == 0 && (dj.fn[NS] & NS_DOT))
		res = -ENOENT;
	if (res == 0) {
		dir = dj.dir;
		if (!dir) {		/* Root directory */
			res = -EINVAL;
		} else {		/* File or sub-directory */
			ST_WORD(dir+DIR_WrtTime, fno->ftime);
			ST_WORD(dir+DIR_WrtDate, fno->fdate);
			dj.fs->wflag = 1;
			res = sync(dj.fs);
		}
	}

	return res;
}

/*
 * Rename File/Directory
 */
int f_rename (
	FATFS *fatfs,
	const TCHAR *path_old,	/* Pointer to the old name */
	const TCHAR *path_new	/* Pointer to the new name */
)
{
	int res;
	FF_DIR djo, djn;
	BYTE buf[21], *dir;
	DWORD dw;
	DEF_NAMEBUF;

	djo.fs = fatfs;

	djn.fs = djo.fs;
	INIT_BUF(djo);
	res = follow_path(&djo, path_old);	/* Check old object */
	if (res == 0 && (djo.fn[NS] & NS_DOT)) {
		res = -EINVAL;
		goto out;
	}

	if (res)
		goto out;

	/* Old object is found */
	if (!djo.dir) {			/* Is root dir? */
		res = -EINVAL;
		goto out;
	}

	memcpy(buf, djo.dir+DIR_Attr, 21);	/* Save the object information except for name */
	memcpy(&djn, &djo, sizeof(FF_DIR));	/* Check new object */
	res = follow_path(&djn, path_new);
	if (res == 0) {
		res = -EEXIST;	/* The new object name is already existing */
		goto out;
	}

	if (res != -ENOENT)
		goto out;

	/* It ia a valid path and no name collision */

	res = dir_register(&djn);	/* Register the new entry */
	if (res)
		goto out;

	dir = djn.dir;		/* Copy object information except for name */
	memcpy(dir+13, buf+2, 19);
	dir[DIR_Attr] = buf[0] | AM_ARC;
	djo.fs->wflag = 1;
	/* Update .. entry in the directory if needed */
	if (djo.sclust != djn.sclust && (dir[DIR_Attr] & AM_DIR)) {
		dw = clust2sect(djn.fs, LD_CLUST(dir));
		if (!dw) {
			res = -ERESTARTSYS;
			goto out;
		}

		res = move_window(djn.fs, dw);
		if (res)
			goto out;
		dir = djn.fs->win+SZ_DIR;	/* .. entry */
		if (dir[1] == '.') {
			dw = (djn.fs->fs_type == FS_FAT32 && djn.sclust == djn.fs->dirbase) ? 0 : djn.sclust;
			ST_CLUST(dir, dw);
			djn.fs->wflag = 1;
		}
	}

	res = dir_remove(&djo);		/* Remove old entry */
	if (res)
		goto out;

	res = sync(djo.fs);

out:
	FREE_BUF();

	return res;
}
#endif /* CONFIG_FS_FAT_WRITE */

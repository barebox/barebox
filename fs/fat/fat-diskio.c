// SPDX-License-Identifier: GPL-2.0-only
/*
 * fat.c - FAT filesystem barebox driver
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <linux/ctype.h>
#include "integer.h"
#include "ff.h"
#include "diskio.h"

DSTATUS disk_status(FATFS *fat)
{
	return 0;
}

DWORD get_fattime(void)
{
	return 0;
}

DRESULT disk_ioctl (FATFS *fat, BYTE command, void *buf)
{
	switch (command) {
	case GET_SECTOR_SIZE:
		*(WORD *)buf = disk_sector_size(fat);
		return RES_OK;
	case CTRL_SYNC:
		return RES_OK;
	default:
		return RES_PARERR;
	}
}

WCHAR ff_convert(WCHAR src, UINT dir)
{
	if (src <= 0x80)
		return src;
	else
		return '?';
}

WCHAR ff_wtoupper(WCHAR chr)
{
	if (chr <= 0x80)
		return toupper(chr);
	else
		return '?';
}

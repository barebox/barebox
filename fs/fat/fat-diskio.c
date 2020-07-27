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
	return 0;
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

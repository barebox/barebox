/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#ifndef __ASM_HTIF_LL__
#define __ASM_HTIF_LL__

#define HTIF_DEFAULT_BASE_ADDR		0x40008000

#define HTIF_DEV_SYSCALL		0
#define		HTIF_CMD_SYSCALL	0

#define HTIF_DEV_CONSOLE		1 /* blocking character device */
#define		HTIF_CMD_GETCHAR	0
#define		HTIF_CMD_PUTCHAR	1

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <io-64-nonatomic-lo-hi.h>

static inline void __htif_tohost(void __iomem *htif, u8 device, u8 command, u64 arg)
{
	writeq(((u64)device << 56) | ((u64)command << 48) | arg, htif);
}

static inline void htif_tohost(u8 device, u8 command, u64 arg)
{
	__htif_tohost(IOMEM(HTIF_DEFAULT_BASE_ADDR), device, command, arg);
}

static inline void htif_putc(void __iomem *base, int c)
{
	__htif_tohost(base, HTIF_DEV_CONSOLE, HTIF_CMD_PUTCHAR, c);
}

#endif

#endif

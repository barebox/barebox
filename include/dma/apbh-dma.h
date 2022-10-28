/*
 * Freescale i.MX28 APBH DMA
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * Based on code from LTIB:
 * Copyright 2008-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __DMA_H__
#define __DMA_H__

#include <linux/list.h>

#define MXS_DMA_ALIGNMENT	32

/*
 * MXS DMA channels
 */
enum {
	MXS_DMA_CHANNEL_AHB_APBH_SSP0 = 0,
	MXS_DMA_CHANNEL_AHB_APBH_SSP1,
	MXS_DMA_CHANNEL_AHB_APBH_SSP2,
	MXS_DMA_CHANNEL_AHB_APBH_SSP3,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI0,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI1,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI2,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI3,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI4,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI5,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI6,
	MXS_DMA_CHANNEL_AHB_APBH_GPMI7,
	MXS_DMA_CHANNEL_AHB_APBH_SSP,
	MXS_MAX_DMA_CHANNELS,
};

/*
 * MXS DMA hardware command.
 *
 * This structure describes the in-memory layout of an entire DMA command,
 * including space for the maximum number of PIO accesses. See the appropriate
 * reference manual for a detailed description of what these fields mean to the
 * DMA hardware.
 */
#define	MXS_DMA_DESC_COMMAND_MASK	0x3
#define	MXS_DMA_DESC_COMMAND_OFFSET	0
#define	MXS_DMA_DESC_COMMAND_NO_DMAXFER	0x0
#define	MXS_DMA_DESC_COMMAND_DMA_WRITE	0x1
#define	MXS_DMA_DESC_COMMAND_DMA_READ	0x2
#define	MXS_DMA_DESC_COMMAND_DMA_SENSE	0x3
#define	MXS_DMA_DESC_CHAIN		(1 << 2)
#define	MXS_DMA_DESC_IRQ		(1 << 3)
#define	MXS_DMA_DESC_NAND_LOCK		(1 << 4)
#define	MXS_DMA_DESC_NAND_WAIT_4_READY	(1 << 5)
#define	MXS_DMA_DESC_DEC_SEM		(1 << 6)
#define	MXS_DMA_DESC_WAIT4END		(1 << 7)
#define	MXS_DMA_DESC_HALT_ON_TERMINATE	(1 << 8)
#define	MXS_DMA_DESC_TERMINATE_FLUSH	(1 << 9)
#define	MXS_DMA_DESC_PIO_WORDS_MASK	(0xf << 12)
#define	MXS_DMA_DESC_PIO_WORDS_OFFSET	12
#define	MXS_DMA_DESC_BYTES_MASK		(0xffff << 16)
#define	MXS_DMA_DESC_BYTES_OFFSET	16

struct mxs_dma_cmd {
	unsigned long		next;
	unsigned long		data;
	union {
		dma_addr_t	address;
		unsigned long	alternate;
	};
#define	APBH_DMA_PIO_WORDS		15
	unsigned long		pio_words[APBH_DMA_PIO_WORDS];
};

int mxs_dma_go(int chan, struct mxs_dma_cmd *cmd, int ncmds);
int mxs_dma_init(void);

#endif	/* __DMA_H__ */

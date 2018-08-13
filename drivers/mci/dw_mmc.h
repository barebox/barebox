/*
 * Copyright (C) 2013 Altera Corporation <www.altera.com>
 *
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
 * Rajeshawari Shinde <rajeshwari.s@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef  __DW_MMC_H__
#define	__DW_MMC_H__

#include <linux/bitops.h>

#define DWMCI_CTRL		0x000
#define	DWMCI_PWREN		0x004
#define DWMCI_CLKDIV		0x008
#define DWMCI_CLKSRC		0x00C
#define DWMCI_CLKENA		0x010
#define DWMCI_TMOUT		0x014
#define DWMCI_CTYPE		0x018
#define DWMCI_BLKSIZ		0x01C
#define DWMCI_BYTCNT		0x020
#define DWMCI_INTMASK		0x024
#define DWMCI_CMDARG		0x028
#define DWMCI_CMD		0x02C
#define DWMCI_RESP0		0x030
#define DWMCI_RESP1		0x034
#define DWMCI_RESP2		0x038
#define DWMCI_RESP3		0x03C
#define DWMCI_MINTSTS		0x040
#define DWMCI_RINTSTS		0x044
#define DWMCI_STATUS		0x048
#define DWMCI_FIFOTH		0x04C
#define DWMCI_CDETECT		0x050
#define DWMCI_WRTPRT		0x054
#define DWMCI_GPIO		0x058
#define DWMCI_TCMCNT		0x05C
#define DWMCI_TBBCNT		0x060
#define DWMCI_DEBNCE		0x064
#define DWMCI_USRID		0x068
#define DWMCI_VERID		0x06C
#define DWMCI_HCON		0x070
#define DWMCI_UHS_REG		0x074
#define DWMCI_BMOD		0x080
#define DWMCI_PLDMND		0x084
#define DWMCI_DBADDR		0x088
#define DWMCI_IDSTS		0x08C
#define DWMCI_IDINTEN		0x090
#define DWMCI_DSCADDR		0x094
#define DWMCI_BUFADDR		0x098
#define DWMCI_DATA		0x200

/* Interrupt Mask register */
#define DWMCI_INTMSK_ALL	0xffffffff
#define DWMCI_INTMSK_RE		BIT(1)
#define DWMCI_INTMSK_CDONE	BIT(2)
#define DWMCI_INTMSK_DTO	BIT(3)
#define DWMCI_INTMSK_TXDR	BIT(4)
#define DWMCI_INTMSK_RXDR	BIT(5)
#define DWMCI_INTMSK_RCRC	BIT(6)
#define DWMCI_INTMSK_DCRC	BIT(7)
#define DWMCI_INTMSK_RTO	BIT(8)
#define DWMCI_INTMSK_DRTO	BIT(9)
#define DWMCI_INTMSK_HTO	BIT(10)
#define DWMCI_INTMSK_FRUN	BIT(11)
#define DWMCI_INTMSK_HLE	BIT(12)
#define DWMCI_INTMSK_SBE	BIT(13)
#define DWMCI_INTMSK_ACD	BIT(14)
#define DWMCI_INTMSK_EBE	BIT(15)

/* Raw interrupt Register */
#define DWMCI_DATA_ERR	(DWMCI_INTMSK_EBE | DWMCI_INTMSK_SBE | DWMCI_INTMSK_HLE |\
			DWMCI_INTMSK_FRUN | DWMCI_INTMSK_DCRC)
#define DWMCI_DATA_TOUT	(DWMCI_INTMSK_HTO | DWMCI_INTMSK_DRTO)

/* CTRL register */
#define DWMCI_CTRL_RESET	BIT(0)
#define DWMCI_CTRL_FIFO_RESET	BIT(1)
#define DWMCI_CTRL_DMA_RESET	BIT(2)
#define DWMCI_DMA_EN		BIT(5)
#define DWMCI_CTRL_SEND_AS_CCSD	BIT(10)
#define DWMCI_IDMAC_EN		BIT(25)
#define DWMCI_RESET_ALL		(DWMCI_CTRL_RESET | DWMCI_CTRL_FIFO_RESET |	\
				DWMCI_CTRL_DMA_RESET)

/* CMD register */
#define DWMCI_CMD_RESP_EXP	BIT(6)
#define DWMCI_CMD_RESP_LENGTH	BIT(7)
#define DWMCI_CMD_CHECK_CRC	BIT(8)
#define DWMCI_CMD_DATA_EXP	BIT(9)
#define DWMCI_CMD_RW		BIT(10)
#define DWMCI_CMD_SEND_STOP	BIT(12)
#define DWMCI_CMD_ABORT_STOP	BIT(14)
#define DWMCI_CMD_PRV_DAT_WAIT	BIT(13)
#define DWMCI_CMD_UPD_CLK	BIT(21)
#define DWMCI_CMD_USE_HOLD_REG	BIT(29)
#define DWMCI_CMD_START		BIT(31)

/* CLKENA register */
#define DWMCI_CLKEN_ENABLE	BIT(0)
#define DWMCI_CLKEN_LOW_PWR	BIT(16)

/* Card-type register */
#define DWMCI_CTYPE_1BIT	0
#define DWMCI_CTYPE_4BIT	BIT(0)
#define DWMCI_CTYPE_8BIT	BIT(16)

/* Status Register */
#define DWMCI_STATUS_FIFO_EMPTY	BIT(2)
#define DWMCI_STATUS_FIFO_FULL	BIT(3)
#define DWMCI_STATUS_BUSY	BIT(9)

/* FIFOTH Register */
#define DWMCI_FIFOTH_MSIZE(x)		((x) << 28)
#define DWMCI_FIFOTH_RX_WMARK(x)	((x) << 16)
#define DWMCI_FIFOTH_TX_WMARK(x)	(x)
#define DWMCI_FIFOTH_FIFO_DEPTH(x)	((((x) >> 16) & 0x3ff) + 1)

#define DWMCI_IDMAC_OWN	BIT(31)
#define DWMCI_IDMAC_CH	BIT(4)
#define DWMCI_IDMAC_FS	BIT(3)
#define DWMCI_IDMAC_LD	BIT(2)

/* Bus Mode Register */
#define DWMCI_BMOD_IDMAC_RESET	BIT(0)
#define DWMCI_BMOD_IDMAC_FB	BIT(1)
#define DWMCI_BMOD_IDMAC_EN	BIT(7)

#endif

/*
 * Freescale MXS SSP
 *
 * Copyright (C) 2013 Michael Grzeschik <mgr@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SSP_H__
#define __SSP_H__

#ifdef CONFIG_ARCH_IMX23
# define HW_SSP_CTRL0		0x000
# define HW_SSP_CMD0		0x010
# define HW_SSP_CMD1		0x020
# define HW_SSP_COMPREF		0x030
# define HW_SSP_COMPMASK	0x040
# define HW_SSP_TIMING		0x050
# define HW_SSP_CTRL1		0x060
# define HW_SSP_DATA		0x070
# define HW_SSP_SDRESP0		0x080
# define HW_SSP_SDRESP1		0x090
# define HW_SSP_SDRESP2		0x0A0
# define HW_SSP_SDRESP3		0x0B0
# define HW_SSP_STATUS		0x0C0
# define HW_SSP_DEBUG		0x100
# define HW_SSP_VERSION		0x110
#endif

#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_CTRL0		0x000
# define HW_SSP_CMD0		0x010
# define HW_SSP_CMD1		0x020
# define HW_SSP_XFER_COUNT	0x030
# define HW_SSP_BLOCK_SIZE	0x040
# define HW_SSP_COMPREF		0x050
# define HW_SSP_COMPMASK	0x060
# define HW_SSP_TIMING		0x070
# define HW_SSP_CTRL1		0x080
# define HW_SSP_DATA		0x090
# define HW_SSP_SDRESP0		0x0A0
# define HW_SSP_SDRESP1		0x0B0
# define HW_SSP_SDRESP2		0x0C0
# define HW_SSP_SDRESP3		0x0D0
# define HW_SSP_DDR_CTRL	0x0E0
# define HW_SSP_DLL_CTRL	0x0F0
# define HW_SSP_STATUS		0x100
# define HW_SSP_DLL_STS		0x110
# define HW_SSP_DEBUG		0x120
# define HW_SSP_VERSION		0x130
#endif

#define SSP_CTRL0_SFTRST		(1 << 31)
#define SSP_CTRL0_CLKGATE		(1 << 30)
#define SSP_CTRL0_RUN			(1 << 29)
#define SSP_CTRL0_LOCK_CS		(1 << 27)
#define SSP_CTRL0_READ			(1 << 25)
#define SSP_CTRL0_IGNORE_CRC		(1 << 26)
#define SSP_CTRL0_DATA_XFER		(1 << 24)
#define SSP_CTRL0_BUS_WIDTH(x)		(((x) & 0x3) << 22)
#define SSP_CTRL0_WAIT_FOR_IRQ		(1 << 21)
#define SSP_CTRL0_WAIT_FOR_CMD		(1 << 20)
#define SSP_CTRL0_SSP_ASSERT_OUT(x)	(((x) & 0x3) << 20)
#define SSP_CTRL0_LONG_RESP		(1 << 19)
#define SSP_CTRL0_GET_RESP		(1 << 17)
#define SSP_CTRL0_ENABLE		(1 << 16)

#define SSP_CMD0_SLOW_CLK		(1 << 22)
#define SSP_CMD0_CONT_CLK		(1 << 21)
#define SSP_CMD0_APPEND_8CYC		(1 << 20)
#ifdef CONFIG_ARCH_IMX23
# define SSP_CTRL0_XFER_COUNT(x)	((x) & 0xffff)
# define SSP_CMD0_BLOCK_SIZE(x)		(((x) & 0xf) << 16)
# define SSP_CMD0_BLOCK_COUNT(x)	(((x) & 0xff) << 8)
#endif
#define SSP_CMD0_CMD(x)			((x) & 0xff)

#ifdef CONFIG_ARCH_IMX28
# define SSP_BLOCK_SIZE(x)		((x) & 0xf)
# define SSP_BLOCK_COUNT(x)		(((x) & 0xffffff) << 4)
#endif

/* bit definition for register HW_SSP_TIMING */
#define SSP_TIMING_TIMEOUT_MASK	(0xffff0000)
#define SSP_TIMING_TIMEOUT(x)		((x) << 16)
#define SSP_TIMING_CLOCK_DIVIDE(x)	(((x) & 0xff) << 8)
#define SSP_TIMING_CLOCK_RATE(x)	((x) & 0xff)

/* bit definition for register HW_SSP_CTRL1 */
#define SSP_CTRL1_POLARITY		(1 << 9)
#define SSP_CTRL1_PHASE			(1 << 10)
#define SSP_CTRL1_DMA_ENABLE		(1 << 13)
#define SSP_CTRL1_WORD_LENGTH(x)	(((x) & 0xf) << 4)
#define SSP_CTRL1_SSP_MODE(x)		((x) & 0xf)

/* bit definition for register HW_SSP_STATUS */
# define SSP_STATUS_PRESENT		(1 << 31)
# define SSP_STATUS_SD_PRESENT		(1 << 29)
# define SSP_STATUS_CARD_DETECT		(1 << 28)
# define SSP_STATUS_RESP_CRC_ERR	(1 << 16)
# define SSP_STATUS_RESP_ERR		(1 << 15)
# define SSP_STATUS_RESP_TIMEOUT	(1 << 14)
# define SSP_STATUS_DATA_CRC_ERR	(1 << 13)
# define SSP_STATUS_TIMEOUT		(1 << 12)
# define SSP_STATUS_FIFO_OVRFLW		(1 << 9)
# define SSP_STATUS_FIFO_FULL		(1 << 8)
# define SSP_STATUS_FIFO_EMPTY		(1 << 5)
# define SSP_STATUS_FIFO_UNDRFLW	(1 << 4)
# define SSP_STATUS_CMD_BUSY		(1 << 3)
# define SSP_STATUS_DATA_BUSY		(1 << 2)
# define SSP_STATUS_BUSY		(1 << 0)
# define SSP_STATUS_ERROR		(SSP_STATUS_FIFO_OVRFLW | SSP_STATUS_FIFO_UNDRFLW | \
	SSP_STATUS_RESP_CRC_ERR | SSP_STATUS_RESP_ERR | \
	SSP_STATUS_RESP_TIMEOUT | SSP_STATUS_DATA_CRC_ERR | SSP_STATUS_TIMEOUT)

#endif	/* __SSP_H__ */

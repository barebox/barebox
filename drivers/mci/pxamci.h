/*
 *  PXA MCI driver
 *
 * Copyright (C) 2011 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Insprired by linux kernel driver
 */

#define MMC_FIFO_LENGTH			32

#define MMC_STRPCL			0x0000
#define STOP_CLOCK			(1 << 0)
#define START_CLOCK			(2 << 0)

#define MMC_STAT			0x0004
#define STAT_END_CMD_RES		(1 << 13)
#define STAT_PRG_DONE			(1 << 12)
#define STAT_DATA_TRAN_DONE		(1 << 11)
#define STAT_CLK_EN			(1 << 8)
#define STAT_RECV_FIFO_FULL		(1 << 7)
#define STAT_XMIT_FIFO_EMPTY		(1 << 6)
#define STAT_RES_CRC_ERR		(1 << 5)
#define STAT_SPI_READ_ERROR_TOKEN	(1 << 4)
#define STAT_CRC_READ_ERROR		(1 << 3)
#define STAT_CRC_WRITE_ERROR		(1 << 2)
#define STAT_TIME_OUT_RESPONSE		(1 << 1)
#define STAT_READ_TIME_OUT		(1 << 0)

#define MMC_CLKRT			0x0008		/* 3 bit */

#define MMC_SPI				0x000c
#define SPI_CS_ADDRESS			(1 << 3)
#define SPI_CS_EN			(1 << 2)
#define CRC_ON				(1 << 1)
#define SPI_EN				(1 << 0)

#define MMC_CMDAT			0x0010
#define CMDAT_SDIO_INT_EN		(1 << 11)
#define CMDAT_STOP_TRAN			(1 << 10)
#define CMDAT_SD_4DAT			(1 << 8)
#define CMDAT_DMAEN			(1 << 7)
#define CMDAT_INIT			(1 << 6)
#define CMDAT_BUSY			(1 << 5)
#define CMDAT_STREAM			(1 << 4)	/* 1 = stream */
#define CMDAT_WRITE			(1 << 3)	/* 1 = write */
#define CMDAT_DATAEN			(1 << 2)
#define CMDAT_RESP_NONE			(0 << 0)
#define CMDAT_RESP_SHORT		(1 << 0)
#define CMDAT_RESP_R2			(2 << 0)
#define CMDAT_RESP_R3			(3 << 0)

#define MMC_RESTO			0x0014		/* 7 bit */
#define MMC_RDTO			0x0018		/* 16 bit */
#define MMC_BLKLEN			0x001c		/* 10 bit */
#define MMC_NOB				0x0020		/* 16 bit */

#define MMC_PRTBUF			0x0024
#define BUF_PART_FULL			(1 << 0)

#define MMC_I_REG			0x002c
#define TXFIFO_WR_REQ			(1 << 6)
#define RXFIFO_RD_REQ			(1 << 5)
#define CLK_IS_OFF			(1 << 4)
#define STOP_CMD			(1 << 3)
#define END_CMD_RES			(1 << 2)
#define PRG_DONE			(1 << 1)
#define DATA_TRAN_DONE			(1 << 0)

#define MMC_I_MASK			0x0028
#define MMC_CMD				0x0030
#define MMC_ARGH			0x0034		/* 16 bit */
#define MMC_ARGL			0x0038		/* 16 bit */
#define MMC_RES				0x003c		/* 16 bit */
#define MMC_RXFIFO			0x0040		/* 8 bit */
#define MMC_TXFIFO			0x0044		/* 8 bit */

struct pxamci_host {
	struct mci_host			mci;
	void __iomem			*base;
	struct pxamci_platform_data	*pdata;

	unsigned int			cmdat;
	int				clkrt;
};

#define to_pxamci(mci) container_of(mci, struct pxamci_host, mci)

#define mmc_readb(reg) readb(host->base + (reg))
#define mmc_readl(reg) readl(host->base + (reg))
#define mmc_writeb(val, reg) writeb((val), host->base + (reg))
#define mmc_writel(val, reg) writel((val), host->base + (reg))

#define mci_dbg(fmt, arg...) \
	dev_dbg(host->mci.hw_dev, "%s: " fmt, __func__, ## arg)
#define mci_err(fmt, arg...) \
	dev_err(host->mci.hw_dev, "%s: " fmt, __func__, ## arg)

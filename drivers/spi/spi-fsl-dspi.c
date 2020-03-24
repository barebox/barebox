// SPDX-License-Identifier: GPL-2.0+
//
// Copyright 2013 Freescale Semiconductor, Inc.
//
// Freescale DSPI driver
// This file contains a driver for the Freescale DSPI

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <regmap.h>
#include <spi/spi.h>
#include <linux/clk.h>
#include <linux/math64.h>

#define DRIVER_NAME			"fsl-dspi"

#define DSPI_FIFO_SIZE			4
#define DSPI_DMA_BUFSIZE		(DSPI_FIFO_SIZE * 1024)

#define SPI_MCR				0x00
#define SPI_MCR_MASTER			BIT(31)
#define SPI_MCR_PCSIS			(0x3F << 16)
#define SPI_MCR_CLR_TXF			BIT(11)
#define SPI_MCR_CLR_RXF			BIT(10)
#define SPI_MCR_XSPI			BIT(3)

#define SPI_TCR				0x08
#define SPI_TCR_GET_TCNT(x)		(((x) & GENMASK(31, 16)) >> 16)

#define SPI_CTAR(x)			(0x0c + (((x) & GENMASK(1, 0)) * 4))
#define SPI_CTAR_FMSZ(x)		(((x) << 27) & GENMASK(30, 27))
#define SPI_CTAR_CPOL			BIT(26)
#define SPI_CTAR_CPHA			BIT(25)
#define SPI_CTAR_LSBFE			BIT(24)
#define SPI_CTAR_PCSSCK(x)		(((x) << 22) & GENMASK(23, 22))
#define SPI_CTAR_PASC(x)		(((x) << 20) & GENMASK(21, 20))
#define SPI_CTAR_PDT(x)			(((x) << 18) & GENMASK(19, 18))
#define SPI_CTAR_PBR(x)			(((x) << 16) & GENMASK(17, 16))
#define SPI_CTAR_CSSCK(x)		(((x) << 12) & GENMASK(15, 12))
#define SPI_CTAR_ASC(x)			(((x) << 8) & GENMASK(11, 8))
#define SPI_CTAR_DT(x)			(((x) << 4) & GENMASK(7, 4))
#define SPI_CTAR_BR(x)			((x) & GENMASK(3, 0))
#define SPI_CTAR_SCALE_BITS		0xf

#define SPI_CTAR0_SLAVE			0x0c

#define SPI_SR				0x2c
#define SPI_SR_TCFQF			BIT(31)
#define SPI_SR_EOQF			BIT(28)
#define SPI_SR_TFUF			BIT(27)
#define SPI_SR_TFFF			BIT(25)
#define SPI_SR_CMDTCF			BIT(23)
#define SPI_SR_SPEF			BIT(21)
#define SPI_SR_RFOF			BIT(19)
#define SPI_SR_TFIWF			BIT(18)
#define SPI_SR_RFDF			BIT(17)
#define SPI_SR_CMDFFF			BIT(16)
#define SPI_SR_CLEAR			(SPI_SR_TCFQF | SPI_SR_EOQF | \
					SPI_SR_TFUF | SPI_SR_TFFF | \
					SPI_SR_CMDTCF | SPI_SR_SPEF | \
					SPI_SR_RFOF | SPI_SR_TFIWF | \
					SPI_SR_RFDF | SPI_SR_CMDFFF)

#define SPI_RSER_TFFFE			BIT(25)
#define SPI_RSER_TFFFD			BIT(24)
#define SPI_RSER_RFDFE			BIT(17)
#define SPI_RSER_RFDFD			BIT(16)

#define SPI_RSER			0x30
#define SPI_RSER_TCFQE			BIT(31)
#define SPI_RSER_EOQFE			BIT(28)

#define SPI_PUSHR			0x34
#define SPI_PUSHR_CMD_CONT		BIT(15)
#define SPI_PUSHR_CMD_CTAS(x)		(((x) << 12 & GENMASK(14, 12)))
#define SPI_PUSHR_CMD_EOQ		BIT(11)
#define SPI_PUSHR_CMD_CTCNT		BIT(10)
#define SPI_PUSHR_CMD_PCS(x)		(BIT(x) & GENMASK(5, 0))

#define SPI_PUSHR_SLAVE			0x34

#define SPI_POPR			0x38

#define SPI_TXFR0			0x3c
#define SPI_TXFR1			0x40
#define SPI_TXFR2			0x44
#define SPI_TXFR3			0x48
#define SPI_RXFR0			0x7c
#define SPI_RXFR1			0x80
#define SPI_RXFR2			0x84
#define SPI_RXFR3			0x88

#define SPI_CTARE(x)			(0x11c + (((x) & GENMASK(1, 0)) * 4))
#define SPI_CTARE_FMSZE(x)		(((x) & 0x1) << 16)
#define SPI_CTARE_DTCP(x)		((x) & 0x7ff)

#define SPI_SREX			0x13c

#define SPI_FRAME_BITS(bits)		SPI_CTAR_FMSZ((bits) - 1)
#define SPI_FRAME_EBITS(bits)		SPI_CTARE_FMSZE(((bits) - 1) >> 4)

/* Register offsets for regmap_pushr */
#define PUSHR_CMD			0x0
#define PUSHR_TX			0x2

#define DMA_COMPLETION_TIMEOUT		msecs_to_jiffies(3000)

struct chip_data {
	u32			ctar_val;
	u16			void_write_data;
};

struct fsl_dspi_devtype_data {
	u8			max_clock_factor;
	bool			ptp_sts_supported;
	bool			xspi_mode;
};

static const struct fsl_dspi_devtype_data ls1021a_v1_data = {
	.max_clock_factor	= 8,
	.ptp_sts_supported	= true,
	.xspi_mode		= true,
};

static const struct fsl_dspi_devtype_data ls2085a_data = {
	.max_clock_factor	= 8,
	.ptp_sts_supported	= true,
};

struct fsl_dspi {
	struct spi_controller			ctlr;
	struct device_d				*dev;

	struct regmap				*regmap;
	struct regmap				*regmap_pushr;
	int					irq;
	struct clk				*clk;

	struct spi_transfer			*cur_transfer;
	struct spi_message			*cur_msg;
	struct chip_data			*cur_chip;
	size_t					progress;
	size_t					len;
	const void				*tx;
	void					*rx;
	void					*rx_end;
	u16					void_write_data;
	u16					tx_cmd;
	u8					bits_per_word;
	u8					bytes_per_word;
	const struct fsl_dspi_devtype_data	*devtype_data;
};

static u32 dspi_pop_tx(struct fsl_dspi *dspi)
{
	u32 txdata = 0;

	if (dspi->tx) {
		if (dspi->bytes_per_word == 1)
			txdata = *(u8 *)dspi->tx;
		else if (dspi->bytes_per_word == 2)
			txdata = *(u16 *)dspi->tx;
		else  /* dspi->bytes_per_word == 4 */
			txdata = *(u32 *)dspi->tx;
		dspi->tx += dspi->bytes_per_word;
	}
	dspi->len -= dspi->bytes_per_word;
	return txdata;
}

static u32 dspi_pop_tx_pushr(struct fsl_dspi *dspi)
{
	u16 cmd = dspi->tx_cmd, data = dspi_pop_tx(dspi);

	if (dspi->len > 0)
		cmd |= SPI_PUSHR_CMD_CONT;
	return cmd << 16 | data;
}

static void dspi_push_rx(struct fsl_dspi *dspi, u32 rxdata)
{
	if (!dspi->rx)
		return;

	/* Mask off undefined bits */
	rxdata &= (1 << dspi->bits_per_word) - 1;

	if (dspi->bytes_per_word == 1)
		*(u8 *)dspi->rx = rxdata;
	else if (dspi->bytes_per_word == 2)
		*(u16 *)dspi->rx = rxdata;
	else /* dspi->bytes_per_word == 4 */
		*(u32 *)dspi->rx = rxdata;
	dspi->rx += dspi->bytes_per_word;
}

static void hz_to_spi_baud(char *pbr, char *br, int speed_hz,
			   unsigned long clkrate)
{
	/* Valid baud rate pre-scaler values */
	int pbr_tbl[4] = {2, 3, 5, 7};
	int brs[16] = {	2,	4,	6,	8,
			16,	32,	64,	128,
			256,	512,	1024,	2048,
			4096,	8192,	16384,	32768 };
	int scale_needed, scale, minscale = INT_MAX;
	int i, j;

	scale_needed = clkrate / speed_hz;
	if (clkrate % speed_hz)
		scale_needed++;

	for (i = 0; i < ARRAY_SIZE(brs); i++)
		for (j = 0; j < ARRAY_SIZE(pbr_tbl); j++) {
			scale = brs[i] * pbr_tbl[j];
			if (scale >= scale_needed) {
				if (scale < minscale) {
					minscale = scale;
					*br = i;
					*pbr = j;
				}
				break;
			}
		}

	if (minscale == INT_MAX) {
		pr_warn("Can not find valid baud rate,speed_hz is %d,clkrate is %ld, we use the max prescaler value.\n",
			speed_hz, clkrate);
		*pbr = ARRAY_SIZE(pbr_tbl) - 1;
		*br =  ARRAY_SIZE(brs) - 1;
	}
}

static void ns_delay_scale(char *psc, char *sc, int delay_ns,
			   unsigned long clkrate)
{
	int scale_needed, scale, minscale = INT_MAX;
	int pscale_tbl[4] = {1, 3, 5, 7};
	u32 remainder;
	int i, j;

	scale_needed = div_u64_rem((u64)delay_ns * clkrate, NSEC_PER_SEC,
				   &remainder);
	if (remainder)
		scale_needed++;

	for (i = 0; i < ARRAY_SIZE(pscale_tbl); i++)
		for (j = 0; j <= SPI_CTAR_SCALE_BITS; j++) {
			scale = pscale_tbl[i] * (2 << j);
			if (scale >= scale_needed) {
				if (scale < minscale) {
					minscale = scale;
					*psc = i;
					*sc = j;
				}
				break;
			}
		}

	if (minscale == INT_MAX) {
		pr_warn("Cannot find correct scale values for %dns delay at clkrate %ld, using max prescaler value",
			delay_ns, clkrate);
		*psc = ARRAY_SIZE(pscale_tbl) - 1;
		*sc = SPI_CTAR_SCALE_BITS;
	}
}

static void fifo_write(struct fsl_dspi *dspi)
{
	regmap_write(dspi->regmap, SPI_PUSHR, dspi_pop_tx_pushr(dspi));
}

static void cmd_fifo_write(struct fsl_dspi *dspi)
{
	u16 cmd = dspi->tx_cmd;

	if (dspi->len > 0)
		cmd |= SPI_PUSHR_CMD_CONT;
	regmap_write(dspi->regmap_pushr, PUSHR_CMD, cmd);
}

static void tx_fifo_write(struct fsl_dspi *dspi, u16 txdata)
{
	regmap_write(dspi->regmap_pushr, PUSHR_TX, txdata);
}

static void dspi_tcfq_write(struct fsl_dspi *dspi)
{
	/* Clear transfer count */
	dspi->tx_cmd |= SPI_PUSHR_CMD_CTCNT;

	if (dspi->devtype_data->xspi_mode && dspi->bits_per_word > 16) {
		/* Write the CMD FIFO entry first, and then the two
		 * corresponding TX FIFO entries.
		 */
		u32 data = dspi_pop_tx(dspi);

		cmd_fifo_write(dspi);
		tx_fifo_write(dspi, data & 0xFFFF);
		tx_fifo_write(dspi, data >> 16);
	} else {
		/* Write one entry to both TX FIFO and CMD FIFO
		 * simultaneously.
		 */
		fifo_write(dspi);
	}
}

static u32 fifo_read(struct fsl_dspi *dspi)
{
	u32 rxdata = 0;

	regmap_read(dspi->regmap, SPI_POPR, &rxdata);
	return rxdata;
}

static void dspi_tcfq_read(struct fsl_dspi *dspi)
{
	dspi_push_rx(dspi, fifo_read(dspi));
}

static int dspi_rxtx(struct fsl_dspi *dspi)
{
	struct spi_message *msg = dspi->cur_msg;
	u16 spi_tcnt;
	u32 spi_tcr;

	/* Get transfer counter (in number of SPI transfers). It was
	 * reset to 0 when transfer(s) were started.
	 */
	regmap_read(dspi->regmap, SPI_TCR, &spi_tcr);
	spi_tcnt = SPI_TCR_GET_TCNT(spi_tcr);
	/* Update total number of bytes that were transferred */
	msg->actual_length += spi_tcnt * dspi->bytes_per_word;
	dspi->progress += spi_tcnt;

	dspi_tcfq_read(dspi);
	if (!dspi->len)
		/* Success! */
		return 0;

	dspi_tcfq_write(dspi);

	return -EINPROGRESS;
}

static int dspi_poll(struct fsl_dspi *dspi)
{
	int tries = 1000;
	u32 spi_sr;

	do {
		regmap_read(dspi->regmap, SPI_SR, &spi_sr);
		regmap_write(dspi->regmap, SPI_SR, spi_sr);

		if (spi_sr & (SPI_SR_EOQF | SPI_SR_TCFQF))
			break;
		udelay(1);
	} while (--tries);

	if (!tries)
		return -ETIMEDOUT;

	return dspi_rxtx(dspi);
}

static int dspi_transfer_one_message(struct spi_device *spi,
				     struct spi_message *message)
{
	struct fsl_dspi *dspi = container_of(spi->master, struct fsl_dspi, ctlr);
	struct spi_transfer *transfer;
	int status = 0;

	message->actual_length = 0;

	list_for_each_entry(transfer, &message->transfers, transfer_list) {
		dspi->cur_transfer = transfer;
		dspi->cur_msg = message;
		dspi->cur_chip = spi->controller_data;
		/* Prepare command word for CMD FIFO */
		dspi->tx_cmd = SPI_PUSHR_CMD_CTAS(0) |
			       SPI_PUSHR_CMD_PCS(spi->chip_select);
		if (list_is_last(&dspi->cur_transfer->transfer_list,
				 &dspi->cur_msg->transfers)) {
			/* Leave PCS activated after last transfer when
			 * cs_change is set.
			 */
			if (transfer->cs_change)
				dspi->tx_cmd |= SPI_PUSHR_CMD_CONT;
		} else {
			/* Keep PCS active between transfers in same message
			 * when cs_change is not set, and de-activate PCS
			 * between transfers in the same message when
			 * cs_change is set.
			 */
			if (!transfer->cs_change)
				dspi->tx_cmd |= SPI_PUSHR_CMD_CONT;
		}

		dspi->void_write_data = dspi->cur_chip->void_write_data;

		dspi->tx = transfer->tx_buf;
		dspi->rx = transfer->rx_buf;
		dspi->rx_end = dspi->rx + transfer->len;
		dspi->len = transfer->len;
		dspi->progress = 0;
		/* Validated transfer specific frame size (defaults applied) */
		dspi->bits_per_word = transfer->bits_per_word;

		if (transfer->bits_per_word <= 8)
			dspi->bytes_per_word = 1;
		else if (transfer->bits_per_word <= 16)
			dspi->bytes_per_word = 2;
		else
			dspi->bytes_per_word = 4;

		regmap_update_bits(dspi->regmap, SPI_MCR,
				   SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF,
				   SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF);
		regmap_write(dspi->regmap, SPI_CTAR(0),
			     dspi->cur_chip->ctar_val |
			     SPI_FRAME_BITS(transfer->bits_per_word));
		if (dspi->devtype_data->xspi_mode)
			regmap_write(dspi->regmap, SPI_CTARE(0),
				     SPI_FRAME_EBITS(transfer->bits_per_word) |
				     SPI_CTARE_DTCP(1));

		regmap_write(dspi->regmap, SPI_RSER, SPI_RSER_TCFQE);
		dspi_tcfq_write(dspi);

		do {
			status = dspi_poll(dspi);
		} while (status == -EINPROGRESS);

		if (status)
			dev_err(dspi->dev,
				"Waiting for transfer to complete failed!\n");
	}

	message->status = status;

	return status;
}

static int dspi_setup(struct spi_device *spi)
{
	struct fsl_dspi *dspi = container_of(spi->master, struct fsl_dspi, ctlr);
	unsigned char br = 0, pbr = 0, pcssck = 0, cssck = 0;
	u32 cs_sck_delay = 0, sck_cs_delay = 0;
	unsigned char pasc = 0, asc = 0;
	struct chip_data *chip;
	unsigned long clkrate;

	/* Only alloc on first setup */
	chip = spi->controller_data;
	if (chip == NULL) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip)
			return -ENOMEM;
	}

	of_property_read_u32(spi->dev.device_node, "fsl,spi-cs-sck-delay",
			     &cs_sck_delay);

	of_property_read_u32(spi->dev.device_node, "fsl,spi-sck-cs-delay",
			     &sck_cs_delay);

	chip->void_write_data = 0;

	clkrate = clk_get_rate(dspi->clk);
	hz_to_spi_baud(&pbr, &br, spi->max_speed_hz, clkrate);

	/* Set PCS to SCK delay scale values */
	ns_delay_scale(&pcssck, &cssck, cs_sck_delay, clkrate);

	/* Set After SCK delay scale values */
	ns_delay_scale(&pasc, &asc, sck_cs_delay, clkrate);

	chip->ctar_val = 0;
	if (spi->mode & SPI_CPOL)
		chip->ctar_val |= SPI_CTAR_CPOL;
	if (spi->mode & SPI_CPHA)
		chip->ctar_val |= SPI_CTAR_CPHA;

	chip->ctar_val |= SPI_CTAR_PCSSCK(pcssck) |
			  SPI_CTAR_CSSCK(cssck) |
			  SPI_CTAR_PASC(pasc) |
			  SPI_CTAR_ASC(asc) |
			  SPI_CTAR_PBR(pbr) |
			  SPI_CTAR_BR(br);

	if (spi->mode & SPI_LSB_FIRST)
		chip->ctar_val |= SPI_CTAR_LSBFE;

	spi->controller_data = chip;

	return 0;
}

static void dspi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi->controller_data;

	dev_dbg(&spi->dev, "spi_device %u.%u cleanup\n",
		spi->controller->bus_num, spi->chip_select);

	kfree(chip);
}

static const struct of_device_id fsl_dspi_dt_ids[] = {
	{ .compatible = "fsl,ls1021a-v1.0-dspi", .data = &ls1021a_v1_data, },
	{ .compatible = "fsl,ls2085a-dspi", .data = &ls2085a_data, },
	{ /* sentinel */ }
};

static const struct regmap_config dspi_regmap_config = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
	.max_register	= 0x88,
};

static const struct regmap_config dspi_xspi_regmap_config[] = {
	{
		.reg_bits	= 32,
		.val_bits	= 32,
		.reg_stride	= 4,
		.max_register	= 0x13c,
	}, {
		.name		= "pushr",
		.reg_bits	= 16,
		.val_bits	= 16,
		.reg_stride	= 2,
		.max_register	= 0x2,
	},
};

static void dspi_init(struct fsl_dspi *dspi)
{
	unsigned int mcr = SPI_MCR_PCSIS;

	if (dspi->devtype_data->xspi_mode)
		mcr |= SPI_MCR_XSPI;
	mcr |= SPI_MCR_MASTER;

	regmap_write(dspi->regmap, SPI_MCR, mcr);
	regmap_write(dspi->regmap, SPI_SR, SPI_SR_CLEAR);
	if (dspi->devtype_data->xspi_mode)
		regmap_write(dspi->regmap, SPI_CTARE(0),
			     SPI_CTARE_FMSZE(0) | SPI_CTARE_DTCP(1));
}

static int dspi_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	const struct regmap_config *regmap_config;
	struct spi_master *master;
	int ret, cs_num, bus_num = -1;
	struct fsl_dspi *dspi;
	struct resource *res;
	void __iomem *base;

	dspi = xzalloc(sizeof(*dspi));

	dspi->dev = dev;
	master = &dspi->ctlr;

	master->dev = dev;
	master->setup = dspi_setup;
	master->transfer = dspi_transfer_one_message;

	master->cleanup = dspi_cleanup;

	ret = of_property_read_u32(np, "spi-num-chipselects", &cs_num);
	if (ret < 0) {
		dev_err(dev, "can't get spi-num-chipselects\n");
		goto out_ctlr_put;
	}
	master->num_chipselect = cs_num;

	of_property_read_u32(np, "bus-num", &bus_num);
	master->bus_num = bus_num;

	ret = dev_get_drvdata(dev, (const void **)&dspi->devtype_data);
	if (ret)
		return -ENODEV;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	base = IOMEM(res->start);

	if (dspi->devtype_data->xspi_mode)
		regmap_config = &dspi_xspi_regmap_config[0];
	else
		regmap_config = &dspi_regmap_config;

	dspi->regmap = regmap_init_mmio(dev, base, regmap_config);
	if (IS_ERR(dspi->regmap)) {
		dev_err(dev, "failed to init regmap: %ld\n",
				PTR_ERR(dspi->regmap));
		ret = PTR_ERR(dspi->regmap);
		goto out_ctlr_put;
	}

	if (dspi->devtype_data->xspi_mode) {
		dspi->regmap_pushr = regmap_init_mmio(
			dev, base + SPI_PUSHR,
			&dspi_xspi_regmap_config[1]);
		if (IS_ERR(dspi->regmap_pushr)) {
			dev_err(dev,
				"failed to init pushr regmap: %ld\n",
				PTR_ERR(dspi->regmap_pushr));
			ret = PTR_ERR(dspi->regmap_pushr);
			goto out_ctlr_put;
		}
	}

	dspi->clk = clk_get(dev, "dspi");
	if (IS_ERR(dspi->clk)) {
		ret = PTR_ERR(dspi->clk);
		dev_err(dev, "unable to get clock\n");
		goto out_ctlr_put;
	}
	ret = clk_enable(dspi->clk);
	if (ret)
		goto out_ctlr_put;

	dspi_init(dspi);

	ret = spi_register_master(master);
	if (ret != 0) {
		dev_err(dev, "Problem registering DSPI ctlr\n");
		goto out_clk_put;
	}

	return ret;

out_clk_put:
	clk_disable(dspi->clk);
out_ctlr_put:

	return ret;
}

static struct driver_d fsl_dspi_driver = {
	.name  = "fsl-dspi",
	.probe = dspi_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_dspi_dt_ids),
};
coredevice_platform_driver(fsl_dspi_driver);

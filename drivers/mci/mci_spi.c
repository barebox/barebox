// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Franck JULLIEN <elec4fun@gmail.com>
// SPDX-FileCopyrightText: 2010 Thomas Chou <thomas@wytron.com.tw>
// SPDX-FileCopyrightText: 2005 Intec Automation (Mike Lavender <mike@steroidmicros>)
// SPDX-FileCopyrightText: 2006-2007 David Brownell
// SPDX-FileCopyrightText: 2007 Axis Communications (Hans-Peter Nilsson <hp@axis.com>)
// SPDX-FileCopyrightText: 2007 ATRON electronic GmbH (Jan Nikitenko <jan.nikitenko@gmail.com>)

/* This code was inspired from u-boot mmc_spi.c and linux mmc_spi.c. */

#include <common.h>
#include <init.h>
#include <errno.h>
#include <clock.h>
#include <asm/io.h>
#include <driver.h>
#include <spi/spi.h>
#include <mci.h>
#include <crc.h>
#include <crc7.h>
#include <of.h>
#include <linux/gpio/consumer.h>

#define to_spi_host(mci) container_of(mci, struct mmc_spi_host, mci)
#define spi_setup(spi) spi->master->setup(spi)

/* Response tokens used to ack each block written: */
#define SPI_MMC_RESPONSE_CODE(x)	((x) & 0x1f)
#define SPI_RESPONSE_ACCEPTED		((2 << 1)|1)

/* Read and write blocks start with these tokens and end with crc;
 * on error, read tokens act like a subset of R2_SPI_* values.
 */
#define SPI_TOKEN_SINGLE	0xFE	/* single block r/w, multiblock read */
#define SPI_TOKEN_MULTI_WRITE	0xFC	/* multiblock write */
#define SPI_TOKEN_STOP_TRAN	0xFD	/* terminate multiblock write */

/* MMC SPI commands start with a start bit "0" and a transmit bit "1" */
#define MMC_SPI_CMD(x) (0x40 | (x & 0x3F))

#define MMC_SPI_BLOCKSIZE       512

/* timeout value */
#define CTOUT 8
#define RTOUT 3000000 /* 1 sec */
#define WTOUT 3000000 /* 1 sec */

struct mmc_spi_host {
	struct mci_host	mci;
	struct spi_device	*spi;
	struct device	*dev;
	struct gpio_desc *detect_pin;

	/* for bulk data transfers */
	struct spi_transfer	t_tx;
	struct spi_message	m_tx;

	/* for status readback */
	struct spi_transfer	t_rx;
	struct spi_message	m_rx;

	void			*ones;
};

static char *maptype(struct mci_cmd *cmd)
{
	switch (cmd->resp_type) {
	case MMC_RSP_NONE:	return "NONE";
	case MMC_RSP_R1:	return "R1";
	case MMC_RSP_R1b:	return "R1B";
	case MMC_RSP_R2:	return "R2/R5";
	case MMC_RSP_R3:	return "R3/R4/R7";
	default:		return "?";
	}
}

static inline int mmc_cs_off(struct mmc_spi_host *host)
{
	/* chipselect will always be inactive after setup() */
	return spi_setup(host->spi);
}

static int
mmc_spi_readbytes(struct mmc_spi_host *host, unsigned len, void *data)
{
	int status;

	host->t_rx.len = len;
	host->t_rx.rx_buf = data;

	status = spi_sync(host->spi, &host->m_rx);

	return status;
}

static int
mmc_spi_writebytes(struct mmc_spi_host *host, unsigned len, void *data)
{
	int status;

	host->t_tx.len = len;
	host->t_tx.tx_buf = data;

	status = spi_sync(host->spi, &host->m_tx);

	return status;
}

static int mmc_spi_command_send(struct mmc_spi_host *host, struct mci_cmd *cmd)
{
	uint8_t r1;
	uint8_t command[7];
	int i;

	command[0] = 0xff;
	command[1] = MMC_SPI_CMD(cmd->cmdidx);
	command[2] = cmd->cmdarg >> 24;
	command[3] = cmd->cmdarg >> 16;
	command[4] = cmd->cmdarg >> 8;
	command[5] = cmd->cmdarg;
	command[6] = (crc7(0, &command[1], 5) << 1) | 0x01;

	mmc_spi_writebytes(host, 7, command);

	for (i = 0; i < CTOUT; i++) {
		mmc_spi_readbytes(host, 1, &r1);
		if (i && ((r1 & 0x80) == 0)) {  /* r1 response */
			dev_dbg(host->dev, "%s: CMD%d, TRY %d, RESP %x\n", __func__, cmd->cmdidx, i, r1);
			break;
		}
	}

	return r1;
}

static uint mmc_spi_readdata(struct mmc_spi_host *host, void *xbuf,
				uint32_t bcnt, uint32_t bsize)
{
	uint8_t *buf = xbuf;
	uint8_t r1;
	uint16_t crc;
	int i;

	while (bcnt--) {
		for (i = 0; i < RTOUT; i++) {
			mmc_spi_readbytes(host, 1, &r1);
			if (r1 != 0xff) /* data token */
				break;
		}
		if (r1 == SPI_TOKEN_SINGLE) {
			mmc_spi_readbytes(host, bsize, buf);
			mmc_spi_readbytes(host, 2, &crc);
#ifdef CONFIG_MMC_SPI_CRC_ON
			if (be16_to_cpu(crc_itu_t(0, buf, bsize)) != crc) {
				dev_dbg(host->dev, "%s: CRC error\n", __func__);
				r1 = R1_SPI_COM_CRC;
				break;
			}
#endif
			r1 = 0;
		} else {
			r1 = R1_SPI_ERROR;
			break;
		}
		buf += bsize;
	}

	return r1;
}

static uint mmc_spi_writedata(struct mmc_spi_host *host, const void *xbuf,
			      uint32_t bcnt, uint32_t bsize, int multi)
{
	const uint8_t *buf = xbuf;
	uint8_t r1;
	uint16_t crc = 0;
	uint8_t tok[2];
	int i;

	tok[0] = 0xff;
	tok[1] = multi ? SPI_TOKEN_MULTI_WRITE : SPI_TOKEN_SINGLE;

	while (bcnt--) {
#ifdef CONFIG_MMC_SPI_CRC_ON
		crc = be16_to_cpu(crc_itu_t(0, (u8 *)buf, bsize));
#endif
		mmc_spi_writebytes(host, 2, tok);
		mmc_spi_writebytes(host, bsize, (void *)buf);
		mmc_spi_writebytes(host, 2, &crc);

		for (i = 0; i < CTOUT; i++) {
			mmc_spi_readbytes(host, 1, &r1);
			if ((r1 & 0x11) == 0x01) /* response token */
				break;
		}

		dev_dbg(host->dev,"%s   : TOKEN%d RESP 0x%X\n", __func__, i, r1);
		if (SPI_MMC_RESPONSE_CODE(r1) == SPI_RESPONSE_ACCEPTED) {
			for (i = 0; i < WTOUT; i++) { /* wait busy */
				mmc_spi_readbytes(host, 1, &r1);
				if (i && r1 == 0xff) {
					r1 = 0;
					break;
				}
			}
			if (i == WTOUT) {
				dev_dbg(host->dev, "%s: wtout %x\n", __func__, r1);
				r1 = R1_SPI_ERROR;
				break;
			}
		} else {
			dev_dbg(host->dev, "%s: err %x\n", __func__, r1);
			r1 = R1_SPI_COM_CRC;
			break;
		}
		buf += bsize;
	}

	if (multi && bcnt == -1) { /* stop multi write */
		tok[1] = SPI_TOKEN_STOP_TRAN;
		mmc_spi_writebytes(host, 2, tok);
		for (i = 0; i < WTOUT; i++) { /* wait busy */
			mmc_spi_readbytes(host, 1, &r1);
			if (i && r1 == 0xff) {
				r1 = 0;
				break;
			}
		}
		if (i == WTOUT) {
			dev_dbg(host->dev, "%s: wstop %x\n", __func__, r1);
			r1 = R1_SPI_ERROR;
		}
	}

	return r1;
}

static int mmc_spi_request(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct mmc_spi_host	*host = to_spi_host(mci);
	uint8_t r1;
	int i;
	int ret = 0;

	dev_dbg(host->dev, "%s     : CMD%02d, RESP %s, ARG 0x%X\n", __func__,
	      cmd->cmdidx, maptype(cmd), cmd->cmdarg);

	r1 = mmc_spi_command_send(host, cmd);

	cmd->response[0] = r1;

	if (r1 == 0xff) { /* no response */
		ret = -ETIME;
		goto done;
	} else if (r1 & R1_SPI_COM_CRC) {
		ret = -ECOMM;
		goto done;
	} else if (r1 & ~R1_SPI_IDLE) { /* other errors */
		ret = -ETIME;
		goto done;
	} else if (cmd->resp_type == MMC_RSP_R2) {
		r1 = mmc_spi_readdata(host, cmd->response, 1, 16);
		for (i = 0; i < 4; i++)
			cmd->response[i] = be32_to_cpu(cmd->response[i]);
		dev_dbg(host->dev, "MMC_RSP_R2 -> %x %x %x %x\n", cmd->response[0], cmd->response[1],
		      cmd->response[2], cmd->response[3]);
	} else if (!data) {
		switch (cmd->cmdidx) {
		case SD_CMD_SEND_IF_COND:
		case MMC_CMD_SPI_READ_OCR:
			mmc_spi_readbytes(host, 4, cmd->response);
			cmd->response[0] = be32_to_cpu(cmd->response[0]);
			break;
		}
	} else {
		if (data->flags == MMC_DATA_READ) {
			dev_dbg(host->dev, "%s     : DATA READ, %x blocks, bsize = 0x%X\n", __func__,
			      data->blocks, data->blocksize);
			r1 = mmc_spi_readdata(host, data->dest,
				data->blocks, data->blocksize);
		} else if  (data->flags == MMC_DATA_WRITE) {
			dev_dbg(host->dev, "%s     : DATA WRITE, %x blocks, bsize = 0x%X\n", __func__,
			      data->blocks, data->blocksize);
			r1 = mmc_spi_writedata(host, data->src,
				data->blocks, data->blocksize,
				(cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK));
		}
		if (r1 & R1_SPI_COM_CRC)
			ret = -ECOMM;
		else if (r1)
			ret = -ETIME;
	}

done:
	mmc_cs_off(host);
	return ret;

return  0;

}

static void mmc_spi_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct mmc_spi_host	*host = to_spi_host(mci);

	if (host->spi->max_speed_hz != ios->clock && ios->clock != 0) {
		int		status;

		host->spi->max_speed_hz = ios->clock;
		status = spi_setup(host->spi);
		dev_dbg(&host->spi->dev,
			"mmc_spi:  clock to %d Hz, %d\n",
			host->spi->max_speed_hz, status);
	}
}

static int mmc_spi_init(struct mci_host *mci, struct device *mci_dev)
{
	struct mmc_spi_host	*host = to_spi_host(mci);
	mmc_spi_readbytes(host, 10, NULL);

	/*
	 * Do a burst with chipselect active-high.  We need to do this to
	 * meet the requirement of 74 clock cycles with both chipselect
	 * and CMD (MOSI) high before CMD0 ... after the card has been
	 * powered up to Vdd(min), and so is ready to take commands.
	 *
	 * Some cards are particularly needy of this (e.g. Viking "SD256")
	 * while most others don't seem to care.
	 *
	 * Note that this is one of the places MMC/SD plays games with the
	 * SPI protocol.  Another is that when chipselect is released while
	 * the card returns BUSY status, the clock must issue several cycles
	 * with chipselect high before the card will stop driving its output.
	 */

	host->spi->mode |= SPI_CS_HIGH;
	if (spi_setup(host->spi) != 0) {
		/* Just warn; most cards work without it. */
		dev_warn(&host->spi->dev,
				"can't change chip-select polarity\n");
		host->spi->mode &= ~SPI_CS_HIGH;
	} else {
		mmc_spi_readbytes(host, 18, NULL);

		host->spi->mode &= ~SPI_CS_HIGH;
		if (spi_setup(host->spi) != 0) {
			/* Wot, we can't get the same setup we had before? */
			dev_err(&host->spi->dev,
					"can't restore chip-select polarity\n");
		}
	}

	return 0;
}

static int spi_mci_card_present(struct mci_host *mci)
{
	struct mmc_spi_host	*host = to_spi_host(mci);
	int			ret;

	/* No gpio, assume card is present */
	if (IS_ERR_OR_NULL(host->detect_pin))
		return 1;

	ret = gpiod_get_value(host->detect_pin);

	return ret == 0 ? 1 : 0;
}

static int spi_mci_probe(struct device *dev)
{
	struct device_node	*np = dev_of_node(dev);
	struct spi_device	*spi = (struct spi_device *)dev->type_data;
	struct mmc_spi_host	*host;
	void			*ones;
	int			status;

	host = xzalloc(sizeof(*host));
	host->mci.send_cmd = mmc_spi_request;
	host->mci.set_ios = mmc_spi_set_ios;
	host->mci.init = mmc_spi_init;
	host->mci.card_present = spi_mci_card_present;
	host->mci.hw_dev = dev;

	/* MMC and SD specs only seem to care that sampling is on the
	 * rising edge ... meaning SPI modes 0 or 3.  So either SPI mode
	 * should be legit.  We'll use mode 0 since the steady state is 0,
	 * which is appropriate for hotplugging, unless the platform data
	 * specify mode 3 (if hardware is not compatible to mode 0).
	 */
	if (spi->mode != SPI_MODE_3)
		spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;

	status = spi_setup(spi);
	if (status < 0) {
		dev_dbg(&spi->dev, "needs SPI mode %02x, %d KHz; %d\n",
				spi->mode, spi->max_speed_hz / 1000,
				status);
		return status;
	}

	/* SPI doesn't need the lowspeed device identification thing for
	 * MMC or SD cards, since it never comes up in open drain mode.
	 * That's good; some SPI masters can't handle very low speeds!
	 *
	 * However, low speed SDIO cards need not handle over 400 KHz;
	 * that's the only reason not to use a few MHz for f_min (until
	 * the upper layer reads the target frequency from the CSD).
	 */
	host->mci.f_min = 400000;
	host->mci.f_max = spi->max_speed_hz;

	host->dev = dev;
	host->spi = spi;
	dev->priv = host;

	ones = xmalloc(MMC_SPI_BLOCKSIZE);
	memset(ones, 0xff, MMC_SPI_BLOCKSIZE);

	host->ones = ones;

	spi_message_init(&host->m_tx);
	spi_message_init(&host->m_rx);

	spi_message_add_tail(&host->t_tx, &host->m_tx);
	spi_message_add_tail(&host->t_rx, &host->m_rx);

	host->t_rx.tx_buf = host->ones;
	host->t_rx.cs_change = 1;

	host->t_tx.cs_change = 1;

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	host->mci.host_caps = MMC_CAP_SPI;

	if (np) {
		host->mci.devname = xstrdup(of_alias_get(np));
		host->detect_pin = gpiod_get_optional(dev, NULL, GPIOD_IN);
		if (IS_ERR(host->detect_pin))
			dev_warn(dev, "Failed to get 'reset' GPIO (ignored)\n");
	}

	mci_register(&host->mci);

	return 0;
}

static __maybe_unused struct of_device_id spi_mci_compatible[] = {
	{ .compatible = "mmc-spi-slot" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, spi_mci_compatible);

static struct driver spi_mci_driver = {
	.name	= "spi_mci",
	.probe	= spi_mci_probe,
	.of_compatible = DRV_OF_COMPAT(spi_mci_compatible),
};
device_spi_driver(spi_mci_driver);

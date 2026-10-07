// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 Pengutronix, Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * derived from Linux kernel driver by
 *
 * Driver for Cadence QSPI Controller
 *
 * Copyright Altera Corporation (C) 2012-2014. All rights reserved.
 */

#include <common.h>
#include <io.h>
#include <errno.h>
#include <linux/time.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>

#include "cadence-quadspi.h"

static void cqspi_fifo_read(void *dest, const void *src_ahb_addr,
			    unsigned int bytes)
{
	unsigned int temp;
	int remaining = bytes;
	unsigned int *dest_ptr = (unsigned int *)dest;
	unsigned int *src_ptr = (unsigned int *)src_ahb_addr;

	while (remaining > CQSPI_FIFO_WIDTH) {
		*dest_ptr = readl(src_ptr);
		dest_ptr++;
		remaining -= CQSPI_FIFO_WIDTH;
	}
	if (remaining > 0) {
		/* dangling bytes */
		temp = readl(src_ptr);
		memcpy(dest_ptr, &temp, remaining);
	}
}

static int cqspi_find_chipselect(struct spi_nor *nor)
{
	int cs = -1;
	struct cqspi_st *cqspi = nor->priv;

	for (cs = 0; cs < CQSPI_MAX_CHIPSELECT; cs++)
		if (nor == &cqspi->f_pdata[cs].nor)
			break;
	return cs;
}

unsigned int cqspi_calc_rdreg(struct spi_nor *nor, u8 opcode)
{
	unsigned int rdreg = 0;
	struct cqspi_st *cqspi = nor->priv;
	struct cqspi_flash_pdata *f_pdata;

	f_pdata = &cqspi->f_pdata[cqspi->current_cs];

	rdreg |= f_pdata->inst_width << CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB;
	rdreg |= f_pdata->addr_width << CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB;
	rdreg |= f_pdata->data_width << CQSPI_REG_RD_INSTR_TYPE_DATA_LSB;

	return rdreg;
}

unsigned int cqspi_wait_idle(struct cqspi_st *cqspi)
{
	void __iomem *reg_base = cqspi->iobase;

	if (wait_on_timeout(CQSPI_TIMEOUT_MS,
		CQSPI_REG_IS_IDLE(reg_base))) {
		/* Timeout, in busy mode. */
		dev_err(cqspi->dev, "QSPI is still busy after %llums timeout.\n",
			CQSPI_TIMEOUT_MS);
		return -ETIMEDOUT;
	}

	return 0;
}

int cqspi_exec_flash_cmd(struct cqspi_st *cqspi, unsigned int reg)
{
	void __iomem *reg_base = cqspi->iobase;

	/* Write the CMDCTRL without start execution. */
	writel(reg, reg_base + CQSPI_REG_CMDCTRL);
	/* Start execute */
	reg |= CQSPI_REG_CMDCTRL_EXECUTE_MASK;
	writel(reg, reg_base + CQSPI_REG_CMDCTRL);

	if (wait_on_timeout(CQSPI_TIMEOUT_MS,
		(readl(reg_base + CQSPI_REG_CMDCTRL) &
			CQSPI_REG_CMDCTRL_INPROGRESS_MASK) == 0)) {
		dev_err(cqspi->dev, "flash cmd execute time out (0x%08x)\n",
			readl(reg_base + CQSPI_REG_CMDCTRL));
		return -EIO;
	}

	/* Polling QSPI idle status. */
	return cqspi_wait_idle(cqspi);
}

static int cqspi_command_read(struct spi_nor *nor,
			      const u8 *txbuf, unsigned n_tx,
			      u8 *rxbuf, unsigned n_rx)
{
	unsigned int reg;
	unsigned int read_len;
	int status;
	struct cqspi_st *cqspi = nor->priv;
	void __iomem *reg_base = cqspi->iobase;
	unsigned int rdreg;

	if (!n_rx || n_rx > CQSPI_STIG_DATA_LEN_MAX || rxbuf == NULL) {
		dev_err(nor->dev,
			"Invalid input argument, len %d rxbuf 0x%p\n", n_rx, rxbuf);
		return -EINVAL;
	}

	reg = txbuf[0] << CQSPI_REG_CMDCTRL_OPCODE_LSB;

	rdreg = cqspi_calc_rdreg(nor, txbuf[0]);
	writel(rdreg, reg_base + CQSPI_REG_RD_INSTR);

	reg |= (0x1 << CQSPI_REG_CMDCTRL_RD_EN_LSB);

	/* 0 means 1 byte. */
	reg |= (((n_rx - 1) & CQSPI_REG_CMDCTRL_RD_BYTES_MASK)
		<< CQSPI_REG_CMDCTRL_RD_BYTES_LSB);
	status = cqspi_exec_flash_cmd(cqspi, reg);
	if (status != 0)
		return status;

	reg = readl(reg_base + CQSPI_REG_CMDREADDATALOWER);

	/* Put the read value into rx_buf */
	read_len = (n_rx > 4) ? 4 : n_rx;
	memcpy(rxbuf, &reg, read_len);
	rxbuf += read_len;

	if (n_rx > 4) {
		reg = readl(reg_base + CQSPI_REG_CMDREADDATAUPPER);

		read_len = n_rx - read_len;
		memcpy(rxbuf, &reg, read_len);
	}

	return 0;
}

static int cqspi_command_write(struct spi_nor *nor,
			       u8 opcode, const u8 *txbuf, unsigned n_tx)
{
	unsigned int reg;
	struct cqspi_st *cqspi = nor->priv;
	unsigned int data;
	void __iomem *reg_base = cqspi->iobase;

	if (n_tx > 4 || (n_tx && txbuf == NULL)) {
		dev_err(nor->dev,
			"Invalid input argument, cmdlen %d txbuf 0x%p\n", n_tx, txbuf);
		return -EINVAL;
	}

	reg = opcode << CQSPI_REG_CMDCTRL_OPCODE_LSB;
	if (n_tx) {
		reg |= (0x1 << CQSPI_REG_CMDCTRL_WR_EN_LSB);
		reg |= ((n_tx - 1) & CQSPI_REG_CMDCTRL_WR_BYTES_MASK)
		    << CQSPI_REG_CMDCTRL_WR_BYTES_LSB;
		data = 0;
		memcpy(&data, txbuf, n_tx);
		writel(data, reg_base + CQSPI_REG_CMDWRITEDATALOWER);
	}

	return cqspi_exec_flash_cmd(cqspi, reg);
}

static int cqspi_indirect_read_setup(struct spi_nor *nor,
			      unsigned int from_addr)
{
	struct cqspi_flash_pdata *f_pdata;
	struct cqspi_st *cqspi = nor->priv;
	phys_addr_t ahb_base = virt_to_phys(cqspi->ahb_base);
	void __iomem *reg_base = cqspi->iobase;
	unsigned int dummy_clk = 0;
	unsigned int dummy_bytes;
	unsigned int reg = 0;

	writel(ahb_base & CQSPI_INDIRECTTRIGGER_ADDR_MASK,
	       reg_base + CQSPI_REG_INDIRECTTRIGGER);
	writel(from_addr, reg_base + CQSPI_REG_INDIRECTRDSTARTADDR);
	f_pdata = &cqspi->f_pdata[cqspi->current_cs];

	reg = nor->read_opcode << CQSPI_REG_RD_INSTR_OPCODE_LSB;
	reg |= cqspi_calc_rdreg(nor, nor->read_opcode);

	/* Setup dummy clock cycles */
	dummy_bytes = nor->read_dummy / 8;

	if (dummy_bytes > CQSPI_DUMMY_BYTES_MAX)
		dummy_bytes = CQSPI_DUMMY_BYTES_MAX;

	if (dummy_bytes) {
		reg |= (1 << CQSPI_REG_RD_INSTR_MODE_EN_LSB);
		/* Set mode bits high to ensure chip doesn't enter XIP */
		writel(0xFF, reg_base + CQSPI_REG_MODE_BIT);

		/* Convert to clock cycles. */
		dummy_clk = dummy_bytes * CQSPI_DUMMY_CLKS_PER_BYTE;
		/* Need to subtract the mode byte (8 clocks). */
		if (f_pdata->inst_width != CQSPI_INST_TYPE_QUAD)
			dummy_clk -= CQSPI_DUMMY_CLKS_PER_BYTE;

		if (dummy_clk)
			reg |= (dummy_clk & CQSPI_REG_RD_INSTR_DUMMY_MASK)
			    << CQSPI_REG_RD_INSTR_DUMMY_LSB;
	}

	writel(reg, reg_base + CQSPI_REG_RD_INSTR);

	/* Set address width */
	reg = readl(reg_base + CQSPI_REG_SIZE);
	reg &= ~CQSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (nor->addr_width) - 1;
	writel(reg, reg_base + CQSPI_REG_SIZE);
	return 0;
}

static int cqspi_indirect_read_execute(struct spi_nor *nor,
				u8 *rxbuf, unsigned n_rx)
{
	int ret = 0;
	unsigned int reg = 0;
	unsigned int bytes_to_read = 0;
	unsigned int watermark;
	struct cqspi_st *cqspi = nor->priv;
	void __iomem *reg_base = cqspi->iobase;
	void __iomem *ahb_base = cqspi->ahb_base;
	int remaining = (int)n_rx;

	watermark = cqspi->fifo_depth * CQSPI_FIFO_WIDTH / 2;
	writel(watermark, reg_base + CQSPI_REG_INDIRECTRDWATERMARK);
	writel(remaining, reg_base + CQSPI_REG_INDIRECTRDBYTES);

	/* Clear all interrupts. */
	writel(CQSPI_IRQ_STATUS_MASK, reg_base + CQSPI_REG_IRQSTATUS);

	cqspi->irq_mask = CQSPI_IRQ_MASK_RD;
	writel(cqspi->irq_mask, reg_base + CQSPI_REG_IRQMASK);

	writel(CQSPI_REG_INDIRECTRD_START_MASK,
		reg_base + CQSPI_REG_INDIRECTRD);

	while (remaining > 0) {
		unsigned int irq_status;

		ret = wait_on_timeout(CQSPI_READ_TIMEOUT_MS,
			readl(reg_base + CQSPI_REG_IRQSTATUS) & cqspi->irq_mask);

		irq_status = readl(reg_base + CQSPI_REG_IRQSTATUS);
		bytes_to_read = CQSPI_GET_RD_SRAM_LEVEL(reg_base);

		/* Clear all interrupts. */
		writel(irq_status, reg_base + CQSPI_REG_IRQSTATUS);

		if (!ret && bytes_to_read == 0) {
			dev_err(nor->dev, "Indirect read timeout, no bytes\n");
			ret = -ETIMEDOUT;
			goto failrd;
		}

		while (bytes_to_read != 0) {
			bytes_to_read *= CQSPI_FIFO_WIDTH;
			bytes_to_read = bytes_to_read > remaining
					? remaining : bytes_to_read;
			cqspi_fifo_read(rxbuf, ahb_base, bytes_to_read);
			rxbuf += bytes_to_read;
			remaining -= bytes_to_read;
			bytes_to_read = CQSPI_GET_RD_SRAM_LEVEL(reg_base);
		}
	}

	/* Check indirect done status */
	if (wait_on_timeout(CQSPI_TIMEOUT_MS,
		readl(reg_base + CQSPI_REG_INDIRECTRD) &
			CQSPI_REG_INDIRECTRD_DONE_MASK)) {
		dev_err(nor->dev,
			"Indirect read completion error 0x%08x\n", reg);
		ret = -ETIMEDOUT;
		goto failrd;
	}

	/* Disable interrupt */
	writel(0, reg_base + CQSPI_REG_IRQMASK);

	/* Clear indirect completion status */
	writel(CQSPI_REG_INDIRECTRD_DONE_MASK, reg_base + CQSPI_REG_INDIRECTRD);

	return 0;

 failrd:
	/* Disable interrupt */
	writel(0, reg_base + CQSPI_REG_IRQMASK);

	/* Cancel the indirect read */
	writel(CQSPI_REG_INDIRECTWR_CANCEL_MASK,
	       reg_base + CQSPI_REG_INDIRECTRD);
	return ret;
}

static void cqspi_controller_enable(struct cqspi_st *cqspi)
{
	void __iomem *reg_base = cqspi->iobase;
	unsigned int reg;

	reg = readl(reg_base + CQSPI_REG_CONFIG);
	reg |= CQSPI_REG_CONFIG_ENABLE_MASK;
	writel(reg, reg_base + CQSPI_REG_CONFIG);
}

static void cqspi_controller_disable(struct cqspi_st *cqspi)
{
	void __iomem *reg_base = cqspi->iobase;
	unsigned int reg;

	reg = readl(reg_base + CQSPI_REG_CONFIG);
	reg &= ~CQSPI_REG_CONFIG_ENABLE_MASK;
	writel(reg, reg_base + CQSPI_REG_CONFIG);
}

static void cqspi_chipselect(struct cqspi_st *cqspi,
			     unsigned int chip_select,
			     unsigned int decoder_enable)
{
	void __iomem *reg_base = cqspi->iobase;
	unsigned int reg;

	reg = readl(reg_base + CQSPI_REG_CONFIG);
	if (decoder_enable) {
		reg |= CQSPI_REG_CONFIG_DECODE_MASK;
	} else {
		reg &= ~CQSPI_REG_CONFIG_DECODE_MASK;

		/* Convert CS if without decoder.
		 * CS0 to 4b'1110
		 * CS1 to 4b'1101
		 * CS2 to 4b'1011
		 * CS3 to 4b'0111
		 */
		chip_select = 0xF & ~(1 << chip_select);
	}

	reg &= ~(CQSPI_REG_CONFIG_CHIPSELECT_MASK
		 << CQSPI_REG_CONFIG_CHIPSELECT_LSB);
	reg |= (chip_select & CQSPI_REG_CONFIG_CHIPSELECT_MASK)
		<< CQSPI_REG_CONFIG_CHIPSELECT_LSB;
	writel(reg, reg_base + CQSPI_REG_CONFIG);
}

static unsigned int calculate_ticks_for_ns(unsigned int ref_clk_hz,
					   unsigned int ns_val)
{
	unsigned int ticks;

	ticks = ref_clk_hz;
	ticks /= 1000;
	ticks *= ns_val;
	ticks += 999999;
	ticks /= 1000000;
	return ticks;
}

static void cqspi_delay(struct cqspi_st *cqspi,
			unsigned int ref_clk_hz, unsigned int sclk_hz)
{
	void __iomem *reg_base = cqspi->iobase;
	struct cqspi_flash_pdata *f_pdata;
	unsigned int ref_clk_ns;
	unsigned int sclk_ns;
	unsigned int tshsl, tchsh, tslch, tsd2d;
	unsigned int reg;
	unsigned int tsclk;

	if (cqspi->no_reconfig)
		return;

	f_pdata = &cqspi->f_pdata[cqspi->current_cs];

	/* Convert to ns. */
	ref_clk_ns = NSEC_PER_SEC / ref_clk_hz;

	/* Convert to ns. */
	sclk_ns = NSEC_PER_SEC / sclk_hz;

	/* calculate the number of ref ticks for one sclk tick */
	tsclk = (ref_clk_hz + sclk_hz - 1) / sclk_hz;

	tshsl = calculate_ticks_for_ns(ref_clk_hz, f_pdata->tshsl_ns);
	/* this particular value must be at least one sclk */
	if (tshsl < tsclk)
		tshsl = tsclk;

	tchsh = calculate_ticks_for_ns(ref_clk_hz, f_pdata->tchsh_ns);
	tslch = calculate_ticks_for_ns(ref_clk_hz, f_pdata->tslch_ns);
	tsd2d = calculate_ticks_for_ns(ref_clk_hz, f_pdata->tsd2d_ns);

	reg = ((tshsl & CQSPI_REG_DELAY_TSHSL_MASK)
	       << CQSPI_REG_DELAY_TSHSL_LSB);
	reg |= ((tchsh & CQSPI_REG_DELAY_TCHSH_MASK)
		<< CQSPI_REG_DELAY_TCHSH_LSB);
	reg |= ((tslch & CQSPI_REG_DELAY_TSLCH_MASK)
		<< CQSPI_REG_DELAY_TSLCH_LSB);
	reg |= ((tsd2d & CQSPI_REG_DELAY_TSD2D_MASK)
		<< CQSPI_REG_DELAY_TSD2D_LSB);
	writel(reg, reg_base + CQSPI_REG_DELAY);
}

static void cqspi_config_baudrate_div(struct cqspi_st *cqspi,
				      unsigned int ref_clk_hz,
				      unsigned int sclk_hz)
{
	void __iomem *reg_base = cqspi->iobase;
	unsigned int reg;
	unsigned int div;

	reg = readl(reg_base + CQSPI_REG_CONFIG);
	reg &= ~(CQSPI_REG_CONFIG_BAUD_MASK << CQSPI_REG_CONFIG_BAUD_LSB);

	div = ref_clk_hz / sclk_hz;

	/* Recalculate the baudrate divisor based on QSPI specification. */
	if (div > 32)
		div = 32;

	/* Check if even number. */
	if (div & 1)
		div = (div / 2);
	else
		div = (div / 2) - 1;

	div = (div & CQSPI_REG_CONFIG_BAUD_MASK) << CQSPI_REG_CONFIG_BAUD_LSB;
	reg |= div;
	writel(reg, reg_base + CQSPI_REG_CONFIG);
}

static void cqspi_readdata_capture(struct cqspi_st *cqspi,
				   unsigned int bypass, unsigned int delay)
{
	void __iomem *reg_base = cqspi->iobase;
	unsigned int reg;

	if (cqspi->no_reconfig)
		return;

	reg = readl(reg_base + CQSPI_REG_READCAPTURE);

	if (bypass)
		reg |= (1 << CQSPI_REG_READCAPTURE_BYPASS_LSB);
	else
		reg &= ~(1 << CQSPI_REG_READCAPTURE_BYPASS_LSB);

	reg &= ~(CQSPI_REG_READCAPTURE_DELAY_MASK
		 << CQSPI_REG_READCAPTURE_DELAY_LSB);

	reg |= ((delay & CQSPI_REG_READCAPTURE_DELAY_MASK)
		<< CQSPI_REG_READCAPTURE_DELAY_LSB);

	writel(reg, reg_base + CQSPI_REG_READCAPTURE);
}

static void cqspi_switch_cs(struct cqspi_st *cqspi, unsigned int cs)
{
	unsigned int reg;
	struct cqspi_flash_pdata *f_pdata = &cqspi->f_pdata[cs];
	void __iomem *reg_base = cqspi->iobase;
	struct spi_nor *nor = &f_pdata->nor;

	cqspi_controller_disable(cqspi);

	/* configure page size and block size. */
	reg = readl(reg_base + CQSPI_REG_SIZE);
	reg &= ~(CQSPI_REG_SIZE_PAGE_MASK << CQSPI_REG_SIZE_PAGE_LSB);
	reg &= ~(CQSPI_REG_SIZE_BLOCK_MASK << CQSPI_REG_SIZE_BLOCK_LSB);
	reg |= (nor->page_size << CQSPI_REG_SIZE_PAGE_LSB);
	reg |= (f_pdata->block_size << CQSPI_REG_SIZE_BLOCK_LSB);
	reg &= ~CQSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (nor->addr_width - 1);
	writel(reg, reg_base + CQSPI_REG_SIZE);

	/* configure the chip select */
	cqspi_chipselect(cqspi, cs, cqspi->is_decoded_cs);

	cqspi_controller_enable(cqspi);
}

static int cqspi_configure(struct spi_nor *nor)
{
	struct cqspi_st *cqspi = nor->priv;
	int cs = cqspi_find_chipselect(nor);
	struct cqspi_flash_pdata *f_pdata;
	unsigned int sclk;

	/* Switch chip select. */
	if (cqspi->current_cs != cs) {
		cqspi->current_cs = cs;
		cqspi_switch_cs(cqspi, cs);
	}

	/* Setup baudrate divisor and delays */
	f_pdata = &cqspi->f_pdata[cqspi->current_cs];
	sclk = f_pdata->clk_rate;
	if (cqspi->sclk != sclk) {
		cqspi->sclk = sclk;
		cqspi_controller_disable(cqspi);
		cqspi_config_baudrate_div(cqspi,
					  cqspi->master_ref_clk_hz, sclk);
		cqspi_delay(cqspi, cqspi->master_ref_clk_hz, sclk);
		cqspi_readdata_capture(cqspi, 1, f_pdata->read_delay);
		cqspi_controller_enable(cqspi);
	}
	return 0;
}

int cqspi_set_protocol(struct spi_nor *nor, const int read)
{
	struct cqspi_st *cqspi = nor->priv;
	struct cqspi_flash_pdata *f_pdata;
	int cs = cqspi_find_chipselect(nor);

	if (cs < 0)
		return -EINVAL;

	f_pdata = &cqspi->f_pdata[cs];

	f_pdata->inst_width = CQSPI_INST_TYPE_SINGLE;
	f_pdata->addr_width = CQSPI_INST_TYPE_SINGLE;
	f_pdata->data_width = CQSPI_INST_TYPE_SINGLE;

	if (read) {
		switch (nor->read_proto) {
		case SNOR_PROTO_1_1_1:
			f_pdata->data_width = CQSPI_INST_TYPE_SINGLE;
			break;
		case SNOR_PROTO_1_1_2:
			f_pdata->data_width = CQSPI_INST_TYPE_DUAL;
			break;
		case SNOR_PROTO_1_1_4:
			f_pdata->data_width = CQSPI_INST_TYPE_QUAD;
			break;
		default:
			return -EINVAL;
		}
	}

	cqspi_configure(nor);

	return 0;
}

int cqspi_read(struct spi_nor *nor, loff_t from,
		      size_t len, size_t *retlen, u_char *buf)
{
	int ret;

	ret = cqspi_set_protocol(nor, 1);
	if (ret)
		return ret;

	ret = cqspi_indirect_read_setup(nor, from);
	if (ret == 0) {
		ret = cqspi_indirect_read_execute(nor, buf, len);
		if (ret == 0)
			*retlen += len;
	}
	return ret;
}

int cqspi_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
	int ret;

	ret = cqspi_set_protocol(nor, 0);
	if (!ret)
		ret = cqspi_command_read(nor, &opcode, 1, buf, len);

	return ret;
}

int cqspi_write_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
	int ret = 0;

	if (!IS_ENABLED(CONFIG_MTD_WRITE))
		return -ENOTSUPP;

	ret = cqspi_set_protocol(nor, 0);
	if (!ret)
		ret = cqspi_command_write(nor, opcode, buf, len);

	return ret;
}

void cqspi_controller_init(struct cqspi_st *cqspi)
{
	cqspi_controller_disable(cqspi);

	/* Configure the remap address register, no remap */
	writel(0, cqspi->iobase + CQSPI_REG_REMAP);

	/* Disable all interrupts */
	writel(0, cqspi->iobase + CQSPI_REG_IRQMASK);

	cqspi_controller_enable(cqspi);
}

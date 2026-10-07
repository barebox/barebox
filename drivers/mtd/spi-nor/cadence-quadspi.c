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

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <of.h>
#include <platform_data/cadence_qspi.h>
#include <spi/spi.h>

#include "cadence-quadspi.h"

static void cqspi_fifo_write(void *dest_ahb_addr,
			     const void *src, unsigned int bytes)
{
	unsigned int temp;
	int remaining = bytes;
	unsigned int *dest_ptr = (unsigned int *)dest_ahb_addr;
	unsigned int *src_ptr = (unsigned int *)src;

	while (remaining > CQSPI_FIFO_WIDTH) {
		writel(*src_ptr, dest_ptr);
		src_ptr++;
		remaining -= CQSPI_FIFO_WIDTH;
	}
	if (remaining > 0) {
		/* dangling bytes */
		memcpy(&temp, src_ptr, remaining);
		writel(temp, dest_ptr);
	}
}

static int cqspi_command_write_addr(struct spi_nor *nor,
				    u8 opcode, unsigned int addr)
{
	unsigned int reg;
	struct cqspi_st *cqspi = nor->priv;
	void __iomem *reg_base = cqspi->iobase;

	reg = opcode << CQSPI_REG_CMDCTRL_OPCODE_LSB;
	reg |= (0x1 << CQSPI_REG_CMDCTRL_ADDR_EN_LSB);
	reg |= ((nor->addr_width - 1) & CQSPI_REG_CMDCTRL_ADD_BYTES_MASK)
		<< CQSPI_REG_CMDCTRL_ADD_BYTES_LSB;

	writel(addr, reg_base + CQSPI_REG_CMDADDRESS);

	return cqspi_exec_flash_cmd(cqspi, reg);
}

static __maybe_unused int cqspi_indirect_write_setup(struct spi_nor *nor,
						     unsigned int to_addr)
{
	unsigned int reg;
	struct cqspi_st *cqspi = nor->priv;
	void __iomem *reg_base = cqspi->iobase;

	/* Set opcode. */
	reg = nor->program_opcode << CQSPI_REG_WR_INSTR_OPCODE_LSB;
	writel(reg, reg_base + CQSPI_REG_WR_INSTR);
	reg = cqspi_calc_rdreg(nor, nor->program_opcode);
	writel(reg, reg_base + CQSPI_REG_RD_INSTR);

	writel(to_addr, reg_base + CQSPI_REG_INDIRECTWRSTARTADDR);

	reg = readl(reg_base + CQSPI_REG_SIZE);
	reg &= ~CQSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (nor->addr_width - 1);
	writel(reg, reg_base + CQSPI_REG_SIZE);
	return 0;
}

static __maybe_unused int cqspi_indirect_write_execute(struct spi_nor *nor,
						const u8 *txbuf, unsigned n_tx)
{
	int ret;
	unsigned int reg = 0;
	struct cqspi_st *cqspi = nor->priv;
	void __iomem *reg_base = cqspi->iobase;
	void __iomem *ahb_base = cqspi->ahb_base;
	int remaining = (int)n_tx;
	unsigned int page_size;
	unsigned int write_bytes;

	page_size = nor->page_size;

	writel(remaining, reg_base + CQSPI_REG_INDIRECTWRBYTES);

	writel(CQSPI_REG_SRAM_THRESHOLD_BYTES, reg_base +
	       CQSPI_REG_INDIRECTWRWATERMARK);

	/* Clear all interrupts. */
	writel(CQSPI_IRQ_STATUS_MASK, reg_base + CQSPI_REG_IRQSTATUS);

	cqspi->irq_mask = CQSPI_IRQ_MASK_WR;
	writel(cqspi->irq_mask, reg_base + CQSPI_REG_IRQMASK);

	writel(CQSPI_REG_INDIRECTWR_START_MASK,
	       reg_base + CQSPI_REG_INDIRECTWR);

	/* Write a page or remaining bytes. */
	write_bytes = remaining > page_size ? page_size : remaining;
	/* Fill up the data at the beginning */
	cqspi_fifo_write(ahb_base, txbuf, write_bytes);
	txbuf += write_bytes;
	remaining -= write_bytes;

	while (remaining > 0) {
		ret = wait_on_timeout(CQSPI_READ_TIMEOUT_MS,
			readl(reg_base + CQSPI_REG_IRQSTATUS) & cqspi->irq_mask);

		/* Clear all interrupts. */
		writel(CQSPI_IRQ_STATUS_MASK, reg_base + CQSPI_REG_IRQSTATUS);

		if (ret < 0) {
			dev_err(nor->dev, "Indirect write timeout\n");
			ret = -ETIMEDOUT;
			goto failwr;
		}

		write_bytes = remaining > page_size ? page_size : remaining;
		cqspi_fifo_write(ahb_base, txbuf, write_bytes);
		txbuf += write_bytes;
		remaining -= write_bytes;

		writel(cqspi->irq_mask, reg_base + CQSPI_REG_IRQMASK);
	}
	ret = wait_on_timeout(CQSPI_READ_TIMEOUT_MS,
			readl(reg_base + CQSPI_REG_IRQSTATUS) & cqspi->irq_mask);

	/* Clear all interrupts. */
	writel(CQSPI_IRQ_STATUS_MASK, reg_base + CQSPI_REG_IRQSTATUS);
	if (ret < 0) {
		dev_err(nor->dev, "Indirect write timeout\n");
		ret = -ETIMEDOUT;
		goto failwr;
	}

	/* Check indirect done status */
	if (wait_on_timeout(CQSPI_TIMEOUT_MS,
		readl(reg_base + CQSPI_REG_INDIRECTWR)
			& CQSPI_REG_INDIRECTWR_DONE_MASK)) {
		dev_err(nor->dev,
			"Indirect write completion error 0x%08x\n", reg);
		ret = -ETIMEDOUT;
		goto failwr;
	}

	/* Disable interrupt. */
	writel(0, reg_base + CQSPI_REG_IRQMASK);

	/* Clear indirect completion status */
	writel(CQSPI_REG_INDIRECTWR_DONE_MASK, reg_base + CQSPI_REG_INDIRECTWR);

	cqspi_wait_idle(cqspi);

	return 0;

failwr:
	/* Disable interrupt. */
	writel(0, reg_base + CQSPI_REG_IRQMASK);

	/* Cancel the indirect write */
	writel(CQSPI_REG_INDIRECTWR_CANCEL_MASK,
	       reg_base + CQSPI_REG_INDIRECTWR);
	return ret;
}

static void cqspi_write(struct spi_nor *nor, loff_t to,
			size_t len, size_t *retlen, const u_char *buf)
{
	int ret;

	if (!IS_ENABLED(CONFIG_MTD_WRITE))
		return;

	ret = cqspi_set_protocol(nor, 0);
	if (ret)
		return;

	ret = cqspi_indirect_write_setup(nor, to);
	if (ret == 0) {
		ret = cqspi_indirect_write_execute(nor, buf, len);
		if (ret == 0)
			*retlen += len;
	}
}

static int cqspi_erase(struct spi_nor *nor, loff_t offs)
{
	int ret;

	ret = cqspi_set_protocol(nor, 0);
	if (ret)
		return ret;

	/* Send write enable, then erase commands. */
	ret = nor->write_reg(nor, SPINOR_OP_WREN, NULL, 0);
	if (ret)
		return ret;

	/* Set up command buffer. */
	ret = cqspi_command_write_addr(nor, nor->erase_opcode, offs);
	if (ret)
		return ret;

	return 0;
}

static int cqspi_of_get_flash_pdata(struct device *dev,
				    struct cqspi_flash_pdata *f_pdata,
				    struct device_node *np)
{
	struct cqspi_st *cqspi = dev->priv;
	void __iomem *reg_base = cqspi->iobase;

	if (!np) {
		f_pdata->block_size = (readl(reg_base + CQSPI_REG_SIZE) >>
					CQSPI_REG_SIZE_BLOCK_LSB) &
					CQSPI_REG_SIZE_BLOCK_MASK;

		cqspi->no_reconfig = true;

		return 0;
	}

	if (of_property_read_u32(np, "cdns,block-size", &f_pdata->block_size)) {
		dev_err(dev, "couldn't determine block-size\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "cdns,read-delay", &f_pdata->read_delay)) {
		dev_err(dev, "couldn't determine read-delay\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "cdns,tshsl-ns", &f_pdata->tshsl_ns)) {
		dev_err(dev, "couldn't determine tshsl-ns\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "cdns,tsd2d-ns", &f_pdata->tsd2d_ns)) {
		dev_err(dev, "couldn't determine tsd2d-ns\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "cdns,tchsh-ns", &f_pdata->tchsh_ns)) {
		dev_err(dev, "couldn't determine tchsh-ns\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "cdns,tslch-ns", &f_pdata->tslch_ns)) {
		dev_err(dev, "couldn't determine tslch-ns\n");
		return -ENXIO;
	}

	if (of_property_read_u32(np, "spi-max-frequency", &f_pdata->clk_rate)) {
		dev_err(dev, "couldn't determine spi-max-frequency\n");
		return -ENXIO;
	}

	return 0;
}

static int cqspi_parse_dt(struct cqspi_st *cqspi)
{
	struct device_node *np = cqspi->dev->of_node;
	struct device *dev = cqspi->dev;

	cqspi->is_decoded_cs = of_property_read_bool(np, "cdns,is-decoded-cs");

	if (of_property_read_u32(np, "cdns,fifo-depth", &cqspi->fifo_depth)) {
		dev_err(dev, "couldn't determine fifo-depth\n");
		return -ENXIO;
	}

	return 0;
}

static int cqspi_setup_flash(struct device *dev,
			     struct cqspi_flash_pdata *f_pdata,
			     struct device_node *np)
{
	const struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_READ_1_1_2 |
			SNOR_HWCAPS_READ_1_1_4 |
			SNOR_HWCAPS_PP,
	};
	struct cqspi_st *cqspi = dev->priv;
	struct mtd_info *mtd;
	struct spi_nor *nor;
	int ret;

	ret = cqspi_of_get_flash_pdata(dev, f_pdata, np);
	if (ret)
		goto probe_failed;

	nor = &f_pdata->nor;
	mtd = &f_pdata->mtd;

	nor->mtd = mtd;

	if (np) {
		nor->dev = xzalloc(sizeof(*nor->dev));

		dev_set_name(nor->dev, "%s", np->name);

		nor->dev->of_node = np;
		nor->dev->id = DEVICE_ID_SINGLE;
		nor->dev->parent = dev;
		ret = register_device(nor->dev);

		if (ret)
			return ret;

		mtd->dev.parent = nor->dev;
	} else {
		nor->dev = dev;
	}

	nor->priv = cqspi;
	mtd->priv = nor;

	nor->read_reg = cqspi_read_reg;
	nor->write_reg = cqspi_write_reg;
	nor->read = cqspi_read;
	nor->write = cqspi_write;
	nor->erase = cqspi_erase;

	ret = spi_nor_scan(nor, NULL, &hwcaps, false);
	if (ret)
		goto probe_failed;

	ret = add_mtd_device(mtd, NULL, DEVICE_ID_DYNAMIC);
	if (ret)
		goto probe_failed;

	return 0;

probe_failed:
	dev_err(dev, "probing for flashchip failed\n");
	return ret;

}

static int cqspi_probe(struct device *dev)
{
	struct resource *iores;
	struct device_node *np = dev->of_node;
	struct cqspi_st *cqspi;
	struct cadence_qspi_platform_data *pdata = dev->platform_data;
	int ret;

	cqspi = kzalloc(sizeof(*cqspi), GFP_KERNEL);
	if (!cqspi)
		return -ENOMEM;

	cqspi->dev = dev;
	dev->priv = cqspi;

	if (pdata) {
		cqspi->is_decoded_cs = pdata->is_decoded_cs;
		cqspi->fifo_depth = pdata->fifo_depth;
	} else {
		ret = cqspi_parse_dt(cqspi);
		if (ret) {
			dev_err(dev, "Get platform data failed.\n");
			return -ENODEV;
		}
	}

	cqspi->qspi_clk = clk_get(dev, NULL);
	if (IS_ERR(cqspi->qspi_clk)) {
		dev_err(dev, "cannot get qspi clk\n");
		ret = PTR_ERR(cqspi->qspi_clk);
		goto probe_failed;
	}
	cqspi->master_ref_clk_hz = clk_get_rate(cqspi->qspi_clk);

	clk_enable(cqspi->qspi_clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	cqspi->iobase = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	cqspi->ahb_base = IOMEM(iores->start);

	cqspi_wait_idle(cqspi);
	cqspi_controller_init(cqspi);
	cqspi->current_cs = -1;
	cqspi->sclk = 0;

	if (!dev->of_node) {
		struct cqspi_flash_pdata *f_pdata;

		f_pdata = &cqspi->f_pdata[0];

		ret = cqspi_setup_flash(dev, f_pdata, NULL);
		if (ret)
			goto probe_failed;
	} else {
		/* Get flash device data */
		for_each_available_child_of_node(dev->of_node, np) {
			struct cqspi_flash_pdata *f_pdata;
			unsigned int cs;
			if (of_property_read_u32(np, "reg", &cs)) {
				dev_err(dev, "couldn't determine chip select\n");
				return -ENXIO;
			}
			if (cs > CQSPI_MAX_CHIPSELECT) {
				dev_err(dev, "chip select %d out of range\n", cs);
				return -ENXIO;
			}
			f_pdata = &cqspi->f_pdata[cs];

			ret = cqspi_setup_flash(dev, f_pdata, np);
			if (ret)
				goto probe_failed;
		}
	}

	dev_info(dev, "Cadence QSPI NOR flash driver\n");
	return 0;

probe_failed:
	dev_err(dev, "Cadence QSPI NOR probe failed\n");
	return ret;
}

static __maybe_unused struct of_device_id cqspi_dt_ids[] = {
	{.compatible = "cdns,qspi-nor",},
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, cqspi_dt_ids);

static struct driver cqspi_driver = {
	.name = "cadence_qspi",
	.probe = cqspi_probe,
	.of_compatible = DRV_OF_COMPAT(cqspi_dt_ids),
};
device_platform_driver(cqspi_driver);

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) STMicroelectronics 2018 - All Rights Reserved
 * Author: Ludovic Barre <ludovic.barre@st.com> for STMicroelectronics.
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/reset.h>
#include <linux/sizes.h>
#include <linux/spi/spi-mem.h>

#define QSPI_CR			0x00
#define CR_EN			BIT(0)
#define CR_ABORT		BIT(1)
#define CR_DMAEN		BIT(2)
#define CR_TCEN			BIT(3)
#define CR_SSHIFT		BIT(4)
#define CR_DFM			BIT(6)
#define CR_FSEL			BIT(7)
#define CR_FTHRES_SHIFT		8
#define CR_TEIE			BIT(16)
#define CR_TCIE			BIT(17)
#define CR_FTIE			BIT(18)
#define CR_SMIE			BIT(19)
#define CR_TOIE			BIT(20)
#define CR_APMS			BIT(22)
#define CR_PRESC_MASK		GENMASK(31, 24)

#define QSPI_DCR		0x04
#define DCR_FSIZE_MASK		GENMASK(20, 16)

#define QSPI_SR			0x08
#define SR_TEF			BIT(0)
#define SR_TCF			BIT(1)
#define SR_FTF			BIT(2)
#define SR_SMF			BIT(3)
#define SR_TOF			BIT(4)
#define SR_BUSY			BIT(5)
#define SR_FLEVEL_MASK		GENMASK(13, 8)

#define QSPI_FCR		0x0c
#define FCR_CTEF		BIT(0)
#define FCR_CTCF		BIT(1)
#define FCR_CSMF		BIT(3)

#define QSPI_DLR		0x10

#define QSPI_CCR		0x14
#define CCR_INST_MASK		GENMASK(7, 0)
#define CCR_IMODE_MASK		GENMASK(9, 8)
#define CCR_ADMODE_MASK		GENMASK(11, 10)
#define CCR_ADSIZE_MASK		GENMASK(13, 12)
#define CCR_DCYC_MASK		GENMASK(22, 18)
#define CCR_DMODE_MASK		GENMASK(25, 24)
#define CCR_FMODE_MASK		GENMASK(27, 26)
#define CCR_FMODE_INDW		(0U << 26)
#define CCR_FMODE_INDR		(1U << 26)
#define CCR_FMODE_APM		(2U << 26)
#define CCR_FMODE_MM		(3U << 26)
#define CCR_BUSWIDTH_0		0x0
#define CCR_BUSWIDTH_1		0x1
#define CCR_BUSWIDTH_2		0x2
#define CCR_BUSWIDTH_4		0x3

#define QSPI_AR			0x18
#define QSPI_DR			0x20

#define STM32_QSPI_MAX_MMAP_SZ	SZ_256M
#define STM32_QSPI_MAX_NORCHIP	2

#define STM32_FIFO_TIMEOUT_US	30000
#define STM32_BUSY_TIMEOUT_US	100000
#define STM32_ABT_TIMEOUT_US	100000

struct stm32_qspi_flash {
	u32 cs;
	u32 presc;
};

struct stm32_qspi {
	struct spi_controller ctrl;
	struct device *dev;
	void __iomem *io_base;
	void __iomem *mm_base;
	resource_size_t mm_size;
	struct clk *clk;
	u32 clk_rate;
	struct stm32_qspi_flash flash[STM32_QSPI_MAX_NORCHIP];
	u32 fmode;

	u32 dcr_reg;
};

static void stm32_qspi_read_fifo(u8 *val, void __iomem *addr)
{
	*val = readb_relaxed(addr);
}

static void stm32_qspi_write_fifo(u8 *val, void __iomem *addr)
{
	writeb_relaxed(*val, addr);
}

static int stm32_qspi_tx_poll(struct stm32_qspi *qspi,
			      const struct spi_mem_op *op)
{
	void (*tx_fifo)(u8 *val, void __iomem *addr);
	u32 len = op->data.nbytes, sr;
	u8 *buf;
	int ret;

	if (op->data.dir == SPI_MEM_DATA_IN) {
		tx_fifo = stm32_qspi_read_fifo;
		buf = op->data.buf.in;

	} else {
		tx_fifo = stm32_qspi_write_fifo;
		buf = (u8 *)op->data.buf.out;
	}

	while (len--) {
		ret = readl_relaxed_poll_timeout(qspi->io_base + QSPI_SR,
						 sr, (sr & SR_FTF),
						 STM32_FIFO_TIMEOUT_US);
		if (ret) {
			dev_err(qspi->dev, "fifo timeout (len:%d stat:%#x)\n",
				len, sr);
			return ret;
		}
		tx_fifo(buf++, qspi->io_base + QSPI_DR);
	}

	return 0;
}

static int stm32_qspi_tx_mm(struct stm32_qspi *qspi,
			    const struct spi_mem_op *op)
{
	memcpy_fromio(op->data.buf.in, qspi->mm_base + op->addr.val,
		      op->data.nbytes);
	return 0;
}

static int stm32_qspi_tx(struct stm32_qspi *qspi, const struct spi_mem_op *op)
{
	if (!op->data.nbytes)
		return 0;

	if (qspi->fmode == CCR_FMODE_MM)
		return stm32_qspi_tx_mm(qspi, op);

	return stm32_qspi_tx_poll(qspi, op);
}

static int stm32_qspi_wait_nobusy(struct stm32_qspi *qspi)
{
	u32 sr;

	return readl_relaxed_poll_timeout(qspi->io_base + QSPI_SR, sr,
					  !(sr & SR_BUSY),
					  STM32_BUSY_TIMEOUT_US);
}

static int stm32_qspi_wait_cmd(struct stm32_qspi *qspi)
{
	u32 sr;
	int err;

	err = readl_poll_timeout(qspi->io_base + QSPI_SR,
				 sr, (sr & SR_TCF),
				 STM32_FIFO_TIMEOUT_US);

	if (!err) {
		sr = readl_relaxed(qspi->io_base + QSPI_SR);
		if (sr & SR_TEF)
			err = -EIO;
	}

	/* clear flags */
	writel_relaxed(FCR_CTCF | FCR_CTEF, qspi->io_base + QSPI_FCR);
	if (!err)
		err = stm32_qspi_wait_nobusy(qspi);

	return err;
}

static int stm32_qspi_get_mode(u8 buswidth)
{
	if (buswidth >= 4)
		return CCR_BUSWIDTH_4;

	return buswidth;
}

static int stm32_qspi_send(struct spi_device *spi, const struct spi_mem_op *op)
{
	struct stm32_qspi *qspi = spi_controller_get_devdata(spi->controller);
	struct stm32_qspi_flash *flash = &qspi->flash[spi_get_chipselect(spi, 0)];
	u32 ccr, cr;
	int timeout, err = 0, err_poll_status = 0;

	dev_dbg(qspi->dev, "cmd:%#x mode:%d.%d.%d.%d addr:%#llx len:%#x\n",
		op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	cr = readl_relaxed(qspi->io_base + QSPI_CR);
	cr &= ~CR_PRESC_MASK & ~CR_FSEL;
	cr |= FIELD_PREP(CR_PRESC_MASK, flash->presc);
	cr |= FIELD_PREP(CR_FSEL, flash->cs);
	writel_relaxed(cr, qspi->io_base + QSPI_CR);

	setbits_le32(qspi->io_base + QSPI_CR, CR_EN);

	if (op->data.nbytes)
		writel_relaxed(op->data.nbytes - 1,
			       qspi->io_base + QSPI_DLR);

	ccr = qspi->fmode;
	ccr |= FIELD_PREP(CCR_INST_MASK, op->cmd.opcode);
	ccr |= FIELD_PREP(CCR_IMODE_MASK,
			  stm32_qspi_get_mode(op->cmd.buswidth));

	if (op->addr.nbytes) {
		ccr |= FIELD_PREP(CCR_ADMODE_MASK,
				  stm32_qspi_get_mode(op->addr.buswidth));
		ccr |= FIELD_PREP(CCR_ADSIZE_MASK, op->addr.nbytes - 1);
	}

	if (op->dummy.nbytes)
		ccr |= FIELD_PREP(CCR_DCYC_MASK,
				  op->dummy.nbytes * 8 / op->dummy.buswidth);

	if (op->data.nbytes) {
		ccr |= FIELD_PREP(CCR_DMODE_MASK,
				  stm32_qspi_get_mode(op->data.buswidth));
	}

	writel_relaxed(ccr, qspi->io_base + QSPI_CCR);

	if (op->addr.nbytes && qspi->fmode != CCR_FMODE_MM)
		writel_relaxed(op->addr.val, qspi->io_base + QSPI_AR);

	err = stm32_qspi_tx(qspi, op);

	/*
	 * Abort in:
	 * -error case
	 * -read memory map: prefetching must be stopped if we read the last
	 *  byte of device (device size - fifo size). like device size is not
	 *  knows, the prefetching is always stop.
	 */
	if (err || err_poll_status || qspi->fmode == CCR_FMODE_MM)
		goto abort;

	/* wait end of tx in indirect mode */
	err = stm32_qspi_wait_cmd(qspi);
	if (err)
		goto abort;

	clrbits_le32(qspi->io_base + QSPI_CR, CR_EN);

	return 0;

abort:
	cr = readl_relaxed(qspi->io_base + QSPI_CR) | CR_ABORT;
	writel_relaxed(cr, qspi->io_base + QSPI_CR);

	/* wait clear of abort bit by hw */
	timeout = readl_relaxed_poll_timeout(qspi->io_base + QSPI_CR,
					     cr, !(cr & CR_ABORT),
					     STM32_ABT_TIMEOUT_US);

	writel_relaxed(FCR_CTCF | FCR_CSMF, qspi->io_base + QSPI_FCR);

	if (err || err_poll_status || timeout)
		dev_err(qspi->dev, "%s err:%d err_poll_status:%d abort timeout:%d\n",
			__func__, err, err_poll_status, timeout);

	clrbits_le32(qspi->io_base + QSPI_CR, CR_EN);

	return err;
}

static int stm32_qspi_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct stm32_qspi *qspi = spi_controller_get_devdata(mem->spi->controller);
	int ret;

	if (op->data.dir == SPI_MEM_DATA_IN && op->data.nbytes)
		qspi->fmode = CCR_FMODE_INDR;
	else
		qspi->fmode = CCR_FMODE_INDW;

	ret = stm32_qspi_send(mem->spi, op);

	return ret;
}

static int stm32_qspi_dirmap_create(struct spi_mem_dirmap_desc *desc)
{
	struct stm32_qspi *qspi = spi_controller_get_devdata(desc->mem->spi->controller);

	if (desc->info.op_tmpl.data.dir == SPI_MEM_DATA_OUT)
		return -EOPNOTSUPP;

	/* should never happen, as mm_base == null is an error probe exit condition */
	if (!qspi->mm_base && desc->info.op_tmpl.data.dir == SPI_MEM_DATA_IN)
		return -EOPNOTSUPP;

	if (!qspi->mm_size)
		return -EOPNOTSUPP;

	return 0;
}

static ssize_t stm32_qspi_dirmap_read(struct spi_mem_dirmap_desc *desc,
				      u64 offs, size_t len, void *buf)
{
	struct stm32_qspi *qspi = spi_controller_get_devdata(desc->mem->spi->controller);
	struct spi_mem_op op;
	u32 addr_max;
	int ret;

	/* make a local copy of desc op_tmpl and complete dirmap rdesc
	 * spi_mem_op template with offs, len and *buf in  order to get
	 * all needed transfer information into struct spi_mem_op
	 */
	memcpy(&op, &desc->info.op_tmpl, sizeof(struct spi_mem_op));
	dev_dbg(qspi->dev, "%s len = 0x%zx offs = 0x%llx buf = 0x%p\n", __func__, len, offs, buf);

	op.data.nbytes = len;
	op.addr.val = desc->info.offset + offs;
	op.data.buf.in = buf;

	addr_max = op.addr.val + op.data.nbytes + 1;
	if (addr_max < qspi->mm_size && op.addr.buswidth)
		qspi->fmode = CCR_FMODE_MM;
	else
		qspi->fmode = CCR_FMODE_INDR;

	ret = stm32_qspi_send(desc->mem->spi, &op);

	return ret ?: len;
}

static int stm32_qspi_setup(struct spi_device *spi)
{
	struct spi_controller *ctrl = spi->controller;
	struct stm32_qspi *qspi = spi_controller_get_devdata(ctrl);
	struct stm32_qspi_flash *flash;
	u32 presc, mode;

	if (!spi->max_speed_hz)
		return -EINVAL;

	mode = spi->mode & (SPI_TX_OCTAL | SPI_RX_OCTAL);
	if (mode && gpiod_count(qspi->dev, "cs") == -ENOENT) {
		dev_err(qspi->dev, "spi-rx-bus-width\\/spi-tx-bus-width\\/cs-gpios\n");
		dev_err(qspi->dev, "configuration not supported\n");

		return -EINVAL;
	}

	presc = DIV_ROUND_UP(qspi->clk_rate, spi->max_speed_hz) - 1;

	flash = &qspi->flash[spi_get_chipselect(spi, 0)];
	flash->cs = spi_get_chipselect(spi, 0);
	flash->presc = presc;

	setbits_le32(qspi->io_base + QSPI_CR, CR_SSHIFT);

	/* set dcr fsize to max address */
	qspi->dcr_reg = DCR_FSIZE_MASK;
	writel_relaxed(qspi->dcr_reg, qspi->io_base + QSPI_DCR);

	return 0;
}

static const struct spi_controller_mem_ops stm32_qspi_mem_ops = {
	.exec_op	= stm32_qspi_exec_op,
	.dirmap_create	= stm32_qspi_dirmap_create,
	.dirmap_read	= stm32_qspi_dirmap_read,
};

static int stm32_qspi_probe(struct device *dev)
{
	struct spi_controller *ctrl;
	struct reset_control *rstc;
	struct stm32_qspi *qspi;
	struct resource *res;
	int ret;

	qspi = xzalloc(sizeof(*qspi));

	ctrl = &qspi->ctrl;

	res = dev_request_mem_resource_by_name(dev, "qspi");
	if (IS_ERR(res))
		return PTR_ERR(res);

	qspi->io_base = IOMEM(res->start);

	res = dev_request_mem_resource_by_name(dev, "qspi_mm");
	if (IS_ERR(res))
		return PTR_ERR(res);

	qspi->mm_base = IOMEM(res->start);

	qspi->mm_size = resource_size(res);
	if (qspi->mm_size > STM32_QSPI_MAX_MMAP_SZ)
		return -EINVAL;

	qspi->clk = clk_get(dev, NULL);
	if (IS_ERR(qspi->clk))
		return PTR_ERR(qspi->clk);

	qspi->clk_rate = clk_get_rate(qspi->clk);
	if (!qspi->clk_rate)
		return -EINVAL;

	ret = clk_prepare_enable(qspi->clk);
	if (ret) {
		dev_err(dev, "can not enable the clock\n");
		return ret;
	}

	rstc = reset_control_get(dev, NULL);
	if (IS_ERR(rstc)) {
		ret = PTR_ERR(rstc);
		if (ret == -EPROBE_DEFER)
			goto err_clk_disable;
	} else {
		reset_control_assert(rstc);
		udelay(2);
		reset_control_deassert(rstc);
	}

	qspi->dev = dev;
	dev->priv = qspi;

	/* TODO: add support for multiple bits
	 * ctrl->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD | SPI_TX_OCTAL
	 *	| SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_OCTAL;
	 */
	ctrl->setup = stm32_qspi_setup;
	ctrl->bus_num = -1;
	ctrl->mem_ops = &stm32_qspi_mem_ops;
	ctrl->use_gpio_descriptors = true;
	ctrl->num_chipselect = STM32_QSPI_MAX_NORCHIP;
	ctrl->dev = dev;

	spi_controller_set_devdata(ctrl, qspi);

	ret = spi_register_controller(ctrl);
	if (ret)
		goto err_pm_runtime_free;

	return 0;

err_pm_runtime_free:
	/* disable qspi */
	writel_relaxed(0, qspi->io_base + QSPI_CR);
err_clk_disable:
	clk_disable_unprepare(qspi->clk);

	return ret;
}

static void stm32_qspi_remove(struct device *dev)
{
	struct stm32_qspi *qspi = dev->priv;

	/* disable qspi */
	writel_relaxed(0, qspi->io_base + QSPI_CR);
	clk_disable_unprepare(qspi->clk);
}

static const struct of_device_id stm32_qspi_match[] = {
	{.compatible = "st,stm32f469-qspi"},
	{}
};
MODULE_DEVICE_TABLE(of, stm32_qspi_match);

static struct driver stm32_qspi_driver = {
	.probe	= stm32_qspi_probe,
	.remove = stm32_qspi_remove,
	.name = "stm32-qspi",
	.of_match_table = stm32_qspi_match,
};
device_platform_driver(stm32_qspi_driver);

MODULE_AUTHOR("Ludovic Barre <ludovic.barre@st.com>");
MODULE_DESCRIPTION("STMicroelectronics STM32 quad spi driver");
MODULE_LICENSE("GPL v2");

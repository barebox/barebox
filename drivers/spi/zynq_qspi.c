// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019 Xilinx, Inc.
 *
 * Author: Naga Sureshkumar Relli <nagasure@xilinx.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/spi/spi-mem.h>
#include <spi/spi.h>

/* Register offset definitions */
#define ZYNQ_QSPI_CONFIG_OFFSET		0x00 /* Configuration  Register, RW */
#define ZYNQ_QSPI_STATUS_OFFSET		0x04 /* Interrupt Status Register, RO */
#define ZYNQ_QSPI_IEN_OFFSET		0x08 /* Interrupt Enable Register, WO */
#define ZYNQ_QSPI_IDIS_OFFSET		0x0C /* Interrupt Disable Reg, WO */
#define ZYNQ_QSPI_IMASK_OFFSET		0x10 /* Interrupt Enabled Mask Reg,RO */
#define ZYNQ_QSPI_ENABLE_OFFSET		0x14 /* Enable/Disable Register, RW */
#define ZYNQ_QSPI_DELAY_OFFSET		0x18 /* Delay Register, RW */
#define ZYNQ_QSPI_TXD_00_00_OFFSET	0x1C /* Transmit 4-byte inst, WO */
#define ZYNQ_QSPI_TXD_00_01_OFFSET	0x80 /* Transmit 1-byte inst, WO */
#define ZYNQ_QSPI_TXD_00_10_OFFSET	0x84 /* Transmit 2-byte inst, WO */
#define ZYNQ_QSPI_TXD_00_11_OFFSET	0x88 /* Transmit 3-byte inst, WO */
#define ZYNQ_QSPI_RXD_OFFSET		0x20 /* Data Receive Register, RO */
#define ZYNQ_QSPI_SIC_OFFSET		0x24 /* Slave Idle Count Register, RW */
#define ZYNQ_QSPI_TX_THRESH_OFFSET	0x28 /* TX FIFO Watermark Reg, RW */
#define ZYNQ_QSPI_RX_THRESH_OFFSET	0x2C /* RX FIFO Watermark Reg, RW */
#define ZYNQ_QSPI_GPIO_OFFSET		0x30 /* GPIO Register, RW */
#define ZYNQ_QSPI_LINEAR_CFG_OFFSET	0xA0 /* Linear Adapter Config Ref, RW */
#define ZYNQ_QSPI_MOD_ID_OFFSET		0xFC /* Module ID Register, RO */

/*
 * QSPI Configuration Register bit Masks
 *
 * This register contains various control bits that effect the operation
 * of the QSPI controller
 */
#define ZYNQ_QSPI_CONFIG_IFMODE_MASK	BIT(31) /* Flash Memory Interface */
#define ZYNQ_QSPI_CONFIG_MANSRT_MASK	BIT(16) /* Manual TX Start */
#define ZYNQ_QSPI_CONFIG_MANSRTEN_MASK	BIT(15) /* Enable Manual TX Mode */
#define ZYNQ_QSPI_CONFIG_SSFORCE_MASK	BIT(14) /* Manual Chip Select */
#define ZYNQ_QSPI_CONFIG_BDRATE_MASK	GENMASK(5, 3) /* Baud Rate Mask */
#define ZYNQ_QSPI_CONFIG_CPHA_MASK	BIT(2) /* Clock Phase Control */
#define ZYNQ_QSPI_CONFIG_CPOL_MASK	BIT(1) /* Clock Polarity Control */
#define ZYNQ_QSPI_CONFIG_SSCTRL_MASK	BIT(10) /* Slave Select Mask */
#define ZYNQ_QSPI_CONFIG_FWIDTH_MASK	GENMASK(7, 6) /* FIFO width */
#define ZYNQ_QSPI_CONFIG_MSTREN_MASK	BIT(0) /* Master Mode */

/*
 * QSPI Configuration Register - Baud rate and slave select
 *
 * These are the values used in the calculation of baud rate divisor and
 * setting the slave select.
 */
#define ZYNQ_QSPI_BAUD_DIV_MAX		GENMASK(2, 0) /* Baud rate maximum */
#define ZYNQ_QSPI_BAUD_DIV_SHIFT	3 /* Baud rate divisor shift in CR */
#define ZYNQ_QSPI_SS_SHIFT		10 /* Slave Select field shift in CR */

/*
 * QSPI Interrupt Registers bit Masks
 *
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same
 * bit definitions.
 */
#define ZYNQ_QSPI_IXR_RX_OVERFLOW_MASK	BIT(0) /* QSPI RX FIFO Overflow */
#define ZYNQ_QSPI_IXR_TXNFULL_MASK	BIT(2) /* QSPI TX FIFO Overflow */
#define ZYNQ_QSPI_IXR_TXFULL_MASK	BIT(3) /* QSPI TX FIFO is full */
#define ZYNQ_QSPI_IXR_RXNEMTY_MASK	BIT(4) /* QSPI RX FIFO Not Empty */
#define ZYNQ_QSPI_IXR_RXF_FULL_MASK	BIT(5) /* QSPI RX FIFO is full */
#define ZYNQ_QSPI_IXR_TXF_UNDRFLOW_MASK	BIT(6) /* QSPI TX FIFO Underflow */
#define ZYNQ_QSPI_IXR_ALL_MASK		(ZYNQ_QSPI_IXR_RX_OVERFLOW_MASK | \
					ZYNQ_QSPI_IXR_TXNFULL_MASK | \
					ZYNQ_QSPI_IXR_TXFULL_MASK | \
					ZYNQ_QSPI_IXR_RXNEMTY_MASK | \
					ZYNQ_QSPI_IXR_RXF_FULL_MASK | \
					ZYNQ_QSPI_IXR_TXF_UNDRFLOW_MASK)
#define ZYNQ_QSPI_IXR_RXTX_MASK		(ZYNQ_QSPI_IXR_TXNFULL_MASK | \
					ZYNQ_QSPI_IXR_RXNEMTY_MASK)

/*
 * QSPI Enable Register bit Masks
 *
 * This register is used to enable or disable the QSPI controller
 */
#define ZYNQ_QSPI_ENABLE_ENABLE_MASK	BIT(0) /* QSPI Enable Bit Mask */

/*
 * QSPI Linear Configuration Register
 *
 * It is named Linear Configuration but it controls other modes when not in
 * linear mode also.
 */
#define ZYNQ_QSPI_LCFG_TWO_MEM_MASK	BIT(30) /* LQSPI Two memories Mask */
#define ZYNQ_QSPI_LCFG_SEP_BUS_MASK	BIT(29) /* LQSPI Separate bus Mask */
#define ZYNQ_QSPI_LCFG_U_PAGE_MASK	BIT(28) /* LQSPI Upper Page Mask */

#define ZYNQ_QSPI_LCFG_DUMMY_SHIFT	8

#define ZYNQ_QSPI_FAST_READ_QOUT_CODE	0x6B /* read instruction code */
#define ZYNQ_QSPI_FIFO_DEPTH		63 /* FIFO depth in words */
#define ZYNQ_QSPI_RX_THRESHOLD		32 /* Rx FIFO threshold level */
#define ZYNQ_QSPI_TX_THRESHOLD		1 /* Tx FIFO threshold level */

/*
 * The modebits configurable by the driver to make the SPI support different
 * data formats
 */
#define ZYNQ_QSPI_MODEBITS			(SPI_CPOL | SPI_CPHA)

/**
 * struct zynq_qspi - Defines qspi driver instance
 * @regs:		Virtual address of the QSPI controller registers
 * @refclk:		Pointer to the peripheral clock
 * @pclk:		Pointer to the APB clock
 * @irq:		IRQ number
 * @txbuf:		Pointer to the TX buffer
 * @rxbuf:		Pointer to the RX buffer
 * @tx_bytes:		Number of bytes left to transfer
 * @rx_bytes:		Number of bytes left to receive
 * @data_completion:	completion structure
 */
struct zynq_qspi {
	struct spi_controller ctlr;
	struct device *dev;
	void __iomem *regs;
	struct clk *refclk;
	struct clk *pclk;
	int tx_bytes;
	int rx_bytes;
	u8 *txbuf;
	u8 *rxbuf;
};

/*
 * Inline functions for the QSPI controller read/write
 */
static inline u32 zynq_qspi_read(struct zynq_qspi *xqspi, u32 offset)
{
	return readl(xqspi->regs + offset);
}

static inline void zynq_qspi_write(struct zynq_qspi *xqspi, u32 offset, u32 val)
{
	writel(val, xqspi->regs + offset);
}

/**
 * zynq_qspi_init_hw - Initialize the hardware
 * @xqspi:	Pointer to the zynq_qspi structure
 *
 * The default settings of the QSPI controller's configurable parameters on
 * reset are
 *	- Master mode
 *	- Baud rate divisor is set to 2
 *	- Tx threshold set to 1l Rx threshold set to 32
 *	- Flash memory interface mode enabled
 *	- Size of the word to be transferred as 8 bit
 * This function performs the following actions
 *	- Disable and clear all the interrupts
 *	- Enable manual slave select
 *	- Enable manual start
 *	- Deselect all the chip select lines
 *	- Set the size of the word to be transferred as 32 bit
 *	- Set the little endian mode of TX FIFO and
 *	- Enable the QSPI controller
 */
static void zynq_qspi_init_hw(struct zynq_qspi *xqspi)
{
	u32 config_reg;

	zynq_qspi_write(xqspi, ZYNQ_QSPI_ENABLE_OFFSET, 0);
	zynq_qspi_write(xqspi, ZYNQ_QSPI_IDIS_OFFSET, ZYNQ_QSPI_IXR_ALL_MASK);

	/* Disable linear mode as the boot loader may have used it */
	zynq_qspi_write(xqspi, ZYNQ_QSPI_LINEAR_CFG_OFFSET, 0);

	/* Clear the RX FIFO */
	while (zynq_qspi_read(xqspi, ZYNQ_QSPI_STATUS_OFFSET) &
			      ZYNQ_QSPI_IXR_RXNEMTY_MASK)
		zynq_qspi_read(xqspi, ZYNQ_QSPI_RXD_OFFSET);

	zynq_qspi_write(xqspi, ZYNQ_QSPI_STATUS_OFFSET, ZYNQ_QSPI_IXR_ALL_MASK);
	config_reg = zynq_qspi_read(xqspi, ZYNQ_QSPI_CONFIG_OFFSET);
	config_reg &= ~(ZYNQ_QSPI_CONFIG_MSTREN_MASK |
			ZYNQ_QSPI_CONFIG_CPOL_MASK |
			ZYNQ_QSPI_CONFIG_CPHA_MASK |
			ZYNQ_QSPI_CONFIG_BDRATE_MASK |
			ZYNQ_QSPI_CONFIG_SSFORCE_MASK |
			ZYNQ_QSPI_CONFIG_MANSRTEN_MASK |
			ZYNQ_QSPI_CONFIG_MANSRT_MASK);
	config_reg |= (ZYNQ_QSPI_CONFIG_MSTREN_MASK |
		       ZYNQ_QSPI_CONFIG_SSFORCE_MASK |
		       ZYNQ_QSPI_CONFIG_FWIDTH_MASK |
		       ZYNQ_QSPI_CONFIG_IFMODE_MASK);
	zynq_qspi_write(xqspi, ZYNQ_QSPI_CONFIG_OFFSET, config_reg);

	zynq_qspi_write(xqspi, ZYNQ_QSPI_RX_THRESH_OFFSET,
			ZYNQ_QSPI_RX_THRESHOLD);
	zynq_qspi_write(xqspi, ZYNQ_QSPI_TX_THRESH_OFFSET,
			ZYNQ_QSPI_TX_THRESHOLD);

	zynq_qspi_write(xqspi, ZYNQ_QSPI_ENABLE_OFFSET,
			ZYNQ_QSPI_ENABLE_ENABLE_MASK);
}

/**
 * zynq_qspi_rxfifo_op - Read 1..4 bytes from RxFIFO to RX buffer
 * @xqspi:	Pointer to the zynq_qspi structure
 * @size:	Number of bytes to be read (1..4)
 */
static void zynq_qspi_rxfifo_op(struct zynq_qspi *xqspi, unsigned int size)
{
	u32 data;

	data = zynq_qspi_read(xqspi, ZYNQ_QSPI_RXD_OFFSET);

	if (xqspi->rxbuf) {
		memcpy(xqspi->rxbuf, ((u8 *)&data) + 4 - size, size);
		xqspi->rxbuf += size;
	}

	xqspi->rx_bytes -= size;
	if (xqspi->rx_bytes < 0)
		xqspi->rx_bytes = 0;
}

/**
 * zynq_qspi_txfifo_op - Write 1..4 bytes from TX buffer to TxFIFO
 * @xqspi:	Pointer to the zynq_qspi structure
 * @size:	Number of bytes to be written (1..4)
 */
static void zynq_qspi_txfifo_op(struct zynq_qspi *xqspi, unsigned int size)
{
	static const unsigned int offset[4] = {
		ZYNQ_QSPI_TXD_00_01_OFFSET, ZYNQ_QSPI_TXD_00_10_OFFSET,
		ZYNQ_QSPI_TXD_00_11_OFFSET, ZYNQ_QSPI_TXD_00_00_OFFSET };
	u32 data;

	if (xqspi->txbuf) {
		data = 0xffffffff;
		memcpy(&data, xqspi->txbuf, size);
		xqspi->txbuf += size;
	} else {
		data = 0;
	}

	xqspi->tx_bytes -= size;
	zynq_qspi_write(xqspi, offset[size - 1], data);
}

/**
 * zynq_qspi_chipselect - Select or deselect the chip select line
 * @spi:	Pointer to the spi_device structure
 * @assert:	1 for select or 0 for deselect the chip select line
 */
static void zynq_qspi_chipselect(struct spi_device *spi, bool assert)
{
	struct spi_controller *ctrl = spi->master;
	struct zynq_qspi *xqspi = spi_controller_get_devdata(ctrl);
	u32 config_reg;

	config_reg = zynq_qspi_read(xqspi, ZYNQ_QSPI_CONFIG_OFFSET);
	if (assert) {
		/* Select the slave */
		config_reg &= ~ZYNQ_QSPI_CONFIG_SSCTRL_MASK;
		config_reg |= (((~(BIT(spi->chip_select))) <<
				ZYNQ_QSPI_SS_SHIFT) &
				ZYNQ_QSPI_CONFIG_SSCTRL_MASK);
	} else {
		config_reg |= ZYNQ_QSPI_CONFIG_SSCTRL_MASK;
	}

	zynq_qspi_write(xqspi, ZYNQ_QSPI_CONFIG_OFFSET, config_reg);
}

/**
 * zynq_qspi_config_op - Configure QSPI controller for specified transfer
 * @xqspi:	Pointer to the zynq_qspi structure
 * @qspi:	Pointer to the spi_device structure
 *
 * Sets the operational mode of QSPI controller for the next QSPI transfer and
 * sets the requested clock frequency.
 *
 * Return:	0 on success and -EINVAL on invalid input parameter
 *
 * Note: If the requested frequency is not an exact match with what can be
 * obtained using the prescalar value, the driver sets the clock frequency which
 * is lower than the requested frequency (maximum lower) for the transfer. If
 * the requested frequency is higher or lower than that is supported by the QSPI
 * controller the driver will set the highest or lowest frequency supported by
 * controller.
 */
static int zynq_qspi_config_op(struct zynq_qspi *xqspi, struct spi_device *spi)
{
	u32 config_reg, baud_rate_val = 0;

	/*
	 * Set the clock frequency
	 * The baud rate divisor is not a direct mapping to the value written
	 * into the configuration register (config_reg[5:3])
	 * i.e. 000 - divide by 2
	 *      001 - divide by 4
	 *      ----------------
	 *      111 - divide by 256
	 */
	while ((baud_rate_val < ZYNQ_QSPI_BAUD_DIV_MAX)  &&
	       (clk_get_rate(xqspi->refclk) / (2 << baud_rate_val)) >
		spi->max_speed_hz)
		baud_rate_val++;

	config_reg = zynq_qspi_read(xqspi, ZYNQ_QSPI_CONFIG_OFFSET);

	/* Set the QSPI clock phase and clock polarity */
	config_reg &= (~ZYNQ_QSPI_CONFIG_CPHA_MASK) &
		      (~ZYNQ_QSPI_CONFIG_CPOL_MASK);
	if (spi->mode & SPI_CPHA)
		config_reg |= ZYNQ_QSPI_CONFIG_CPHA_MASK;
	if (spi->mode & SPI_CPOL)
		config_reg |= ZYNQ_QSPI_CONFIG_CPOL_MASK;

	config_reg &= ~ZYNQ_QSPI_CONFIG_BDRATE_MASK;
	config_reg |= (baud_rate_val << ZYNQ_QSPI_BAUD_DIV_SHIFT);
	zynq_qspi_write(xqspi, ZYNQ_QSPI_CONFIG_OFFSET, config_reg);

	return 0;
}

static int zynq_qspi_setup_op(struct spi_device *spi)
{
	struct spi_controller *ctrl = spi->master;
	struct zynq_qspi *xqspi = spi_controller_get_devdata(ctrl);

	zynq_qspi_write(xqspi, ZYNQ_QSPI_ENABLE_OFFSET,
			ZYNQ_QSPI_ENABLE_ENABLE_MASK);

	return 0;
}

/**
 * zynq_qspi_write_op - Fills the TX FIFO with as many bytes as possible
 * @xqspi:	Pointer to the zynq_qspi structure
 * @txcount:	Maximum number of words to write
 * @txempty:	Indicates that TxFIFO is empty
 */
static void zynq_qspi_write_op(struct zynq_qspi *xqspi, int txcount,
			       bool txempty)
{
	int count, len, k;

	len = xqspi->tx_bytes;
	if (len && len < sizeof(u32)) {
		/*
		 * We must empty the TxFIFO between accesses to TXD0,
		 * TXD1, TXD2, TXD3.
		 */
		if (txempty)
			zynq_qspi_txfifo_op(xqspi, len);

		return;
	}

	count = len / sizeof(u32);
	if (count > txcount)
		count = txcount;

	if (xqspi->txbuf) {
		u32 *buf = (u32 *)xqspi->txbuf;
		for (k = 0; k < count; k++, buf++)
			zynq_qspi_write(xqspi, ZYNQ_QSPI_TXD_00_00_OFFSET, *buf);
		xqspi->txbuf += count * sizeof(u32);
	} else {
		for (k = 0; k < count; k++)
			zynq_qspi_write(xqspi, ZYNQ_QSPI_TXD_00_00_OFFSET, 0);
	}

	xqspi->tx_bytes -= count * sizeof(u32);
}

/**
 * zynq_qspi_read_op - Drains the RX FIFO by as many bytes as possible
 * @xqspi:	Pointer to the zynq_qspi structure
 * @rxcount:	Maximum number of words to read
 */
static void zynq_qspi_read_op(struct zynq_qspi *xqspi, int rxcount)
{
	int count, len, k;

	len = xqspi->rx_bytes - xqspi->tx_bytes;
	count = len / sizeof(u32);
	if (count > rxcount)
		count = rxcount;
	if (xqspi->rxbuf) {
		u32 *buf = (u32 *)xqspi->rxbuf;
		for (k = 0; k < count; k++, buf++)
			*buf = zynq_qspi_read(xqspi, ZYNQ_QSPI_RXD_OFFSET);
		xqspi->rxbuf += count * sizeof(u32);
	} else {
		for (k = 0; k < count; k++)
			zynq_qspi_read(xqspi, ZYNQ_QSPI_RXD_OFFSET);
	}
	xqspi->rx_bytes -= count * sizeof(u32);
	len -= count * sizeof(u32);

	if (len && len < 4 && count < rxcount)
		zynq_qspi_rxfifo_op(xqspi, len);
}

static int zynq_qspi_poll_irq(struct zynq_qspi *xqspi)
{
	u32 intr_status;
	bool txempty;
	int ret;

	for (;;) {
		ret = readl_poll_timeout(xqspi->regs + ZYNQ_QSPI_STATUS_OFFSET,
					 intr_status, intr_status,
					 100 * USEC_PER_MSEC);
		if (ret)
			return ret;

		zynq_qspi_write(xqspi, ZYNQ_QSPI_STATUS_OFFSET, intr_status);

		if ((intr_status & ZYNQ_QSPI_IXR_TXNFULL_MASK) ||
		    (intr_status & ZYNQ_QSPI_IXR_RXNEMTY_MASK)) {
			/*
			 * This bit is set when Tx FIFO has < THRESHOLD entries.
			 * We have the THRESHOLD value set to 1,
			 * so this bit indicates Tx FIFO is empty.
			 */
			txempty = !!(intr_status & ZYNQ_QSPI_IXR_TXNFULL_MASK);
			/* Read out the data from the RX FIFO */
			zynq_qspi_read_op(xqspi, ZYNQ_QSPI_RX_THRESHOLD);
			if (xqspi->tx_bytes) {
				/* There is more data to send */
				zynq_qspi_write_op(xqspi, ZYNQ_QSPI_RX_THRESHOLD,
						   txempty);
			} else if (!xqspi->rx_bytes){
				/* No more RX or TX bytes -> transfer done */
				return 0;
			}
		}
	}
}

/**
 * zynq_qspi_exec_mem_op() - Initiates the QSPI transfer
 * @mem: the SPI memory
 * @op: the memory operation to execute
 *
 * Executes a memory operation.
 *
 * This function first selects the chip and starts the memory operation.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
static int zynq_qspi_exec_mem_op(struct spi_mem *mem,
				 const struct spi_mem_op *op)
{
	struct zynq_qspi *xqspi = spi_controller_get_devdata(mem->spi->master);
	int ret, i;
	u8 *tmpbuf;

	dev_dbg(xqspi->dev, "cmd:%#x mode:%d.%d.%d.%d\n",
		op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth);

	zynq_qspi_chipselect(mem->spi, true);
	zynq_qspi_config_op(xqspi, mem->spi);

	if (op->cmd.opcode) {
		xqspi->txbuf = (u8 *)&op->cmd.opcode;
		xqspi->rxbuf = NULL;
		xqspi->tx_bytes = sizeof(op->cmd.opcode);
		xqspi->rx_bytes = sizeof(op->cmd.opcode);
		zynq_qspi_write_op(xqspi, ZYNQ_QSPI_FIFO_DEPTH, true);
		ret = zynq_qspi_poll_irq(xqspi);
		if (ret)
			return ret;
	}

	if (op->addr.nbytes) {
		for (i = 0; i < op->addr.nbytes; i++) {
			xqspi->txbuf[i] = op->addr.val >>
					(8 * (op->addr.nbytes - i - 1));
		}

		xqspi->rxbuf = NULL;
		xqspi->tx_bytes = op->addr.nbytes;
		xqspi->rx_bytes = op->addr.nbytes;
		zynq_qspi_write_op(xqspi, ZYNQ_QSPI_FIFO_DEPTH, true);
		ret = zynq_qspi_poll_irq(xqspi);
		if (ret)
			return ret;
	}

	if (op->dummy.nbytes) {
		tmpbuf = kzalloc(op->dummy.nbytes, GFP_KERNEL);
		memset(tmpbuf, 0xff, op->dummy.nbytes);
		xqspi->txbuf = tmpbuf;
		xqspi->rxbuf = NULL;
		xqspi->tx_bytes = op->dummy.nbytes;
		xqspi->rx_bytes = op->dummy.nbytes;
		zynq_qspi_write_op(xqspi, ZYNQ_QSPI_FIFO_DEPTH, true);
		ret = zynq_qspi_poll_irq(xqspi);
		kfree(tmpbuf);
		if (ret)
			return ret;
	}

	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_OUT) {
			xqspi->txbuf = (u8 *)op->data.buf.out;
			xqspi->tx_bytes = op->data.nbytes;
			xqspi->rxbuf = NULL;
			xqspi->rx_bytes = op->data.nbytes;
		} else {
			xqspi->txbuf = NULL;
			xqspi->rxbuf = (u8 *)op->data.buf.in;
			xqspi->rx_bytes = op->data.nbytes;
			xqspi->tx_bytes = op->data.nbytes;
		}

		zynq_qspi_write_op(xqspi, ZYNQ_QSPI_FIFO_DEPTH, true);
		ret = zynq_qspi_poll_irq(xqspi);
		if (ret)
			return ret;
	}

	zynq_qspi_chipselect(mem->spi, false);

	return 0;
}

static const struct spi_controller_mem_ops zynq_qspi_mem_ops = {
	.exec_op = zynq_qspi_exec_mem_op,
};

static int zynq_qspi_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct spi_controller *ctlr;
	struct zynq_qspi *xqspi;
	struct resource *iores;
	u32 num_cs;
	int ret;

	xqspi = xzalloc(sizeof(*xqspi));

	ctlr = &xqspi->ctlr;
	xqspi->dev = dev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	xqspi->regs = IOMEM(iores->start);

	xqspi->pclk = clk_get(dev, "pclk");
	if (IS_ERR(xqspi->pclk)) {
		dev_err(dev, "pclk clock not found.\n");
		return PTR_ERR(xqspi->pclk);
	}

	xqspi->refclk = clk_get(dev, "ref_clk");
	if (IS_ERR(xqspi->refclk)) {
		dev_err(dev, "ref_clk clock not found.\n");
		return PTR_ERR(xqspi->refclk);
	}

	ret = clk_enable(xqspi->pclk);
	if (ret) {
		dev_err(dev, "Unable to enable APB clock.\n");
		return ret;
	}

	ret = clk_enable(xqspi->refclk);
	if (ret) {
		dev_err(dev, "Unable to enable device clock.\n");
		return ret;
	}

	/* QSPI controller initializations */
	zynq_qspi_init_hw(xqspi);

	if (of_property_read_u32(np, "num-cs", &num_cs))
		ctlr->num_chipselect = 1;
	else
		ctlr->num_chipselect = num_cs;

	ctlr->dev = dev;
	ctlr->mem_ops = &zynq_qspi_mem_ops;
	ctlr->setup = zynq_qspi_setup_op;

	spi_controller_set_devdata(ctlr, xqspi);

	return spi_register_controller(ctlr);
}

static const struct of_device_id zynq_qspi_of_match[] = {
	{ .compatible = "xlnx,zynq-qspi-1.0", },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, zynq_qspi_of_match);

static struct driver zynq_qspi_driver = {
	.name  = "zynq-qspi",
	.probe = zynq_qspi_probe,
	.of_compatible = DRV_OF_COMPAT(zynq_qspi_of_match),
};
device_platform_driver(zynq_qspi_driver);

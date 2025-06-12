// SPDX-License-Identifier: GPL-2.0-only
#include <mach/zynq/zynq-pcap.h>
#include <linux/iopoll.h>

struct zynq_private {
	void __iomem *base_addr;
};

int zynq_init(struct fpgamgr *mgr, struct device *dev)
{
	struct resource *iores;
	struct zynq_private *priv;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv = xzalloc(sizeof(struct zynq_private));

	if (!priv)
		return -ENOMEM;

	mgr->private = priv;
	mgr->features = ZYNQ_PM_FEATURE_SIZE_NOT_NEEDED;

	priv->base_addr = IOMEM(iores->start);

	/* Unlock DevC in case BootROM did not do it */
	writel(DEVC_UNLOCK_CODE, priv->base_addr + UNLOCK_OFFSET);

	return 0;
}

int zynq_programmed_get(struct fpgamgr *mgr)
{
	struct zynq_private *priv = (struct zynq_private *) mgr->private;

	mgr->programmed = (readl(priv->base_addr + INT_STS_OFFSET)
				& INT_STS_DONE_INT_MASK) > 0;

	return 0;
}

int zynq_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags)
{
	int ret;
	unsigned long reg;
	struct zynq_private *priv = (struct zynq_private *) mgr->private;

	writel(0x0000DF0D, ZYNQ_SLCR_UNLOCK);
	writel(0x0000000f, ZYNQ_SLCR_BASE + 0x240); // assert FPGA resets

	writel(0x00000000, ZYNQ_SLCR_BASE + 0x900); // disable levelshifter
	writel(0x0000000a, ZYNQ_SLCR_BASE + 0x900); // enable levelshifter PS-PL

	/*
	 * The Programming Seqenze, see ug585 (v.12.2) Juny 1, 2018 Chapter
	 * 6.4.2 on page 211 Configure the PL via PCAP Bridge Example for
	 * detailed information to this Sequenze
	 */

	/* Enable the PCAP bridge and select PCAP for reconfiguration */
	reg = readl(priv->base_addr + CTRL_OFFSET);
	reg |= (CTRL_PCAP_PR_MASK | CTRL_PCAP_MODE_MASK);
	writel(reg, priv->base_addr + CTRL_OFFSET);

	/* Clear the Interrupts */
	writel(0xffffffff, priv->base_addr + INT_STS_OFFSET);

	/* Initialize the PL */
	reg = readl(priv->base_addr + CTRL_OFFSET);
	reg |= CTRL_PCFG_PROG_B_MASK;
	writel(reg, priv->base_addr + CTRL_OFFSET);

	reg = readl(priv->base_addr + CTRL_OFFSET);
	reg &= ~CTRL_PCFG_PROG_B_MASK;
	writel(reg, priv->base_addr + CTRL_OFFSET);

	ret = readl_poll_timeout(priv->base_addr + STATUS_OFFSET, reg,
			!(reg & STATUS_PCFG_INIT_MASK), 100 * USEC_PER_MSEC);
	if (ret < 0) {
		dev_err(&mgr->dev, "Timeout 1");
		return ret;
	}

	reg = readl(priv->base_addr + CTRL_OFFSET);
	reg |= CTRL_PCFG_PROG_B_MASK;
	writel(reg, priv->base_addr + CTRL_OFFSET);

	/* Clear the Interrupts */
	writel(0xffffffff, priv->base_addr + INT_STS_OFFSET);

	/* Ensure that the PL is ready for programming */
	ret = readl_poll_timeout(priv->base_addr + STATUS_OFFSET, reg,
			(reg & STATUS_PCFG_INIT_MASK), 100 * USEC_PER_MSEC);
	if (ret < 0) {
		dev_err(&mgr->dev, "Timeout 2");
		return ret;
	}

	/* Check that there is room in the Command Queue */
	ret = readl_poll_timeout(priv->base_addr + STATUS_OFFSET, reg,
			!(reg & STATUS_DMA_CMD_Q_F_MASK), 100 * USEC_PER_MSEC);
	if (ret < 0) {
		dev_err(&mgr->dev, "Timeout 3");
		return ret;
	}

	/* Disable the PCAP loopback */
	reg = readl(priv->base_addr + MCTRL_OFFSET);
	reg &= ~MCTRL_INT_PCAP_LPBK_MASK;
	writel(reg, priv->base_addr + MCTRL_OFFSET);

	/* Program the PCAP_2x clock divider */
	reg = readl(priv->base_addr + CTRL_OFFSET);
	reg &= ~CTRL_PCAP_RATE_EN_MASK;
	writel(reg, priv->base_addr + CTRL_OFFSET);

	/* Source Address: Location of bitstream */
	writel(addr, priv->base_addr + DMA_SRC_ADDR_OFFSET);

	/* Destination Address: 0xFFFF_FFFF */
	writel(0xffffffff, priv->base_addr + DMA_DST_ADDR_OFFSET);

	/* Source Length: Total number of 32-bit words in the bitstream */
	writel((buf_size >> 2), priv->base_addr + DMA_SRC_LEN_OFFSET);

	/* Destination Length: Total number of 32-bit words in the bitstream */
	writel((buf_size >> 2), priv->base_addr + DMA_DEST_LEN_OFFSET);

	/* Wait for the DMA transfer to be done */
	ret = readl_poll_timeout(priv->base_addr + INT_STS_OFFSET, reg,
			(reg & INT_STS_D_P_DONE_MASK), 100 * USEC_PER_MSEC);
	if (ret < 0) {
		dev_err(&mgr->dev, "Timeout 4: reg: 0x%lx\n", reg);
		return ret;
	}

	/* Check for errors */
	if (reg & INT_STS_ERROR_FLAGS_MASK) {
		dev_err(&mgr->dev, "interrupt status register (0x%04lx)\n",
			reg);
		return -EIO;
	}

	/* Wait for the DMA transfer to be done */
	ret = readl_poll_timeout(priv->base_addr + INT_STS_OFFSET, reg,
			(reg & INT_STS_DONE_INT_MASK), 100 * USEC_PER_MSEC);
	if (ret < 0) {
		dev_err(&mgr->dev, "Timeout 5");
		return ret;
	}

	dev_info(&mgr->dev, "FPGA config done\n");

	writel(0x0000000f, ZYNQ_SLCR_BASE + 0x900); // enable all levelshifter
	writel(0x00000000, ZYNQ_SLCR_BASE + 0x240); // deassert FPGA resets

	writel(0x0000767B, ZYNQ_SLCR_LOCK);

	return 0;
}

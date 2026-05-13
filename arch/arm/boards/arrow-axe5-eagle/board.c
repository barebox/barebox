// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <io.h>
#include <bbu.h>
#include <mach/socfpga/soc64-regs.h>

static void axe5_ethernet_phy_reset(void)
{
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x224);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x228);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x23c);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x234);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x248);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x24c);

	writel(0x410, 0x10c03304);
	writel(0x410, 0x10c03300);
	/*
	 * reset the phy via GPIO10. We currently haven't got enough space
	 * to enable the gpio driver in barebox.
	 */
	writel(0x000, 0x10c03300);
	/* FIXME:  can this be decreased? */
	mdelay(1000);
	writel(0x410, 0x10c03300);
}

static int axe5_probe(struct device *dev)
{
	axe5_ethernet_phy_reset();

	return 0;
}

static const struct of_device_id axe5_of_match[] = {
	{ .compatible = "arrow,axe5-eagle" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(axe5_of_match);

static struct driver axe5_board_driver = {
	.name = "board-arrow-axe5-eagle",
	.probe = axe5_probe,
	.of_compatible = axe5_of_match,
};
device_platform_driver(axe5_board_driver);

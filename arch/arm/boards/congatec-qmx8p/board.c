// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2023 Juergen Borleis, Pengutronix
// SPDX-FileCopyrightText: 2023 Johannes Zink, Pengutronix

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <mach/imx/bbu.h>
#include <mach/imx/generic.h>
#include <mach/imx/iomux-mx8mp.h>
#include <gpio.h>
#include <envfs.h>

/* Phy regulator handling in Linux is broken for the MX8 EQOs, as the
 * 'phy-regulators' properties are not handed down properly, so this is
 * currently not set in the kernel DT.
 * As a workaround, enable the regulator manually via GPIO. */
#define EQOS_PWR_PIN IMX_GPIO_NR(1, 5) /* ENET_PWREN# */
static void setup_ethernet_phy(void)
{
	u32 val;

	of_device_ensure_probed_by_alias("gpio0");

	if (gpio_direction_output(EQOS_PWR_PIN, 0)) {
		pr_err("eqos phy power: failed to request pin\n");
		return;
	}

	/* the phy needs roughly 200ms delay after power-on */
	mdelay(200);

	/* Enable RGMII TX clk output */
	val = readl(MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);
	val |= MX8MP_IOMUXC_GPR1_ENET1_RGMII_EN;
	writel(val, MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);
}

static int congatec_qmx8p_probe(struct device *dev)
{
	setup_ethernet_phy();

	imx8m_bbu_internal_flexspi_nor_register_handler("QSPI",
		"/dev/m25p0.boot", BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}

static const struct of_device_id congatec_qmx8p_of_match[] = {
	{ .compatible = "congatec,qmx8p" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(congatec_qmx8p_of_match);

static struct driver congatec_qmx8p_som_driver = {
	.name = "som-congatec-qmx8p",
	.probe = congatec_qmx8p_probe,
	.of_compatible = congatec_qmx8p_of_match,
};
coredevice_platform_driver(congatec_qmx8p_som_driver);

// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix
// SPDX-FileCopyrightText: Leica Geosystems AG

#include <debug_ll.h>
#include <pbl.h>

#include <mach/imx/debug_ll.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx-gpio.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/iomux-mx8mp.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/iomux-v3.h>

#include <boards/hgs/common.h>

struct hgs_hw_early_cfg {;
	void __iomem *uart_base;
	void __iomem *iomux_base;

	unsigned int pp4_gpio_num;
	void __iomem *pp4_gpio_base;

	iomux_v3_cfg_t *mux_cfg_rel;
	unsigned int mux_cfg_rel_num;
	iomux_v3_cfg_t *mux_cfg_dev;
	unsigned int mux_cfg_dev_num;
};

#define GS05_UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)
#define GS05_PP4_PAD_CTRL	MUX_PAD_CTRL(MX8MM_PAD_CTL_PE | MX8MM_PAD_CTL_DSE6)

/* Mux console pads explicit to GPIO */
static iomux_v3_cfg_t hgs_gs05_iomuxc_release[] = {
	IMX8MM_PAD_UART3_TXD_GPIO5_IO27 | GS05_UART_PAD_CTRL,
	IMX8MM_PAD_UART3_RXD_GPIO5_IO26 | GS05_UART_PAD_CTRL,
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10 | GS05_PP4_PAD_CTRL,
};

static iomux_v3_cfg_t hgs_gs05_iomuxc_development[] = {
	IMX8MM_PAD_UART3_TXD_UART3_TX | GS05_UART_PAD_CTRL,
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10 | GS05_PP4_PAD_CTRL,
};

static struct hgs_hw_early_cfg hgs_hw_early_config[] = {
	[HGS_HW_GS05] = {
		.uart_base = IOMEM(MX8M_UART3_BASE_ADDR),
		.iomux_base = IOMEM(MX8MM_IOMUXC_BASE_ADDR),
		.pp4_gpio_num = 10,
		.pp4_gpio_base = IOMEM(MX8MM_GPIO1_BASE_ADDR),
		.mux_cfg_rel = hgs_gs05_iomuxc_release,
		.mux_cfg_rel_num = ARRAY_SIZE(hgs_gs05_iomuxc_release),
		.mux_cfg_dev = hgs_gs05_iomuxc_development,
		.mux_cfg_dev_num = ARRAY_SIZE(hgs_gs05_iomuxc_development),
	},
};

static void hgs_early_setup_iomuxc(struct hgs_hw_early_cfg *cfg)
{
	unsigned int mux_cfg_num, i;
	iomux_v3_cfg_t *mux_cfg;

	if (!cfg)
		return;

	if (hgs_get_built_type() == HGS_DEV_BUILD) {
		mux_cfg_num = cfg->mux_cfg_dev_num;
		mux_cfg = cfg->mux_cfg_dev;
	} else {
		mux_cfg_num = cfg->mux_cfg_rel_num;
		mux_cfg = cfg->mux_cfg_rel;
	}

	for (i = 0; i < mux_cfg_num; i++)
		imx8m_setup_pad(cfg->iomux_base, mux_cfg[i]);
}

void hgs_early_hw_init(const enum hgs_hw hw)
{
	struct hgs_hw_early_cfg *cfg = &hgs_hw_early_config[hw];
	void __iomem *uart = cfg->uart_base;

	hgs_early_setup_iomuxc(cfg);

	/* Set PP4 CPU_RDY signal to 0 till barebox is fully booted */
	imx8m_gpio_direction_output(cfg->pp4_gpio_base, cfg->pp4_gpio_num, 0);

	/*
	 * UART enable could be skipped in release use-case but the TF-A
	 * requires a running UART. Therefore we keep it on but mux the pads to
	 * GPIO functions.
	 */
	imx8m_early_setup_uart_clock();
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

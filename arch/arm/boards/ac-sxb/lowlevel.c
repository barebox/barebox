// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2017 Atlas Copco Industrial Technique
 */

#include <debug_ll.h>
#include <io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx7-ccm-regs.h>
#include <mach/iomux-mx7.h>
#include <mach/debug_ll.h>
#include <asm/cache.h>
#include <mach/esdctl.h>
#include <mach/xload.h>
#include <mach/imx7-ddr-regs.h>

struct reginit {
	u32 address;
	u32 value;
};

static const struct reginit imx7d_ixb_dcd[] = {
	{0x30340004, 0x4f400005},
	{0x30391000, 0x00000002},
	{MX7_DDRC_MSTR, 0x03020004},
	{MX7_DDRC_RFSHTMG, 0x00200038},
	{MX7_DDRC_MP_PCTRL_0, 0x00000001},
	{MX7_DDRC_INIT0, 0x00350001},
	{MX7_DDRC_INIT2, 0x00001105},
	{MX7_DDRC_INIT3, 0x00c20006},
	{MX7_DDRC_INIT4, 0x00020000},
	{MX7_DDRC_INIT5, 0x00110006},
	{MX7_DDRC_RANKCTL, 0x0000033f},
	{MX7_DDRC_DRAMTMG0, 0x080e110b},
	{MX7_DDRC_DRAMTMG1, 0x00020211},
	{MX7_DDRC_DRAMTMG2, 0x02040705},
	{MX7_DDRC_DRAMTMG3, 0x00504000},
	{MX7_DDRC_DRAMTMG4, 0x05010307},
	{MX7_DDRC_DRAMTMG5, 0x02020404},
	{MX7_DDRC_DRAMTMG6, 0x02020003},
	{MX7_DDRC_DRAMTMG7, 0x00000202},
	{MX7_DDRC_DRAMTMG8, 0x00000202},
	{MX7_DDRC_ZQCTL0, 0x20600018},
	{MX7_DDRC_ZQCTL1, 0x00e00100},
	{MX7_DDRC_DFITMG0, 0x02098203},
	{MX7_DDRC_DFITMG1, 0x00060303},
	{MX7_DDRC_DFIUPD0, 0x80400003},
	{MX7_DDRC_DFIUPD1, 0x00100020},
	{MX7_DDRC_DFIUPD2, 0x80100004},
	{MX7_DDRC_ADDRMAP0, 0x00000015},
	{MX7_DDRC_ADDRMAP1, 0x00080808},
	{MX7_DDRC_ADDRMAP4, 0x00000f0f},
	{MX7_DDRC_ADDRMAP5, 0x07070707},
	{MX7_DDRC_ADDRMAP6, 0x0f0f0707},
	{MX7_DDRC_ODTCFG, 0x06000600},
	{MX7_DDRC_ODTMAP, 0x00000000},
	{0x30391000, 0x00000000},
	{MX7_DDR_PHY_PHY_CON0, 0x17421640},
	{MX7_DDR_PHY_PHY_CON1, 0x10210100},
	{MX7_DDR_PHY_PHY_CON2, 0x00010000},
	{MX7_DDR_PHY_PHY_CON4, 0x00050408},
	{MX7_DDR_PHY_MDLL_CON0, 0x1010007e},
	{MX7_DDR_PHY_RODT_CON0, 0x01010000},
	{MX7_DDR_PHY_DRVDS_CON0, 0x00000d6e},
	{MX7_DDR_PHY_OFFSET_WR_CON0, 0x06060606},
	{MX7_DDR_PHY_OFFSET_RD_CON0, 0x0a0a0a0a},
	{MX7_DDR_PHY_CMD_SDLL_CON0, 0x01000008},
	{MX7_DDR_PHY_CMD_SDLL_CON0, 0x00000008},
	{MX7_DDR_PHY_LP_CON0, 0x0000000f},
	{MX7_DDR_PHY_ZQ_CON0, 0x0e487304},
	{MX7_DDR_PHY_ZQ_CON0, 0x0e4c7304},
	{MX7_DDR_PHY_ZQ_CON0, 0x0e4c7306},
	{MX7_DDR_PHY_ZQ_CON0, 0x0e487304},
	{0x30384130, 0x00000000},
	{0x30340020, 0x00000178},
	{0x30384130, 0x00000002},
};

static inline void write_regs(const struct reginit *initvals, int count)
{
	int i;

	for (i = 0; i < count; i++)
		writel(initvals[i].value, initvals[i].address);
}

extern char __dtb_z_ac_sxb_start[];

static inline void setup_uart(void)
{
	imx7_early_setup_uart_clock();

	imx7_setup_pad(MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX);

	imx7_uart_setup_ll();

	putc_ll('>');
}

static noinline void imx7d_sxb_sram_setup(void)
{
	int ret;

	relocate_to_current_adr();
	setup_c();

	pr_debug("configuring ddr...\n");
	write_regs(imx7d_ixb_dcd, ARRAY_SIZE(imx7d_ixb_dcd));

	ret = imx7_esdhc_start_image(2);

	BUG_ON(ret);
}

ENTRY_FUNCTION(start_ac_sxb, r0, r1, r2)
{
	imx7_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	if (get_pc() < 0x80000000)
		imx7d_sxb_sram_setup();

	imx7d_barebox_entry(__dtb_z_ac_sxb_start + get_runtime_offset());
}

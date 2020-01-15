// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx8-ccm-regs.h>
#include <mach/iomux-mx8.h>
#include <mach/imx8-ddrc.h>
#include <mach/xload.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <mach/atf.h>
#include <mach/esdctl.h>

#include "ddr.h"

extern char __dtb_imx8mq_evk_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *iomux = IOMEM(MX8MQ_IOMUXC_BASE_ADDR);
	void __iomem *ccm   = IOMEM(MX8MQ_CCM_BASE_ADDR);

	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_CLR(CCM_CCGR_UART1));
	writel(CCM_TARGET_ROOTn_ENABLE | UART1_CLK_ROOT__25M_REF_CLK,
	       ccm + CCM_TARGET_ROOTn(UART1_CLK_ROOT));
	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_SET(CCM_CCGR_UART1));

	imx_setup_pad(iomux, IMX8MQ_PAD_UART1_TXD__UART1_TX | UART_PAD_CTRL);

	imx8_uart_setup_ll();

	putc_ll('>');
}

/*
 * Power-on execution flow of start_nxp_imx8mq_evk() might not be
 * obvious for a very first read, so here's, hopefully helpful,
 * summary:
 *
 * 1. MaskROM uploads PBL into OCRAM and that's where this function is
 *    executed for the first time. At entry the exception level is EL3.
 *
 * 2. DDR is initialized and the image is loaded from storage into DRAM. The PBL
 *    part is copied from OCRAM to the TF-A return address in DRAM.
 *
 * 3. TF-A is executed and exits into the PBL code in DRAM. TF-A has taken us
 *    from EL3 to EL2.
 *
 * 4. Standard barebox boot flow continues
 */
static __noreturn noinline void nxp_imx8mq_evk_start(void)
{
	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() == 3) {
		enum bootsource src = BOOTSOURCE_UNKNOWN;
		int instance = BOOTSOURCE_INSTANCE_UNKNOWN;
		int ret = -ENOTSUPP;
		size_t bl31_size;
		const u8 *bl31;

		ddr_init();

		/*
		 * On completion the TF-A will jump to MX8MQ_ATF_BL33_BASE_ADDR
		 * in EL2. Copy the image there, but replace the PBL part of
		 * that image with ourselves. On a high assurance boot only the
		 * currently running code is validated and contains the checksum
		 * for the piggy data, so we need to ensure that we are running
		 * the same code in DRAM.
		 */
		imx8_get_boot_source(&src, &instance);
		if (src == BOOTSOURCE_MMC)
			ret = imx8_esdhc_load_image(instance, false);
		BUG_ON(ret);

		memcpy((void *)MX8MQ_ATF_BL33_BASE_ADDR,
		       __image_start, barebox_pbl_size);

		get_builtin_firmware(imx8mq_bl31_bin, &bl31, &bl31_size);
		imx8mq_atf_load_bl31(bl31, bl31_size);
		/* not reached */
	}

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mq_barebox_entry(__dtb_imx8mq_evk_start);
}

ENTRY_FUNCTION(start_nxp_imx8mq_evk, r0, r1, r2)
{
	imx8mq_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	nxp_imx8mq_evk_start();
}

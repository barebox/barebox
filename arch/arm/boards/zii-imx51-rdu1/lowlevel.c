#include <debug_ll.h>
#include <mach/clock-imx51_53.h>
#include <mach/iomux-mx51.h>
#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX51_IOMUXC_BASE_ADDR);
	void __iomem *ccmbase = IOMEM(MX51_CCM_BASE_ADDR);

	/*
	 * Restore CCM values that might be changed by the Mask ROM
	 * code.
	 *
	 * Source: RealView debug scripts provided by Freescale
	 */
	writel(MX5_CCM_CBCDR_RESET_VALUE,  ccmbase + MX5_CCM_CBCDR);
	writel(MX5_CCM_CSCMR1_RESET_VALUE, ccmbase + MX5_CCM_CSCMR1);
	writel(MX5_CCM_CSCDR1_RESET_VALUE, ccmbase + MX5_CCM_CSCDR1);

	imx_setup_pad(iomuxbase, MX51_PAD_UART1_TXD__UART1_TXD);

	imx51_uart_setup_ll();

	putc_ll('>');
}

enum zii_platform_imx51_type {
	ZII_PLATFORM_IMX51_RDU_REV_B = 0b0000,
	ZII_PLATFORM_IMX51_SCU2_ESB  = 0b0001,
	ZII_PLATFORM_IMX51_SCU_MEZZ  = 0b0010,
	ZII_PLATFORM_IMX51_NIU_REV   = 0b0011,
	ZII_PLATFORM_IMX51_SCU2_MEZZ = 0b0100,
	ZII_PLATFORM_IMX51_SCU3_ESB  = 0b0101,
	ZII_PLATFORM_IMX51_RDU_REV_C = 0b1101,
};

static unsigned int get_system_type(void)
{
#define GPIO_DR		0x000
#define GPIO_GDIR	0x004
#define SYSTEM_TYPE	GENMASK(6, 3)

	u32 gdir, dr;
	void __iomem *gpio4 = IOMEM(MX51_GPIO4_BASE_ADDR);
	void __iomem *iomuxbase = IOMEM(MX51_IOMUXC_BASE_ADDR);

	/*
	 * System type is encoded as a 4-bit number specified by the
	 * following pins (pulled up or down with resistors on the
	 * board).
	*/
	imx_setup_pad(iomuxbase, MX51_PAD_NANDF_D2__GPIO4_6);
	imx_setup_pad(iomuxbase, MX51_PAD_NANDF_D3__GPIO4_5);
	imx_setup_pad(iomuxbase, MX51_PAD_NANDF_D4__GPIO4_4);
	imx_setup_pad(iomuxbase, MX51_PAD_NANDF_D5__GPIO4_3);

	gdir = readl(gpio4 + GPIO_GDIR);
	gdir &= ~SYSTEM_TYPE;
	writel(gdir, gpio4 + GPIO_GDIR);

	dr = readl(gpio4 + GPIO_DR);

	return FIELD_GET(SYSTEM_TYPE, dr);
}

extern char __dtb_z_imx51_zii_rdu1_start[];
extern char __dtb_z_imx51_zii_scu2_mezz_start[];
extern char __dtb_z_imx51_zii_scu3_esb_start[];

ENTRY_FUNCTION(start_imx51_zii_rdu1, r0, r1, r2)
{
	void *fdt;
	const unsigned int system_type = get_system_type();

	imx5_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	switch (system_type) {
	default:
		if (IS_ENABLED(CONFIG_DEBUG_LL)) {
			relocate_to_current_adr();
			setup_c();
			puts_ll("\n*********************************\n");
			puts_ll("* Unknown system type: ");
			puthex_ll(system_type);
			puts_ll("\n* Assuming RDU1\n");
			puts_ll("*********************************\n");
		}
		/* FALLTHROUGH */
	case ZII_PLATFORM_IMX51_RDU_REV_B:
	case ZII_PLATFORM_IMX51_RDU_REV_C:
		fdt = __dtb_z_imx51_zii_rdu1_start;
		break;
	case ZII_PLATFORM_IMX51_SCU2_MEZZ:
		fdt = __dtb_z_imx51_zii_scu2_mezz_start;
		break;
	case ZII_PLATFORM_IMX51_SCU3_ESB:
		fdt = __dtb_z_imx51_zii_scu3_esb_start;
		break;
	}

	imx51_barebox_entry(fdt + get_runtime_offset());
}

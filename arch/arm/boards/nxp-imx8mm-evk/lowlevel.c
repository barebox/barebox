// SPDX-License-Identifier: GPL-2.0

#include <io.h>
#include <common.h>
#include <debug_ll.h>
#include <firmware.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <i2c/i2c-early.h>
#include <linux/sizes.h>
#include <mach/atf.h>
#include <mach/xload.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx8mm-regs.h>
#include <mach/iomux-mx8mm.h>
#include <mach/imx8m-ccm-regs.h>
#include <mfd/bd71837.h>
#include <soc/imx8m/ddr.h>
#include <soc/fsl/fsl_udc.h>

extern char __dtb_imx8mm_evk_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mm_setup_pad(IMX8MM_PAD_UART2_TXD_UART2_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static void pmic_reg_write(void *i2c, int reg, uint8_t val)
{
	int ret;
	u8 buf[32];
	struct i2c_msg msgs[] = {
		{
			.addr = 0x4b,
			.buf = buf,
		},
	};

	buf[0] = reg;
	buf[1] = val;

	msgs[0].len = 2;

	ret = i2c_fsl_xfer(i2c, msgs, ARRAY_SIZE(msgs));
	if (ret != 1)
		pr_err("Failed to write to pmic\n");
}

static int power_init_board(void)
{
	void *i2c;

	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SCL_I2C1_SCL);
	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SDA_I2C1_SDA);

	imx8mm_early_clock_init();
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MQ_I2C1_BASE_ADDR));

	/* decrease RESET key long push time from the default 10s to 10ms */
	pmic_reg_write(i2c, BD718XX_PWRONCONFIG1, 0x0);

	/* unlock the PMIC regs */
	pmic_reg_write(i2c, BD718XX_REGLOCK, 0x1);

	/* increase VDD_SOC to typical value 0.85v before first DRAM access */
	pmic_reg_write(i2c, BD718XX_BUCK1_VOLT_RUN, 0x0f);

	/* increase VDD_DRAM to 0.975v for 3Ghz DDR */
	pmic_reg_write(i2c, BD718XX_1ST_NODVS_BUCK_VOLT, 0x83);

	/* lock the PMIC regs */
	pmic_reg_write(i2c, BD718XX_REGLOCK, 0x11);

	return 0;
}

extern struct dram_timing_info imx8mm_evk_dram_timing;

static void start_atf(void)
{
	size_t bl31_size;
	const u8 *bl31;
	enum bootsource src;
	int instance;

	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	power_init_board();
	imx8mm_ddr_init(&imx8mm_evk_dram_timing);

	imx8mm_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8m_esdhc_load_image(instance, false);
		break;
	case BOOTSOURCE_SERIAL:
		imx8mm_barebox_load_usb((void *)MX8M_ATF_BL33_BASE_ADDR);
	break;
	default:
		printf("Unhandled bootsource BOOTSOURCE_%d\n", src);
		hang();
	}

	/*
	 * On completion the TF-A will jump to MX8M_ATF_BL33_BASE_ADDR
	 * in EL2. Copy the image there, but replace the PBL part of
	 * that image with ourselves. On a high assurance boot only the
	 * currently running code is validated and contains the checksum
	 * for the piggy data, so we need to ensure that we are running
	 * the same code in DRAM.
	 */
	memcpy((void *)MX8M_ATF_BL33_BASE_ADDR,
	       __image_start, barebox_pbl_size);

	get_builtin_firmware(imx8mm_bl31_bin, &bl31, &bl31_size);

	imx8mm_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}

/*
 * Power-on execution flow of start_nxp_imx8mm_evk() might not be
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
static __noreturn noinline void nxp_imx8mm_evk_start(void)
{
	setup_uart();

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mm_barebox_entry(__dtb_imx8mm_evk_start);
}

ENTRY_FUNCTION(start_nxp_imx8mm_evk, r0, r1, r2)
{
	imx8mm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	nxp_imx8mm_evk_start();
}

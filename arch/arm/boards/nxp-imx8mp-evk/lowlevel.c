// SPDX-License-Identifier: GPL-2.0

#include <io.h>
#include <common.h>
#include <debug_ll.h>
#include <firmware.h>
#include <image-metadata.h>
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
#include <mach/imx8mp-regs.h>
#include <mach/iomux-mx8mp.h>
#include <mach/imx8m-ccm-regs.h>
#include <mfd/pca9450.h>
#include <soc/imx8m/ddr.h>
#include <soc/fsl/fsl_udc.h>

extern char __dtb_imx8mp_evk_start[];

#define UART_PAD_CTRL   MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_FSEL)

#define I2C_PAD_CTRL	MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_HYS | \
				     MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_PE)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mp_setup_pad(MX8MP_PAD_UART2_TXD__UART2_DCE_TX | UART_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_UART2_RXD__UART2_DCE_RX | UART_PAD_CTRL);
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
			.addr = 0x25,
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

	imx8mp_setup_pad(MX8MP_PAD_I2C1_SCL__I2C1_SCL | I2C_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_I2C1_SDA__I2C1_SDA | I2C_PAD_CTRL);

	imx8mm_early_clock_init();
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MP_I2C1_BASE_ADDR));

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(i2c, PCA9450_BUCK123_DVS, 0x29);

	/*
	 * increase VDD_SOC to typical value 0.95V before first
	 * DRAM access, set DVS1 to 0.85v for suspend.
	 * Enable DVS control through PMIC_STBY_REQ and
	 * set B1_ENMODE=1 (ON by PMIC_ON_REQ=H)
	 */
	pmic_reg_write(i2c, PCA9450_BUCK1OUT_DVS0, 0x1C);
	pmic_reg_write(i2c, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(i2c, PCA9450_BUCK1CTRL, 0x59);

	/* set WDOG_B_CFG to cold reset */
	pmic_reg_write(i2c, PCA9450_RESET_CTRL, 0xA1);

	return 0;
}

extern struct dram_timing_info imx8mp_evk_dram_timing;

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

	imx8mp_ddr_init(&imx8mp_evk_dram_timing);

	imx8mp_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mp_esdhc_load_image(instance, false);
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

	get_builtin_firmware(imx8mp_bl31_bin, &bl31, &bl31_size);

	imx8mp_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}

/*
 * Power-on execution flow of start_nxp_imx8mp_evk() might not be
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
static __noreturn noinline void nxp_imx8mp_evk_start(void)
{
	setup_uart();

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mp_barebox_entry(__dtb_imx8mp_evk_start);
}

ENTRY_FUNCTION(start_nxp_imx8mp_evk, r0, r1, r2)
{
	imx8mp_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	IMD_USED_OF(imx8mp_evk);

	nxp_imx8mp_evk_start();
}

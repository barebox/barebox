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
#include <mach/imx8mn-regs.h>
#include <mach/iomux-mx8mn.h>
#include <mach/imx8m-ccm-regs.h>
#include <mfd/pca9450.h>
#include <mfd/bd71837.h>
#include <soc/imx8m/ddr.h>

extern char __dtb_z_imx8mn_evk_start[];

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mn_setup_pad(IMX8MN_PAD_UART2_TXD__UART2_DCE_TX);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static void pmic_reg_write(void *i2c, int addr, int reg, uint8_t val)
{
	int ret;
	u8 buf[32];
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.buf = buf,
		},
	};

	buf[0] = reg;
	buf[1] = val;

	msgs[0].len = 2;

	ret = i2c_fsl_xfer(i2c, msgs, ARRAY_SIZE(msgs));
	if (ret != 1)
		pr_err("Failed to write to pmic@%x: %d\n", addr, ret);
}

static int power_init_board_pca9450(void *i2c, int addr)
{
	u8 buf[1];
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.buf = buf,
			.flags = I2C_M_RD,
			.len = 1,
		},
	};

	if (i2c_fsl_xfer(i2c, msgs, 1) != 1)
		return -ENODEV;

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(i2c, addr, PCA9450_BUCK123_DVS, 0x29);

	/*
	 * increase VDD_SOC to typical value 0.95V before first
	 * DRAM access, set DVS1 to 0.85v for suspend.
	 * Enable DVS control through PMIC_STBY_REQ and
	 * set B1_ENMODE=1 (ON by PMIC_ON_REQ=H)
	 */
	pmic_reg_write(i2c, addr, PCA9450_BUCK1OUT_DVS0, 0x1C);

	/* Set DVS1 to 0.85v for suspend */
	/* Enable DVS control through PMIC_STBY_REQ and set B1_ENMODE=1 (ON by PMIC_ON_REQ=H) */
	pmic_reg_write(i2c, addr, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(i2c, addr, PCA9450_BUCK1CTRL, 0x59);

	/* set VDD_SNVS_0V8 from default 0.85V */
	pmic_reg_write(i2c, addr, PCA9450_LDO2CTRL, 0xC0);

	/* enable LDO4 to 1.2v */
	pmic_reg_write(i2c, addr, PCA9450_LDO4CTRL, 0x44);

	/* set WDOG_B_CFG to cold reset */
	pmic_reg_write(i2c, addr, PCA9450_RESET_CTRL, 0xA1);

	return 0;
}

static int power_init_board_bd71837(void *i2c, int addr)
{
	/* decrease RESET key long push time from the default 10s to 10ms */
	pmic_reg_write(i2c, addr, BD718XX_PWRONCONFIG1, 0x0);

	/* unlock the PMIC regs */
	pmic_reg_write(i2c, addr, BD718XX_REGLOCK, 0x1);

	/* Set VDD_ARM to typical value 0.85v for 1.2Ghz */
	pmic_reg_write(i2c, addr, BD718XX_BUCK2_VOLT_RUN, 0xf);

	/* Set VDD_SOC/VDD_DRAM to typical value 0.85v for nominal mode */
	pmic_reg_write(i2c, addr, BD718XX_BUCK1_VOLT_RUN, 0xf);

	/* Set VDD_SOC 0.85v for suspend */
	pmic_reg_write(i2c, addr, BD718XX_BUCK1_VOLT_SUSP, 0xf);

	/* increase NVCC_DRAM_1V2 to 1.2v for DDR4
	 * */
	pmic_reg_write(i2c, addr, BD718XX_4TH_NODVS_BUCK_CTRL, 0x28);

	/* lock the PMIC regs */
	pmic_reg_write(i2c, addr, BD718XX_REGLOCK, 0x11);

	return 0;
}

extern struct dram_timing_info imx8mn_evk_ddr4_timing, imx8mn_evk_lpddr4_timing;

static void start_atf(void)
{
	struct dram_timing_info *dram_timing = &imx8mn_evk_lpddr4_timing;
	size_t bl31_size;
	const u8 *bl31;
	enum bootsource src;
	void *i2c;
	int instance;
	int ret;

	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	imx8mn_setup_pad(IMX8MN_PAD_I2C1_SCL__I2C1_SCL);
	imx8mn_setup_pad(IMX8MN_PAD_I2C1_SDA__I2C1_SDA);

	imx8mn_early_clock_init();
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MN_I2C1_BASE_ADDR));

	ret = power_init_board_pca9450(i2c, 0x25);
	if (ret) {
		power_init_board_bd71837(i2c, 0x4b);
		dram_timing = &imx8mn_evk_ddr4_timing;
	}

	imx8mn_ddr_init(dram_timing);

	imx8mn_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mn_esdhc_load_image(instance, false);
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

	get_builtin_firmware(imx8mn_bl31_bin, &bl31, &bl31_size);

	imx8mn_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}

/*
 * Power-on execution flow of start_nxp_imx8mn_evk() might not be
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
static __noreturn noinline void nxp_imx8mn_evk_start(void)
{
	setup_uart();

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mn_barebox_entry(__dtb_z_imx8mn_evk_start);
}

ENTRY_FUNCTION(start_nxp_imx8mn_evk, r0, r1, r2)
{
	imx8mn_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	nxp_imx8mn_evk_start();
}

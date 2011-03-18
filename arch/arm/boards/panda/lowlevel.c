/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/io.h>
#include <mach/omap4-mux.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-clock.h>
#include <mach/syslib.h>
#include <asm/barebox-arm.h>

void set_muxconf_regs(void);

static const struct ddr_regs ddr_regs_400_mhz_2cs = {
	/* tRRD changed from 10ns to 12.5ns because of the tFAW requirement*/
	.tim1		= 0x10eb0662,
	.tim2		= 0x20370dd2,
	.tim3		= 0x00b1c33f,
	.phy_ctrl_1	= 0x849FF408,
	.ref_ctrl	= 0x00000618,
	.config_init	= 0x80000eb9,
	.config_final	= 0x80001ab9,
	.zq_config	= 0xD00b3215,
	.mr1		= 0x83,
	.mr2		= 0x4
};

#define I2C_SLAVE 0x12

static int noinline scale_vcores(void)
{
	unsigned int rev = omap4_revision();

	/* For VC bypass only VCOREx_CGF_FORCE  is necessary and
	 * VCOREx_CFG_VOLTAGE  changes can be discarded
	 */
	writel(0, OMAP44XX_PRM_VC_CFG_I2C_MODE);
	writel(0x6026, OMAP44XX_PRM_VC_CFG_I2C_CLK);

	/* set VCORE1 force VSEL */
	omap4_power_i2c_send((0x3A55 << 8) | I2C_SLAVE);

	/* FIXME: set VCORE2 force VSEL, Check the reset value */
	omap4_power_i2c_send((0x295B << 8) | I2C_SLAVE);

	/* set VCORE3 force VSEL */
	switch (rev) {
	case OMAP4430_ES2_0:
		omap4_power_i2c_send((0x2961 << 8) | I2C_SLAVE);
		break;
	case OMAP4430_ES2_1:
		omap4_power_i2c_send((0x2A61 << 8) | I2C_SLAVE);
		break;
	}

	return 0;
}

static void noinline panda_init_lowlevel(void)
{
	struct dpll_param core = OMAP4_CORE_DPLL_PARAM_38M4_DDR400;
	struct dpll_param mpu = OMAP4_MPU_DPLL_PARAM_38M4_MPU600;
	struct dpll_param iva = OMAP4_IVA_DPLL_PARAM_38M4;
	struct dpll_param per = OMAP4_PER_DPLL_PARAM_38M4;
	struct dpll_param abe = OMAP4_ABE_DPLL_PARAM_38M4;
	struct dpll_param usb = OMAP4_USB_DPLL_PARAM_38M4;

	writel(CM_SYS_CLKSEL_38M4, CM_SYS_CLKSEL);

	/* Configure all DPLL's at 100% OPP */
	omap4_configure_mpu_dpll(&mpu);
	omap4_configure_iva_dpll(&iva);
	omap4_configure_per_dpll(&per);
	omap4_configure_abe_dpll(&abe);
	omap4_configure_usb_dpll(&usb);

	/* Enable all clocks */
	omap4_enable_all_clocks();

	set_muxconf_regs();

	omap4_ddr_init(&ddr_regs_400_mhz_2cs, &core);

	/* Set VCORE1 = 1.3 V, VCORE2 = VCORE3 = 1.21V */
	scale_vcores();

	board_init_lowlevel_return();
}

void board_init_lowlevel(void)
{
	u32 r;

	if (get_pc() > 0x80000000)
		return;

	r = 0x4030d000;
        __asm__ __volatile__("mov sp, %0" : : "r"(r));

	panda_init_lowlevel();
}


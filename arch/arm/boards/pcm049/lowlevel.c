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

/* Erstmal 200Mhz... */
static const struct ddr_regs ddr_regs_mt42L64M64_3_200_mhz = {
	.tim1		= 0x0aa8d4e3,
	.tim2		= 0x202e0b92,
	.tim3		= 0x009da2b3,
	.phy_ctrl_1	= 0x849FF404, /* mostly from elpida */
	.ref_ctrl	= 0x0000030c, /* from elpida 200MHz! */
	.config_init	= 0x80000eb1,
	.config_final	= 0x80000eb1,
	.zq_config	= 0x500b3215, /* mostly from elpida */
	.mr1		= 0x23,	      /* from elpida 200MHz! */
	.mr2		= 0x1	      /* from elpida 200MHz! */
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

static void noinline pcm049_init_lowlevel(void)
{
	struct dpll_param core = OMAP4_CORE_DPLL_PARAM_19M2_DDR200;
	struct dpll_param mpu = OMAP4_MPU_DPLL_PARAM_19M2_MPU1000;
	struct dpll_param iva = OMAP4_IVA_DPLL_PARAM_19M2;
	struct dpll_param per = OMAP4_PER_DPLL_PARAM_19M2;
	struct dpll_param abe = OMAP4_ABE_DPLL_PARAM_19M2;
	struct dpll_param usb = OMAP4_USB_DPLL_PARAM_19M2;

	set_muxconf_regs();

	omap4_ddr_init(&ddr_regs_mt42L64M64_3_200_mhz, &core);

	/* Set VCORE1 = 1.3 V, VCORE2 = VCORE3 = 1.21V */
	scale_vcores();

	writel(CM_SYS_CLKSEL_19M2, CM_SYS_CLKSEL);

	/* Configure all DPLL's at 100% OPP */
	omap4_configure_mpu_dpll(&mpu);
	omap4_configure_iva_dpll(&iva);
	omap4_configure_per_dpll(&per);
	omap4_configure_abe_dpll(&abe);
	omap4_configure_usb_dpll(&usb);

	/* Enable all clocks */
	omap4_enable_all_clocks();

	sr32(0x4A30a31C, 8, 1, 0x1);  /* enable software ioreq */
	sr32(0x4A30a31C, 1, 2, 0x0);  /* set for sys_clk (19.2MHz) */
	sr32(0x4A30a31C, 16, 4, 0x0); /* set divisor to 1 */
	sr32(0x4A30a110, 0, 1, 0x1);  /* set the clock source to active */
	sr32(0x4A30a110, 2, 2, 0x3);  /* enable clocks */

	board_init_lowlevel_return();
}

void board_init_lowlevel(void)
{
	u32 r;

	if (get_pc() > 0x80000000)
		return;

	r = 0x4030d000;
        __asm__ __volatile__("mov sp, %0" : : "r"(r));

	pcm049_init_lowlevel();
}


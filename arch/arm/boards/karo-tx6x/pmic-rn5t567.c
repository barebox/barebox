/*
 * Copyright (C) 2014 Lothar Waßmann <LW@KARO-electronics.de>
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
 */

#include <common.h>
#include <i2c/i2c.h>
#include "pmic.h"

#define RN5T567_NOETIMSET	0x11
#define RN5T567_LDORTC1_SLOT	0x2a
#define RN5T567_DC1CTL		0x2c
#define RN5T567_DC1CTL2		0x2d
#define RN5T567_DC2CTL		0x2e
#define RN5T567_DC2CTL2		0x2f
#define RN5T567_DC3CTL		0x30
#define RN5T567_DC3CTL2		0x31
#define RN5T567_DC1DAC		0x36 /* CORE */
#define RN5T567_DC2DAC		0x37 /* SOC */
#define RN5T567_DC3DAC		0x38 /* DDR */
#define RN5T567_DC1DAC_SLP	0x3b
#define RN5T567_DC2DAC_SLP	0x3c
#define RN5T567_DC3DAC_SLP	0x3d
#define RN5T567_LDOEN1		0x44
#define RN5T567_LDODIS		0x46
#define RN5T567_LDOEN2		0x48
#define RN5T567_LDO3DAC		0x4e /* IO */
#define RN5T567_LDORTC1DAC	0x56 /* VBACKUP */

#define NOETIMSET_DIS_OFF_NOE_TIM	(1 << 3)

#define VDD_RTC_VAL		mV_to_regval_rtc(3000)
#define VDD_HIGH_VAL		mV_to_regval3(3000)
#define VDD_HIGH_VAL_LP		mV_to_regval3(3000)
#define VDD_CORE_VAL		mV_to_regval(1350)		/* DCDC1 */
#define VDD_CORE_VAL_LP		mV_to_regval(900)
#define VDD_SOC_VAL		mV_to_regval(1350)		/* DCDC2 */
#define VDD_SOC_VAL_LP		mV_to_regval(900)
#define VDD_DDR_VAL		mV_to_regval(1350)		/* DCDC3 */
#define VDD_DDR_VAL_LP		mV_to_regval(1350)

/* calculate voltages in 10mV */
#define v2r(v,n,m)		DIV_ROUND(((((v) < (n)) ? (n) : (v)) - (n)), (m))
#define r2v(r,n,m)		(((r) * (m) + (n)) / 10)

/* DCDC1-3 */
#define mV_to_regval(mV)	v2r((mV) * 10, 6000, 125)
#define regval_to_mV(r)		r2v(r, 6000, 125)

/* LDO1-2 */
#define mV_to_regval2(mV)	v2r((mV) * 10, 9000, 250)
#define regval2_to_mV(r)	r2v(r, 9000, 250)

/* LDO3 */
#define mV_to_regval3(mV)	v2r((mV) * 10, 6000, 250)
#define regval3_to_mV(r)	r2v(r, 6000, 250)

/* LDORTC */
#define mV_to_regval_rtc(mV)	v2r((mV) * 10, 17000, 250)
#define regval_rtc_to_mV(r)	r2v(r, 17000, 250)

static struct rn5t567_regs {
	u8 addr;
	u8 val;
	u8 mask;
} rn5t567_regs[] = {
	{ RN5T567_NOETIMSET, NOETIMSET_DIS_OFF_NOE_TIM | 0x5, },
	{ RN5T567_DC1DAC, VDD_CORE_VAL, },
	{ RN5T567_DC2DAC, VDD_SOC_VAL, },
	{ RN5T567_DC3DAC, VDD_DDR_VAL, },
	{ RN5T567_DC1DAC_SLP, VDD_CORE_VAL_LP, },
	{ RN5T567_DC2DAC_SLP, VDD_SOC_VAL_LP, },
	{ RN5T567_DC3DAC_SLP, VDD_DDR_VAL_LP, },
	{ RN5T567_LDOEN1, 0x01f, ~0x1f, },
	{ RN5T567_LDOEN2, 0x10, ~0x30, },
	{ RN5T567_LDODIS, 0x00, },
	{ RN5T567_LDO3DAC, VDD_HIGH_VAL, },
	{ RN5T567_LDORTC1DAC, VDD_RTC_VAL, },
	{ RN5T567_LDORTC1_SLOT, 0x0f, ~0x3f, },
};

static int rn5t567_setup_regs(struct i2c_client *client, struct rn5t567_regs *r,
			size_t count)
{
	int ret;
	int i;

	for (i = 0; i < count; i++, r++) {
#ifdef DEBUG
		unsigned char value;

		ret = i2c_read_reg(client, r->addr, &value, 1);
		if ((value & ~r->mask) != r->val) {
			pr_debug("Changing PMIC reg %02x from %02x to %02x\n",
				r->addr, value, r->val);
		}
		if (ret != 1) {
			pr_debug("%s: failed to read PMIC register %02x: %d\n",
				__func__, r->addr, ret);
			return ret;
		}
#endif
		ret = i2c_write_reg(client, r->addr, &r->val, 1);
		if (ret != 1) {
			pr_err("%s: failed to write PMIC register %02x: %d\n",
				__func__, r->addr, ret);
			return ret;
		}
	}
	return 0;
}

int rn5t567_pmic_setup(struct i2c_client *client)
{
	int ret;
	unsigned char value;

	ret = i2c_read_reg(client, 0x11, &value, 1);
	if (ret != 1) {
		pr_err("i2c read error\n");
		return ret;
	}

	ret = rn5t567_setup_regs(client, rn5t567_regs,
				ARRAY_SIZE(rn5t567_regs));
	if (ret)
		return ret;

	ret = i2c_read_reg(client, RN5T567_DC1DAC, &value, 1);
	if (ret == 1) {
		pr_debug("VDDCORE set to %umV\n", regval_to_mV(value));
	} else {
		pr_err("i2c read error\n");
		return ret;
	}

	ret = i2c_read_reg(client, RN5T567_DC2DAC, &value, 1);
	if (ret == 1) {
		pr_debug("VDDSOC  set to %umV\n", regval_to_mV(value));
	} else {
		pr_err("i2c read error\n");
		return ret;
	}

	return 0;
}

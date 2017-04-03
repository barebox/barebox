/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <mach/generic.h>
#include <mach/cyclone5-freeze-controller.h>

#define SYSMGR_FRZCTRL_LOOP_PARAM       (1000)
#define SYSMGR_FRZCTRL_DELAY_LOOP_PARAM (10)

/*
 * sys_mgr_frzctrl_freeze_req
 * Freeze HPS IOs
 */
int sys_mgr_frzctrl_freeze_req(enum frz_channel_id channel_id)
{
	uint32_t reg, val;
	void *sm = (void *)CYCLONE5_SYSMGR_ADDRESS;

	/* select software FSM */
	writel(SYSMGR_FRZCTRL_SRC_VIO1_ENUM_SW,
		(sm + SYSMGR_FRZCTRL_SRC_ADDRESS));

	/* Freeze channel ID checking and base address */
	switch (channel_id) {
	case FREEZE_CHANNEL_0:
	case FREEZE_CHANNEL_1:
	case FREEZE_CHANNEL_2:
		reg = SYSMGR_FRZCTRL_VIOCTRL_ADDRESS + (channel_id << SYSMGR_FRZCTRL_VIOCTRL_SHIFT);

		/*
		 * Assert active low enrnsl, plniotri
		 * and niotri signals
		 */
		val = readl(sm + reg);
		val &= ~(SYSMGR_FRZCTRL_VIOCTRL_SLEW_MASK
			| SYSMGR_FRZCTRL_VIOCTRL_WKPULLUP_MASK
			| SYSMGR_FRZCTRL_VIOCTRL_TRISTATE_MASK);
		writel(val, sm + reg);

		/*
		 * Note: Delay for 20ns at min
		 * Assert active low bhniotri signal and de-assert
		 * active high csrdone
		 */
		val = readl(sm + reg);
		val &= ~(SYSMGR_FRZCTRL_VIOCTRL_BUSHOLD_MASK | SYSMGR_FRZCTRL_VIOCTRL_CFG_MASK);
		writel(val, sm + reg);

		break;

	case FREEZE_CHANNEL_3:
		/*
		 * Assert active low enrnsl, plniotri and
		 * niotri signals
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~(SYSMGR_FRZCTRL_HIOCTRL_SLEW_MASK
			| SYSMGR_FRZCTRL_HIOCTRL_WKPULLUP_MASK
			| SYSMGR_FRZCTRL_HIOCTRL_TRISTATE_MASK);
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Note: Delay for 40ns at min
		 * assert active low bhniotri & nfrzdrv signals,
		 * de-assert active high csrdone and assert
		 * active high frzreg and nfrzdrv signals
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~(SYSMGR_FRZCTRL_HIOCTRL_BUSHOLD_MASK
			| SYSMGR_FRZCTRL_HIOCTRL_CFG_MASK);
		val |= SYSMGR_FRZCTRL_HIOCTRL_REGRST_MASK
			| SYSMGR_FRZCTRL_HIOCTRL_OCTRST_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Note: Delay for 40ns at min
		 * assert active high reinit signal and de-assert
		 * active high pllbiasen signals
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~(SYSMGR_FRZCTRL_HIOCTRL_OCT_CFGEN_CALSTART_MASK);
		val |= SYSMGR_FRZCTRL_HIOCTRL_DLLRST_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * sys_mgr_frzctrl_thaw_req
 * Unfreeze/Thaw HPS IOs
 */
int sys_mgr_frzctrl_thaw_req(enum frz_channel_id channel_id)
{
	uint32_t reg, val;
	void *sm = (void *)CYCLONE5_SYSMGR_ADDRESS;

	/* select software FSM */
	writel(SYSMGR_FRZCTRL_SRC_VIO1_ENUM_SW, sm + SYSMGR_FRZCTRL_SRC_ADDRESS);

	/* Freeze channel ID checking and base address */
	switch (channel_id) {
	case FREEZE_CHANNEL_0:
	case FREEZE_CHANNEL_1:
	case FREEZE_CHANNEL_2:
		reg = SYSMGR_FRZCTRL_VIOCTRL_ADDRESS +
			(channel_id << SYSMGR_FRZCTRL_VIOCTRL_SHIFT);

		/*
		 * Assert active low bhniotri signal and
		 * de-assert active high csrdone
		 */
		val = readl(sm + reg);
		val |= SYSMGR_FRZCTRL_VIOCTRL_BUSHOLD_MASK |
			SYSMGR_FRZCTRL_VIOCTRL_CFG_MASK;
		writel(val, sm + reg);

		/*
		 * Note: Delay for 20ns at min
		 * de-assert active low plniotri and niotri signals
		 */
		val = readl(sm + reg);
		val |= SYSMGR_FRZCTRL_VIOCTRL_WKPULLUP_MASK |
			SYSMGR_FRZCTRL_VIOCTRL_TRISTATE_MASK;
		writel(val, sm + reg);

		/*
		 * Note: Delay for 20ns at min
		 * de-assert active low enrnsl signal
		 */
		val = readl(sm + reg);
		val |= SYSMGR_FRZCTRL_VIOCTRL_SLEW_MASK;
		writel(val, sm + reg);

		break;

	case FREEZE_CHANNEL_3:
		/* de-assert active high reinit signal */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~SYSMGR_FRZCTRL_HIOCTRL_DLLRST_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Note: Delay for 40ns at min
		 * assert active high pllbiasen signals
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val |= SYSMGR_FRZCTRL_HIOCTRL_OCT_CFGEN_CALSTART_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Delay 1000 intosc. intosc is based on eosc1
		 * At 25MHz this would be 40us. Play safe, we have time...
		 */
		__udelay(1000);

		/*
		 * de-assert active low bhniotri signals,
		 * assert active high csrdone and nfrzdrv signal
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val |= SYSMGR_FRZCTRL_HIOCTRL_BUSHOLD_MASK |
			SYSMGR_FRZCTRL_HIOCTRL_CFG_MASK;
		val &= ~SYSMGR_FRZCTRL_HIOCTRL_OCTRST_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/* Delay 33 intosc */
		__udelay(100);

		/* de-assert active low plniotri and niotri signals */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val |= SYSMGR_FRZCTRL_HIOCTRL_WKPULLUP_MASK |
			SYSMGR_FRZCTRL_HIOCTRL_TRISTATE_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Note: Delay for 40ns at min
		 * de-assert active high frzreg signal
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~SYSMGR_FRZCTRL_HIOCTRL_REGRST_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		/*
		 * Note: Delay for 40ns at min
		 * de-assert active low enrnsl signal
		 */
		val = readl(sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val |= SYSMGR_FRZCTRL_HIOCTRL_SLEW_MASK;
		writel(val, sm + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

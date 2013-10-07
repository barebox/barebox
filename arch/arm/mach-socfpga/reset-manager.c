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
#include <mach/socfpga-regs.h>
#include <mach/reset-manager.h>

/* Disable the watchdog (toggle reset to watchdog) */
void watchdog_disable(void)
{
	void __iomem *rm = (void *)CYCLONE5_RSTMGR_ADDRESS;
	uint32_t val;

	/* assert reset for watchdog */
	val = readl(rm + RESET_MGR_PER_MOD_RESET_OFS);
	val |= 1 << RSTMGR_PERMODRST_L4WD0_LSB;
	writel(val, rm + RESET_MGR_PER_MOD_RESET_OFS);

	/* deassert watchdog from reset (watchdog in not running state) */
	val = readl(rm + RESET_MGR_PER_MOD_RESET_OFS);
	val &= ~(1 << RSTMGR_PERMODRST_L4WD0_LSB);
	writel(val, rm + RESET_MGR_PER_MOD_RESET_OFS);
}

/* Write the reset manager register to cause reset */
void reset_cpu(ulong addr)
{
	/* request a warm reset */
	writel((1 << RSTMGR_CTRL_SWWARMRSTREQ_LSB),
		CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_CTRL_OFS);
	/*
	 * infinite loop here as watchdog will trigger and reset
	 * the processor
	 */
	while (1);
}

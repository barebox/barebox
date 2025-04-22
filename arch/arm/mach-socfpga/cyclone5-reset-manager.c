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
#include <init.h>
#include <restart.h>
#include <mach/socfpga/cyclone5-regs.h>
#include <mach/socfpga/cyclone5-reset-manager.h>

/* Write the reset manager register to cause reset */
static void __noreturn socfpga_restart_soc(struct restart_handler *rst,
					   unsigned long flags)
{
	/* request a warm reset */
	writel((1 << RSTMGR_CTRL_SWWARMRSTREQ_LSB),
		CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_CTRL_OFS);
	/*
	 * infinite loop here as watchdog will trigger and reset
	 * the processor
	 */
	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn("soc", socfpga_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);

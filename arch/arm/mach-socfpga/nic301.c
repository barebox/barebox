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
#include <mach/nic301.h>
#include <mach/cyclone5-regs.h>

/*
 * Convert all slave from secure to non secure
 */
void nic301_slave_ns(void)
{
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_LWHPS2FPGAREGS_ADDRESS));
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_HPS2FPGAREGS_ADDRESS));
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_ACP_ADDRESS));
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_ROM_ADDRESS));
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_OCRAM_ADDRESS));
	writel(0x1, (CYCLONE5_L3REGS_ADDRESS +
		L3REGS_SECGRP_SDRDATA_ADDRESS));
}

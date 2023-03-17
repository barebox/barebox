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

#ifndef	_NIC301_H_
#define	_NIC301_H_

void nic301_slave_ns(void);

#define L3REGS_SECGRP_LWHPS2FPGAREGS_ADDRESS 0x20
#define L3REGS_SECGRP_HPS2FPGAREGS_ADDRESS 0x90
#define L3REGS_SECGRP_ACP_ADDRESS 0x94
#define L3REGS_SECGRP_ROM_ADDRESS 0x98
#define L3REGS_SECGRP_OCRAM_ADDRESS 0x9c
#define L3REGS_SECGRP_SDRDATA_ADDRESS 0xa0

#define L3REGS_REMAP_LWHPS2FPGA_MASK 0x00000010
#define L3REGS_REMAP_HPS2FPGA_MASK 0x00000008
#define L3REGS_REMAP_OCRAM_MASK 0x00000001

#endif /* _NIC301_H_ */

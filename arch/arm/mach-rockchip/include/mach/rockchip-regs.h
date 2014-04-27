/*
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_ROCKCHIP_REGS_H
#define __MACH_ROCKCHIP_REGS_H

#define RK_CRU_BASE		0x20000000
#define RK_GRF_BASE		0x20008000

#define RK_CRU_GLB_SRST_SND	0x0104
#define RK_GRF_SOC_CON0		0x00a0

#define RK_SOC_CON0_REMAP	(1 << 12)

#endif /* __MACH_ROCKCHIP_REGS_H */

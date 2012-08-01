/**
 * @file
 * @brief Provide Generic GPMC configuration option
 *
 * FileName: arch/arm/mach-omap/gpmc.c
 *
 * This file contains the generic GPMC implementation
 *
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/silicon.h>
#include <mach/gpmc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

/**
 * @brief Do a Generic initialization of GPMC. if you choose otherwise,
 * Use gpmc registers to modify the values. The defaults configured are:
 * No idle, L3 free running, no timeout and no IRQs.
 * we allow for gpmc_config data to be programmed, and will also disable
 * ALL CS configurations
 *
 * @param cfg - GPMC_CFG register value
 *
 * @return void
 */
void gpmc_generic_init(unsigned int cfg)
{
	uint64_t start;
	unsigned int reg = GPMC_REG(CONFIG7_0);
	char x = 0;

	debug("gpmccfg=%x\n", cfg);
	/* Generic Configurations */
	/* reset gpmc */
	start = get_time_ns();
	/* No idle, L3 clock free running */
	writel(0x12, GPMC_REG(SYS_CONFIG));
	while (!readl(GPMC_REG(SYS_STATUS)))
		if (is_timeout(start, MSECOND)) {
			printf("timeout on gpmc reset\n");
			break;
		}

	/* No Timeout */
	writel(0x00, GPMC_REG(TIMEOUT_CONTROL));
	/* No IRQs */
	writel(0x00, GPMC_REG(IRQ_ENABLE));
	/* Write the gpmc_config value */
	writel(cfg, GPMC_REG(CFG));

	/* Disable all CS - prevents remaps
	 * But NEVER run me in XIP mode! I will Die!
	 */
	while (x < GPMC_NUM_CS) {
		debug("gpmccs=%d Reg:%x <-0x0\n", x, reg);
		writel(0x0, reg);
		reg += GPMC_CONFIG_CS_SIZE;
		x++;
	}
	/* Give me a while to settle things down */
	mdelay(1);
}
EXPORT_SYMBOL(gpmc_generic_init);

/**
 * @brief Configure the registers and enable a single CS.
 *
 * @param cs chip select index
 * @param config gpmc_config structure describing the CS params
 *
 * @return void
 */
void gpmc_cs_config(char cs, struct gpmc_config *config)
{
	unsigned int reg = GPMC_REG(CONFIG1_0) + (cs * GPMC_CONFIG_CS_SIZE);
	unsigned char x = 0;
	debug("gpmccs=%x cfg=%x\n", cs, (unsigned int)config);

	/* Disable the CS before reconfiguring */
	writel(0x0, GPMC_REG(CONFIG7_0) + (cs * GPMC_CONFIG_CS_SIZE));
	mdelay(1);		/* Settling time */

	/* Write the CFG1-6 regs */
	while (x < 6) {
		debug("gpmccfg=%d Reg:%x <-0x%x\n",
				x, reg, config->cfg[x]);
		writel(config->cfg[x], reg);
		reg += GPMC_CONFIG_REG_OFF;
		x++;
	}
	/* reg now points to CFG7 */
	debug("gpmccfg=%d Reg:%x <-0x%x\n",
			x, reg, (0x1 << 6) |		/* CS enable */
		     ((config->size & 0xF) << 8) |	/* Size */
		     ((config->base >> 24) & 0x3F));

	writel((0x1 << 6) |			/* CS enable */
		     ((config->size & 0xF) << 8) |	/* Size */
		     ((config->base >> 24) & 0x3F),	/* Address */
		     reg);
	mdelay(1);		/* Settling time */
}
EXPORT_SYMBOL(gpmc_cs_config);

/**
 * @file
 * @brief Provide Generic GPMC configuration option
 *
 * This file contains the generic GPMC implementation
 *
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
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <errno.h>
#include <mach/omap3-silicon.h>
#include <mach/omap4-silicon.h>
#include <mach/am33xx-silicon.h>
#include <mach/gpmc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

void __iomem *omap_gpmc_base;

static int gpmc_init(void)
{
#if defined(CONFIG_ARCH_OMAP3)
	omap_gpmc_base = (void *)OMAP3_GPMC_BASE;
#elif defined(CONFIG_ARCH_OMAP4)
	omap_gpmc_base = (void *)OMAP44XX_GPMC_BASE;
#elif defined(CONFIG_ARCH_AM33XX)
	omap_gpmc_base = (void *)AM33XX_GPMC_BASE;
#else
#error "Unknown ARCH"
#endif

	return 0;
}
pure_initcall(gpmc_init);

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
	void __iomem *reg = GPMC_REG(CONFIG7_0);
	char x = 0;

	debug("gpmccfg=0x%x\n", cfg);
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
		debug("gpmccs=%d Reg:0x%p <-0x0\n", x, reg);
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
	void __iomem *reg = GPMC_REG(CONFIG1_0) + (cs * GPMC_CONFIG_CS_SIZE);
	unsigned char x = 0;
	debug("gpmccs=0x%x cfg=0x%p\n", cs, config);

	/* Disable the CS before reconfiguring */
	writel(0x0, GPMC_REG(CONFIG7_0) + (cs * GPMC_CONFIG_CS_SIZE));
	mdelay(1);		/* Settling time */

	/* Write the CFG1-6 regs */
	while (x < 6) {
		debug("gpmccfg%d Reg:0x%p <-0x%08x\n",
				x, reg, config->cfg[x]);
		writel(config->cfg[x], reg);
		reg += GPMC_CONFIG_REG_OFF;
		x++;
	}
	/* reg now points to CFG7 */
	debug("gpmccfg%d Reg:0x%p <-0x%08x\n",
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

#define CS_NUM_SHIFT		24
#define ENABLE_PREFETCH		(0x1 << 7)
#define DMA_MPU_MODE		2

/**
 * gpmc_prefetch_enable - configures and starts prefetch transfer
 * @cs: cs (chip select) number
 * @fifo_th: fifo threshold to be used for read/ write
 * @dma_mode: dma mode enable (1) or disable (0)
 * @u32_count: number of bytes to be transferred
 * @is_write: prefetch read(0) or write post(1) mode
 */
int gpmc_prefetch_enable(int cs, int fifo_th, int dma_mode,
				unsigned int u32_count, int is_write)
{
	if (fifo_th > PREFETCH_FIFOTHRESHOLD_MAX) {
		pr_err("gpmc: fifo threshold is not supported\n");
		return -EINVAL;
	}

	/* Set the amount of bytes to be prefetched */
	writel(u32_count, GPMC_REG(PREFETCH_CONFIG2));

	/* Set dma/mpu mode, the prefetch read / post write and
	 * enable the engine. Set which cs is has requested for.
	 */
	writel(((cs << CS_NUM_SHIFT) | PREFETCH_FIFOTHRESHOLD(fifo_th) |
				ENABLE_PREFETCH | (dma_mode << DMA_MPU_MODE) |
				(0x1 & is_write)),
			GPMC_REG(PREFETCH_CONFIG1));

	/*  Start the prefetch engine */
	writel(0x1, GPMC_REG(PREFETCH_CONTROL));

	return 0;
}
EXPORT_SYMBOL(gpmc_prefetch_enable);

/**
 * gpmc_prefetch_reset - disables and stops the prefetch engine
 */
int gpmc_prefetch_reset(int cs)
{
	u32 config1;

	/* check if the same module/cs is trying to reset */
	config1 = readl(GPMC_REG(PREFETCH_CONFIG1));
	if (((config1 >> CS_NUM_SHIFT) & 0x7) != cs)
		return -EINVAL;

	/* Stop the PFPW engine */
	writel(0x0, GPMC_REG(PREFETCH_CONTROL));

	/* Reset/disable the PFPW engine */
	writel(0x0, GPMC_REG(PREFETCH_CONFIG1));

	return 0;
}
EXPORT_SYMBOL(gpmc_prefetch_reset);

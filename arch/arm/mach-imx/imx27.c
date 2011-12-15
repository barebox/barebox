/*
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
#include <mach/imx-regs.h>
#include <sizes.h>
#include <init.h>
#include <io.h>

#include "gpio.h"

int imx_silicon_revision(void)
{
	return CID >> 28;
}

void *imx_gpio_base[] = {
	(void *)0x10015000,
	(void *)0x10015100,
	(void *)0x10015200,
	(void *)0x10015300,
	(void *)0x10015400,
	(void *)0x10015500,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

/*
 * Initialize MAX on i.MX27. necessary to give the DMA engine
 * higher priority to the memory than the CPU. Needed for proper
 * audio support
 */
#define MAX_SLAVE_MPR_OFFSET	0x0	/* Master Priority register */
#define MAX_SLAVE_AMPR_OFFSET	0x4	/* Alternate Master Priority register */
#define MAX_SLAVE_PORT0_OFFSET	0x0
#define MAX_SLAVE_PORT1_OFFSET	0x100
#define MAX_SLAVE_PORT2_OFFSET	0x200
#define MAX_MASTER_PRIO(master, prio)	(((prio) << (master) * 4))

#define MASTER_IAHB	0
#define MASTER_DAHB	1
#define MASTER_EMMA	2
#define MASTER_DMA	3
#define MASTER_SLDC	4
#define MASTER_CODEC	5

static void imx27_init_max(void)
{
	void __iomem *max_base = (void *)IMX_MAX_BASE;
	u32 val;

	/* 0 is the highest priority */
	val = MAX_MASTER_PRIO(MASTER_IAHB, 5) |
		MAX_MASTER_PRIO(MASTER_DAHB, 4) |
		MAX_MASTER_PRIO(MASTER_EMMA, 1) |
		MAX_MASTER_PRIO(MASTER_DMA, 2) |
		MAX_MASTER_PRIO(MASTER_SLDC, 0) |
		MAX_MASTER_PRIO(MASTER_CODEC, 3);

	writel(val, max_base + MAX_SLAVE_PORT0_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT1_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT2_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT0_OFFSET + MAX_SLAVE_AMPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT1_OFFSET + MAX_SLAVE_AMPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT2_OFFSET + MAX_SLAVE_AMPR_OFFSET);
}

static int imx27_init(void)
{
	add_generic_device("imx_iim", 0, NULL, IMX_IIM_BASE, SZ_4K,
			IORESOURCE_MEM, NULL);

	imx27_init_max();

	return 0;
}
coredevice_initcall(imx27_init);

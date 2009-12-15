/*
 * barebox Header File
 *
 * Generic Clocksource for V4E
 *
 * Copyright (c) 2007 Carsten Schlote <c.schlote\@konzeptpark.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <init.h>
#include <mach/mcf54xx-regs.h>
#include <proc/mcdapi/MCD_dma.h>
#include <proc/dma_utils.h>


static int mcdapi_init(void)
{
	/*
	* Initialize the Multi-channel DMA
	*/
	MCD_initDma ((dmaRegs*)(__MBAR+0x8000),
		(void *)CFG_SYS_SRAM_ADDRESS,
		MCD_COMM_PREFETCH_EN | MCD_RELOC_TASKS);

	/*
	* Enable interrupts in DMA and INTC
	*/
	dma_irq_enable(DMA_INTC_LVL, DMA_INTC_PRI);

	return 0;
}

postcore_initcall(mcdapi_init);


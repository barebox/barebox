/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
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
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>

#include <asm/mmu.h>
#include <asm/cache-l2x0.h>

#include "core.h"

static void highbank_l2x0_disable(void)
{
	/* Disable PL310 L2 Cache controller */
	highbank_smc1(0x102, 0x0);
}

static int highbank_l2x0_init(void)
{
	/* Enable PL310 L2 Cache controller */
	highbank_smc1(0x102, 0x1);
	l2x0_init(IOMEM(0xfff12000), 0, ~0UL);
	outer_cache.disable = highbank_l2x0_disable;

	return 0;
}
postmmu_initcall(highbank_l2x0_init);

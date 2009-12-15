/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Implements a watchdog triggered reset for V4e Coldfire cores
 */
#include <common.h>
#include <mach/mcf54xx-regs.h>

/**
 * Reset the cpu by setting up the watchdog timer and let it time out
 */
void reset_cpu (ulong ignored)
{
        while ( ignored ) { ; };

	/* Disable watchdog and set Time-Out field to minimum timeout value */
	MCF_GPT_GMS0 = 0;
	MCF_GPT_GCIR0 =  MCF_GPT_GCIR_PRE(1) | MCF_GPT_GCIR_CNT(0xffff);

	/* Enable watchdog */
	MCF_GPT_GMS0 = MCF_GPT_GMS_OCPW(0xA5) | MCF_GPT_GMS_WDEN | MCF_GPT_GMS_CE | MCF_GPT_GMS_TMS_GPIO;

	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);


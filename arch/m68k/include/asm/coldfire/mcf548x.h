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
 *  Register and bit definitions for the MCF547X and MCF548X processors
 */
#ifndef __MCF548X_H__
#define __MCF548X_H__

/*
 * useful padding structure for register maps
 */
typedef struct
{
	vuint8_t a;
	vuint16_t b;
} __attribute ((packed)) vuint24_t;

/*
 *  Include all internal hardware register macros and defines for this CPU.
 */
#include "asm/coldfire/mcf548x/mcf548x_fec.h"
#include "asm/coldfire/mcf548x/mcf548x_siu.h"
#include "asm/coldfire/mcf548x/mcf548x_ctm.h"
#include "asm/coldfire/mcf548x/mcf548x_dspi.h"
#include "asm/coldfire/mcf548x/mcf548x_eport.h"
#include "asm/coldfire/mcf548x/mcf548x_fbcs.h"
#include "asm/coldfire/mcf548x/mcf548x_gpio.h"
#include "asm/coldfire/mcf548x/mcf548x_gpt.h"
#include "asm/coldfire/mcf548x/mcf548x_i2c.h"
#include "asm/coldfire/mcf548x/mcf548x_intc.h"
#include "asm/coldfire/mcf548x/mcf548x_sdramc.h"
#include "asm/coldfire/mcf548x/mcf548x_sec.h"
#include "asm/coldfire/mcf548x/mcf548x_slt.h"
#include "asm/coldfire/mcf548x/mcf548x_usb.h"
#include "asm/coldfire/mcf548x/mcf548x_psc.h"
#include "asm/coldfire/mcf548x/mcf548x_uart.h"
#include "asm/coldfire/mcf548x/mcf548x_sram.h"
#include "asm/coldfire/mcf548x/mcf548x_pci.h"
#include "asm/coldfire/mcf548x/mcf548x_pciarb.h"
#include "asm/coldfire/mcf548x/mcf548x_dma.h"
#include "asm/coldfire/mcf548x/mcf548x_dma_ereq.h"
#include "asm/coldfire/mcf548x/mcf548x_can.h"
#include "asm/coldfire/mcf548x/mcf548x_xlbarb.h"

#endif /* __MCF548X_H__ */

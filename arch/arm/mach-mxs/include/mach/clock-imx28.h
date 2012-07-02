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
 */

#ifndef MACH_CLOCK_IMX28_H
#define MACH_CLOCK_IMX28_H

unsigned imx_get_mpllclk(void);
unsigned imx_get_emiclk(void);
unsigned imx_get_ioclk(unsigned);
unsigned imx_get_armclk(void);
unsigned imx_get_hclk(void);
unsigned imx_set_hclk(unsigned);
unsigned imx_get_xclk(void);
unsigned imx_get_sspclk(unsigned);
unsigned imx_set_sspclk(unsigned, unsigned, int);
unsigned imx_set_ioclk(unsigned, unsigned);
unsigned imx_set_lcdifclk(unsigned);
unsigned imx_get_lcdifclk(void);
unsigned imx_get_fecclk(void);
void imx_enable_enetclk(void);
void imx_enable_nandclk(void);

#endif /* MACH_CLOCK_IMX28_H */


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

#include <init.h>
#include <common.h>
#include <io.h>
#include <sizes.h>
#include <mach/imx53-regs.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)MX53_GPIO1_BASE_ADDR,
	(void *)MX53_GPIO2_BASE_ADDR,
	(void *)MX53_GPIO3_BASE_ADDR,
	(void *)MX53_GPIO4_BASE_ADDR,
	(void *)MX53_GPIO5_BASE_ADDR,
	(void *)MX53_GPIO6_BASE_ADDR,
	(void *)MX53_GPIO7_BASE_ADDR,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

static int imx53_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX53_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx53_init);

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
#include <mach/imx6-regs.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)MX6_GPIO1_BASE_ADDR,
	(void *)MX6_GPIO2_BASE_ADDR,
	(void *)MX6_GPIO3_BASE_ADDR,
	(void *)MX6_GPIO4_BASE_ADDR,
	(void *)MX6_GPIO5_BASE_ADDR,
	(void *)MX6_GPIO6_BASE_ADDR,
	(void *)MX6_GPIO7_BASE_ADDR,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

void imx6_init_lowlevel(void)
{
	void __iomem *aips1 = (void *)MX6_AIPS1_ON_BASE_ADDR;
	void __iomem *aips2 = (void *)MX6_AIPS2_ON_BASE_ADDR;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, aips1);
	writel(0x77777777, aips1 + 0x4);
	writel(0, aips1 + 0x40);
	writel(0, aips1 + 0x44);
	writel(0, aips1 + 0x48);
	writel(0, aips1 + 0x4c);
	writel(0, aips1 + 0x50);

	writel(0x77777777, aips2);
	writel(0x77777777, aips2 + 0x4);
	writel(0, aips2 + 0x40);
	writel(0, aips2 + 0x44);
	writel(0, aips2 + 0x48);
	writel(0, aips2 + 0x4c);
	writel(0, aips2 + 0x50);

	/* enable all clocks */
	writel(0xffffffff, 0x020c4068);
	writel(0xffffffff, 0x020c406c);
	writel(0xffffffff, 0x020c4070);
	writel(0xffffffff, 0x020c4074);
	writel(0xffffffff, 0x020c4078);
	writel(0xffffffff, 0x020c407c);
	writel(0xffffffff, 0x020c4080);
}

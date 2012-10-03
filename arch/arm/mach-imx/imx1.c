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
#include <init.h>

static int imx1_init(void)
{
	add_generic_device("imx1-gpt", 0, NULL, 0x00202000, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, 0x0021c000, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, 0x0021c100, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, 0x0021c200, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, 0x0021c300, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx1_init);

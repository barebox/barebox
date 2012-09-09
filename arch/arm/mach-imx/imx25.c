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
#include <mach/imx-regs.h>
#include <mach/iim.h>
#include <io.h>
#include <sizes.h>

u64 imx_uid(void)
{
	u64 uid = 0;
	int i;

	/*
	 * This code assumes that the UID is stored little-endian. The
	 * Freescale AN3682 document is silent about the endianess, but
	 * experimentation shows that this is the case. All other multi-byte
	 * values in IIM are big-endian as per AN3682.
	 */
	for (i = 0; i < 8; i++)
		uid |= (u64)readb(IIM_UID + i*4) << (i*8);

	return uid;
}

static struct imx_iim_platform_data imx25_iim_pdata = {
	.mac_addr_base	= IIM_MAC_ADDR,
};

static int imx25_init(void)
{
	add_generic_device("imx_iim", 0, NULL, IMX_IIM_BASE, SZ_4K,
			IORESOURCE_MEM, &imx25_iim_pdata);

	add_generic_device("imx31-gpt", 0, NULL, 0x53f90000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, 0x53fcc000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, 0x53fd0000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, 0x53fa4000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 3, NULL, 0x53f9c000, 0x1000, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx25_init);

/*
 *  (C) 2007, Sascha Hauer <sha@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <common.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <mach/iomux-mx31.h>

/*
 * IOMUX register (base) addresses
 */
#define IOMUXINT_OBS1	(IOMUXC_BASE + 0x000)
#define IOMUXINT_OBS2	(IOMUXC_BASE + 0x004)
#define IOMUXGPR	(IOMUXC_BASE + 0x008)
#define IOMUXSW_MUX_CTL	(IOMUXC_BASE + 0x00C)
#define IOMUXSW_PAD_CTL	(IOMUXC_BASE + 0x154)

#define IOMUX_REG_MASK (IOMUX_PADNUM_MASK & ~0x3)
/*
 * set the mode for a IOMUX pin.
 */
int imx_iomux_mode(unsigned int pin_mode)
{
	u32 field, l, mode, ret = 0;
	void __iomem *reg;

	reg = (void *)(IOMUXSW_MUX_CTL + (pin_mode & IOMUX_REG_MASK));
	field = pin_mode & 0x3;
	mode = (pin_mode & IOMUX_MODE_MASK) >> IOMUX_MODE_SHIFT;

	pr_debug("%s: reg offset = 0x%x field = %d mode = 0x%02x\n",
			__func__, (pin_mode & IOMUX_REG_MASK), field, mode);

	l = readl(reg);
	l &= ~(0xff << (field * 8));
	l |= mode << (field * 8);
	writel(l, reg);

	return ret;
}
EXPORT_SYMBOL(mxc_iomux_mode);

/*
 * This function configures the pad value for a IOMUX pin.
 */
void imx_iomux_set_pad(enum iomux_pins pin, u32 config)
{
	u32 field, l;
	void __iomem *reg;

	pin &= IOMUX_PADNUM_MASK;
	reg = (void *)(IOMUXSW_PAD_CTL + (pin + 2) / 3 * 4);
	field = (pin + 2) % 3;

	pr_debug("%s: reg offset = 0x%x, field = %d\n",
			__func__, (pin + 2) / 3, field);

	l = readl(reg);
	l &= ~(0x1ff << (field * 10));
	l |= config << (field * 10);
	writel(l, reg);
}
EXPORT_SYMBOL(mxc_iomux_set_pad);

/*
 * This function enables/disables the general purpose function for a particular
 * signal.
 */
void imx_iomux_set_gpr(enum iomux_gp_func gp, int en)
{
	u32 l;

	l = readl(IOMUXGPR);
	if (en)
		l |= gp;
	else
		l &= ~gp;

	writel(l, IOMUXGPR);
}
EXPORT_SYMBOL(mxc_iomux_set_gpr);



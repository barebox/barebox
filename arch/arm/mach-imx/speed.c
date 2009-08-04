/*
 *
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
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

#include <asm-generic/div64.h>
#include <common.h>
#include <command.h>

/*
 *  get the system pll clock in Hz
 *
 *                  mfi + mfn / (mfd +1)
 *  f = 2 * f_ref * --------------------
 *                        pd + 1
 */
unsigned int imx_decode_pll(unsigned int pll, unsigned int f_ref)
{
	unsigned long long ll;
	unsigned int quot;

	unsigned int mfi = (pll >> 10) & 0xf;
	unsigned int mfn = pll & 0x3ff;
	unsigned int mfd = (pll >> 16) & 0x3ff;
	unsigned int pd =  (pll >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	ll = 2 * (unsigned long long)f_ref * ( (mfi << 16) + (mfn << 16) / (mfd + 1));
	quot = (pd + 1) * (1 << 16);
	ll += quot / 2;
	do_div(ll, quot);
	return (unsigned int) ll;
}

extern void imx_dump_clocks(void);

static int do_clocks (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	imx_dump_clocks();

	return 0;
}

U_BOOT_CMD_START(dump_clocks)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_clocks,
	.usage		= "show clock frequencies",
U_BOOT_CMD_END


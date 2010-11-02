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
#include <mach/clock.h>

/*
 *  get the system pll clock in Hz
 *
 *                  mfi + mfn / (mfd +1)
 *  f = 2 * f_ref * --------------------
 *                        pd + 1
 */
unsigned int imx_decode_pll(unsigned int reg_val, unsigned int freq)
{
	unsigned long long ll;
	int mfn_abs;
	unsigned int mfi, mfn, mfd, pd;

	mfi = (reg_val >> 10) & 0xf;
	mfn = reg_val & 0x3ff;
	mfd = (reg_val >> 16) & 0x3ff;
	pd =  (reg_val >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	mfn_abs = mfn;

#if !defined CONFIG_ARCH_MX1 && !defined CONFIG_ARCH_MX21
	if (mfn >= 0x200) {
		mfn |= 0xFFFFFE00;
		mfn_abs = -mfn;
	}
#endif

	freq *= 2;
	freq /= pd + 1;

	ll = (unsigned long long)freq * mfn_abs;

	do_div(ll, mfd + 1);
	if (mfn < 0)
		ll = -ll;
	ll = (freq * mfi) + ll;

	return ll;
}

extern void imx_dump_clocks(void);

static int do_clocks(struct command *cmdtp, int argc, char *argv[])
{
	imx_dump_clocks();

	return 0;
}

BAREBOX_CMD_START(dump_clocks)
	.cmd		= do_clocks,
	.usage		= "show clock frequencies",
BAREBOX_CMD_END


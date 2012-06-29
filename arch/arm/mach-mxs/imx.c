/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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

#include <common.h>
#include <command.h>
#include <complete.h>
#include <init.h>
#include <io.h>
#include <mach/imx-regs.h>

#define HW_RTC_PERSISTENT1     0x070

static int imx_reset_usb_bootstrap(void)
{
	/*
	 * Clear USB boot mode.
	 *
	 * When the i.MX28 boots from USB, the ROM code sets this bit. When
	 * after a reset the ROM code detects that this bit is set it will
	 * boot from USB again. This means that if we boot once from USB the
	 * chip will continue to boot from USB until the next power cycle.
	 *
	 * To prevent this (and boot from the configured bootsource instead)
	 * clear this bit here.
	 */
	writel(0x2, IMX_WDT_BASE + HW_RTC_PERSISTENT1 + BIT_CLR);

	return 0;
}
device_initcall(imx_reset_usb_bootstrap);

extern void imx_dump_clocks(void);

static int do_clocks(int argc, char *argv[])
{
	imx_dump_clocks();

	return 0;
}

BAREBOX_CMD_START(dump_clocks)
	.cmd		= do_clocks,
	.usage		= "show clock frequencies",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

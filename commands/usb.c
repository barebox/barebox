/*
 * usb.c - rescan for USB devices
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>
#include <command.h>
#include <usb/usb.h>

static int do_usb(struct command *cmdtp, int argc, char *argv[])
{
	usb_rescan();

	return 0;
}

static const __maybe_unused char cmd_usb_help[] =
"Usage: usb\n"
"(re-)detect USB devices\n";

BAREBOX_CMD_START(usb)
	.cmd		= do_usb,
	.usage		= "(re-)detect USB devices",
	BAREBOX_CMD_HELP(cmd_usb_help)
BAREBOX_CMD_END

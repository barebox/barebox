// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2006-2008 Nishanth Menon <x0nishan@ti.com>, Texas Instruments (http://www.ti.com/)

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <asm/armlinux.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-devices.h>
#include <mach/gpmc.h>
#include <errno.h>

/**
 * @brief UART serial port initialization - remember to enable COM clocks in arch
 *
 * @return result of device registration
 */
static int sdp3430_console_init(void)
{
	barebox_set_model("Texas Instruments SDP343x");
	barebox_set_hostname("sdp343x");

	omap3_add_uart3();

	return 0;
}

console_initcall(sdp3430_console_init);

static int sdp3430_mem_init(void)
{
	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(sdp3430_mem_init);

static int sdp3430_devices_init(void)
{
#ifdef CONFIG_OMAP_GPMC
	/* WP is made high and WAIT1 active Low */
	gpmc_generic_init(0x10);
#endif

	return 0;
}

device_initcall(sdp3430_devices_init);

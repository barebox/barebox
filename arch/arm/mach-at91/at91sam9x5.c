// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <restart.h>
#include <mach/at91/at91sam9x5.h>
#include <mach/at91/board.h>
#include <mach/at91/at91_rstc.h>

static void at91sam9x5_restart(struct restart_handler *rst,
			       unsigned long flags)
{
	at91sam9g45_reset(IOMEM(AT91SAM9X5_BASE_DDRSDRC0),
			  IOMEM(AT91SAM9X5_BASE_RSTC + AT91_RSTC_CR));
}

static struct restart_handler restart;

static int at91sam9x5_initialize(void)
{
	if (IS_ENABLED(CONFIG_OFDEVICE) && !of_machine_is_compatible("atmel,at91sam9x5"))
		return 0;

	restart.name = "soc";
	restart.priority = 110;
	restart.restart = at91sam9x5_restart;

	return restart_handler_register(&restart);
}
coredevice_initcall(at91sam9x5_initialize);

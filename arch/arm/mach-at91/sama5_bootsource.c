// SPDX-License-Identifier: GPL-2.0-only

#include <mach/at91/sama5_bootsource.h>
#include <linux/export.h>
#include <bootsource.h>
#include <init.h>
#include <of.h>

/*
 * sama5_bootsource_init - initialize bootsource
 *
 * BootROM will populate r4 when loading first stage bootloader
 * with information about boot source. The entry points for
 * multi-image capable SAMA5 boards will pass this information
 * along. If you use a bootloader before barebox, you need to
 * ensure that r4 is initialized for $bootsource to be correct
 * in barebox. Example implementing it for AT91Bootstrap:
 * https://github.com/linux4sam/at91bootstrap/pull/159
 */
static int sama5_bootsource_init(void)
{
	if (of_machine_is_compatible("atmel,sama5d2"))
		at91_bootsource = __sama5d2_stashed_bootrom_r4;
	else if (of_machine_is_compatible("atmel,sama5d3"))
		at91_bootsource = __sama5d3_stashed_bootrom_r4;
	else if (of_machine_is_compatible("atmel,sama5d4"))
		at91_bootsource = __sama5d4_stashed_bootrom_r4;
	else
		return 0;

	if (at91_bootsource)
		bootsource_set_raw(sama5_bootsource(at91_bootsource),
				   sama5_bootsource_instance(at91_bootsource));

	return 0;
}
postcore_initcall(sama5_bootsource_init);

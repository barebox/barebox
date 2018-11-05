#include <common.h>
#include <init.h>
#include <restart.h>
#include <mach/at91sam9x5.h>
#include <mach/board.h>
#include <mach/at91_rstc.h>

static void at91sam9x5_restart(struct restart_handler *rst)
{
	at91sam9g45_reset(IOMEM(AT91SAM9X5_BASE_DDRSDRC0),
			  IOMEM(AT91SAM9X5_BASE_RSTC + AT91_RSTC_CR));
}

static int at91sam9x5_initialize(void)
{
	restart_handler_register_fn(at91sam9x5_restart);

	return 0;
}
coredevice_initcall(at91sam9x5_initialize);

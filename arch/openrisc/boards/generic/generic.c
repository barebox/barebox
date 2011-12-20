#include <common.h>
#include <init.h>
#include <driver.h>
#include <partition.h>
#include <ns16550.h>

static struct NS16550_plat serial_plat = {
	.clock = 50000000,      /* 48MHz (APLL96/2) */
	.shift = 0,
};

static int openrisc_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(-1, OPENRISC_SOPC_UART_BASE, 1024, IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(openrisc_console_init);

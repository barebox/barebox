#include <common.h>
#include <init.h>
#include <driver.h>
#include <partition.h>
#include <ns16550.h>

static struct NS16550_plat serial_plat = {
	.clock = OPENRISC_SOPC_UART_FREQ,
	.shift = 0,
};

static int openrisc_console_init(void)
{
	barebox_set_model("OpenRISC or1k");
	barebox_set_hostname("or1k");

	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, OPENRISC_SOPC_UART_BASE, 1024, IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(openrisc_console_init);

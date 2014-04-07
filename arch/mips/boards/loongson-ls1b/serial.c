#include <common.h>
#include <init.h>
#include <ns16550.h>

#include <mach/loongson1.h>

static struct NS16550_plat serial_plat = {
	.clock = 83000000,
	.shift = 0,
};

static int console_init(void)
{
	barebox_set_model("Loongson Tech LS1B Demo Board");
	barebox_set_hostname("ls1b");

	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(LS1X_UART2_BASE),
		8, IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(console_init);

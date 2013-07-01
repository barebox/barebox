#include <driver.h>
#include <ns16550.h>
#include <asm/armlinux.h>

#include <mach/omap3-devices.h>

void omap_add_ram0(resource_size_t size)
{
	arm_add_mem_device("ram0", 0x80000000, size);
}

void omap_add_sram0(resource_size_t base, resource_size_t size)
{
	add_mem_device("sram0", base, size, IORESOURCE_MEM_WRITEABLE);
}

static struct NS16550_plat serial_plat = {
	.clock = 48000000,      /* 48MHz (APLL96/2) */
	.shift = 2,
};

struct device_d *omap_add_uart(int id, unsigned long base)
{
	return add_ns16550_device(id, base, 1024,
			IORESOURCE_MEM_8BIT, &serial_plat);
}

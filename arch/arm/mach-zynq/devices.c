#include <common.h>
#include <driver.h>
#include <mach/devices.h>

struct device_d *zynq_add_uart(resource_size_t base, int id)
{
	return add_generic_device("cadence-uart", id, NULL, base, 0x1000, IORESOURCE_MEM, NULL);
}

struct device_d *zynq_add_eth(resource_size_t base, int id, struct macb_platform_data *pdata)
{
	return add_generic_device("macb", id, NULL, base, 0x1000, IORESOURCE_MEM, pdata);
}

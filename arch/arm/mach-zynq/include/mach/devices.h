#include <mach/zynq7000-regs.h>
#include <platform_data/macb.h>

struct device_d *zynq_add_uart(resource_size_t base, int id);
struct device_d *zynq_add_eth(resource_size_t base, int id, struct macb_platform_data *pdata);

static inline struct device_d *zynq_add_uart0(void)
{
	return zynq_add_uart((resource_size_t)ZYNQ_UART0_BASE_ADDR, 0);
}

static inline struct device_d *zynq_add_uart1(void)
{
	return zynq_add_uart((resource_size_t)ZYNQ_UART1_BASE_ADDR, 1);
}

static inline struct device_d *zynq_add_eth0(struct macb_platform_data *pdata)
{
	return zynq_add_eth((resource_size_t)ZYNQ_GEM0_BASE_ADDR, 0, pdata);
}

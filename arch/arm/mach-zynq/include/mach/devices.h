#include <mach/zynq7000-regs.h>

struct device_d *zynq_add_uart(resource_size_t base, int id);

static inline struct device_d *zynq_add_uart0(void)
{
	return zynq_add_uart((resource_size_t)ZYNQ_UART0_BASE_ADDR, 0);
}

static inline struct device_d *zynq_add_uart1(void)
{
	return zynq_add_uart((resource_size_t)ZYNQ_UART1_BASE_ADDR, 1);
}

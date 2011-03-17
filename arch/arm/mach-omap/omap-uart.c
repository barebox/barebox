#include <common.h>
#include <asm/io.h>

/**
 * @brief Uart port register read function for OMAP3
 *
 * @param base base address of UART
 * @param reg_idx register index
 *
 * @return character read from register
 */
unsigned int omap_uart_read(unsigned long base, unsigned char reg_idx)
{
	unsigned int *reg_addr = (unsigned int *)base;
	reg_addr += reg_idx;
	return readb(reg_addr);
}
EXPORT_SYMBOL(omap_uart_read);

/**
 * @brief Uart port register write function for OMAP3
 *
 * @param val value to write
 * @param base base address of UART
 * @param reg_idx register index
 *
 * @return void
 */
void omap_uart_write(unsigned int val, unsigned long base,
		     unsigned char reg_idx)
{
	unsigned int *reg_addr = (unsigned int *)base;
	reg_addr += reg_idx;
	writeb(val, reg_addr);
}
EXPORT_SYMBOL(omap_uart_write);

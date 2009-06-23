/*
 *
 * (c) 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <gpio.h>

#define OFS_PIO_PER	0x00	/* Enable Register */
#define OFS_PIO_PDR	0x04	/* Disable Register */
#define OFS_PIO_PSR	0x08	/* Status Register */
#define OFS_PIO_OER	0x10	/* Output Enable Register */
#define OFS_PIO_ODR	0x14	/* Output Disable Register */
#define OFS_PIO_OSR	0x18	/* Output Status Register */
#define OFS_PIO_IFER	0x20	/* Glitch Input Filter Enable */
#define OFS_PIO_IFDR	0x24	/* Glitch Input Filter Disable */
#define OFS_PIO_IFSR	0x28	/* Glitch Input Filter Status */
#define OFS_PIO_SODR	0x30	/* Set Output Data Register */
#define OFS_PIO_CODR	0x34	/* Clear Output Data Register */
#define OFS_PIO_ODSR	0x38	/* Output Data Status Register */
#define OFS_PIO_PDSR	0x3c	/* Pin Data Status Register */
#define OFS_PIO_IER	0x40	/* Interrupt Enable Register */
#define OFS_PIO_IDR	0x44	/* Interrupt Disable Register */
#define OFS_PIO_IMR	0x48	/* Interrupt Mask Register */
#define OFS_PIO_ISR	0x4c	/* Interrupt Status Register */
#define OFS_PIO_MDER	0x50	/* Multi-driver Enable Register */
#define OFS_PIO_MDDR	0x54	/* Multi-driver Disable Register */
#define OFS_PIO_MDSR	0x58	/* Multi-driver Status Register */
#define OFS_PIO_PUDR	0x60	/* Pull-up Disable Register */
#define OFS_PIO_PUER	0x64	/* Pull-up Enable Register */
#define OFS_PIO_PUSR	0x68	/* Pull-up Status Register */
#define OFS_PIO_ASR	0x70	/* Peripheral A Select Register */
#define OFS_PIO_BSR	0x74	/* Peripheral B Select Register */
#define OFS_PIO_ABSR	0x78	/* AB Status Register */
#define OFS_PIO_OWER	0xa0	/* Output Write Enable Register */
#define OFS_PIO_OWDR	0xa4	/* Output Write Disable Register */
#define OFS_PIO_OWSR	0xa8	/* Output Write Status Register */

static int gpio_banks;
static struct at91_gpio_bank *gpio;

static inline void __iomem *pin_to_controller(unsigned pin)
{
	pin /= 32;
	if (likely(pin < gpio_banks))
		return gpio[pin].regbase;

	return NULL;
}

static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << (pin % 32);
}

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral), and
 * configure it for an input.
 */
int at91_set_gpio_input(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	writel(mask, pio + OFS_PIO_IDR);
	writel(mask, pio + (use_pullup ? OFS_PIO_PUER : OFS_PIO_PUDR));
	writel(mask, pio + OFS_PIO_ODR);
	writel(mask, pio + OFS_PIO_PER);
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_input);

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral),
 * and configure it for an output.
 */
int at91_set_gpio_output(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	writel(mask, pio + OFS_PIO_IDR);
	writel(mask, pio + OFS_PIO_PUDR);
	writel(mask, pio + (value ? OFS_PIO_SODR : OFS_PIO_CODR));
	writel(mask, pio + OFS_PIO_OER);
	writel(mask, pio + OFS_PIO_PER);
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_output);

int gpio_direction_input(unsigned pin)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(readl(pio + OFS_PIO_PSR) & mask))
		return -EINVAL;
	writel(mask, pio + OFS_PIO_ODR);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_input);

int gpio_direction_output(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(readl(pio + OFS_PIO_PSR) & mask))
		return -EINVAL;
	writel(mask, pio + (value ? OFS_PIO_SODR : OFS_PIO_CODR));
	writel(mask, pio + OFS_PIO_OER);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_output);

/*--------------------------------------------------------------------------*/

/*
 * assuming the pin is muxed as a gpio output, set its value.
 */
void gpio_set_value(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return;
	writel(mask, pio + (value ? OFS_PIO_SODR : OFS_PIO_CODR));
}
EXPORT_SYMBOL(gpio_set_value);


/*
 * read the pin's value (works even if it's not muxed as a gpio).
 */
int gpio_get_value(unsigned pin)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);
	u32		pdsr;

	if (!pio)
		return -EINVAL;
	pdsr = readl(pio + OFS_PIO_PDSR);
	return (pdsr & mask) != 0;
}
EXPORT_SYMBOL(gpio_get_value);

int at91_gpio_init(struct at91_gpio_bank *data, int nr_banks)
{
	unsigned		i;
	struct at91_gpio_bank	*last;

	gpio = data;
	gpio_banks = nr_banks;

	for (i = 0, last = NULL; i < nr_banks; i++, last = data, data++) {
		data->chipbase = i * 32;

		/* AT91SAM9263_ID_PIOCDE groups PIOC, PIOD, PIOE */
		if (last && last->id == data->id)
			last->next = data;
	}

	return 0;
}

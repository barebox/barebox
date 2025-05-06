/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2022 Sam Ravnborg */

#include <debug_ll.h>
#include <common.h>
#include <pbl/bio.h>

#include <linux/sizes.h>
#include <asm/cache.h>

#include <mach/at91/at91_pmc_ll.h>
#include <mach/at91/at91sam9263.h>
#include <mach/at91/at91sam926x.h>
#include <mach/at91/hardware.h>
#include <mach/at91/iomux.h>
#include <mach/at91/xload.h>
#include <mach/at91/gpio.h>

typedef void (*func)(int zero, int arch, void *params);

/*
 * Load barebox.bin and start executing the first byte in the barebox image.
 * barebox.bin is loaded to AT91_CHIPSELECT_1.
 *
 * To be able to load barebox.bin do a minimal init of the pheriferals
 * used by MCI.
 * This functions runs in PBL code and uses the PBL variant of the
 * atmel_mci driver.
 */
void __noreturn sam9263_atmci_start_image(u32 mmc_id, unsigned int clock,
					  bool slot_b)
{
	void __iomem *pio = IOMEM(AT91SAM9263_BASE_PIOA);
	void *buf = (void *)AT91_CHIPSELECT_1;
	void __iomem *base;
	struct pbl_bio bio;
	int ret;

	at91_pmc_enable_periph_clock(IOMEM(AT91SAM926X_BASE_PMC), AT91SAM9263_ID_PIOA);

	if (mmc_id == 0) {
		base = IOMEM(AT91SAM9263_BASE_MCI0);

		/* CLK */
		at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA12), AT91_MUX_PERIPH_A, 0);

		if (!slot_b) {
			/* CMD */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA1), AT91_MUX_PERIPH_A, GPIO_PULL_UP);

			/* DAT0 to DAT3 */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA0), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA3), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA4), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA5), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
		} else {
			/* CMD */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA16), AT91_MUX_PERIPH_A, GPIO_PULL_UP);

			/* DAT0 to DAT3 */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA17), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA18), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA19), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA20), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
		}

		at91_pmc_enable_periph_clock(IOMEM(AT91SAM926X_BASE_PMC),  AT91SAM9263_ID_MCI0);
	} else {
		base = IOMEM(AT91SAM9263_BASE_MCI1);

		/* CLK */
		at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA6), AT91_MUX_PERIPH_A, 0);

		if (!slot_b) {
			/* CMD */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA7), AT91_MUX_PERIPH_A, GPIO_PULL_UP);

			/* DAT0 to DAT3 */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA8), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA9), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA10), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA11), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
		} else {
			/* CMD */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA21), AT91_MUX_PERIPH_A, GPIO_PULL_UP);

			/* DAT0 to DAT3 */
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA22), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA23), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA24), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
			at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PA25), AT91_MUX_PERIPH_A, GPIO_PULL_UP);
		}

		at91_pmc_enable_periph_clock(IOMEM(AT91SAM926X_BASE_PMC),  AT91SAM9263_ID_MCI1);
	}

	ret = at91_mci_bio_init(&bio, base, clock, (int)slot_b, PBL_MCI_STANDARD_CAPACITY);
	if (ret) {
		pr_err("atmci_start_image: bio init faild: %d\n", ret);
		goto out_panic;
	}

	ret = pbl_fat_load(&bio, "barebox.bin", buf, SZ_16M);
	if (ret < 0) {
		pr_err("pbl_fat_load: error %d\n", ret);
		goto out_panic;
	}

	sync_caches_for_execution();

	((func)buf)(0, 0, NULL);

out_panic:
	panic("FAT chainloading failed\n");
}

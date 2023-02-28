// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <mach/at91/xload.h>
#include <mach/at91/sama5_bootsource.h>
#include <mach/at91/hardware.h>
#include <mach/at91/sama5d2_ll.h>
#include <mach/at91/sama5d3_ll.h>
#include <mach/at91/gpio.h>
#include <linux/sizes.h>
#include <asm/cache.h>
#include <pbl/bio.h>

static void at91_fat_start_image(struct pbl_bio *bio,
				 void *buf, unsigned int len,
				 u32 r4)
{
	void __noreturn (*bb)(void);
	int ret;

	ret = pbl_fat_load(bio, "barebox.bin", buf, len);
	if (ret < 0) {
		pr_err("pbl_fat_load: error %d\n", ret);
		return;
	}

	bb = buf;

	sync_caches_for_execution();

	sama5_boot_xload(bb, r4);
}

static const struct sdhci_instance {
	void __iomem *base;
	unsigned id;
	u8 periph;
	s8 pins[15];
} sdhci_instances[] = {
	[0] = {
		.base = SAMA5D2_BASE_SDHC0, .id = SAMA5D2_ID_SDMMC0, .periph = AT91_MUX_PERIPH_A,
		.pins = { 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 13, 10, 11, 12, -1 }
	},
	[1] = {
		.base = SAMA5D2_BASE_SDHC1, .id = SAMA5D2_ID_SDMMC1, .periph = AT91_MUX_PERIPH_E,
		.pins = { 18, 19, 20, 21, 22, 28, 30, -1 }
	},
};

/**
 * sama5d2_sdhci_start_image - Load and start an image from FAT-formatted SDHCI
 * @r4: value of r4 passed by BootROM
 */
void __noreturn sama5d2_sdhci_start_image(u32 r4)
{
	void *buf = (void *)SAMA5_DDRCS;
	const struct sdhci_instance *instance;
	struct pbl_bio bio;
	const s8 *pin;
	int ret;

	ret = sama5_bootsource_instance(r4);
	if (ret > 1)
		panic("Couldn't determine boot MCI instance\n");

	instance = &sdhci_instances[ret];

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_PIOA);
	for (pin = instance->pins; *pin >= 0; pin++) {
		at91_mux_pio4_set_periph(SAMA5D2_BASE_PIOA,
					 BIT(*pin), instance->periph);
	}

	sama5d2_pmc_enable_periph_clock(instance->id);
	sama5d2_pmc_enable_generic_clock(instance->id, AT91_PMC_GCKCSS_UPLL_CLK, 1);

	ret = at91_sdhci_bio_init(&bio, instance->base);
	if (ret)
		goto out_panic;

	/* TODO: eMMC boot partition handling: they are not FAT-formatted */

	at91_fat_start_image(&bio, buf, SZ_16M, r4);

out_panic:
	panic("FAT chainloading failed\n");
}

static const struct atmci_instance {
	void __iomem *base;
	unsigned id;
	u8 periph;
	s8 pins[15];
} sama5d3_atmci_instances[] = {
	[0] = {
		.base = IOMEM(SAMA5D3_BASE_HSMCI0),
		.id = SAMA5D3_ID_HSMCI0,
		.periph = AT91_MUX_PERIPH_A,
		.pins = {
			AT91_PIN_PD0, AT91_PIN_PD1, AT91_PIN_PD2, AT91_PIN_PD3,
			AT91_PIN_PD4, AT91_PIN_PD5, AT91_PIN_PD6, AT91_PIN_PD7,
			AT91_PIN_PD8, AT91_PIN_PD9, -1 }
	},
};

void __noreturn sama5d3_atmci_start_image(u32 boot_src, unsigned int clock,
					  unsigned int slot)
{
	void *buf = (void *)SAMA5_DDRCS;
	const struct atmci_instance *instance;
	struct pbl_bio bio;
	const s8 *pin;
	int ret;

	ret = sama5_bootsource_instance(boot_src);
	if (ret > ARRAY_SIZE(sama5d3_atmci_instances) - 1)
		panic("Couldn't determine boot MCI instance\n");

	instance = &sama5d3_atmci_instances[boot_src];

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_PIOD);
	for (pin = instance->pins; *pin >= 0; pin++) {
		at91_mux_pio3_pin(IOMEM(SAMA5D3_BASE_PIOD),
					 pin_to_mask(*pin), instance->periph, 0);
	}

	sama5d3_pmc_enable_periph_clock(instance->id);

	ret = at91_mci_bio_init(&bio, instance->base, clock, slot);
	if (ret)
		goto out_panic;

	at91_fat_start_image(&bio, buf, SZ_16M, boot_src);

out_panic:
	panic("FAT chainloading failed\n");
}

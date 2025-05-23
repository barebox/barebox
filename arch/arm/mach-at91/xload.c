// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <filetype.h>
#include <mach/at91/xload.h>
#include <mach/at91/sama5_bootsource.h>
#include <mach/at91/hardware.h>
#include <mach/at91/sama5d2_ll.h>
#include <mach/at91/sama5d3_ll.h>
#include <mach/at91/gpio.h>
#include <linux/sizes.h>
#include <asm/cache.h>
#include <pbl/bio.h>

struct xload_instance {
	void __iomem *base;
	unsigned id;
	u8 periph;
	s8 pins[15];
};

static void at91_fat_start_image(struct pbl_bio *bio, void *buf, u32 r4)
{
	int ret;

	ret = pbl_fat_load(bio, "barebox.bin", buf, SZ_2M);
	if (ret < 0) {
		pr_err("pbl_fat_load: error %d\n", ret);
		return;
	}

	sync_caches_for_execution();

	sama5_boot_xload(buf, r4);
}

static const struct xload_instance sama5d2_mci_instances[] = {
	[0] = {
		.base = SAMA5D2_BASE_SDHC0,
		.id = SAMA5D2_ID_SDMMC0,
		.periph = AT91_MUX_PERIPH_A,
		.pins = {
			AT91_PIN_PA2, AT91_PIN_PA3, AT91_PIN_PA4, AT91_PIN_PA5,
			AT91_PIN_PA6, AT91_PIN_PA7, AT91_PIN_PA8, AT91_PIN_PA9,
			AT91_PIN_PA0, AT91_PIN_PA1, AT91_PIN_PA13, AT91_PIN_PA10,
			AT91_PIN_PA11, AT91_PIN_PA12, -1
		}
	},
	[1] = {
		.base = SAMA5D2_BASE_SDHC1,
		.id = SAMA5D2_ID_SDMMC1,
		.periph = AT91_MUX_PERIPH_E,
		.pins = {
			AT91_PIN_PA18, AT91_PIN_PA19, AT91_PIN_PA20,
			AT91_PIN_PA21, AT91_PIN_PA22, AT91_PIN_PA28,
			AT91_PIN_PA30, -1
		}
	},
};

/**
 * sama5d2_sdhci_start_image - Load and start an image from FAT-formatted SDHCI
 * @r4: value of r4 passed by BootROM
 */
static void __noreturn sama5d2_sdhci_start_image(u32 r4)
{
	void *buf = (void *)SAMA5_DDRCS;
	const struct xload_instance *instance;
	struct pbl_bio bio;
	const s8 *pin;
	int ret;

	ret = sama5_bootsource_instance(r4);
	if (ret > ARRAY_SIZE(sama5d2_mci_instances) - 1)
		panic("Couldn't determine boot MCI instance\n");

	instance = &sama5d2_mci_instances[ret];

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

	at91_fat_start_image(&bio, buf, r4);

out_panic:
	panic("FAT chainloading failed\n");
}

static const struct xload_instance sama5d2_qspi_ioset1_instances[] = {
	[0] = {
		.base = SAMA5D2_BASE_QSPI0,
		.id = SAMA5D2_ID_QSPI0,
		.periph = AT91_MUX_PERIPH_B,
		.pins = {
			AT91_PIN_PA0, AT91_PIN_PA1, AT91_PIN_PA2,
			AT91_PIN_PA3, AT91_PIN_PA4, AT91_PIN_PA5, -1
		}
	},
	[1] = {
		.base = SAMA5D2_BASE_QSPI1,
		.id = SAMA5D2_ID_QSPI1,
		.periph = AT91_MUX_PERIPH_B,
		.pins = {
			AT91_PIN_PA6, AT91_PIN_PA7, AT91_PIN_PA8,
			AT91_PIN_PA9, AT91_PIN_PA10, AT91_PIN_PA11, -1
		}
	},
};

/**
 * sama5d2_qspi_start_image - Start an image from QSPI NOR flash
 * @r4: value of r4 passed by BootROM
 */
static void __noreturn sama5d2_qspi_start_image(u32 r4)
{
	void __iomem *mem, *dest = IOMEM(SAMA5_DDRCS);
	const struct xload_instance *instance;
	const s8 *pin;
	u32 offs;
	int ret;

	ret = sama5_bootsource_instance(r4);
	if (ret == 0)
		mem = SAMA5D2_BASE_QSPI0_MEM;
	else if (ret == 1)
		mem = SAMA5D2_BASE_QSPI1_MEM;
	else
		panic("Couldn't determine boot QSPI instance\n");

	instance = &sama5d2_qspi_ioset1_instances[ret];

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_PIOA);
	for (pin = instance->pins; *pin >= 0; pin++)
		at91_mux_pio4_set_periph(SAMA5D2_BASE_PIOA,
					 BIT(*pin), instance->periph);

	sama5d2_pmc_enable_periph_clock(instance->id);

	/*
	 * Since we booted from QSPI, we expect the QSPI registers to be
	 * properly initialized already.
	 * Let's just read the memory-mapped data.
	 */

	/* Find barebox pattern first */
	for (offs = SZ_128K; offs <= SZ_1M; offs += SZ_128K) {
		/* Fix cache coherency issue by reading each sector only once */
		memcpy(dest, mem + offs, SZ_128K);

		if (is_barebox_arm_head(dest)) {
			u32 size = readl(dest + ARM_HEAD_SIZE_OFFSET);

			pr_info("Image found at 0x%08x, size %u\n", offs, size);

			/* Copy remaining barebox code */
			if (size > SZ_128K)
				memcpy(dest + SZ_128K, mem + offs + SZ_128K,
				       size - SZ_128K);

			sync_caches_for_execution();

			sama5_boot_xload(dest, r4);
		}
	}

	panic("No barebox image found!\n");
}

void __noreturn sama5d2_start_image(u32 r4)
{
	switch (sama5_bootsource(r4)) {
	case BOOTSOURCE_MMC:
		sama5d2_sdhci_start_image(r4);
		break;
	case BOOTSOURCE_SPI:
		sama5d2_qspi_start_image(r4);
		break;
	default:
		break;
	}

	panic("Unsupported boot configuration!\n");
}

static const struct xload_instance sama5d3_mci_instances[] = {
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

void __noreturn sama5d3_atmci_start_image(u32 r4, unsigned int clock,
					  unsigned int slot)
{
	void *buf = (void *)SAMA5_DDRCS;
	const struct xload_instance *instance;
	struct pbl_bio bio;
	const s8 *pin;
	int ret;

	ret = sama5_bootsource_instance(r4);
	if (ret > ARRAY_SIZE(sama5d3_mci_instances) - 1)
		panic("Couldn't determine boot MCI instance\n");

	instance = &sama5d3_mci_instances[ret];

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_PIOD);
	for (pin = instance->pins; *pin >= 0; pin++) {
		at91_mux_pio3_pin(IOMEM(SAMA5D3_BASE_PIOD),
				  pin_to_mask(*pin), instance->periph, 0);
	}

	sama5d3_pmc_enable_periph_clock(instance->id);

	ret = at91_mci_bio_init(&bio, instance->base, clock, slot,
				PBL_MCI_UNKNOWN_CAPACITY);
	if (ret)
		goto out_panic;

	at91_fat_start_image(&bio, buf, r4);

out_panic:
	panic("FAT chainloading failed\n");
}

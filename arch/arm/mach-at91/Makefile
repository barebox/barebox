# SPDX-License-Identifier: GPL-2.0-only

obj-y += setup.o aic.o
lwl-y += at91_pmc_ll.o ddramc_ll.o at91sam9_sdramc_ll.o matrix.o
lwl-$(CONFIG_CLOCKSOURCE_ATMEL_PIT) += early_udelay.o

obj-$(CONFIG_HAVE_AT91_LEGACY_CLK) += clock.o

obj-$(CONFIG_CMD_AT91_BOOT_TEST) += boot_test_cmd.o

obj-$(CONFIG_AT91_BOOTSTRAP)	+= bootstrap.o
obj-$(CONFIG_BOOTM) += bootm-barebox.o

obj-y += at91sam9_reset.o
obj-y += at91sam9g45_reset.o
obj-pbl-$(CONFIG_HAVE_AT91_DDRAMC) += ddramc.o
obj-pbl-$(CONFIG_HAVE_AT91_SDRAMC) += sdramc.o
pbl-$(CONFIG_AT91_MCI_PBL) +=  xload-mmc.o
pbl-$(CONFIG_AT91_MCI_PBL) +=  at91sam9_xload_mmc.o

obj-$(CONFIG_AT91SAM9_SMC) += sam9_smc.o
obj-$(CONFIG_HAVE_AT91SAM9_RST) += at91sam9_rst.o

# CPU-specific support
obj-$(CONFIG_SOC_AT91RM9200)	+= at91rm9200.o at91rm9200_time.o at91rm9200_devices.o
obj-$(CONFIG_SOC_AT91SAM9261) += at91sam9261.o at91sam9261_devices.o
obj-$(CONFIG_SOC_AT91SAM9G10) += at91sam9261.o at91sam9261_devices.o
ifeq ($(CONFIG_AT91_MULTI_BOARDS),)
obj-$(CONFIG_SOC_AT91SAM9260) += at91sam9260.o at91sam9260_devices.o
obj-$(CONFIG_SOC_AT91SAM9263) += at91sam9263.o at91sam9263_devices.o
obj-$(CONFIG_SOC_SAMA5D3)	+= sama5d3.o sama5d3_devices.o
obj-$(CONFIG_SOC_SAMA5D4)	+= sama5d4.o sama5d4_devices.o
endif
lwl-$(CONFIG_SOC_AT91SAM9263)	+= sam9263_ll.o
lwl-$(CONFIG_SOC_SAMA5D2)	+= sama5d2_ll.o
obj-$(CONFIG_SOC_SAMA5D2)	+= sama5d2.o
lwl-$(CONFIG_SOC_SAMA5D3)	+= sama5d3_ll.o
obj-$(CONFIG_SOC_AT91SAM9G20) += at91sam9260.o at91sam9260_devices.o
obj-$(CONFIG_SOC_AT91SAM9G45) += at91sam9g45.o at91sam9g45_devices.o
obj-$(CONFIG_SOC_AT91SAM9X5)	+= at91sam9x5.o at91sam9x5_devices.o
obj-$(CONFIG_SOC_AT91SAM9N12)	+= at91sam9n12.o at91sam9n12_devices.o
obj-$(CONFIG_SOC_SAMA5_MULTI)	+= sama5_bootsource.o

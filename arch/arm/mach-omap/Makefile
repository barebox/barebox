# OMAP Architecture Makefile
#
# (C) Copyright 2008
# Texas Instruments, <www.ti.com>
# Nishanth Menon <x0nishan@ti.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#
obj-$(CONFIG_ARCH_OMAP) += syslib.o omap_devices.o omap_generic.o omap_fb.o
pbl-$(CONFIG_ARCH_OMAP) += syslib.o
obj-$(CONFIG_ARCH_OMAP3) += omap3_generic.o auxcr.o
pbl-$(CONFIG_ARCH_OMAP3) += omap3_generic.o auxcr.o
obj-$(CONFIG_ARCH_OMAP4) += omap4_generic.o omap4_clock.o
pbl-$(CONFIG_ARCH_OMAP4) += omap4_generic.o omap4_clock.o
obj-pbl-$(CONFIG_ARCH_AM33XX) += am33xx_generic.o am33xx_clock.o am33xx_mux.o am3xxx.o
obj-pbl-$(CONFIG_ARCH_AM35XX) += am3xxx.o am35xx_emif4.o
obj-$(CONFIG_ARCH_AM33XX) += am33xx_scrm.o
obj-$(CONFIG_ARCH_OMAP3) += omap3_clock.o
pbl-$(CONFIG_ARCH_OMAP3) += omap3_clock.o
obj-$(CONFIG_OMAP_GPMC) += gpmc.o devices-gpmc-nand.o
obj-$(CONFIG_SHELL_NONE) += xload.o
obj-$(CONFIG_MFD_TWL6030) += omap4_twl6030_mmc.o
obj-$(CONFIG_OMAP4_USBBOOT) += omap4_rom_usb.o
obj-$(CONFIG_OMAP3_USBBOOT) += omap3_xload_usb.o
pbl-$(CONFIG_OMAP3_USBBOOT) += omap3_xload_usb.o
obj-$(CONFIG_CMD_BOOT_ORDER) += boot_order.o
obj-$(CONFIG_BAREBOX_UPDATE_AM33XX_SPI_NOR_MLO) += am33xx_bbu_spi_mlo.o
obj-$(CONFIG_BAREBOX_UPDATE_AM33XX_NAND) += am33xx_bbu_nand.o
obj-$(CONFIG_BAREBOX_UPDATE_AM33XX_EMMC) += am33xx_bbu_emmc.o

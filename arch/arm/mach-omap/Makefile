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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#
obj-$(CONFIG_ARCH_OMAP) += syslib.o
obj-$(CONFIG_OMAP_CLOCK_SOURCE_S32K) += s32k_clksource.o
obj-$(CONFIG_ARCH_OMAP3) += omap3_core.o omap3_generic.o
obj-$(CONFIG_ARCH_OMAP4) += omap4_generic.o omap4_clock.o
obj-$(CONFIG_OMAP3_CLOCK_CONFIG) += omap3_clock_core.o omap3_clock.o
obj-$(CONFIG_OMAP_GPMC) += gpmc.o devices-gpmc-nand.o
obj-y += omap-uart.o gpio.o xload.o

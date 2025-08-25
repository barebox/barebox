# SPDX-License-Identifier: GPL-2.0-only

pbl-$(CONFIG_ARCH_SOCFPGA_CYCLONE5) += cyclone5-init.o cyclone5-freeze-controller.o cyclone5-scan-manager.o cyclone5-system-manager.o
pbl-$(CONFIG_ARCH_SOCFPGA_CYCLONE5) += cyclone5-clock-manager.o
obj-$(CONFIG_ARCH_SOCFPGA_CYCLONE5) += cyclone5-generic.o nic301.o cyclone5-bootsource.o cyclone5-reset-manager.o
lwl-y += cpu_init.o

pbl-$(CONFIG_ARCH_SOCFPGA_ARRIA10) += arria10-xload.o \
				      arria10-xload-emmc.o
obj-pbl-$(CONFIG_ARCH_SOCFPGA_ARRIA10) += arria10-bootsource.o \
	    				  arria10-clock-manager.o \
					  arria10-generic.o \
					  arria10-reset-manager.o \
					  arria10-init.o \
					  arria10-sdram.o

pbl-$(CONFIG_ARCH_SOCFPGA_AGILEX5) += soc64-system-manager.o \
				soc64-wrap-handoff.o \
				iossm_mailbox.o \
				mailbox_s10.o \
				agilex5-sdram.o \
				agilex5-secreg.o \
				agilex5-clock-manager.o \
				atf.o

obj-$(CONFIG_ARCH_SOCFPGA_AGILEX5) += secure_reg_helper.o \
			        mailbox_s10.o \
				      smc_api.o

ifdef CONFIG_ARCH_SOCFPGA_CYCLONE5
obj-$(CONFIG_ARCH_SOCFPGA_XLOAD) += xload.o
endif

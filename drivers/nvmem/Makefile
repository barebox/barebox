# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for nvmem drivers.
#

obj-$(CONFIG_NVMEM)		+= nvmem_core.o
nvmem_core-y			:= core.o regmap.o partition.o

obj-$(CONFIG_NVMEM_RMEM)	+= rmem.o

# Devices
obj-$(CONFIG_NVMEM_SNVS_LPGPR)	+= nvmem_snvs_lpgpr.o
nvmem_snvs_lpgpr-y		:= snvs_lpgpr.o

obj-$(CONFIG_IMX_OCOTP)		+= nvmem_ocotp.o
nvmem_ocotp-y			:= ocotp.o

obj-$(CONFIG_RAVE_SP_EEPROM)	+= nvmem-rave-sp-eeprom.o
nvmem-rave-sp-eeprom-y		:= rave-sp-eeprom.o

obj-$(CONFIG_EEPROM_93XX46)	+= nvmem_eeprom_93xx46.o
nvmem_eeprom_93xx46-y		:= eeprom_93xx46.o

obj-$(CONFIG_STM32_BSEC)	+= nvmem_bsec.o
nvmem_bsec-y			:= bsec.o

obj-$(CONFIG_KVX_OTP_NV)	+= nvmem-kvx-otp-nv.o
nvmem-kvx-otp-nv-y		:= kvx-otp-nv.o

obj-$(CONFIG_NVMEM_ROCKCHIP_OTP)+= rockchip-otp.o
obj-$(CONFIG_STARFIVE_OTP)	+= starfive-otp.o
obj-$(CONFIG_IMX_OCOTP_ELE)	+= imx-ocotp-ele.o

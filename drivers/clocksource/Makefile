# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_AMBA_SP804) += amba-sp804.o
obj-$(CONFIG_ARM_SMP_TWD) += arm_smp_twd.o
obj-$(CONFIG_CLOCKSOURCE_BCM283X) += bcm2835.o
obj-$(CONFIG_CLOCKSOURCE_CLPS711X) += clps711x.o
obj-$(CONFIG_CLOCKSOURCE_DIGIC)   += digic.o
obj-$(CONFIG_CLOCKSOURCE_EFI) += efi.o
obj-$(CONFIG_CLOCKSOURCE_EFI_X86) += efi_x86.o
obj-$(CONFIG_CLOCKSOURCE_KVX)     += kvx_timer.o
obj-$(CONFIG_CLOCKSOURCE_MVEBU)   += mvebu.o
obj-$(CONFIG_CLOCKSOURCE_NOMADIK) += nomadik.o
obj-$(CONFIG_CLOCKSOURCE_ORION)   += orion.o
obj-$(CONFIG_CLOCKSOURCE_UEMD)    += uemd.o
obj-$(CONFIG_CLOCKSOURCE_ROCKCHIP)+= rk_timer.o
obj-$(CONFIG_CLOCKSOURCE_ATMEL_PIT) += timer-atmel-pit.o
obj-$(CONFIG_CLOCKSOURCE_ARM_ARCHITECTED_TIMER) += arm_architected_timer.o
ifneq ($(CONFIG_CPU_V8),y)
CFLAGS_arm_architected_timer.o := -march=armv7-a
endif
obj-$(CONFIG_CLOCKSOURCE_ARM_GLOBAL_TIMER) += arm_global_timer.o
obj-$(CONFIG_CLOCKSOURCE_IMX_GPT) += timer-imx-gpt.o
obj-$(CONFIG_CLOCKSOURCE_DW_APB_TIMER) += dw_apb_timer.o
obj-pbl-$(CONFIG_CLOCKSOURCE_TI_DM) += timer-ti-dm.o
obj-$(CONFIG_CLOCKSOURCE_TI_32K) += timer-ti-32k.o
obj-$(CONFIG_CLINT_TIMER) += timer-clint.o
obj-$(CONFIG_RISCV_TIMER) += timer-riscv.o
obj-$(CONFIG_ARMV7M_SYSTICK) += armv7m_systick.o
obj-$(CONFIG_CLKSRC_STM32) += timer-stm32.o

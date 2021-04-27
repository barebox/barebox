# SPDX-License-Identifier: GPL-2.0

extra-y += barebox.lds

obj-y += dtb.o
obj-pbl-y += sections.o setupc.o reloc.o sections.o runtime-offset.o
obj-$(CONFIG_HAS_ARCH_SJLJ) += setjmp.o longjmp.o
obj-$(CONFIG_RISCV_OPTIMZED_STRING_FUNCTIONS) += memcpy.o memset.o memmove.o
obj-$(CONFIG_RISCV_SBI) += sbi.o
obj-$(CONFIG_CMD_RISCV_CPUINFO) += cpuinfo.o

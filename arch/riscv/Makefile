# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := rv64i_defconfig

KBUILD_CPPFLAGS += -fno-strict-aliasing

ifeq ($(CONFIG_ARCH_RV32I),y)
	KBUILD_CPPFLAGS += -mabi=ilp32
	riscv-ldflags-y += -melf32lriscv
else
	KBUILD_CPPFLAGS += -mabi=lp64
	riscv-ldflags-y += -melf64lriscv
endif

# ISA string setting
riscv-march-$(CONFIG_ARCH_RV32I)	:= rv32im
riscv-march-$(CONFIG_ARCH_RV64I)	:= rv64im


KBUILD_CPPFLAGS += -march=$(riscv-march-y)
KBUILD_CPPFLAGS += -Wstrict-prototypes -mcmodel=medany -fpic
riscv-ldflags-y += -pie -static

LDFLAGS_pbl += $(riscv-ldflags-y)
LDFLAGS_barebox += $(riscv-ldflags-y)

ifndef CONFIG_MODULES
# Add cleanup flags
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif

KBUILD_BINARY := barebox.bin
KBUILD_IMAGE := $(KBUILD_BINARY)

archprepare: maketools

PHONY += maketools

common-y += arch/riscv/boards/
common-y += arch/riscv/cpu/
common-y += arch/riscv/lib/
common-y += arch/riscv/boot/

common-$(CONFIG_OFTREE) += arch/riscv/dts/

lds-y	:= arch/riscv/lib/barebox.lds

CLEAN_FILES += arch/riscv/lib/barebox.lds

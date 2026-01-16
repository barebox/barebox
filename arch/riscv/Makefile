# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := rv64i_defconfig

generated_configs += virt32_mmu_defconfig rv64i_mmu_defconfig

virt32_mmu_defconfig:
	$(call merge_into_defconfig,virt32_defconfig,enable_mmu)
rv64i_mmu_defconfig:
	$(call merge_into_defconfig,rv64i_defconfig,enable_mmu)

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

# Newer binutils versions default to ISA spec version 20191213 which moves some
# instructions from the I extension to the Zicsr and Zifencei extensions.
toolchain-need-zicsr-zifencei := $(call cc-option-yn, -march=$(riscv-march-y)_zicsr_zifencei)
riscv-march-$(toolchain-need-zicsr-zifencei) := $(riscv-march-y)_zicsr_zifencei

KBUILD_CPPFLAGS += -march=$(riscv-march-y)
KBUILD_CFLAGS += -mno-save-restore
KBUILD_CFLAGS += -fno-asynchronous-unwind-tables -fno-unwind-tables
KBUILD_CFLAGS += $(call cc-option,-mstrict-align)
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

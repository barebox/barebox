KBUILD_DEFCONFIG := virt64_defconfig

KBUILD_CPPFLAGS += -fno-strict-aliasing

ifeq ($(CONFIG_ARCH_RV32I),y)
	riscv-cflags-y += -march=rv32im -mabi=ilp32
	riscv-ldflags-y += -melf32lriscv
else
	riscv-cflags-y += -march=rv64im -mabi=lp64
	riscv-ldflags-y += -melf64lriscv
endif

riscv-cflags-y += -Wstrict-prototypes -mcmodel=medany -fpic
riscv-ldflags-y += -pie -static

PBL_CPPFLAGS += $(riscv-cflags-y)
LDFLAGS_pbl += $(riscv-ldflags-y)

cflags-y += $(riscv-cflags-y)
LDFLAGS_barebox += -nostdlib

machine-$(CONFIG_MACH_ERIZO)	:= erizo
machine-$(CONFIG_MACH_VIRT)	:= virt

LDFLAGS_barebox += $(riscv-ldflags-y)

ifndef CONFIG_MODULES
# Add cleanup flags
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif

KBUILD_BINARY := barebox.bin

machdirs := $(patsubst %,arch/riscv/mach-%/,$(machine-y))

KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

archprepare: maketools

PHONY += maketools

ifneq ($(machine-y),)
MACH  := arch/riscv/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(MACH)
common-y += arch/riscv/boards/
common-y += arch/riscv/cpu/
common-y += arch/riscv/lib/
common-y += arch/riscv/boot/

common-$(CONFIG_OFTREE) += arch/riscv/dts/

KBUILD_CPPFLAGS += $(cflags-y)
KBUILD_CFLAGS += $(cflags-y)

lds-y	:= arch/riscv/lib/barebox.lds

CLEAN_FILES += arch/riscv/lib/barebox.lds

KBUILD_IMAGE := $(KBUILD_BINARY)

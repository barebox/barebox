# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := efi_defconfig

KBUILD_CPPFLAGS += -D__X86__

TEXT_BASE = $(CONFIG_TEXT_BASE)

machine-y := efi
KBUILD_CFLAGS += -fpic -mno-sse -mno-mmx
ifeq ($(CONFIG_X86_32),y)
	TARGET = efi-app-ia32
else
	TARGET = efi-app-x86_64
endif

ifeq ($(CONFIG_X86_32),y)
	UTS_MACHINE := i386
	biarch := $(call cc-option,-m32)
	KBUILD_AFLAGS += $(biarch)
	KBUILD_CFLAGS += $(biarch)
	KBUILD_CPPFLAGS += $(biarch)
else
	UTS_MACHINE := x86_64
	KBUILD_AFLAGS += -m64
	KBUILD_CFLAGS += -m64 -mno-red-zone
	KBUILD_CPPFLAGS += -m64
endif


ifndef CONFIG_MODULES
# Add cleanup flags
ifeq ($(CONFIG_X86_EFI),y)
LDFLAGS_barebox += -shared -Bsymbolic -znocombreloc
else
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif
endif

machdirs := $(patsubst %,arch/x86/mach-%/,$(machine-y))

KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

ifneq ($(machine-y),)
MACH  := arch/x86/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(MACH)
common-y += arch/x86/lib/

# arch/x86/cpu/

lds-$(CONFIG_X86_32)   := arch/x86/mach-efi/elf_ia32_efi.lds
lds-$(CONFIG_X86_64)   := arch/x86/mach-efi/elf_x86_64_efi.lds

KBUILD_BINARY := barebox

KBUILD_LDFLAGS := --no-undefined

OBJCOPYFLAGS_barebox.efi = \
	-j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela \
	-j .reloc -j .bbenv.* -j .bblogo.* -j .barebox_imd --target=$(TARGET)

barebox.efi: $(KBUILD_BINARY) FORCE
	$(call if_changed,objcopy)

KBUILD_IMAGE := barebox.efi

KBUILD_LDFLAGS += -m elf_$(UTS_MACHINE)

CLEAN_FILES += $(lds-y)

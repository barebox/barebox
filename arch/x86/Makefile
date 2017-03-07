KBUILD_DEFCONFIG := generic_defconfig

CPPFLAGS += -D__X86__

board-$(CONFIG_MACH_X86_GENERIC) := x86_generic

TEXT_BASE = $(CONFIG_TEXT_BASE)

ifeq ($(CONFIG_X86_EFI),y)
machine-y := efi
CFLAGS += -fpic -fshort-wchar -mno-sse -mno-mmx
ifeq ($(CONFIG_X86_32),y)
	TARGET = efi-app-ia32
else
	TARGET = efi-app-x86_64
endif
else
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -march=i386 -DTEXT_BASE=$(TEXT_BASE) \
	-fno-unwind-tables -fno-asynchronous-unwind-tables

machine-y := i386
endif

ifeq ($(CONFIG_X86_32),y)
	UTS_MACHINE := i386
	biarch := $(call cc-option,-m32)
	AFLAGS += $(biarch)
	CFLAGS += $(biarch)
	CPPFLAGS += $(biarch)
else
	UTS_MACHINE := x86_64
	AFLAGS += -m64
	CFLAGS += -m64 -mno-red-zone
	CPPFLAGS += -m64
endif


ifndef CONFIG_MODULES
# Add cleanup flags
ifneq ($(CONFIG_X86_EFI),y)
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif
endif

machdirs := $(patsubst %,arch/x86/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

ifneq ($(board-y),)
BOARD  := arch/x86/boards/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/x86/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/x86/lib/
common-$(CONFIG_X86_BIOS_BRINGUP) += arch/x86/bios/

# arch/x86/cpu/

ifeq ($(CONFIG_X86_EFI),y)
lds-$(CONFIG_X86_32)   := arch/x86/mach-efi/elf_ia32_efi.lds
lds-$(CONFIG_X86_64)   := arch/x86/mach-efi/elf_x86_64_efi.lds

cmd_barebox__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_barebox) -o $@	\
		-T $(lds-y)					\
		-shared -Bsymbolic -nostdlib -znocombreloc	\
		--start-group $(barebox-common)			\
		--end-group					\
		$(filter-out $(barebox-lds) $(barebox-common) FORCE ,$^)

quiet_cmd_efi_image = EFI-IMG $@
      cmd_efi_image = $(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
		      -j .dynsym -j .rel -j .rela -j .reloc -j __barebox_initcalls \
		      -j __barebox_exitcalls -j __barebox_cmd -j .barebox_magicvar \
		      -j .bbenv.* -j .bblogo.* --target=$(TARGET) $< $@

KBUILD_BINARY := barebox

LDFLAGS := --no-undefined

barebox.efi: $(KBUILD_BINARY) FORCE
	$(call if_changed,efi_image)

KBUILD_IMAGE := barebox.efi

else
common-y += arch/x86/boot/

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)   := arch/x86/lib/barebox.lds
lds-$(CONFIG_BOARD_LINKER_SCRIPT)     := $(BOARD)/barebox.lds

endif

LDFLAGS += -m elf_$(UTS_MACHINE)

CLEAN_FILES += $(lds-y)
CLEAN_FILES    += arch/x86/lib/barebox.lds barebox.map barebox.S


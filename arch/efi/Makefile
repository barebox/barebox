KBUILD_DEFCONFIG := efi_defconfig

CFLAGS += -fpic -fshort-wchar -mno-sse -mno-mmx

ifeq ($(CONFIG_X86_32),y)
	UTS_MACHINE := i386
	biarch := $(call cc-option,-m32)
	AFLAGS += $(biarch)
	CFLAGS += $(biarch)
	TARGET = efi-app-ia32
else
	UTS_MACHINE := x86_64
	AFLAGS += -m64
	CFLAGS += -m64 -mno-red-zone
	TARGET = efi-app-x86_64
endif

lds-$(CONFIG_X86_32)   := arch/efi/lib/elf_ia32_efi.lds
lds-$(CONFIG_X86_64)   := arch/efi/lib/elf_x86_64_efi.lds

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
		      -j .bbenv.* --target=$(TARGET) $< $@

KBUILD_BINARY := barebox

LDFLAGS := -m elf_$(UTS_MACHINE) --no-undefined

barebox.efi: $(KBUILD_BINARY) FORCE
	$(call if_changed,efi_image)

KBUILD_IMAGE := barebox.efi

common-y += arch/efi/efi/ arch/efi/lib/

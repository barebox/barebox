
CPPFLAGS += -ffreestanding -nostdinc -Wall 	\
	-pipe -fno-strict-aliasing


board-$(CONFIG_MACH_IPE337)	:= ipe337
cpu-$(CONFIG_BF561)		:= bf561

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P
CFLAGS += -D__blackfin__
# -Ttext $(TEXT_BASE)
KALLSYMS         += --symbol-prefix=_

ifndef CONFIG_BFIN_BOOT_BYPASS
all: uboot.ldr
endif

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-blackfin/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-blackfin/arch -> include/asm-blackfin/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-blackfin
	$(Q)ln -fsn $(srctree)/include/asm-blackfin/$(INCDIR) include/asm-blackfin/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-blackfin/arch
endif
	@touch $@

archprepare: maketools

PHONY += maketools
maketools: include/asm-blackfin/.arch


ifneq ($(board-y),)
BOARD  := board/$(board-y)/
else
BOARD  :=
endif

ifneq ($(cpu-y),)
CPU  := arch/blackfin/cpu-$(cpu-y)/
else
CPU  :=
endif

common-y += $(BOARD)
common-y += arch/blackfin/lib/ $(CPU)

lds-y += $(BOARD)/u-boot.lds

MRPROPER_FILES += include/asm-blackfin/arch include/asm-blackfin/proc

ifdef CONFIG_BFIN_BOOT_FLASH16
FLASHBITS :=-B 16
else
FLASHBITS :=
endif

ifneq ($(cpu-y),)
ifndef CONFIG_BFIN_BOOT_BYPASS
uboot.ldr: uboot
	rm -f $@
	$(CROSS_COMPILE)ldr -T $(cpu-y) -c $(FLASHBITS) -i $(CPU)/init_sdram.o $@ $<
endif
endif

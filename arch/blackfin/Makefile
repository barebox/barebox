
CPPFLAGS += -fno-strict-aliasing


board-$(CONFIG_MACH_IPE337)	:= ipe337
cpu-$(CONFIG_BF561)		:= bf561

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)
CFLAGS += -D__blackfin__
# -Ttext $(TEXT_BASE)
KALLSYMS         += --symbol-prefix=_

ifndef CONFIG_BFIN_BOOT_BYPASS
KBUILD_IMAGE := barebox.ldr
endif

archprepare: maketools

PHONY += maketools


ifneq ($(board-y),)
BOARD  := arch/blackfin/boards/$(board-y)/
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

lds-y += $(BOARD)/barebox.lds

ifdef CONFIG_BFIN_BOOT_FLASH16
FLASHBITS :=-B 16
else
FLASHBITS :=
endif

ifneq ($(cpu-y),)
ifndef CONFIG_BFIN_BOOT_BYPASS
barebox.ldr: barebox
	rm -f $@
	$(CROSS_COMPILE)ldr -T $(cpu-y) -c $(FLASHBITS) -i $(CPU)/init_sdram.o $@ $<
endif
endif

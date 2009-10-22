
CPPFLAGS += -fno-strict-aliasing


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

archprepare: maketools

PHONY += maketools


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

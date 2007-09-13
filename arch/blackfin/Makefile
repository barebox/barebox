
CPPFLAGS += -fno-builtin -ffreestanding -nostdinc -Wall 	\
	-isystem $(gccincdir) -pipe \
	-fno-strict-aliasing


board-$(CONFIG_MACH_IPE337)	:= ipe337
cpu-$(CONFIG_BF561)		:= bf561

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P
CFLAGS := -fno-common -Os -D__blackfin__
LDFLAGS_uboot  :=-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc -Ttext $(TEXT_BASE)

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

MRPROPER_FILES += include/asm-arm/arch include/asm-arm/proc

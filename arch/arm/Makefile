
CPPFLAGS += -fno-builtin -ffreestanding -nostdinc -Wall 	\
	-isystem $(gccincdir) -pipe -D __ARM__ -D CONFIG_ARM \
	-ffixed-r8


machine-$(CONFIG_ARCH_IMX)	   := imx
machine-$(CONFIG_ARCH_AT91RM9200)  := at91rm9200
board-$(CONFIG_MACH_MX1ADS)	   := mx1ads
board-$(CONFIG_MACH_ECO920)	   := eco920
board-$(CONFIG_MACH_SCB9328)	   := scb9328
cpu-$(CONFIG_ARM920T)              := arm920t

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -mabi=apcs-gnu -DTEXT_BASE=$(TEXT_BASE)
CFLAGS := -fno-common -msoft-float -Os
LDFLAGS_vmlinux  :=-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc -Ttext $(TEXT_BASE)

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-arm/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-arm/arch -> include/asm-arm/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	echo incdir1: $(INCDIR)
	$(Q)mkdir -p include/asm-arm
	$(Q)ln -fsn $(srctree)/include/asm-arm/$(INCDIR) include/asm-arm/arch
else
	echo incdir: $(INCDIR)
	$(Q)ln -fsn $(INCDIR) include/asm-arm/arch
endif
	@touch $@

archprepare: maketools

PHONY += maketools
maketools: include/asm-arm/.arch


ifneq ($(board-y),)
BOARD  := board/$(board-y)/
else
BOARD  :=
endif

ifneq ($(cpu-y),)
CPU  := cpu/$(cpu-y)/
else
CPU  :=
endif

common-y += $(BOARD) $(CPU)
common-y += arch/arm/lib/

MRPROPER_FILES += include/asm-arm/arch

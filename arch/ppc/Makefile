
CPPFLAGS += -fno-builtin -ffreestanding -nostdinc -Wall -ffixed-r14 	\
	-isystem $(gccincdir) -meabi -Wstrict-prototypes -D __PPC__ -D CONFIG_PPC \
	-fno-strict-aliasing

ifdef CONFIG_RELOCATABLE
CPPFLAGS += -fPIC -mrelocatable
endif


machine-$(CONFIG_ARCH_MPC5200)			:= mpc5200
board-$(CONFIG_MACH_PHYCORE_MPC5200B_TINY)	:= phycore_mpc5200b_tiny
cpu-$(CONFIG_ARCH_MPC5200)			:= mpc5xxx

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P
CFLAGS := -fno-common -Os
LDFLAGS_uboot  :=-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-ppc/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-ppc/arch -> include/asm-ppc/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-ppc
	$(Q)ln -fsn $(srctree)/include/asm-ppc/$(INCDIR) include/asm-ppc/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-ppc/arch
endif
	@touch $@

archprepare: maketools

PHONY += maketools
maketools: include/asm-ppc/.arch


ifneq ($(board-y),)
BOARD  := board/$(board-y)/
else
BOARD  :=
endif

ifneq ($(cpu-y),)
CPU  := arch/ppc/mach-$(cpu-y)/
else
CPU  :=
endif

common-y += $(BOARD) $(CPU)
common-y += arch/ppc/lib/

MRPROPER_FILES += include/asm-ppc/arch

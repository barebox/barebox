KBUILD_DEFCONFIG := p2020rdb_defconfig

KBUILD_CPPFLAGS += -ffixed-r14 -m32 	\
	-meabi -D __PPC__ \
	-fno-strict-aliasing

KBUILD_CPPFLAGS += $(call cc-option,-mno-spe)
KBUILD_CPPFLAGS += $(call cc-option,-mspe=no)

ifdef CONFIG_RELOCATABLE
KBUILD_CPPFLAGS += -fPIC -mrelocatable
endif

ifdef CONFIG_MPC85xx
KBUILD_CPPFLAGS += -Wa,-me500x2 -msoft-float -mno-string
endif

board-$(CONFIG_MACH_PHYCORE_MPC5200B_TINY)	:= pcm030
board-$(CONFIG_P1010RDB)			:= freescale-p1010rdb
board-$(CONFIG_P2020RDB)			:= freescale-p2020rdb
board-$(CONFIG_P1022DS)				:= freescale-p1022ds
board-$(CONFIG_DA923RC)				:= owc-da923rc

machine-$(CONFIG_ARCH_MPC5200)			:= mpc5xxx
machine-$(CONFIG_ARCH_MPC85XX)			:= mpc85xx

TEXT_BASE = $(CONFIG_TEXT_BASE)

KBUILD_CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

# Add cleanup flags
ifndef CONFIG_MODULES
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += --gc-sections
endif

machdirs := $(patsubst %,arch/powerpc/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
KBUILD_CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

archprepare: maketools

PHONY += maketools


ifneq ($(board-y),)
BOARD  := arch/powerpc/boards/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/powerpc/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(BOARD) $(MACH) arch/powerpc/

ifdef CONFIG_MPC85xx
lds-y += $(MACH)/barebox.lds
else
lds-y += $(BOARD)/barebox.lds
endif


CPPFLAGS += -ffixed-r14 -m32 	\
	-meabi -D __PPC__ -D CONFIG_PPC \
	-fno-strict-aliasing -mno-spe -mspe=no

ifdef CONFIG_RELOCATABLE
CPPFLAGS += -fPIC -mrelocatable
endif

ifdef CONFIG_MPC85xx
CPPFLAGS += -Wa,-me500x2 -msoft-float -mno-string
endif

board-$(CONFIG_MACH_PHYCORE_MPC5200B_TINY)	:= pcm030
board-$(CONFIG_P2020RDB)			:= freescale-p2020rdb

machine-$(CONFIG_ARCH_MPC5200)			:= mpc5xxx
machine-$(CONFIG_ARCH_MPC85XX)			:= mpc85xx

cpu-$(CONFIG_ARCH_MPC85XX)			:= 85xx

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

# Add cleanup flags
ifndef CONFIG_MODULES
CPPFLAGS += -fdata-sections -ffunction-sections
endif

machdirs := $(patsubst %,arch/ppc/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

archprepare: maketools

PHONY += maketools


ifneq ($(board-y),)
BOARD  := arch/ppc/boards/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/ppc/mach-$(machine-y)/
else
MACH  :=
endif

ifneq ($(cpu-y),)
CPU  := arch/ppc/cpu-$(cpu-y)/
else
CPU  :=
endif

common-y += $(BOARD) $(CPU) $(MACH)
common-y += arch/ppc/lib/

lds-y += $(BOARD)/barebox.lds

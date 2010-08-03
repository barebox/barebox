
CPPFLAGS += -ffixed-r14 	\
	-meabi -D __PPC__ -D CONFIG_PPC \
	-fno-strict-aliasing

ifdef CONFIG_RELOCATABLE
CPPFLAGS += -fPIC -mrelocatable
endif


machine-$(CONFIG_ARCH_MPC5200)			:= mpc5200
board-$(CONFIG_MACH_PHYCORE_MPC5200B_TINY)	:= pcm030
cpu-$(CONFIG_ARCH_MPC5200)			:= mpc5xxx

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P

# Add cleanup flags
ifndef CONFIG_MODULES
CPPFLAGS += -fdata-sections -ffunction-sections
endif

machdirs := $(patsubst %,arch/ppc/mach-%/,$(cpu-y))

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

ifneq ($(cpu-y),)
CPU  := arch/ppc/mach-$(cpu-y)/
else
CPU  :=
endif

common-y += $(BOARD) $(CPU)
common-y += arch/ppc/lib/

lds-y += $(BOARD)/barebox.lds

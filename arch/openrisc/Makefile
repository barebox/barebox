CPPFLAGS += -D__OR1K__ -ffixed-r10 -mhard-mul -mhard-div

LIBGCC          := $(shell $(CC) $(KBUILD_CFLAGS) -print-libgcc-file-name)

board-$(CONFIG_GENERIC) := generic

KALLSYMS += --symbol-prefix=_

archprepare: maketools

PHONY += maketools

ifneq ($(board-y),)
BOARD  := arch/openrisc/boards/$(board-y)/
else
BOARD  :=
endif

common-y += $(BOARD)
common-y += arch/openrisc/lib/
common-y += arch/openrisc/cpu/

common-y          += $(LIBGCC)

lds-y += arch/openrisc/cpu/barebox.lds

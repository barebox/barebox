CPPFLAGS += -D__OR1K__ -ffixed-r10 -mhard-mul -mhard-div

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

lds-y += arch/openrisc/cpu/barebox.lds

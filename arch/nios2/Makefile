CPPFLAGS += -fno-strict-aliasing -mno-hw-mul

board-$(CONFIG_GENERIC) := generic

KALLSYMS += --symbol-prefix=_

archprepare: maketools

	@$(kecho) "  SYMLINK include/nios_sopc.h -> arch/nios2/boards/$(board-y)/nios_sopc.h"
	@ln -fsn $(src)/arch/nios2/boards/$(board-y)/nios_sopc.h $(obj)/include/nios_sopc.h

PHONY += maketools

ifneq ($(board-y),)
BOARD  := arch/nios2/boards/$(board-y)/
else
BOARD  :=
endif

common-y += $(BOARD)
common-y += arch/nios2/lib/
common-y += arch/nios2/cpu/

lds-y += arch/nios2/cpu/barebox.lds


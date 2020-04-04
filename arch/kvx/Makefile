KBUILD_DEFCONFIG := generic_defconfig

CPPFLAGS += -fno-strict-aliasing

board-$(CONFIG_GENERIC) := generic

KALLSYMS += --symbol-prefix=_

ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE    := kvx-elf-
endif

DEFAULT_CFLAGS := -nostdlib -fno-builtin -fstrict-align -g
DEFAULT_CFLAGS += -DTEXT_BASE=$(CONFIG_TEXT_BASE)

LIBGCC_PATH = $(dir $(shell $(CC) $(CFLAGS) --print-libgcc-file-name))

CFLAGS += $(DEFAULT_CFLAGS)
AFLAGS += $(DEFAULT_CFLAGS)

LDFLAGS += -m elf64kvx

archprepare: maketools

PHONY += maketools

common-y += arch/kvx/lib/
common-y += arch/kvx/cpu/
common-$(CONFIG_OFTREE) += arch/kvx/dts/

lds-y += arch/kvx/cpu/barebox.lds

cmd_barebox__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_barebox) -o $@ \
      -T $(BAREBOX_LDS) \
      --start-group $(BAREBOX_OBJS) --end-group \
      -L$(LIBGCC_PATH) -lgcc \
      $(filter-out $(BAREBOX_LDS) $(BAREBOX_OBJS) FORCE ,$^)

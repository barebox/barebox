# SPDX-License-Identifier: GPL-2.0
#
# Kbuild for top-level directory of Barebox

# Prepare global headers and check sanity before descending into sub-directories
# ---------------------------------------------------------------------------

targets :=

# Generate asm-offsets.h

offsets-file := include/generated/asm-offsets.h

targets += arch/$(SRCARCH)/lib/asm-offsets.s

$(offsets-file): arch/$(SRCARCH)/lib/asm-offsets.s FORCE
	$(call filechk,offsets,__ASM_OFFSETS_H__)

# A phony target that depends on all the preparation targets

PHONY += prepare
prepare: $(offsets-file)
	@:

# Ordinary directory descending
# ---------------------------------------------------------------------------

obj-y					+= common/
obj-y					+= drivers/
obj-y					+= commands/
obj-y					+= lib/
obj-y					+= security/
obj-y					+= crypto/
obj-y					+= net/
obj-y					+= fs/
obj-y					+= firmware/

obj-y					+= arch/$(SRCARCH)/
obj-y					+= $(ARCH_CORE)
obj-$(CONFIG_EFI)			+= efi/
obj-y					+= test/

obj-$(CONFIG_PBL_IMAGE)			+= pbl/
obj-$(CONFIG_DEFAULT_ENVIRONMENT)	+= defaultenv/

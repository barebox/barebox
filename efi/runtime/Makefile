# SPDX-License-Identifier: GPL-2.0

lib-y += efi_var_mem.o efi_variable.o

KASAN_SANITIZE := n
UBSAN_SANITIZE := n

KBUILD_CFLAGS += $(picflags-y) -include $(srctree)/efi/runtime/common.h

# Even when -mbranch-protection=none is set, Clang will generate a
# .note.gnu.property for code-less object files (like lib/ctype.c),
# so work around this by explicitly removing the unwanted section.
# https://bugs.llvm.org/show_bug.cgi?id=46480
EFIRTCP_FLAGS-y	+= --remove-section=.note.gnu.property

# Cover compiler-generaed symbols like __func__ as well
EFIRTCP_FLAGS-y	+= --prefix-alloc-sections .efi_runtime

EFIRTCP_FLAGS-y	+= --redefine-sym memset=__pi_memset \
		   --redefine-sym memcpy=__pi_memcpy \
		   --redefine-sym memmove=__pi_memmove \
		   --redefine-sym crc32=__pi_crc32

#
# Strip debug sections and some other sections that may legally contain
# absolute relocations, so that we can inspect the remaining sections for
# such relocations. If none are found, regenerate the output object, but
# this time, use objcopy and leave all sections in place.
#
quiet_cmd_efirtcopy = EFIRTCP $@
      cmd_efirtcopy =							\
	$(STRIP) --strip-debug -o $@ $<;				\
	if [ -n "$(EFIRTCP_RELOC-y)" ] && \
	    $(OBJDUMP) -r $@ | grep $(EFIRTCP_RELOC-y); then		\
		echo "$@: absolute symbol references not allowed in the EFI runtime" >&2; \
		/bin/false;						\
	fi;								\
	$(OBJCOPY) $(EFIRTCP_FLAGS-y) $< $@

$(obj)/%.efirt.o: $(obj)/%.o FORCE
	$(call if_changed,efirtcopy)

obj-y := $(patsubst %.o,%.efirt.o,$(lib-y))

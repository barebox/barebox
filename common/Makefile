obj-$(CONFIG_BINFMT)		+= binfmt.o
obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o
obj-$(CONFIG_ENV_HANDLING)	+= environment.o
obj-$(CONFIG_AUTO_COMPLETE)	+= complete.o
obj-$(CONFIG_POLLER)		+= poller.o
obj-$(CONFIG_BLOCK)		+= block.o
obj-$(CONFIG_PARTITION_DISK)	+= partitions.o

obj-$(CONFIG_CMD_LOADS)		+= s_record.o
obj-$(CONFIG_OFTREE)		+= oftree.o

obj-y += memory.o
obj-$(CONFIG_MALLOC_DLMALLOC) += dlmalloc.o
obj-$(CONFIG_MALLOC_TLSF) += tlsf_malloc.o
obj-$(CONFIG_MALLOC_TLSF) += tlsf.o
obj-$(CONFIG_MALLOC_DUMMY) += dummy_malloc.o
obj-y += clock.o
obj-$(CONFIG_BANNER) += version.o
obj-$(CONFIG_COMMAND_SUPPORT) += command.o
obj-$(CONFIG_CONSOLE_FULL) += console.o
obj-$(CONFIG_CONSOLE_SIMPLE) += console_simple.o
obj-$(CONFIG_DIGEST) += digest.o
obj-$(CONFIG_ENVIRONMENT_VARIABLES) += env.o
obj-$(CONFIG_UIMAGE) += image.o
obj-$(CONFIG_UIMAGE) += uimage.o
obj-y += startup.o
obj-y += misc.o
obj-y += memsize.o
obj-$(CONFIG_GLOBALVAR) += globalvar.o
obj-$(CONFIG_RESET_SOURCE) += reset_source.o
obj-$(CONFIG_FILETYPE) += filetype.o
obj-y += resource.o
obj-$(CONFIG_MENU) += menu.o
obj-$(CONFIG_PASSWORD) += password.o
obj-$(CONFIG_MODULES) += module.o
obj-$(CONFIG_FLEXIBLE_BOOTARGS) += bootargs.o
extra-$(CONFIG_MODULES) += module.lds

ifdef CONFIG_DEFAULT_ENVIRONMENT
$(obj)/startup.o: include/generated/barebox_default_env.h
$(obj)/env.o: include/generated/barebox_default_env.h

ifeq ($(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW),y)
DEFAULT_ENVIRONMENT_PATH = "defaultenv-2/base"
ifeq ($(CONFIG_CMD_MENU_MANAGEMENT),y)
DEFAULT_ENVIRONMENT_PATH += "defaultenv-2/menu"
endif
endif

ifeq ($(CONFIG_DEFAULT_ENVIRONMENT_GENERIC),y)
DEFAULT_ENVIRONMENT_PATH = "defaultenv"
endif

ifneq ($(CONFIG_DEFAULT_ENVIRONMENT_PATH),"")
DEFAULT_ENVIRONMENT_PATH += $(CONFIG_DEFAULT_ENVIRONMENT_PATH)
endif

ENV_FILES := $(shell cd $(srctree); for i in $(DEFAULT_ENVIRONMENT_PATH); do find $${i} -type f -exec readlink -f '{}' \;; done)

endif # ifdef CONFIG_DEFAULT_ENVIRONMENT

barebox_default_env: $(ENV_FILES)
	$(Q)$(srctree)/scripts/genenv $(srctree) $(objtree) $(DEFAULT_ENVIRONMENT_PATH)

barebox_default_env_comp =
ifeq ($(CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED_GZIP),y)
barebox_default_env_comp = .gz
endif
ifeq ($(CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED_BZIP2),y)
barebox_default_env_comp = .bz2
endif
ifeq ($(CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED_LZO),y)
barebox_default_env_comp = .lzo
endif

barebox_default_env.gz: barebox_default_env
	$(call if_changed,gzip)

barebox_default_env.bz2: barebox_default_env
	$(call if_changed,bzip2)

barebox_default_env.lzo: barebox_default_env
	$(call if_changed,lzo)

include/generated/barebox_default_env.h: barebox_default_env$(barebox_default_env_comp)
	$(Q)cat $< | $(objtree)/scripts/bin2c default_environment > $@
	$(Q)echo "const int default_environment_uncompress_size=`stat -c%s barebox_default_env`;" >> $@

CLEAN_FILES += include/generated/barebox_default_env.h barebox_default_env
CLEAN_FILES += barebox_default_env.gz barebox_default_env.bz2
CLEAN_FILES += barebox_default_env.lzo

# dependencies on generated files need to be listed explicitly
$(obj)/version.o: include/generated/compile.h

# compile.h changes depending on hostname, generation number, etc,
# so we regenerate it always.
# mkcompile_h will make sure to only update the
# actual file if its content has changed.

       chk_compile.h = :
 quiet_chk_compile.h = echo '  CHK     $@'
silent_chk_compile.h = :
include/generated/compile.h: FORCE
	@$($(quiet)chk_compile.h)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkcompile_h $@ \
	"$(UTS_MACHINE)" "$(CC) $(KBUILD_CFLAGS)"


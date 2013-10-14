obj-$(CONFIG_BINFMT)		+= binfmt.o
obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o
obj-$(CONFIG_ENV_HANDLING)	+= environment.o
obj-$(CONFIG_AUTO_COMPLETE)	+= complete.o
obj-$(CONFIG_POLLER)		+= poller.o
obj-$(CONFIG_BLOCK)		+= block.o
obj-$(CONFIG_PARTITION_DISK)	+= partitions.o partitions/

obj-$(CONFIG_CMD_LOADS)		+= s_record.o
obj-$(CONFIG_OFTREE)		+= oftree.o

obj-$(CONFIG_BLSPEC) += blspec.o
obj-y += memory.o
obj-$(CONFIG_DDR_SPD)  += ddr_spd.o
obj-y += memory_display.o
obj-$(CONFIG_MALLOC_DLMALLOC) += dlmalloc.o
obj-$(CONFIG_MALLOC_TLSF) += tlsf_malloc.o
obj-$(CONFIG_MALLOC_TLSF) += tlsf.o
obj-$(CONFIG_MALLOC_DUMMY) += dummy_malloc.o
obj-$(CONFIG_CMD_MEMTEST) += memtest.o
obj-y += clock.o
obj-$(CONFIG_BANNER) += version.o
obj-$(CONFIG_MEMINFO) += meminfo.o
obj-$(CONFIG_COMMAND_SUPPORT) += command.o
obj-$(CONFIG_CONSOLE_FULL) += console.o
obj-$(CONFIG_CONSOLE_SIMPLE) += console_simple.o
obj-y += console_common.o
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
obj-$(CONFIG_BAREBOX_UPDATE) += bbu.o
obj-y += bootsource.o
obj-$(CONFIG_BOOTM) += bootm.o
extra-$(CONFIG_MODULES) += module.lds
extra-y += barebox_default_env barebox_default_env.h

ifdef CONFIG_DEFAULT_ENVIRONMENT
$(obj)/startup.o: $(obj)/barebox_default_env.h
$(obj)/env.o: $(obj)/barebox_default_env.h

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

ENV_FILES := $(shell cd $(srctree); for i in $(DEFAULT_ENVIRONMENT_PATH); do find $${i} -type f ; done)

endif # ifdef CONFIG_DEFAULT_ENVIRONMENT

#
# Generate a barebox envfs image.
#
# echo $(ENV_FILES) > /dev/null is just for letting if_changed
# recognize that something has changed when the environment has
# other files,
#
quiet_cmd_env = ENV     $@
cmd_env = ($(srctree)/scripts/genenv $(srctree) $(objtree) $@ $(DEFAULT_ENVIRONMENT_PATH)) || \
	(echo $(ENV_FILES) > /dev/null; rm -f $@ ; false)

$(obj)/barebox_default_env: $(ENV_FILES) FORCE
	$(call if_changed,env)

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

$(obj)/barebox_default_env.gz: $(obj)/barebox_default_env FORCE
	$(call if_changed,gzip)

$(obj)/barebox_default_env.bz2: $(obj)/barebox_default_env FORCE
	$(call if_changed,bzip2)

$(obj)/barebox_default_env.lzo: $(obj)/barebox_default_env FORCE
	$(call if_changed,lzo)

targets += barebox_default_env.lzo barebox_default_env.bz2 barebox_default_env.gz

quiet_cmd_env_h = ENVH    $@
cmd_env_h = cat $< | (cd $(obj) && $(objtree)/scripts/bin2c default_environment) > $@; \
	echo "const int default_environment_uncompress_size=`stat -c%s $(obj)/barebox_default_env`;" >> $@

$(obj)/barebox_default_env.h: $(obj)/barebox_default_env$(barebox_default_env_comp) FORCE
	$(call if_changed,env_h)

quiet_cmd_pwd_h = PWDH    $@
ifdef CONFIG_PASSWORD
ifneq ($(CONFIG_PASSWORD_DEFAULT),"")
PASSWD_FILE := $(shell cd $(srctree); find $(CONFIG_PASSWORD_DEFAULT) -type f)
cmd_pwd_h = echo -n "const char default_passwd[] = \"" > $@; \
	cat $< | tr -d '\n' >> $@; \
	echo "\";" >> $@

include/generated/passwd.h: $(PASSWD_FILE)
	$(call if_changed,pwd_h)
else
cmd_pwd_h = echo "const char default_passwd[] = \"\";" > $@

include/generated/passwd.h: FORCE
	$(call if_changed,pwd_h)
endif

targets += include/generated/passwd.h

$(obj)/password.o: include/generated/passwd.h
endif

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


obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_OF_FLAT_TREE)	+= ft_build.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o
obj-$(CONFIG_ENV_HANDLING)	+= environment.o
obj-$(CONFIG_AUTO_COMPLETE)	+= complete.o
obj-$(CONFIG_POLLER)		+= poller.o
obj-$(CONFIG_BLOCK)		+= block.o

obj-y += memory.o
obj-$(CONFIG_MALLOC_DLMALLOC) += dlmalloc.o
obj-$(CONFIG_MALLOC_DUMMY) += dummy_malloc.o
obj-y += clock.o
obj-y += version.o
obj-$(CONFIG_COMMAND_SUPPORT) += command.o
obj-$(CONFIG_CONSOLE_FULL) += console.o
obj-$(CONFIG_CONSOLE_SIMPLE) += console_simple.o
obj-$(CONFIG_DIGEST) += digest.o
obj-$(CONFIG_ENVIRONMENT_VARIABLES) += env.o
obj-$(CONFIG_CMD_BOOTM) += image.o
obj-y += startup.o
obj-y += misc.o
obj-y += memsize.o
obj-$(CONFIG_MENU) += menu.o
obj-$(CONFIG_PASSWORD) += password.o
obj-$(CONFIG_MODULES) += module.o
extra-$(CONFIG_MODULES) += module.lds

ifdef CONFIG_DEFAULT_ENVIRONMENT
$(obj)/startup.o: include/generated/barebox_default_env.h
$(obj)/env.o: include/generated/barebox_default_env.h

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

include/generated/barebox_default_env.h: barebox_default_env
	$(Q)cat $< | $(objtree)/scripts/bin2c default_environment > $@

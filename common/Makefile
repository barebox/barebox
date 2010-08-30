obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_OF_FLAT_TREE)	+= ft_build.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o
obj-$(CONFIG_ENV_HANDLING)	+= environment.o
obj-$(CONFIG_AUTO_COMPLETE)	+= complete.o

obj-y += dlmalloc.o
obj-y += clock.o
obj-y += command.o
obj-$(CONFIG_CONSOLE_FULL) += console.o
obj-$(CONFIG_CONSOLE_SIMPLE) += console_simple.o
obj-y += env.o
obj-y += startup.o
obj-y += misc.o
obj-y += memsize.o
obj-$(CONFIG_MENU) += menu.o
obj-$(CONFIG_MODULES) += module.o
extra-$(CONFIG_MODULES) += module.lds

ifdef CONFIG_DEFAULT_ENVIRONMENT
$(obj)/startup.o: include/barebox_default_env.h
$(obj)/env.o: include/barebox_default_env.h

ENV_FILES := $(shell cd $(srctree); for i in $(CONFIG_DEFAULT_ENVIRONMENT_PATH); do find $${i} -type f -exec readlink -f {} \;; done)

endif # ifdef CONFIG_DEFAULT_ENVIRONMENT

barebox_default_env: $(ENV_FILES)
	$(Q)$(srctree)/scripts/genenv $(srctree) $(objtree) $(CONFIG_DEFAULT_ENVIRONMENT_PATH)

include/barebox_default_env.h: barebox_default_env
	$(Q)cat $< | $(objtree)/scripts/bin2c default_environment > $@

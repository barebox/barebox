obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_OF_FLAT_TREE)	+= ft_build.o
obj-$(CONFIG_MODULE)		+= module.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o

obj-y += dlmalloc.o
obj-y += clock.o
obj-y += command.o
obj-y += console.o
obj-y += partition.o
obj-y += env.o
obj-y += startup.o
obj-y += misc.o
obj-y += memsize.o

ifdef CONFIG_DEFAULT_ENVIRONMENT_PATH

startup.o: include/uboot_default_env.h

include/uboot_default_env.h: $(shell find $(CONFIG_DEFAULT_ENVIRONMENT_PATH) -type f)
	$(Q)scripts/ubootenv -s $(CONFIG_DEFAULT_ENVIRONMENT_PATH) uboot_default_env
	$(Q)cat uboot_default_env | scripts/bin2c default_environment > $@

$(obj)/env.o: include/uboot_default_env.h
endif


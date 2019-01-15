ifeq ($(CONFIG_OFDEVICE),)
obj-y += init.o
endif
obj-$(CONFIG_OFDEVICE) += of_init.o

lwl-y += lowlevel_init.o

bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC) += defaultenv-at91sam9263ek

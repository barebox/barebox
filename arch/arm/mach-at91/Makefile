obj-y += setup.o

ifeq ($(CONFIG_COMMON_CLK_OF_PROVIDER),)
obj-y += clock.o
endif

obj-$(CONFIG_CMD_AT91_BOOT_TEST) += boot_test_cmd.o

obj-$(CONFIG_AT91_BOOTSTRAP)	+= bootstrap.o

obj-y += at91sam9_reset.o
obj-y += at91sam9g45_reset.o

obj-$(CONFIG_AT91SAM9_SMC) += sam9_smc.o

# CPU-specific support
obj-$(CONFIG_ARCH_AT91RM9200)	+= at91rm9200.o at91rm9200_time.o at91rm9200_devices.o
obj-$(CONFIG_ARCH_AT91SAM9260) += at91sam9260.o at91sam9260_devices.o
obj-$(CONFIG_ARCH_AT91SAM9261) += at91sam9261.o at91sam9261_devices.o
obj-$(CONFIG_ARCH_AT91SAM9G10) += at91sam9261.o at91sam9261_devices.o
ifeq ($(CONFIG_OFDEVICE),)
obj-$(CONFIG_ARCH_AT91SAM9263) += at91sam9263.o at91sam9263_devices.o
obj-$(CONFIG_ARCH_SAMA5D3)	+= sama5d3.o sama5d3_devices.o
endif
obj-$(CONFIG_ARCH_AT91SAM9G20) += at91sam9260.o at91sam9260_devices.o
obj-$(CONFIG_ARCH_AT91SAM9G45) += at91sam9g45.o at91sam9g45_devices.o
obj-$(CONFIG_ARCH_AT91SAM9X5)	+= at91sam9x5.o at91sam9x5_devices.o
obj-$(CONFIG_ARCH_AT91SAM9N12)	+= at91sam9n12.o at91sam9n12_devices.o
obj-$(CONFIG_ARCH_SAMA5D4)	+= sama5d4.o sama5d4_devices.o

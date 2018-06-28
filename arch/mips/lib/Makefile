extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds
obj-$(CONFIG_CSRC_R4K_LIB) += csrc-r4k.o
obj-y += cpu-probe.o
obj-y += traps.o
obj-y += genex.o
obj-y += shutdown.o
obj-y += dma-default.o

obj-$(CONFIG_MIPS_OPTIMIZED_STRING_FUNCTIONS) += memcpy.o
obj-$(CONFIG_MIPS_OPTIMIZED_STRING_FUNCTIONS) += memset.o

obj-$(CONFIG_CPU_MIPS32) += c-r4k.o
obj-$(CONFIG_CPU_MIPS64) += c-r4k.o

obj-$(CONFIG_CMD_MIPS_CPUINFO) += cpuinfo.o
obj-$(CONFIG_CMD_BOOTM)	+= bootm.o

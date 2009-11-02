obj-y += cpu.o exceptions.o
obj-y += interrupts.o

#
# Any variants can be called as start-armxyz.S
#
obj-$(CONFIG_CPU_ARM920T) += start-arm.o
obj-$(CONFIG_CPU_ARM926T) += start-arm.o
obj-$(CONFIG_CPU_V7) += start-arm.o
obj-$(CONFIG_ARCH_IMX31) += start-arm.o
obj-$(CONFIG_ARCH_IMX35) += start-arm.o
obj-$(CONFIG_CMD_ARM_CPUINFO) += cpuinfo.o
obj-$(CONFIG_MMU) += mmu.o
obj-$(CONFIG_MMU) += cache.o


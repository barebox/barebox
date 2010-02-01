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
obj-$(CONFIG_CPU_32v4T)	+= cache-armv4.o
obj-$(CONFIG_CPU_32v5)	+= cache-armv4.o
obj-$(CONFIG_CPU_32v6)	+= cache-armv6.o
obj-$(CONFIG_CPU_32v7)	+= cache-armv7.o

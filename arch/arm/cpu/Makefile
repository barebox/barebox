obj-y += cpu.o
obj-$(CONFIG_ARM_EXCEPTIONS) += exceptions.o
obj-$(CONFIG_ARM_EXCEPTIONS) += interrupts.o
obj-y += start.o setupc.o

#
# Any variants can be called as start-armxyz.S
#
obj-$(CONFIG_CMD_ARM_CPUINFO) += cpuinfo.o
obj-$(CONFIG_CMD_ARM_MMUINFO) += mmuinfo.o
obj-$(CONFIG_BUILTIN_DTB) += dtb.o
obj-$(CONFIG_MMU) += mmu.o cache.o mmu-early.o
pbl-$(CONFIG_MMU) += cache.o mmu-early.o
obj-$(CONFIG_CPU_32v4T) += cache-armv4.o
pbl-$(CONFIG_CPU_32v4T) += cache-armv4.o
obj-$(CONFIG_CPU_32v5) += cache-armv5.o
pbl-$(CONFIG_CPU_32v5) += cache-armv5.o
obj-$(CONFIG_CPU_32v6) += cache-armv6.o
pbl-$(CONFIG_CPU_32v6) += cache-armv6.o
obj-$(CONFIG_CPU_32v7) += cache-armv7.o
pbl-$(CONFIG_CPU_32v7) += cache-armv7.o
obj-$(CONFIG_CACHE_L2X0) += cache-l2x0.o

pbl-y += start-pbl.o setupc.o

obj-y += common.o
pbl-y += common.o

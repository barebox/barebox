obj-y += cpu.o

ifeq ($(CONFIG_CPU_64v8), y)
obj-$(CONFIG_ARM_EXCEPTIONS) += exceptions_64.o
obj-$(CONFIG_MMU) += mmu_64.o
lwl-y += lowlevel_64.o
else
obj-$(CONFIG_ARM_EXCEPTIONS) += exceptions.o
obj-$(CONFIG_MMU) += mmu.o mmu-early.o
pbl-$(CONFIG_MMU) += mmu-early.o
lwl-y += lowlevel.o
endif

obj-$(CONFIG_ARM_EXCEPTIONS) += interrupts.o
obj-y += start.o entry.o

ifeq ($(CONFIG_CPU_64v8), y)
obj-y += setupc_64.o
pbl-y += setupc_64.o
else
obj-y += setupc.o
pbl-y += setupc.o
endif

#
# Any variants can be called as start-armxyz.S
#
obj-$(CONFIG_CMD_ARM_CPUINFO) += cpuinfo.o
obj-$(CONFIG_CMD_ARM_MMUINFO) += mmuinfo.o
obj-$(CONFIG_OFDEVICE) += dtb.o
obj-$(CONFIG_MMU) += cache.o

ifeq ($(CONFIG_MMU),)
obj-y += no-mmu.o
endif

obj-$(CONFIG_CPU_32v4T) += cache-armv4.o
pbl-$(CONFIG_CPU_32v4T) += cache-armv4.o
obj-$(CONFIG_CPU_32v5) += cache-armv5.o
pbl-$(CONFIG_CPU_32v5) += cache-armv5.o
obj-$(CONFIG_CPU_32v6) += cache-armv6.o
pbl-$(CONFIG_CPU_32v6) += cache-armv6.o
AFLAGS_cache-armv7.o       :=-Wa,-march=armv7-a
obj-$(CONFIG_CPU_32v7) += cache-armv7.o
AFLAGS_pbl-cache-armv7.o       :=-Wa,-march=armv7-a
pbl-$(CONFIG_CPU_32v7) += cache-armv7.o
obj-$(CONFIG_CACHE_L2X0) += cache-l2x0.o
AFLAGS_cache-armv8.o       :=-Wa,-march=armv8-a
obj-$(CONFIG_CPU_64v8) += cache-armv8.o
AFLAGS_pbl-cache-armv8.o       :=-Wa,-march=armv8-a
pbl-$(CONFIG_CPU_64v8) += cache-armv8.o

pbl-y += entry.o
pbl-$(CONFIG_PBL_SINGLE_IMAGE) += start-pbl.o
pbl-$(CONFIG_PBL_MULTI_IMAGES) += uncompress.o

obj-y += common.o cache.o
pbl-y += common.o cache.o

obj-y += cpu.o

obj-$(CONFIG_ARM_EXCEPTIONS) += exceptions$(S64).o interrupts$(S64).o
obj-$(CONFIG_MMU) += mmu$(S64).o
lwl-y += lowlevel$(S64).o
obj-pbl-$(CONFIG_MMU) += mmu-early$(S64).o

obj-y += start.o entry.o

obj-pbl-y += setupc$(S64).o cache$(S64).o

#
# Any variants can be called as start-armxyz.S
#
obj-$(CONFIG_CMD_ARM_CPUINFO) += cpuinfo.o
obj-$(CONFIG_CMD_ARM_MMUINFO) += mmuinfo.o
obj-$(CONFIG_OFDEVICE) += dtb.o

ifeq ($(CONFIG_MMU),)
obj-y += no-mmu.o
endif

obj-$(CONFIG_ARM_PSCI) += psci.o
obj-$(CONFIG_ARM_SECURE_MONITOR) += smccc-call.o
AFLAGS_smccc-call.o	:=-Wa,-march=armv7-a
obj-$(CONFIG_ARM_SECURE_MONITOR) += sm.o sm_as.o
AFLAGS_sm_as.o		:=-Wa,-march=armv7-a

obj-pbl-$(CONFIG_CPU_32v4T) += cache-armv4.o
obj-pbl-$(CONFIG_CPU_32v5) += cache-armv5.o
obj-pbl-$(CONFIG_CPU_32v6) += cache-armv6.o
AFLAGS_cache-armv7.o       :=-Wa,-march=armv7-a
obj-pbl-$(CONFIG_CPU_32v7) += cache-armv7.o
AFLAGS_pbl-cache-armv7.o       :=-Wa,-march=armv7-a
obj-$(CONFIG_CACHE_L2X0) += cache-l2x0.o
AFLAGS_cache-armv8.o       :=-Wa,-march=armv8-a
obj-pbl-$(CONFIG_CPU_64v8) += cache-armv8.o
AFLAGS_pbl-cache-armv8.o       :=-Wa,-march=armv8-a

pbl-y += entry.o
pbl-$(CONFIG_PBL_SINGLE_IMAGE) += start-pbl.o
pbl-$(CONFIG_PBL_MULTI_IMAGES) += uncompress.o

obj-pbl-y += common.o sections.o

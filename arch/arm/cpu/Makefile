# SPDX-License-Identifier: GPL-2.0-only

obj-y += cpu.o

obj-$(CONFIG_ARM_EXCEPTIONS) += exceptions_$(S64_32).o interrupts_$(S64_32).o
obj-$(CONFIG_MMU) += mmu-common.o
obj-pbl-$(CONFIG_MMU) += mmu_$(S64_32).o
obj-$(CONFIG_MMU) += dma_$(S64_32).o
obj-pbl-y += lowlevel_$(S64_32).o
obj-pbl-$(CONFIG_CPU_32v7) += hyp.o
AFLAGS_hyp.o :=-Wa,-march=armv7-a -Wa,-mcpu=all
AFLAGS_hyp.pbl.o :=-Wa,-march=armv7-a -Wa,-mcpu=all

obj-y += start.o entry.o entry_ll_$(S64_32).o
KASAN_SANITIZE_start.o := n

pbl-$(CONFIG_CPU_64) += head_64.o

pbl-$(CONFIG_BOARD_ARM_GENERIC_DT) += board-dt-2nd.o
pbl-$(CONFIG_BOARD_ARM_GENERIC_DT_AARCH64) += board-dt-2nd-aarch64.o

obj-pbl-y += setupc_$(S64_32).o cache_$(S64_32).o

obj-$(CONFIG_ARM_PSCI_CLIENT) += psci-client.o

#
# Any variants can be called as start-armxyz.S
#
obj-$(CONFIG_CMD_ARM_CPUINFO) += cpuinfo.o
obj-$(CONFIG_MMUINFO) += mmuinfo.o mmuinfo_$(S64_32).o
obj-$(CONFIG_OFDEVICE) += dtb.o

ifeq ($(CONFIG_MMU),)
obj-$(CONFIG_CPU_32v7) += no-mmu.o
endif

obj-$(CONFIG_ARM_PSCI) += psci.o
obj-$(CONFIG_ARM_PSCI_OF) += psci-of.o
obj-pbl-$(CONFIG_ARM_SMCCC) += smccc-call_$(S64_32).o
AFLAGS_smccc-call_$(S64_32).o :=-Wa,-march=armv$(if $(S64),8,7)-a
AFLAGS_smccc-call_$(S64_32).pbl.o :=-Wa,-march=armv$(if $(S64),8,7)-a
obj-$(CONFIG_ARM_SECURE_MONITOR) += sm.o sm_as.o
AFLAGS_sm_as.o		:=-Wa,-march=armv7-a

obj-pbl-$(CONFIG_CPU_32v4T) += cache-armv4.o
obj-pbl-$(CONFIG_CPU_32v5) += cache-armv5.o
obj-pbl-$(CONFIG_CPU_32v6) += cache-armv6.o
obj-pbl-$(CONFIG_CPU_32v7) += cache-armv7.o
AFLAGS_cache-armv7.o       :=-Wa,-march=armv7-a
AFLAGS_cache-armv7.pbl.o   :=-Wa,-march=armv7-a
obj-$(CONFIG_CACHE_L2X0) += cache-l2x0.o
obj-pbl-$(CONFIG_CPU_64v8) += cache-armv8.o
AFLAGS_cache-armv8.o       :=-Wa,-march=armv8-a
AFLAGS-cache-armv8.pbl.o   :=-Wa,-march=armv8-a

pbl-y += entry.o entry_ll_$(S64_32).o
pbl-y += uncompress.o
pbl-$(CONFIG_ARM_ATF) += atf.o

obj-pbl-y += common.o sections.o
KASAN_SANITIZE_common.o := n

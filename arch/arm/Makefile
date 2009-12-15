
CPPFLAGS	+= -D__ARM__ -fno-strict-aliasing
# Explicitly specifiy 32-bit ARM ISA since toolchain default can be -mthumb:
CPPFLAGS	+=$(call cc-option,-marm,)

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
CPPFLAGS	+= -mbig-endian
AS		+= -EB
LD		+= -EB
else
CPPFLAGS	+= -mlittle-endian
AS		+= -EL
LD		+= -EL
endif

comma = ,

# This selects which instruction set is used.
# Note that GCC does not numerically define an architecture version
# macro, but instead defines a whole series of macros which makes
# testing for a specific architecture or later rather impossible.
arch-$(CONFIG_CPU_32v7)		:=-D__LINUX_ARM_ARCH__=7 $(call cc-option,-march=armv7-a,-march=armv5t -Wa$(comma)-march=armv7-a)
arch-$(CONFIG_CPU_32v5)		:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4t)
arch-$(CONFIG_CPU_32v4T)	:=-D__LINUX_ARM_ARCH__=4 -march=armv4t

# This selects how we optimise for the processor.
tune-$(CONFIG_CPU_ARM920T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM926T)	:=-mtune=arm9tdmi

ifeq ($(CONFIG_AEABI),y)
CFLAGS_ABI	:=-mabi=aapcs-linux -mno-thumb-interwork
else
CFLAGS_ABI	:=$(call cc-option,-mapcs-32,-mabi=apcs-gnu) $(call cc-option,-mno-thumb-interwork,)
endif

CPPFLAGS += $(CFLAGS_ABI) $(arch-y) $(tune-y)

# Machine directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
machine-$(CONFIG_ARCH_AT91)		:= at91
machine-$(CONFIG_ARCH_AT91RM9200)	:= at91rm9200
machine-$(CONFIG_ARCH_IMX)		:= imx
machine-$(CONFIG_ARCH_NETX)		:= netx
machine-$(CONFIG_ARCH_OMAP)		:= omap
machine-$(CONFIG_ARCH_S3C24xx)		:= s3c24xx

# Board directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
board-$(CONFIG_MACH_A9M2410)			:= a9m2410
board-$(CONFIG_MACH_A9M2440)			:= a9m2440
board-$(CONFIG_MACH_AT91SAM9260EK)		:= at91sam9260ek
board-$(CONFIG_MACH_AT91SAM9263EK)		:= at91sam9263ek
board-$(CONFIG_MACH_ECO920)			:= eco920
board-$(CONFIG_MACH_EUKREA_CPUIMX27)		:= eukrea_cpuimx27
board-$(CONFIG_MACH_FREESCALE_MX35_3STACK)	:= freescale-mx35-3-stack
board-$(CONFIG_MACH_FREESCALE_MX25_3STACK)	:= freescale-mx25-3-stack
board-$(CONFIG_MACH_IMX21ADS)			:= imx21ads
board-$(CONFIG_MACH_IMX27ADS)			:= imx27ads
board-$(CONFIG_MACH_MMCCPU)			:= mmccpu
board-$(CONFIG_MACH_MX1ADS)			:= mx1ads
board-$(CONFIG_MACH_NXDB500)			:= netx
board-$(CONFIG_ARCH_OMAP)			:= omap
board-$(CONFIG_MACH_PCA100)			:= phycard-i.MX27
board-$(CONFIG_MACH_PCM037)			:= pcm037
board-$(CONFIG_MACH_PCM038)			:= pcm038
board-$(CONFIG_MACH_PCM043)			:= pcm043
board-$(CONFIG_MACH_PM9263)			:= pm9263
board-$(CONFIG_MACH_SCB9328)			:= scb9328

machdirs := $(patsubst %,arch/arm/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P

ifndef CONFIG_MODULES
# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif

barebox.netx: barebox.bin
	$(Q)scripts/gen_netx_image -i barebox.bin -o barebox.netx		\
		--sdramctrl=$(CONFIG_NETX_SDRAM_CTRL)			\
		--sdramtimctrl=$(CONFIG_NETX_SDRAM_TIMING_CTRL)		\
		--memctrl=$(CONFIG_NETX_MEM_CTRL)			\
		--entrypoint=$(CONFIG_TEXT_BASE)			\
		--cookie=$(CONFIG_NETX_COOKIE);

ifeq ($(machine-y),netx)
KBUILD_IMAGE := barebox.netx
endif

all: $(KBUILD_IMAGE)

archprepare: maketools
maketools:
	$(Q)$(MAKE) $(build)=arch/arm/tools arch/arm/include/asm/mach-types.h

PHONY += maketools

ifneq ($(board-y),)
BOARD := board/$(board-y)/
else
BOARD :=
endif

ifneq ($(machine-y),)
MACH := arch/arm/mach-$(machine-y)/
else
MACH :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/arm/lib/ arch/arm/cpu/

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)	:= arch/arm/lib/barebox.lds
lds-$(CONFIG_BOARD_LINKER_SCRIPT)	:= $(BOARD)/barebox.lds

CLEAN_FILES += arch/arm/include/asm/mach-types.h arch/arm/lib/barebox.lds

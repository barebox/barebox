
CPPFLAGS += -D__ARM__ -fno-strict-aliasing


machine-$(CONFIG_ARCH_IMX)	   := imx
machine-$(CONFIG_ARCH_NETX)	   := netx
machine-$(CONFIG_ARCH_AT91RM9200)  := at91rm9200
machine-$(CONFIG_ARCH_OMAP)        := omap
machine-$(CONFIG_ARCH_AT91)	   := at91
machine-$(CONFIG_ARCH_S3C24xx)     := s3c24xx
board-$(CONFIG_MACH_MX1ADS)	   := mx1ads
board-$(CONFIG_MACH_ECO920)	   := eco920
board-$(CONFIG_MACH_SCB9328)	   := scb9328
board-$(CONFIG_MACH_PCM038)	   := pcm038
board-$(CONFIG_MACH_IMX21ADS)	   := imx21ads
board-$(CONFIG_MACH_IMX27ADS)	   := imx27ads
board-$(CONFIG_MACH_NXDB500)       := netx
board-$(CONFIG_MACH_PCM037)	   := pcm037
board-$(CONFIG_MACH_OMAP)	   := omap
board-$(CONFIG_MACH_AT91SAM9260_EK):= at91sam9260ek
board-$(CONFIG_MACH_PM9263)        := pm9263
board-$(CONFIG_MACH_FREESCALE_MX35_3STACK) := freescale-mx35-3-stack
board-$(CONFIG_MACH_FREESCALE_MX25_3STACK) := freescale-mx25-3-stack
board-$(CONFIG_MACH_PCM043)        := pcm043
board-$(CONFIG_MACH_A9M2410)       := a9m2410
board-$(CONFIG_MACH_PCA100)        := phycard-i.MX27
board-$(CONFIG_MACH_MMCCPU)        := mmccpu
board-$(CONFIG_MACH_A9M2440)       := a9m2440
board-$(CONFIG_MACH_EUKREA_CPUIMX27) := eukrea_cpuimx27

# FIXME "cpu-y" never used on ARM!
cpu-$(CONFIG_ARM920T)              := arm920t
cpu-$(CONFIG_ARM926EJS)            := arm926ejs

machdirs := $(patsubst %,arch/arm/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -mabi=apcs-gnu -DTEXT_BASE=$(TEXT_BASE) -P

ifndef CONFIG_MODULES
# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_uboot += -static --gc-sections
endif

uboot.netx: uboot.bin
	$(Q)scripts/gen_netx_image -i uboot.bin -o uboot.netx		\
		--sdramctrl=$(CONFIG_NETX_SDRAM_CTRL)			\
		--sdramtimctrl=$(CONFIG_NETX_SDRAM_TIMING_CTRL)		\
		--memctrl=$(CONFIG_NETX_MEM_CTRL)			\
		--entrypoint=$(CONFIG_TEXT_BASE)			\
		--cookie=$(CONFIG_NETX_COOKIE);

ifeq ($(machine-y),netx)
KBUILD_IMAGE := uboot.netx 
endif

all: $(KBUILD_IMAGE)

archprepare: maketools

PHONY += maketools

# Add architecture specific flags
ifeq ($(CONFIG_ARMCORTEXA8),y)
CPPFLAGS += -march=armv7a
endif

ifneq ($(board-y),)
BOARD  := board/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/arm/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/arm/lib/ arch/arm/cpu/

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)   := arch/arm/lib/u-boot.lds
lds-$(CONFIG_BOARD_LINKER_SCRIPT)     := $(BOARD)/u-boot.lds

CLEAN_FILES    += arch/arm/lib/u-boot.lds

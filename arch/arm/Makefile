
CPPFLAGS += -nostdinc -Wall 	\
	-pipe -D __ARM__ -fno-strict-aliasing


machine-$(CONFIG_ARCH_IMX)	   := imx
machine-$(CONFIG_ARCH_NETX)	   := netx
machine-$(CONFIG_ARCH_AT91RM9200)  := at91rm9200
board-$(CONFIG_MACH_MX1ADS)	   := mx1ads
board-$(CONFIG_MACH_ECO920)	   := eco920
board-$(CONFIG_MACH_SCB9328)	   := scb9328
board-$(CONFIG_MACH_PCM038)	   := pcm038
board-$(CONFIG_MACH_IMX27ADS)	   := imx27ads
board-$(CONFIG_MACH_NXDB500)       := netx
board-$(CONFIG_MACH_PCM037)	   := pcm037
# FIXME "cpu-y" never used on ARM!
cpu-$(CONFIG_ARM920T)              := arm920t
cpu-$(CONFIG_ARM926EJS)            := arm926ejs

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -mabi=apcs-gnu -DTEXT_BASE=$(TEXT_BASE) -P
CFLAGS += -msoft-float

# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_uboot += -static --gc-sections

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-arm/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-arm/arch -> include/asm-arm/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-arm
	$(Q)ln -fsn $(srctree)/include/asm-arm/$(INCDIR) include/asm-arm/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-arm/arch
endif
	@echo '  SYMLINK include/asm-arm/proc -> include/asm-arm/proc-armv'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-arm
	$(Q)ln -fsn $(srctree)/include/asm-arm/proc-armv include/asm-arm/proc
else
	$(Q)ln -fsn proc-armv include/asm-arm/proc
endif
	@touch $@

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
maketools: include/asm-arm/.arch


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
MRPROPER_FILES += include/asm-arm/arch include/asm-arm/proc

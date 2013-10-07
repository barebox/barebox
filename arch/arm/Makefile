
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
arch-$(CONFIG_CPU_32v6)            :=-D__LINUX_ARM_ARCH__=6 $(call cc-option,-march=armv6,-march=armv5t -Wa$(comma)-march=armv6)
arch-$(CONFIG_CPU_32v5)		:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4t)
arch-$(CONFIG_CPU_32v4T)	:=-D__LINUX_ARM_ARCH__=4 -march=armv4t

# This selects how we optimise for the processor.
tune-$(CONFIG_CPU_ARM920T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM926T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_XSCALE)	:=$(call cc-option,-mtune=xscale,-mtune=strongarm110) -Wa,-mcpu=xscale

ifeq ($(CONFIG_AEABI),y)
CFLAGS_ABI	:=-mabi=aapcs-linux -mno-thumb-interwork
else
CFLAGS_ABI	:=$(call cc-option,-mapcs-32,-mabi=apcs-gnu) $(call cc-option,-mno-thumb-interwork,)
endif

ifeq ($(CONFIG_ARM_UNWIND),y)
CFLAGS_ABI	+=-funwind-tables
endif

ifeq ($(CONFIG_THUMB2_BAREBOX),y)
AFLAGS_AUTOIT	:=$(call as-option,-Wa$(comma)-mimplicit-it=always,-Wa$(comma)-mauto-it)
AFLAGS_NOWARN	:=$(call as-option,-Wa$(comma)-mno-warn-deprecated,-Wa$(comma)-W)
CFLAGS_THUMB2	:=-mthumb $(AFLAGS_AUTOIT) $(AFLAGS_NOWARN)
AFLAGS_THUMB2	:=$(CFLAGS_THUMB2) -Wa$(comma)-mthumb
endif

CPPFLAGS += $(CFLAGS_ABI) $(arch-y) $(tune-y) -msoft-float $(CFLAGS_THUMB2)
AFLAGS   += -include asm/unified.h -msoft-float $(AFLAGS_THUMB2)

# Machine directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
machine-$(CONFIG_ARCH_AT91)		:= at91
machine-$(CONFIG_ARCH_BCM2835)		:= bcm2835
machine-$(CONFIG_ARCH_CLPS711X)		:= clps711x
machine-$(CONFIG_ARCH_EP93XX)		:= ep93xx
machine-$(CONFIG_ARCH_HIGHBANK)		:= highbank
machine-$(CONFIG_ARCH_IMX)		:= imx
machine-$(CONFIG_ARCH_MXS)		:= mxs
machine-$(CONFIG_ARCH_MVEBU)		:= mvebu
machine-$(CONFIG_ARCH_NOMADIK)		:= nomadik
machine-$(CONFIG_ARCH_NETX)		:= netx
machine-$(CONFIG_ARCH_OMAP)		:= omap
machine-$(CONFIG_ARCH_PXA)		:= pxa
machine-$(CONFIG_ARCH_SAMSUNG)		:= samsung
machine-$(CONFIG_ARCH_SOCFPGA)		:= socfpga
machine-$(CONFIG_ARCH_VERSATILE)	:= versatile
machine-$(CONFIG_ARCH_VEXPRESS)		:= vexpress
machine-$(CONFIG_ARCH_TEGRA)		:= tegra
machine-$(CONFIG_ARCH_ZYNQ)		:= zynq


# Board directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
#
# DO NOT ADD NEW ENTRIES TO THIS LIST!
# Add to arch/arm/boards/Makefile instead.
#
# These are here only because they have a board specific config.h.
# TODO: Get rid of board specific config.h and move these to
# arch/arm/boards/Makefile aswell.
board-$(CONFIG_MACH_A9M2410)			+= a9m2410
board-$(CONFIG_MACH_A9M2440)			+= a9m2440
board-$(CONFIG_MACH_AT91RM9200EK)		+= at91rm9200ek
board-$(CONFIG_MACH_GLOBALSCALE_GURUPLUG)	+= globalscale-guruplug
board-$(CONFIG_MACH_GLOBALSCALE_MIRABOX)	+= globalscale-mirabox
board-$(CONFIG_MACH_MARVELL_ARMADA_XP_GP)	+= marvell-armada-xp-gp
board-$(CONFIG_MACH_MINI2440)			+= friendlyarm-mini2440
board-$(CONFIG_MACH_MINI6410)			+= friendlyarm-mini6410
board-$(CONFIG_MACH_PCM027)			+= pcm027
board-$(CONFIG_MACH_PLATHOME_OPENBLOCKS_AX3)	+= plathome-openblocks-ax3/
board-$(CONFIG_MACH_SOLIDRUN_CUBOX)		+= solidrun-cubox
board-$(CONFIG_MACH_TINY210)			+= friendlyarm-tiny210
board-$(CONFIG_MACH_TINY6410)			+= friendlyarm-tiny6410
board-$(CONFIG_MACH_USI_TOPKICK)		+= usi-topkick

machdirs := $(patsubst %,arch/arm/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

ifndef CONFIG_MODULES
# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif

ifdef CONFIG_RELOCATABLE
LDFLAGS_barebox += -pie
endif

ifdef CONFIG_IMAGE_COMPRESSION
KBUILD_BINARY := arch/arm/pbl/zbarebox.bin
else
KBUILD_BINARY := barebox.bin
endif

barebox.netx: $(KBUILD_BINARY)
	$(Q)scripts/gen_netx_image -i $< -o barebox.netx		\
		--sdramctrl=$(CONFIG_NETX_SDRAM_CTRL)			\
		--sdramtimctrl=$(CONFIG_NETX_SDRAM_TIMING_CTRL)		\
		--memctrl=$(CONFIG_NETX_MEM_CTRL)			\
		--entrypoint=$(CONFIG_TEXT_BASE)			\
		--cookie=$(CONFIG_NETX_COOKIE);

ifeq ($(machine-y),netx)
KBUILD_IMAGE := barebox.netx
endif

barebox.s5p: $(KBUILD_BINARY)
	$(Q)scripts/s5p_cksum $< barebox.s5p

ifeq ($(CONFIG_ARCH_S5PCxx),y)
KBUILD_IMAGE := barebox.s5p
endif

quiet_cmd_mlo ?= IFT     $@
	cmd_mlo ?= scripts/omap_signGP -o MLO -l $(TEXT_BASE) -c $<

MLO: $(KBUILD_BINARY)
	$(call if_changed,mlo)

ifeq ($(CONFIG_OMAP_BUILD_IFT),y)
KBUILD_IMAGE := MLO
endif

quiet_cmd_davinci_ubl_image = UBL-IMG $@
      cmd_davinci_ubl_image = set -e; \
	 scripts/mkublheader $< > $@; \
	 cat $< >> $@

barebox.ubl: $(KBUILD_BINARY) FORCE
	$(call if_changed,davinci_ubl_image)

ifeq ($(CONFIG_ARCH_DAVINCI),y)
KBUILD_IMAGE := barebox.ubl
endif

quiet_cmd_am35xx_spi_image = SPI-IMG $@
      cmd_am35xx_spi_image = scripts/mk-am35xx-spi-image -a $(TEXT_BASE) $< > $@

barebox.spi: $(KBUILD_BINARY) FORCE
	$(call if_changed,am35xx_spi_image)

ifeq ($(CONFIG_OMAP_BUILD_SPI),y)
KBUILD_IMAGE := barebox.spi
endif

quiet_cmd_zynq_image = ZYNQ-IMG $@
      cmd_zynq_image = scripts/zynq_mkimage $< $@

barebox.zynq: $(KBUILD_BINARY) FORCE
	$(call if_changed,zynq_image)

ifeq ($(machine-y),zynq)
KBUILD_IMAGE := barebox.zynq
endif

KWBIMAGE_OPTS = \
	-c -i $(srctree)/$(BOARD)/kwbimage.cfg -d $(TEXT_BASE) -e $(TEXT_BASE)

quiet_cmd_kwbimage = KWB     $@
      cmd_kwbimage = scripts/kwbimage -p $< $(KWBIMAGE_OPTS) -o $@ || \
	echo "WARNING: Couldn't create KWB image due to previous errors."

quiet_cmd_kwbimage_uart = KWBUART $@
      cmd_kwbimage_uart = scripts/kwbimage -m uart -p $< $(KWBIMAGE_OPTS) -o $@ || \
	echo "WARNING Couldn't create KWB image due to previous errors."

barebox.kwb: $(KBUILD_BINARY) FORCE
	$(call if_changed,kwbimage)

barebox.kwbuart: $(KBUILD_BINARY) FORCE
	$(call if_changed,kwbimage_uart)

ifeq ($(CONFIG_ARCH_MVEBU),y)
KBUILD_IMAGE  := barebox.kwb barebox.kwbuart
endif

pbl := arch/arm/pbl
$(pbl)/zbarebox.S $(pbl)/zbarebox.bin $(pbl)/zbarebox: barebox.bin FORCE
	$(Q)$(MAKE) $(build)=$(pbl) $@

archclean:
	$(MAKE) $(clean)=$(pbl)

dts := arch/arm/dts

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(dts) $(dts)/$@

KBUILD_IMAGE ?= $(KBUILD_BINARY)

archprepare: maketools
maketools:
	$(Q)$(MAKE) $(build)=arch/arm/tools include/generated/mach-types.h

PHONY += maketools

ifneq ($(board-y),)
BOARD := arch/arm/boards/$(board-y)/
else
BOARD :=
endif

ifneq ($(machine-y),)
MACH := arch/arm/mach-$(machine-y)/
else
MACH :=
endif

common-y += $(BOARD) arch/arm/boards/ $(MACH)
common-y += arch/arm/lib/ arch/arm/cpu/

common-$(CONFIG_OFTREE) += arch/arm/dts/

lds-y	:= arch/arm/lib/barebox.lds

common- += $(patsubst %,arch/arm/boards/%/,$(board-))

CLEAN_FILES += include/generated/mach-types.h arch/arm/lib/barebox.lds barebox-flash-image

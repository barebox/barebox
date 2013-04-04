
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
machine-$(CONFIG_ARCH_NOMADIK)		:= nomadik
machine-$(CONFIG_ARCH_NETX)		:= netx
machine-$(CONFIG_ARCH_OMAP)		:= omap
machine-$(CONFIG_ARCH_PXA)		:= pxa
machine-$(CONFIG_ARCH_SAMSUNG)		:= samsung
machine-$(CONFIG_ARCH_VERSATILE)	:= versatile
machine-$(CONFIG_ARCH_VEXPRESS)		:= vexpress
machine-$(CONFIG_ARCH_TEGRA)		:= tegra
machine-$(CONFIG_ARCH_ZYNQ)		:= zynq

# Board directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
board-$(CONFIG_MACH_A9M2410)			:= a9m2410
board-$(CONFIG_MACH_A9M2440)			:= a9m2440
board-$(CONFIG_MACH_ANIMEO_IP)			:= animeo_ip
board-$(CONFIG_MACH_AT91RM9200EK)		:= at91rm9200ek
board-$(CONFIG_MACH_AT91SAM9260EK)		:= at91sam9260ek
board-$(CONFIG_MACH_AT91SAM9261EK)		:= at91sam9261ek
board-$(CONFIG_MACH_AT91SAM9263EK)		:= at91sam9263ek
board-$(CONFIG_MACH_AT91SAM9G10EK)		:= at91sam9261ek
board-$(CONFIG_MACH_AT91SAM9G20EK)		:= at91sam9260ek
board-$(CONFIG_MACH_AT91SAM9N12EK)		:= at91sam9n12ek
board-$(CONFIG_MACH_AT91SAM9X5EK)		:= at91sam9x5ek
board-$(CONFIG_MACH_AT91SAM9M10IHD)		:= at91sam9m10ihd
board-$(CONFIG_MACH_AT91SAM9M10G45EK)		:= at91sam9m10g45ek
board-$(CONFIG_MACH_SAMA5D3XEK)			:= sama5d3xek
board-$(CONFIG_MACH_CLEP7212)			:= clep7212
board-$(CONFIG_MACH_DSS11)			:= dss11
board-$(CONFIG_MACH_EDB9301)			:= edb93xx
board-$(CONFIG_MACH_EDB9302)			:= edb93xx
board-$(CONFIG_MACH_EDB9302A)			:= edb93xx
board-$(CONFIG_MACH_EDB9307)			:= edb93xx
board-$(CONFIG_MACH_EDB9307A)			:= edb93xx
board-$(CONFIG_MACH_EDB93012)			:= edb93xx
board-$(CONFIG_MACH_EDB9315)			:= edb93xx
board-$(CONFIG_MACH_EDB9315A)			:= edb93xx
board-$(CONFIG_MACH_EUKREA_CPUIMX25)		:= eukrea_cpuimx25
board-$(CONFIG_MACH_EUKREA_CPUIMX27)		:= eukrea_cpuimx27
board-$(CONFIG_MACH_EUKREA_CPUIMX35)		:= eukrea_cpuimx35
board-$(CONFIG_MACH_EUKREA_CPUIMX51SD)		:= eukrea_cpuimx51
board-$(CONFIG_MACH_FREESCALE_MX25_3STACK)	:= freescale-mx25-3-stack
board-$(CONFIG_MACH_FREESCALE_MX35_3STACK)	:= freescale-mx35-3-stack
board-$(CONFIG_MACH_GE863)			:= telit-evk-pro3
board-$(CONFIG_MACH_HIGHBANK)			:= highbank
board-$(CONFIG_MACH_IMX21ADS)			:= imx21ads
board-$(CONFIG_MACH_IMX27ADS)			:= imx27ads
board-$(CONFIG_MACH_IMX233_OLINUXINO)	:= imx233-olinuxino
board-$(CONFIG_MACH_MIOA701)			:= mioa701
board-$(CONFIG_MACH_MMCCPU)			:= mmccpu
board-$(CONFIG_MACH_NOMADIK_8815NHK)		:= nhk8815
board-$(CONFIG_MACH_NXDB500)			:= netx
board-$(CONFIG_MACH_OMAP343xSDP)		:= omap343xdsp
board-$(CONFIG_MACH_BEAGLE)			:= beagle
board-$(CONFIG_MACH_BEAGLEBONE)			:= beaglebone
board-$(CONFIG_MACH_OMAP3EVM)			:= omap3evm
board-$(CONFIG_MACH_PANDA)			:= panda
board-$(CONFIG_MACH_ARCHOSG9)			:= archosg9
board-$(CONFIG_MACH_PCM049)			:= pcm049
board-$(CONFIG_MACH_PCA100)			:= phycard-i.MX27
board-$(CONFIG_MACH_PCAAL1)			:= phycard-a-l1
board-$(CONFIG_MACH_PCAAXL2)			:= phycard-a-xl2
board-$(CONFIG_MACH_PCM027)			:= pcm027
board-$(CONFIG_MACH_PCM037)			:= pcm037
board-$(CONFIG_MACH_PCM038)			:= pcm038
board-$(CONFIG_MACH_PCM043)			:= pcm043
board-$(CONFIG_MACH_PCM051)			:= pcm051
board-$(CONFIG_MACH_PM9261)			:= pm9261
board-$(CONFIG_MACH_PM9263)			:= pm9263
board-$(CONFIG_MACH_PM9G45)			:= pm9g45
board-$(CONFIG_MACH_RPI)			:= raspberry-pi
board-$(CONFIG_MACH_SCB9328)			:= scb9328
board-$(CONFIG_MACH_NESO)			:= guf-neso
board-$(CONFIG_MACH_MX23EVK)			:= freescale-mx23-evk
board-$(CONFIG_MACH_CHUMBY)			:= chumby_falconwing
board-$(CONFIG_MACH_TX28)			:= karo-tx28
board-$(CONFIG_MACH_MX28EVK)			:= freescale-mx28-evk
board-$(CONFIG_MACH_CFA10036)			:= crystalfontz-cfa10036
board-$(CONFIG_MACH_FREESCALE_MX51_PDK)		:= freescale-mx51-pdk
board-$(CONFIG_MACH_FREESCALE_MX53_LOCO)	:= freescale-mx53-loco
board-$(CONFIG_MACH_FREESCALE_MX53_SMD)		:= freescale-mx53-smd
board-$(CONFIG_MACH_GUF_CUPID)			:= guf-cupid
board-$(CONFIG_MACH_MINI2440)			:= friendlyarm-mini2440
board-$(CONFIG_MACH_MINI6410)			:= friendlyarm-mini6410
board-$(CONFIG_MACH_TINY6410)			:= friendlyarm-tiny6410
board-$(CONFIG_MACH_QIL_A9260)			:= qil-a9260
board-$(CONFIG_MACH_TNY_A9260)			:= tny-a926x
board-$(CONFIG_MACH_TNY_A9263)			:= tny-a926x
board-$(CONFIG_MACH_TNY_A9G20)			:= tny-a926x
board-$(CONFIG_MACH_USB_A9260)			:= usb-a926x
board-$(CONFIG_MACH_USB_A9263)			:= usb-a926x
board-$(CONFIG_MACH_USB_A9G20)			:= usb-a926x
board-$(CONFIG_MACH_VERSATILEPB)		:= versatile
board-$(CONFIG_MACH_VEXPRESS)			:= vexpress
board-$(CONFIG_MACH_TX25)			:= karo-tx25
board-$(CONFIG_MACH_TQMA53)			:= tqma53
board-$(CONFIG_MACH_TX51)			:= karo-tx51
board-$(CONFIG_MACH_MX6Q_ARM2)			:= freescale-mx6-arm2
board-$(CONFIG_MACH_TOSHIBA_AC100)		:= toshiba-ac100
board-$(CONFIG_MACH_CCMX51)			:= ccxmx51
board-$(CONFIG_MACH_TINY210)			:= friendlyarm-tiny210
board-$(CONFIG_MACH_SABRELITE)			:= freescale-mx6-sabrelite
board-$(CONFIG_MACH_TX53)			:= karo-tx53
board-$(CONFIG_MACH_GUF_VINCELL)		:= guf-vincell
board-$(CONFIG_MACH_EFIKA_MX_SMARTBOOK)		:= efika-mx-smartbook
board-$(CONFIG_MACH_SABRESD)			:= freescale-mx6-sabresd
board-$(CONFIG_MACH_REALQ7)			:= dmo-mx6-realq7
board-$(CONFIG_MACH_ZEDBOARD)			:= avnet-zedboard

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
KBUILD_TARGET := zbarebox.bin
$(KBUILD_BINARY): $(KBUILD_TARGET)
else
KBUILD_BINARY := barebox.bin
KBUILD_TARGET := barebox.bin
endif

barebox.netx: $(KBUILD_BINARY)
	$(Q)scripts/gen_netx_image -i $< -o barebox.netx		\
		--sdramctrl=$(CONFIG_NETX_SDRAM_CTRL)			\
		--sdramtimctrl=$(CONFIG_NETX_SDRAM_TIMING_CTRL)		\
		--memctrl=$(CONFIG_NETX_MEM_CTRL)			\
		--entrypoint=$(CONFIG_TEXT_BASE)			\
		--cookie=$(CONFIG_NETX_COOKIE);

ifeq ($(machine-y),netx)
KBUILD_TARGET := barebox.netx
KBUILD_IMAGE := barebox.netx
endif

barebox.s5p: $(KBUILD_BINARY)
	$(Q)scripts/s5p_cksum $< barebox.s5p

ifeq ($(CONFIG_ARCH_S5PCxx),y)
KBUILD_TARGET := barebox.s5p
KBUILD_IMAGE := barebox.s5p
endif

quiet_cmd_mlo ?= IFT     $@
	cmd_mlo ?= scripts/omap_signGP $< $(TEXT_BASE) 1; \
		     test -e $<.ift && mv $<.ift MLO

MLO: $(KBUILD_BINARY)
	$(call if_changed,mlo)

ifeq ($(CONFIG_OMAP_BUILD_IFT),y)
KBUILD_TARGET := MLO
KBUILD_IMAGE := MLO
endif

quiet_cmd_davinci_ubl_image = UBL-IMG $@
      cmd_davinci_ubl_image = set -e; \
	 scripts/mkublheader $< > $@; \
	 cat $< >> $@

barebox.ubl: $(KBUILD_BINARY) FORCE
	$(call if_changed,davinci_ubl_image)

ifeq ($(CONFIG_ARCH_DAVINCI),y)
KBUILD_TARGET := barebox.ubl
KBUILD_IMAGE := barebox.ubl
endif

quiet_cmd_am35xx_spi_image = SPI-IMG $@
      cmd_am35xx_spi_image = scripts/mk-am35xx-spi-image $< > $@

barebox.spi: $(KBUILD_BINARY) FORCE
	$(call if_changed,am35xx_spi_image)

ifeq ($(CONFIG_OMAP_BUILD_SPI),y)
KBUILD_TARGET := barebox.spi
KBUILD_IMAGE := barebox.spi
endif

barebox.zynq: $(KBUILD_BINARY)
	$(Q)scripts/zynq_mkimage $< $@

ifeq ($(machine-y),zynq)
KBUILD_TARGET := barebox.zynq
KBUILD_IMAGE := barebox.zynq
endif

pbl := arch/arm/pbl
zbarebox.S zbarebox.bin zbarebox: barebox.bin
	$(Q)$(MAKE) $(build)=$(pbl) $(pbl)/$@

archclean:
	$(MAKE) $(clean)=$(pbl)

dts := arch/arm/dts

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(dts) $(dts)/$@

dtbs: scripts
	$(Q)$(MAKE) $(build)=$(dts) dtbs

KBUILD_DTBS := dtbs

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

common-y += $(BOARD) $(MACH)
common-y += arch/arm/lib/ arch/arm/cpu/

common-$(CONFIG_BUILTIN_DTB) += arch/arm/dts/

lds-y	:= arch/arm/lib/barebox.lds

CLEAN_FILES += include/generated/mach-types.h arch/arm/lib/barebox.lds barebox-flash-image

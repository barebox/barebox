KBUILD_DEFCONFIG := qemu_virt64_defconfig

CPPFLAGS	+= -D__ARM__ -fno-strict-aliasing
# Explicitly specifiy 32-bit ARM ISA since toolchain default can be -mthumb:
ifeq ($(CONFIG_CPU_V8),y)
CPPFLAGS	+=$(call cc-option,-maarch64,)
else
CPPFLAGS	+=$(call cc-option,-marm,)
endif

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
CPPFLAGS	+= -mbig-endian
AS		+= -EB
LD		+= -EB
else
CPPFLAGS	+= -mlittle-endian
AS		+= -EL
LD		+= -EL
endif

# Unaligned access is not supported when MMU is disabled, so given how
# at least some of the code would be executed with MMU off, lets be
# conservative and instruct the compiler not to generate any unaligned
# accesses
ifneq ($(CONFIG_CPU_V8),y)
CFLAGS += -mno-unaligned-access
else
CFLAGS += -mstrict-align
endif

# Prevent use of floating point and Advanced SIMD registers.
ifeq ($(CONFIG_CPU_V8),y)
CFLAGS += -mgeneral-regs-only
endif

# This selects which instruction set is used.
# Note that GCC does not numerically define an architecture version
# macro, but instead defines a whole series of macros which makes
# testing for a specific architecture or later rather impossible.
arch-$(CONFIG_CPU_64v8)		:= -D__LINUX_ARM_ARCH__=8 $(call cc-option,-march=armv8-a)
arch-$(CONFIG_CPU_32v7)		:=-D__LINUX_ARM_ARCH__=7 $(call cc-option,-march=armv7-a,-march=armv5t -Wa$(comma)-march=armv7-a)
arch-$(CONFIG_CPU_32v6)		:=-D__LINUX_ARM_ARCH__=6 $(call cc-option,-march=armv6,-march=armv5t -Wa$(comma)-march=armv6)
arch-$(CONFIG_CPU_32v5)		:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4t)
arch-$(CONFIG_CPU_32v4T)	:=-D__LINUX_ARM_ARCH__=4 -march=armv4t

# This selects how we optimise for the processor.
tune-$(CONFIG_CPU_ARM920T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM926T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_XSCALE)	:=$(call cc-option,-mtune=xscale,-mtune=strongarm110) -Wa,-mcpu=xscale

ifeq ($(CONFIG_CPU_V8), y)
CFLAGS_ABI	:=-mabi=lp64
else
ifeq ($(CONFIG_AEABI),y)
CFLAGS_ABI	:=-mabi=aapcs-linux -mno-thumb-interwork
else
CFLAGS_ABI	:=$(call cc-option,-mapcs-32,-mabi=apcs-gnu) $(call cc-option,-mno-thumb-interwork,)
endif
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

ifeq ($(CONFIG_CPU_V8), y)
CPPFLAGS += $(CFLAGS_ABI) $(arch-y) $(tune-y)
AFLAGS   += -include asm/unified.h
export S64 = _64
else
CPPFLAGS += $(CFLAGS_ABI) $(arch-y) $(tune-y) -msoft-float $(CFLAGS_THUMB2)
AFLAGS   += -include asm/unified.h -msoft-float $(AFLAGS_THUMB2)
endif

# Machine directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
machine-$(CONFIG_ARCH_AT91)		:= at91
machine-$(CONFIG_ARCH_BCM283X)		:= bcm283x
machine-$(CONFIG_ARCH_CLPS711X)		:= clps711x
machine-$(CONFIG_ARCH_DAVINCI)		:= davinci
machine-$(CONFIG_ARCH_DIGIC)		:= digic
machine-$(CONFIG_ARCH_EP93XX)		:= ep93xx
machine-$(CONFIG_ARCH_HIGHBANK)		:= highbank
machine-$(CONFIG_ARCH_IMX)		:= imx
machine-$(CONFIG_ARCH_LAYERSCAPE)	:= layerscape
machine-$(CONFIG_ARCH_MXS)		:= mxs
machine-$(CONFIG_ARCH_MVEBU)		:= mvebu
machine-$(CONFIG_ARCH_NOMADIK)		:= nomadik
machine-$(CONFIG_ARCH_OMAP)		:= omap
machine-$(CONFIG_ARCH_PXA)		:= pxa
machine-$(CONFIG_ARCH_ROCKCHIP)		:= rockchip
machine-$(CONFIG_ARCH_SAMSUNG)		:= samsung
machine-$(CONFIG_ARCH_SOCFPGA)		:= socfpga
machine-$(CONFIG_ARCH_STM32MP)		:= stm32mp
machine-$(CONFIG_ARCH_VERSATILE)	:= versatile
machine-$(CONFIG_ARCH_VEXPRESS)		:= vexpress
machine-$(CONFIG_ARCH_TEGRA)		:= tegra
machine-$(CONFIG_ARCH_UEMD)		:= uemd
machine-$(CONFIG_ARCH_ZYNQ)		:= zynq
machine-$(CONFIG_ARCH_ZYNQMP)		:= zynqmp
machine-$(CONFIG_ARCH_QEMU)		:= qemu


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
board-$(CONFIG_MACH_MINI2440)			+= friendlyarm-mini2440
board-$(CONFIG_MACH_MINI6410)			+= friendlyarm-mini6410
board-$(CONFIG_MACH_PCM027)			+= phytec-phycore-pxa270
board-$(CONFIG_MACH_TINY210)			+= friendlyarm-tiny210
board-$(CONFIG_MACH_TINY6410)			+= friendlyarm-tiny6410

machdirs := $(patsubst %,arch/arm/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += --gc-sections

ifdef CONFIG_RELOCATABLE
LDFLAGS_barebox += -pie
else
LDFLAGS_barebox += -static
endif

KBUILD_BINARY := barebox.bin

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
      cmd_am35xx_spi_image = scripts/mk-omap-image -s -a $(TEXT_BASE) $< > $@

barebox.spi: $(KBUILD_BINARY) FORCE
	$(call if_changed,am35xx_spi_image)

MLO.spi: MLO FORCE
	$(call if_changed,am35xx_spi_image)

ifeq ($(CONFIG_OMAP_BUILD_SPI),y)
KBUILD_IMAGE := MLO.spi
endif

quiet_cmd_zynq_image = ZYNQ-IMG $@
      cmd_zynq_image = scripts/zynq_mkimage $< $@

barebox.zynq: $(KBUILD_BINARY) FORCE
	$(call if_changed,zynq_image)

ifeq ($(machine-y),zynq)
KBUILD_IMAGE := barebox.zynq
endif

quiet_cmd_canon_a1100_image = DD      $@
      cmd_canon_a1100_image = scripts/canon-a1100-image $< $@ || \
	echo "WARNING: Couldn't create Canon A1100 image due to previous errors."
barebox.canon-a1100.bin: $(KBUILD_BINARY) FORCE
	$(call if_changed,canon_a1100_image)

ifeq ($(CONFIG_MACH_CANON_A1100),y)
KBUILD_IMAGE := barebox.canon-a1100.bin
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

barebox.imximg: $(KBUILD_BINARY) FORCE
	$(call if_changed,imx_image,$(CFG_$(@F)),)

boarddir = $(srctree)/arch/arm/boards
imxcfg-$(CONFIG_MACH_FREESCALE_MX53_SMD) += $(boarddir)/freescale-mx53-smd/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_TX51) += $(boarddir)/karo-tx51/flash-header-karo-tx51.imxcfg
imxcfg-$(CONFIG_MACH_EUKREA_CPUIMX51SD) += $(boarddir)/eukrea_cpuimx51/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_FREESCALE_MX25_3STACK) += $(boarddir)/freescale-mx25-3ds/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_FREESCALE_MX35_3STACK) += $(boarddir)/freescale-mx35-3ds/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_EUKREA_CPUIMX25) += $(boarddir)/eukrea_cpuimx25/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_EUKREA_CPUIMX35) += $(boarddir)/eukrea_cpuimx35/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_PCM043) += $(boarddir)/phytec-phycore-imx35/flash-header.imxcfg
imxcfg-$(CONFIG_MACH_KINDLE3) += $(boarddir)/kindle3/flash-header.imxcfg
ifneq ($(imxcfg-y),)
CFG_barebox.imximg := $(imxcfg-y)
KBUILD_IMAGE  := barebox.imximg
endif

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
common-y += arch/arm/cpu/

ifeq ($(CONFIG_CPU_V8), y)
common-y += arch/arm/lib64/
else
common-y += arch/arm/lib32/ arch/arm/crypto/
endif

common-$(CONFIG_OFTREE) += arch/arm/dts/

ifeq ($(CONFIG_CPU_V8), y)
lds-y	:= arch/arm/lib64/barebox.lds
else
lds-y	:= arch/arm/lib32/barebox.lds
endif

common- += $(patsubst %,arch/arm/boards/%/,$(board-))

CLEAN_FILES += include/generated/mach-types.h barebox-flash-image
CLEAN_FILES += arch/arm/lib64/barebox.lds
CLEAN_FILES += arch/arm/lib32/barebox.lds

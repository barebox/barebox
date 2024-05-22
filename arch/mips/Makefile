# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := qemu-malta_defconfig

generated_configs += $(patsubst %, qemu-malta%_defconfig, el 64 64el)
qemu-maltael_defconfig:
	$(call merge_into_defconfig,qemu-malta_defconfig,le)
qemu-malta64_defconfig:
	$(call merge_into_defconfig,qemu-malta_defconfig,mips64)
qemu-malta64el_defconfig:
	$(call merge_into_defconfig,qemu-malta_defconfig,mips64 le)

#
# Select the object file format to substitute into the linker script.
#
ifdef CONFIG_CPU_LITTLE_ENDIAN
32bit-emul		= elf32ltsmip
64bit-emul		= elf64ltsmip
else
32bit-emul		= elf32btsmip
64bit-emul		= elf64btsmip
endif

KBUILD_CPPFLAGS += -D__MIPS__ -fno-strict-aliasing -fno-merge-constants

cflags-y += -G 0 -mno-abicalls -fno-pic -pipe
cflags-y += -Wall -Wstrict-prototypes \
	-Wno-uninitialized -Wno-format -Wno-main

ifdef CONFIG_32BIT
ld-emul		= $(32bit-emul)
cflags-y	+= -mabi=32
endif

ifdef CONFIG_64BIT
ld-emul		= $(64bit-emul)
cflags-y	+= -mabi=64
endif

undef-all += -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__
undef-all += -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__
predef-be += -DMIPSEB -D_MIPSEB -D__MIPSEB -D__MIPSEB__
predef-le += -DMIPSEL -D_MIPSEL -D__MIPSEL -D__MIPSEL__

ifdef CONFIG_CPU_BIG_ENDIAN
cflags-y += $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' && echo -EB $(undef-all) $(predef-be))
mips-ldflags-y += $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' && echo -EB )
endif

ifdef CONFIG_CPU_LITTLE_ENDIAN
cflags-y += $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' || echo -EL $(undef-all) $(predef-le))
mips-ldflags-y += $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' || echo -EL )
endif

KBUILD_LDFLAGS += $(mips-ldflags-y) -m $(ld-emul)
LDFLAGS_barebox += $(KBUILD_LDFLAGS)
LDFLAGS_pbl += $(KBUILD_LDFLAGS)

#
# CPU-dependent compiler/assembler options for optimization.
#
cflags-$(CONFIG_CPU_MIPS32_R1) += $(call cc-option,-march=mips32,-mips32 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32) -Wa,-mips32 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS32_R2) += $(call cc-option,-march=mips32r2,-mips32r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32) -Wa,-mips32r2 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS64_R1) += $(call cc-option,-march=mips64,-mips64 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS64) -Wa,-mips64 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS64_R2) += $(call cc-option,-march=mips64r2,-mips64r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS64) -Wa,-mips64r2 -Wa,--trap
cflags-$(CONFIG_CPU_GS232)     += $(call cc-option,-march=mips32r2,-mips32r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32) -Wa,-mips32r2 -Wa,--trap

ifdef CONFIG_64BIT
MIPS_TEXT_BASE = $(subst 0x,0xffffffff,$(CONFIG_TEXT_BASE))
else
MIPS_TEXT_BASE = $(CONFIG_TEXT_BASE)
endif

KBUILD_CPPFLAGS += -DTEXT_BASE=$(MIPS_TEXT_BASE)

ifndef CONFIG_MODULES
# Add cleanup flags
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections --emit-relocs
endif

ifdef CONFIG_IMAGE_COMPRESSION
KBUILD_BINARY := arch/mips/pbl/zbarebox.bin
KBUILD_TARGET := zbarebox.bin
$(KBUILD_BINARY): $(KBUILD_TARGET)
else
KBUILD_BINARY := barebox.bin
KBUILD_TARGET := barebox.bin
endif

machine-$(CONFIG_MACH_MIPS_MALTA)	:= malta
machine-$(CONFIG_MACH_MIPS_AR231X)	:= ar231x
machine-$(CONFIG_MACH_MIPS_ATH79)	:= ath79
machine-$(CONFIG_MACH_MIPS_BCM47XX)	:= bcm47xx
machine-$(CONFIG_MACH_MIPS_LOONGSON)	:= loongson
machine-$(CONFIG_MACH_MIPS_XBURST)	:= xburst
machdirs := $(patsubst %,arch/mips/mach-%/,$(machine-y))

KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

ifneq ($(machine-y),)
MACH  := arch/mips/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(MACH)
common-y += arch/mips/lib/
common-y += arch/mips/boot/
common-y += arch/mips/boards/

KBUILD_CPPFLAGS += $(cflags-y)
KBUILD_CFLAGS += $(cflags-y)

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)   := arch/mips/lib/barebox.lds

cmd_barebox__ ?= $(LD) $(LDFLAGS_barebox) -o $@ \
      -T $(BAREBOX_LDS)                         \
      --whole-archive $(BAREBOX_OBJS) --no-whole-archive        \
      $(filter-out $(BAREBOX_LDS) $(BAREBOX_OBJS) FORCE ,$^);	\
	$(objtree)/scripts/mips-relocs $@


CLEAN_FILES    += arch/mips/lib/barebox.lds barebox.map barebox.S

pbl := arch/mips/pbl
zbarebox.S zbarebox.bin zbarebox: barebox.bin
	$(Q)$(MAKE) $(build)=$(pbl) $(pbl)/$@

archclean:
	$(MAKE) $(clean)=$(pbl)

dts := arch/mips/dts

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(dts) $(dts)/$@

common-$(CONFIG_OFTREE) += arch/mips/dts/

KBUILD_IMAGE ?= $(KBUILD_BINARY)

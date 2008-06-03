#
# (C) Copyright 2007 Carsten Schlote <schlote@vahanus.net>
# See file CREDITS for list of people who contributed to this project.
#
# This file is part of U-Boot V2.
#
# U-Boot V2 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# U-Boot V2 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with U-Boot V2.  If not, see <http://www.gnu.org/licenses/>.
#

CPPFLAGS += -Wall -isystem $(gccincdir) -pipe -D __M68K__ \
	-fno-strict-aliasing


machine-$(CONFIG_ARCH_MCF54xx)	   := mcfv4e
board-$(CONFIG_MACH_KPUKDR1)	   := kp_ukd_r1
board-$(CONFIG_MACH_KPUKDR1_NUM)   := kp_ukd_r1_num
board-$(CONFIG_MACH_KPUKDR2)	   := kp_ukd_r2
board-$(CONFIG_MACH_KPUKDR2_NUM)   := kp_ukd_r2_num
board-$(CONFIG_MACH_PCM982_5485)   := phycore_mcf54xx
board-$(CONFIG_MACH_PCM982_5475)   := phycore_mcf54xx
board-$(CONFIG_MACH_PCM982_5475_NUM) := phycore_mcf54xx_num
board-$(CONFIG_MACH_PCM982_54455)  := phycore_mcf5445x

cpu-$(CONFIG_MCFV4E)               := mcfv4e
cpu-$(CONFIG_MCFV4M)               := mcfv4m

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE) -P
AFLAGS += -gdwarf-2 -save-temps
# FIXME - remove overide
CFLAGS += -msoft-float -mcfv4e -gdwarf-2 -feliminate-unused-debug-types \
	-fmerge-all-constants
# Incompatible code in U-Boot for -std=c99
LDFLAGS_uboot  :=-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
#  --verbose --stats

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-m68k/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-m68k/arch -> include/asm-m68k/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-m68k
	$(Q)ln -fsn $(srctree)/include/asm-m68k/$(INCDIR) include/asm-m68k/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-m68k/arch
endif
	@echo '  SYMLINK include/asm-m68k/proc -> include/asm-m68k/proc-mcfv4e'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-m68k
	$(Q)ln -fsn $(srctree)/include/asm-m68k/proc-mcfv4e include/asm-m68k/proc
else
	$(Q)ln -fsn proc-mcfv4e include/asm-m68k/proc
endif
	@touch $@


all: $(KBUILD_IMAGE)

archprepare: maketools

PHONY += maketools
maketools: include/asm-m68k/.arch


ifneq ($(board-y),)
BOARD  := board/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/m68k/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/m68k/lib/ arch/m68k/cpu/

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)   := arch/m68k/lib/u-boot.lds
lds-$(CONFIG_BOARD_LINKER_SCRIPT)     := $(BOARD)/u-boot.lds

CLEAN_FILES    += arch/m68k/lib/u-boot.lds
MRPROPER_FILES += include/asm-m68k/arch include/asm-m68k/proc

# SPDX-License-Identifier: GPL-2.0
VERSION = 2024
PATCHLEVEL = 10
SUBLEVEL = 0
EXTRAVERSION =
NAME = None

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

# That's our default target when none is given on the command line
PHONY := _all
_all:

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.a targets which finally
# turn into vmlinux), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

ifneq ($(sub_make_done),1)

# Do not use make's built-in rules and variables
# (this increases performance and avoids hard-to-debug behaviour)
MAKEFLAGS += -rR

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
unexport GREP_OPTIONS

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed.
# If it is set to "silent_", nothing will be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.
# If KBUILD_VERBOSE equals 1 then the above command is displayed.
# If KBUILD_VERBOSE equals 2 then give the reason why each target is rebuilt.
#
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands
# make-4.0 (and later) keep single letter options in the 1st word of MAKEFLAGS.

ifeq ($(filter 3.%,$(MAKE_VERSION)),)
silence:=$(findstring s,$(firstword -$(MAKEFLAGS)))
else
silence:=$(findstring s,$(filter-out --%,$(MAKEFLAGS)))
endif

ifeq ($(silence),s)
quiet=silent_
endif

export quiet Q KBUILD_VERBOSE

# Kbuild will save output files in the current working directory.
# This does not need to match to the root of the kernel source tree.
#
# For example, you can do this:
#
#  cd /dir/to/store/output/files; make -f /dir/to/kernel/source/Makefile
#
# If you want to save output files in a different location, there are
# two syntaxes to specify it.
#
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the output directory.
# export KBUILD_OUTPUT=dir/to/store/output/files/; make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.

# Do we want to change the working directory?
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

ifneq ($(KBUILD_OUTPUT),)
# Make's built-in functions such as $(abspath ...), $(realpath ...) cannot
# expand a shell special character '~'. We use a somewhat tedious way here.
abs_objtree := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) && pwd)
$(if $(abs_objtree),, \
     $(error failed to create output directory "$(KBUILD_OUTPUT)"))

# $(realpath ...) resolves symlinks
abs_objtree := $(realpath $(abs_objtree))
else
abs_objtree := $(CURDIR)
endif # ifneq ($(KBUILD_OUTPUT),)

ifeq ($(abs_objtree),$(CURDIR))
# Suppress "Entering directory ..." unless we are changing the work directory.
MAKEFLAGS += --no-print-directory
else
need-sub-make := 1
endif

abs_srctree := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

ifneq ($(words $(subst :, ,$(abs_srctree))), 1)
$(error source directory cannot contain spaces or colons)
endif

ifneq ($(abs_srctree),$(abs_objtree))
# Look for make include files relative to root of kernel src
#
# This does not become effective immediately because MAKEFLAGS is re-parsed
# once after the Makefile is read. We need to invoke sub-make.
MAKEFLAGS += --include-dir=$(abs_srctree)
need-sub-make := 1
endif

ifneq ($(filter 3.%,$(MAKE_VERSION)),)
# 'MAKEFLAGS += -rR' does not immediately become effective for GNU Make 3.x
# We need to invoke sub-make to avoid implicit rules in the top Makefile.
need-sub-make := 1
# Cancel implicit rules for this Makefile.
$(lastword $(MAKEFILE_LIST)): ;
endif

export abs_srctree abs_objtree
export sub_make_done := 1

ifeq ($(need-sub-make),1)

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make $(lastword $(MAKEFILE_LIST)), $(MAKECMDGOALS)) _all: sub-make
	@:

# Invoke a second make in the output directory, passing relevant variables
sub-make:
	$(Q)$(MAKE) -C $(abs_objtree) -f $(abs_srctree)/Makefile $(MAKECMDGOALS)

endif # need-sub-make
endif # sub_make_done

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(need-sub-make),)

# CDPATH can have sideeffects; disable, since we do know where we want to cd to
export CDPATH=

# Do not print "Entering directory ...",
# but we want to display it when entering to the output directory
# so that IDEs/editors are able to understand relative filenames.
MAKEFLAGS += --no-print-directory

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/dev-tools/sparse.rst" for more details,
# including where to get the "sparse" utility.

ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# Use make M=dir or set the environment variable KBUILD_EXTMOD to specify the
# directory of external module to build. Setting M= takes precedence.
ifeq ("$(origin M)", "command line")
  KBUILD_EXTMOD := $(M)
endif

export KBUILD_CHECKSRC KBUILD_EXTMOD

ifeq ($(abs_srctree),$(abs_objtree))
        # building in the source tree
	building_out_of_srctree :=
else
	building_out_of_srctree := 1
endif

srctree		:= $(abs_srctree)
objtree		:= $(abs_objtree)
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)

export building_out_of_srctree srctree objtree VPATH

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

version_h := include/generated/version.h

clean-targets := %clean mrproper cleandocs
no-dot-config-targets := $(clean-targets) \
			 cscope gtags TAGS tags help% %docs \
			 $(version_h) bareboxversion outputmakefile
no-sync-config-targets := $(no-dot-config-targets) install %install \
			   kernelrelease

config-build	:=
mixed-build	:=
need-config	:= 1
may-sync-config	:= 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		need-config :=
	endif
endif

ifneq ($(filter $(no-sync-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-sync-config-targets), $(MAKECMDGOALS)),)
		may-sync-config :=
	endif
endif

ifneq ($(KBUILD_EXTMOD),)
	may-sync-config :=
endif

ifeq ($(KBUILD_EXTMOD),)
        ifneq ($(filter config %config,$(MAKECMDGOALS)),)
		config-build := 1
                ifneq ($(words $(MAKECMDGOALS)),1)
			mixed-build := 1
                endif
        endif
endif

# For "make -j clean all", "make -j mrproper defconfig all", etc.
ifneq ($(filter $(clean-targets),$(MAKECMDGOALS)),)
        ifneq ($(filter-out $(clean-targets),$(MAKECMDGOALS)),)
		mixed-build := 1
        endif
endif

ifdef mixed-build
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

PHONY += $(MAKECMDGOALS) __build_one_by_one

$(filter-out __build_one_by_one, $(MAKECMDGOALS)): __build_one_by_one
	@:

__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

else # !mixed-build

include scripts/Kbuild.include

# Read KERNELRELEASE from include/config/kernel.release (if it exists)
KERNELRELEASE = $(shell cat include/config/kernel.release 2> /dev/null)
KERNELVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)
BUILDSYSTEM_VERSION =
export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION BUILDSYSTEM_VERSION

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=ia64
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=ia64-linux-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables

ARCH            ?= sandbox
CROSS_COMPILE   ?=

# Architecture as present in compile.h
UTS_MACHINE := $(ARCH)
SRCARCH 	:= $(ARCH)

ifeq ($(ARCH),arm64)
       SRCARCH := arm
endif

ifeq ($(ARCH),i386)
       SRCARCH := x86
endif

ifeq ($(ARCH),x86_64)
       SRCARCH := x86
endif

# Support ARCH=ppc for backward compatibility
ifeq ($(ARCH),ppc)
       SRCARCH := powerpc
endif

ifeq ($(ARCH),um)
       SRCARCH := sandbox
endif

KCONFIG_CONFIG	?= .config

PKG_CONFIG ?= pkg-config
CROSS_PKG_CONFIG ?= $(CROSS_COMPILE)pkg-config

export KCONFIG_CONFIG CROSS_PKG_CONFIG PKG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOST_LFS_CFLAGS := $(shell getconf LFS_CFLAGS 2>/dev/null)
HOST_LFS_LDFLAGS := $(shell getconf LFS_LDFLAGS 2>/dev/null)
HOST_LFS_LIBS := $(shell getconf LFS_LIBS 2>/dev/null)

ifneq ($(LLVM),)
ifneq ($(filter %/,$(LLVM)),)
LLVM_PREFIX := $(LLVM)
else ifneq ($(filter -%,$(LLVM)),)
LLVM_SUFFIX := $(LLVM)
endif

HOSTCC	= $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
HOSTCXX	= $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)
else
HOSTCC	= gcc
HOSTCXX	= g++
endif

KBUILD_USERHOSTCFLAGS := -Wall -Wmissing-prototypes -Wstrict-prototypes \
			      -O2 -fomit-frame-pointer -std=gnu11
KBUILD_USERCFLAGS  := $(KBUILD_USERHOSTCFLAGS) $(USERCFLAGS)
KBUILD_USERLDFLAGS := $(USERLDFLAGS)

KBUILD_HOSTCFLAGS   := $(KBUILD_USERHOSTCFLAGS) $(HOST_LFS_CFLAGS) $(HOSTCFLAGS)
KBUILD_HOSTCXXFLAGS := -Wall -O2 $(HOST_LFS_CFLAGS) $(HOSTCXXFLAGS)
KBUILD_HOSTLDFLAGS  := $(HOST_LFS_LDFLAGS) $(HOSTLDFLAGS)
KBUILD_HOSTLDLIBS   := $(HOST_LFS_LIBS) $(HOSTLDLIBS)

# Make variables (CC, etc...)
CPP		= $(CC) -E
ifneq ($(LLVM),)
CC		= $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
LD		= $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
AR		= $(LLVM_PREFIX)llvm-ar$(LLVM_SUFFIX)
NM		= $(LLVM_PREFIX)llvm-nm$(LLVM_SUFFIX)
OBJCOPY		= $(LLVM_PREFIX)llvm-objcopy$(LLVM_SUFFIX)
OBJDUMP		= $(LLVM_PREFIX)llvm-objdump$(LLVM_SUFFIX)
READELF		= $(LLVM_PREFIX)llvm-readelf$(LLVM_SUFFIX)
STRIP		= $(LLVM_PREFIX)llvm-strip$(LLVM_SUFFIX)
else
CC		= $(CROSS_COMPILE)gcc
LD		= $(CROSS_COMPILE)ld
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
READELF		= $(CROSS_COMPILE)readelf
STRIP		= $(CROSS_COMPILE)strip
endif
LEX		= flex
YACC		= bison
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
DEPMOD		= /sbin/depmod
KALLSYMS	= scripts/kallsyms
PERL		= perl
PYTHON3		= python3
CHECK		= sparse
BASH		= bash

CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ -Wbitwise $(CF)
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=
CFLAGS_MODULE	=
AFLAGS_MODULE	=

LDFLAGS_MODULE  = -T common/module.lds

# When compiling out-of-tree modules, put MODVERDIR in the module
# tree rather than in the kernel tree. The kernel tree might
# even be read-only.
export MODVERDIR := $(if $(KBUILD_EXTMOD),$(firstword $(KBUILD_EXTMOD))/).tmp_versions

# Use USERINCLUDE when you must reference the UAPI directories only.
USERINCLUDE    := \
		-I$(srctree)/arch/$(SRCARCH)/include/uapi \
		-I$(objtree)/arch/$(SRCARCH)/include/generated/uapi \
		-I$(srctree)/include/uapi \
		-I$(objtree)/include/generated/uapi \
                -include $(srctree)/include/linux/kconfig.h

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    := -Iinclude \
                   $(if $(building_out_of_srctree), -I$(srctree)/include) \
                   -I$(srctree)/dts/include \
		   -I$(srctree)/arch/$(SRCARCH)/include \
		   -I$(objtree)/arch/$(SRCARCH)/include \
		   $(USERINCLUDE)

KBUILD_CPPFLAGS        := -D__KERNEL__ -D__BAREBOX__ $(LINUXINCLUDE) -fno-builtin -ffreestanding

KBUILD_CFLAGS   := -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs \
		   -fno-strict-aliasing -fno-common -fshort-wchar \
		   -Werror=implicit-function-declaration -Werror=implicit-int \
		   -Werror=int-conversion \
		   -Os -pipe -Wmissing-prototypes -std=gnu11
KBUILD_AFLAGS          := -D__ASSEMBLY__
KBUILD_AFLAGS_KERNEL :=
KBUILD_CFLAGS_KERNEL :=
KBUILD_AFLAGS_MODULE := -DMODULE
KBUILD_CFLAGS_MODULE := -DMODULE
CLANG_FLAGS :=

LDFLAGS_barebox	:= -Map barebox.map

# Avoid 'Not enough room for program headers' error on binutils 2.28 onwards.
LDFLAGS_common += $(call ld-option, --no-dynamic-linker)
# Avoid 'missing .note.GNU-stack section implies executable stack' warnings on binutils 2.39+
LDFLAGS_common += -z noexecstack
# Avoid '... has a LOAD segment with RWX permissions' warnings on binutils 2.39+
LDFLAGS_common += $(call ld-option,--no-warn-rwx-segments)

LDFLAGS_barebox += $(LDFLAGS_common)
LDFLAGS_pbl += $(LDFLAGS_common)
LDFLAGS_elf += $(LDFLAGS_common) --nmagic -s

export ARCH SRCARCH CONFIG_SHELL BASH HOSTCC KBUILD_HOSTCFLAGS CROSS_COMPILE LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP MAKE AWK GENKSYMS PERL PYTHON3 UTS_MACHINE
export LEX YACC
export HOSTCXX CHECK CHECKFLAGS
export KBUILD_HOSTCXXFLAGS KBUILD_HOSTLDFLAGS KBUILD_HOSTLDLIBS LDFLAGS_MODULE
export KBUILD_USERCFLAGS KBUILD_USERLDFLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS KBUILD_LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL CFLAGS_MODULE
export KBUILD_AFLAGS AFLAGS_KERNEL AFLAGS_MODULE
export KBUILD_AFLAGS_MODULE KBUILD_CFLAGS_MODULE
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export LDFLAGS_barebox LDFLAGS_pbl LDFLAGS_elf

export CFLAGS_UBSAN
export CFLAGS_KASAN CFLAGS_KASAN_NOSANITIZE

# Files to ignore in find ... statements

export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn \
			 --exclude CVS --exclude .pc --exclude .hg --exclude .git

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/basic/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

PHONY += outputmakefile
# Before starting out-of-tree build, make sure the source tree is clean.
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
# At the same time when output Makefile generated, generate .gitignore to
# ignore whole output directory
outputmakefile:
ifdef building_out_of_srctree
	$(Q)if [ -f $(srctree)/.config -o \
		 -d $(srctree)/include/config -o \
		 -d $(srctree)/arch/$(SRCARCH)/include/generated ]; then \
		echo >&2 "***"; \
		echo >&2 "*** The source tree is not clean, please run 'make$(if $(findstring command line, $(origin ARCH)), ARCH=$(ARCH)) mrproper'"; \
		echo >&2 "*** in $(abs_srctree)";\
		echo >&2 "***"; \
		false; \
	fi
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile $(srctree)
	$(Q)test -e .gitignore || \
	{ echo "# this is build directory, ignore it"; echo "*"; } > .gitignore
endif

ifeq ($(CONFIG_CC_IS_CLANG),y)
include $(srctree)/scripts/Makefile.clang
endif

ifdef config-build
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

include $(srctree)/scripts/Makefile.defconf

# Read arch specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
include $(srctree)/arch/$(SRCARCH)/Makefile
export KBUILD_DEFCONFIG

config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else #!config-build
# ===========================================================================
# Build targets only - this includes barebox, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
ifeq ($(KBUILD_EXTMOD),)
_all: all
else
_all: modules
endif

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_MODULES :=
KBUILD_BUILTIN := 1

# If we have only "make modules", don't compile built-in objects.
# When we're building modules with modversions, we need to consider
# the built-in objects during the descend as well, in order to
# make sure the checksums are up to date before we record them.

ifeq ($(MAKECMDGOALS),modules)
  KBUILD_BUILTIN := $(if $(CONFIG_MODVERSIONS),1)
endif

# If we have "make <whatever> modules", compile modules
# in addition to whatever we do anyway.
# Just "make" or "make all" shall build modules as well

ifneq ($(filter all _all modules %compile_commands.json,$(MAKECMDGOALS)),)
  KBUILD_MODULES := 1
endif

ifeq ($(MAKECMDGOALS),)
  KBUILD_MODULES := 1
endif

export KBUILD_MODULES KBUILD_BUILTIN

ifdef need-config
include include/config/auto.conf
endif

# We need some generic definitions.
include $(srctree)/scripts/Makefile.lib

# Objects we will link into barebox / subdirs we need to visit
common-y		:= common/ drivers/ commands/ lib/ crypto/ net/ fs/ firmware/

include $(srctree)/arch/$(SRCARCH)/Makefile

common-$(CONFIG_EFI)	+= efi/
common-y		+= test/

ifdef need-config
ifdef may-sync-config
# Read in dependencies to all Kconfig* files, make sure to run syncconfig if
# changes are detected. This should be included after arch/$(SRCARCH)/Makefile
# because some architectures define CROSS_COMPILE there.
include include/config/auto.conf.cmd

$(KCONFIG_CONFIG):
	@echo >&2 '***'
	@echo >&2 '*** Configuration file "$@" not found!'
	@echo >&2 '***'
	@echo >&2 '*** Please run some configurator (e.g. "make oldconfig" or'
	@echo >&2 '*** "make menuconfig" or "make xconfig").'
	@echo >&2 '***'
	@/bin/false

# The actual configuration files used during the build are stored in
# include/generated/ and include/config/. Update them if .config is newer than
# include/config/auto.conf (which mirrors .config).
#
# This exploits the 'multi-target pattern rule' trick.
# The syncconfig should be executed only once to make all the targets.
# (Note: use the grouped target '&:' when we bump to GNU Make 4.3)
quiet_cmd_syncconfig = SYNC    $@
      cmd_syncconfig = $(MAKE) -f $(srctree)/Makefile syncconfig

%/config/auto.conf %/config/auto.conf.cmd %/generated/autoconf.h: $(KCONFIG_CONFIG)
	+$(call cmd,syncconfig)
else # !may-sync-config
# External modules and some install targets need include/generated/autoconf.h
# and include/config/auto.conf but do not care if they are up-to-date.
# Use auto.conf to trigger the test
PHONY += include/config/auto.conf

include/config/auto.conf:
	$(Q)test -e include/generated/autoconf.h -a -e $@ || (		\
	echo >&2;							\
	echo >&2 "  ERROR: Kernel configuration is invalid.";		\
	echo >&2 "         include/generated/autoconf.h or $@ are missing.";\
	echo >&2 "         Run 'make oldconfig && make prepare' on kernel src to fix it.";	\
	echo >&2 ;							\
	/bin/false)

endif # may-sync-config
endif # need-config

KBUILD_CFLAGS		+= -ggdb3

ifdef CONFIG_FRAME_POINTER
KBUILD_CFLAGS	+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
KBUILD_CFLAGS	+= $(call cc-disable-warning,frame-address,)
endif

KBUILD_CFLAGS-$(CONFIG_CC_IS_CLANG) += -Wno-gnu

KBUILD_CFLAGS-$(CONFIG_WERROR) += -Werror

# This warning generated too much noise in a regular build.
# Use make W=1 to enable this warning (see scripts/Makefile.build)
KBUILD_CFLAGS += $(call cc-disable-warning, unused-but-set-variable)

KBUILD_CFLAGS += $(call cc-disable-warning, trampolines)

KBUILD_CFLAGS += $(call cc-option, -fno-delete-null-pointer-checks,)

# Tell gcc to never replace conditional load with a non-conditional one
ifdef CONFIG_CC_IS_GCC
# gcc-10 renamed --param=allow-store-data-races=0 to
# -fno-allow-store-data-races.
KBUILD_CFLAGS	+= $(call cc-option,--param=allow-store-data-races=0)
KBUILD_CFLAGS	+= $(call cc-option,-fno-allow-store-data-races)
endif

# disable invalid "can't wrap" optimizations for signed / pointers
KBUILD_CFLAGS	+= $(call cc-option,-fno-strict-overflow)

# Make sure -fstack-check isn't enabled (like gentoo apparently did)
KBUILD_CFLAGS  += $(call cc-option,-fno-stack-check)

# ensure -fcf-protection is disabled as it is incompatible with our sjlj
# Platforms that have their setjmp appropriately implemented may override this
KBUILD_CFLAGS += $(call cc-option,-fcf-protection=none)

# We don't have the necessary infrastructure to benefit from ARMv8.3+ pointer
# authentication. On older CPUs, they are interpreted as NOPs bloating the code
KBUILD_CFLAGS += $(call cc-option,-mbranch-protection=none)

KBUILD_CFLAGS   += $(call cc-disable-warning, address-of-packed-member)

# Align the bit size of userspace programs with the kernel
KBUILD_USERCFLAGS  += $(filter -m32 -m64, $(KBUILD_CFLAGS))
KBUILD_USERLDFLAGS += $(filter -m32 -m64, $(KBUILD_CFLAGS))

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CHECKFLAGS     += $(NOSTDINC_FLAGS)

# warn about C99 declaration after statement
KBUILD_CFLAGS += $(call cc-option,-Wdeclaration-after-statement,)

# warn about e.g. (unsigned)x < 0
KBUILD_CFLAGS += $(call cc-option,-Wtype-limits)

# disable pointer signed / unsigned warnings in gcc 4.0
KBUILD_CFLAGS += $(call cc-option,-Wno-pointer-sign,)

# change __FILE__ to the relative path from the srctree
KBUILD_CFLAGS += $(call cc-option,-fmacro-prefix-map=$(srctree)/=)

KBUILD_CFLAGS += $(KBUILD_CFLAGS-y)

include-y +=scripts/Makefile.ubsan
include-$(CONFIG_KASAN)         += scripts/Makefile.kasan

include $(addprefix $(srctree)/, $(include-y))

# KBUILD_IMAGE: Default barebox image to build
# Depending on the architecture, this can be either compressed or not.
# It will also include any necessary headers to be bootable.
export KBUILD_IMAGE ?= barebox.bin
# KBUILD_BINARY: Raw barebox binary
# This variable is set in case the architecture prepends a header and
# points to a binary that can be loaded directly into RAM and executed.
export KBUILD_BINARY ?= barebox.bin
# KBUILD_IMAGE and _BINARY may be overruled on the command line or
# set in the environment.
# Also any assignments in arch/$(SRCARCH)/Makefile take precedence over
# the default value.

barebox-flash-image: $(KBUILD_IMAGE) FORCE
	$(call if_changed,ln)

barebox-flash-images: $(KBUILD_IMAGE)
	@echo $^ > $@

images: barebox.bin FORCE
	$(Q)$(MAKE) $(build)=images $@
images/%.s: barebox.bin FORCE
	$(Q)$(MAKE) $(build)=images $@

ifdef CONFIG_EFI_STUB
all: barebox.bin images barebox.efi
barebox.efi: FORCE
	$(Q)ln -fsn images/barebox-dt-2nd.img $@
else ifdef CONFIG_PBL_IMAGE
all: barebox.bin images
else
all: barebox-flash-image barebox-flash-images
endif

common-$(CONFIG_PBL_IMAGE)	+= pbl/
common-$(CONFIG_DEFAULT_ENVIRONMENT) += defaultenv/

barebox-dirs	:= $(patsubst %/,%,$(filter %/, $(common-y)))

barebox-alldirs	:= $(sort $(barebox-dirs) $(patsubst %/,%,$(filter %/, \
		     $(common-n) $(common-) \
		     $(core-n) $(core-) $(drivers-n) $(drivers-) \
		     $(net-n)  $(net-)  $(libs-n)    $(libs-))))

pbl-common-y	:= $(patsubst %/, %/built-in.pbl.a, $(common-y))
common-y	:= $(patsubst %/, %/built-in.a, $(common-y))

ifeq ($(CONFIG_DEFAULT_COMPRESSION_GZIP),y)
DEFAULT_COMPRESSION_SUFFIX := .gz
endif
ifeq ($(CONFIG_DEFAULT_COMPRESSED_BZIP2),y)
DEFAULT_COMPRESSION_SUFFIX := .bz2
endif
ifeq ($(CONFIG_DEFAULT_COMPRESSION_LZO),y)
DEFAULT_COMPRESSION_SUFFIX := .lzo
endif
ifeq ($(CONFIG_DEFAULT_COMPRESSION_LZ4),y)
DEFAULT_COMPRESSION_SUFFIX := .lz4
endif
ifeq ($(CONFIG_DEFAULT_COMPRESSION_NONE),y)
DEFAULT_COMPRESSION_SUFFIX :=
endif
export DEFAULT_COMPRESSION_SUFFIX

# Build barebox
# ---------------------------------------------------------------------------
# barebox is built from the objects selected by $(barebox-init) and
# $(barebox-main). Most are built-in.a files from top-level directories
# in the kernel tree, others are specified in arch/$(SRCARCH)/Makefile.
# Ordering when linking is important, and $(barebox-init) must be first.
#
# FIXME: This picture is wrong for barebox. We have no init, driver, mm
#
# barebox
#   ^
#   |
#   +-< $(barebox-init)
#   |   +--< init/version.o + more
#   |
#   +--< $(barebox-main)
#   |    +--< driver/built-in.a mm/built-in.a + more
#   |
#   +-< kallsyms.o (see description in CONFIG_KALLSYMS section)
#
# barebox version cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update barebox.
#
# System.map is generated to document addresses of all kernel symbols

BAREBOX_OBJS := $(common-y)
export BAREBOX_PBL_OBJS := $(pbl-common-y)
BAREBOX_LDS    := $(lds-y)

# Rule to link barebox
# May be overridden by arch/$(SRCARCH)/Makefile
quiet_cmd_barebox__ ?= LD      $@
      cmd_barebox__ ?= $(LD) $(KBUILD_LDFLAGS) $(LDFLAGS_barebox) -o $@ \
      -T $(BAREBOX_LDS)                         \
      --whole-archive $(BAREBOX_OBJS) --no-whole-archive                  \
      $(filter-out $(BAREBOX_LDS) $(BAREBOX_OBJS) FORCE ,$^)

# Generate new barebox version
quiet_cmd_barebox_version = GEN     .version
      cmd_barebox_version = set -e;                     \
	if [ ! -r .version ]; then			\
	  rm -f .version;				\
	  echo 1 >.version;				\
	else						\
	  mv .version .old_version;			\
	  expr 0$$(cat .old_version) + 1 >.version;	\
	fi;						\
	$(MAKE) $(build)=common

# Generate System.map
quiet_cmd_sysmap = SYSMAP  System.map
      cmd_sysmap = $(CONFIG_SHELL) $(srctree)/scripts/mksysmap $@ System.map

# Link of barebox
# If CONFIG_KALLSYMS is set .version is already updated
# Generate System.map and verify that the content is consistent
# Use + in front of the barebox_version rule to silent warning with make -j2
define rule_barebox__
	$(if $(CONFIG_KALLSYMS),,+$(call cmd,barebox_version))
	$(call cmd,barebox__)
	$(Q)echo 'savedcmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd
	$(call cmd,prelink__)
	$(call cmd,sysmap)
endef

ifdef CONFIG_KALLSYMS
# Generate section listing all symbols and add it into barebox $(kallsyms.o)
# It's a three stage process:
# o .tmp_barebox1 has all symbols and sections, but __kallsyms is
#   empty
#   Running kallsyms on that gives us .tmp_kallsyms1.o with
#   the right size - barebox version is updated during this step
# o .tmp_barebox2 now has a __kallsyms section of the right size,
#   but due to the added section, some addresses have shifted.
#   From here, we generate a correct .tmp_kallsyms2.o
# o The correct .tmp_kallsyms2.o is linked into the final barebox.
# o Verify that the System.map from barebox matches the map from
#   .tmp_barebox2, just in case we did not generate kallsyms correctly.
# o If CONFIG_KALLSYMS_EXTRA_PASS is set, do an extra pass using
#   .tmp_barebox3 and .tmp_kallsyms3.o.  This is only meant as a
#   temporary bypass to allow the kernel to be built while the
#   maintainers work out what went wrong with kallsyms.

ifdef CONFIG_KALLSYMS_EXTRA_PASS
last_kallsyms := 3
else
last_kallsyms := 2
endif

kallsyms.o := .tmp_kallsyms$(last_kallsyms).o

define verify_kallsyms
	$(Q)$(if $($(quiet)cmd_sysmap),                                      \
	  echo '  $($(quiet)cmd_sysmap)  .tmp_System.map' &&)                \
	  $(cmd_sysmap) .tmp_barebox$(last_kallsyms) .tmp_System.map
	$(Q)cmp -s System.map .tmp_System.map ||                             \
		(echo Inconsistent kallsyms data;                            \
		 echo Try setting CONFIG_KALLSYMS_EXTRA_PASS;                \
		 rm .tmp_kallsyms* ; false )
endef

# Update barebox version before link
# Use + in front of this rule to silent warning about make -j1
cmd_ksym_ld = $(cmd_barebox__)
define rule_ksym_ld
	+$(call cmd,barebox_version)
	$(call cmd,barebox__)
	$(Q)echo 'savedcmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd
endef

# Generate .S file with all kernel symbols
quiet_cmd_kallsyms = KSYM    $@
      cmd_kallsyms = $(NM) -n $< | $(KALLSYMS) --all-symbols > $@

.tmp_kallsyms1.o .tmp_kallsyms2.o .tmp_kallsyms3.o: %.o: %.S scripts FORCE
	$(call if_changed_dep,as_o_S)

.tmp_kallsyms%.S: .tmp_barebox% $(KALLSYMS)
	$(call cmd,kallsyms)

# .tmp_barebox1 must be complete except kallsyms, so update barebox version
.tmp_barebox1: $(BAREBOX_LDS) $(BAREBOX_OBJS) FORCE
	$(call if_changed_rule,ksym_ld)

.tmp_barebox2: $(BAREBOX_LDS) $(BAREBOX_OBJS) .tmp_kallsyms1.o FORCE
	$(call if_changed,barebox__)

.tmp_barebox3: $(BAREBOX_LDS) $(BAREBOX_OBJS) .tmp_kallsyms2.o FORCE
	$(call if_changed,barebox__)

# Needs to visit scripts/ before $(KALLSYMS) can be used.
$(KALLSYMS): scripts ;

# Generate some data for debugging strange kallsyms problems
debug_kallsyms: .tmp_map$(last_kallsyms)

.tmp_map%: .tmp_barebox% FORCE
	($(OBJDUMP) -h $< | $(AWK) '/^ +[0-9]/{print $$4 " 0 " $$2}'; $(NM) $<) | sort > $@

.tmp_map3: .tmp_map2

.tmp_map2: .tmp_map1

endif # ifdef CONFIG_KALLSYMS

OBJCOPYFLAGS_barebox.bin = -O binary

barebox.bin: barebox FORCE
	$(call if_changed,objcopy)
ifndef CONFIG_PBL_IMAGE
	$(call cmd,check_file_size,$@,$(CONFIG_BAREBOX_MAX_IMAGE_SIZE))
endif

install:
ifeq ($(INSTALL_PATH),)
	@echo 'error: INSTALL_PATH undefined' >&2
	@exit 1
endif
ifdef CONFIG_PBL_IMAGE
	$(Q)$(MAKE) $(build)=images __images_install
	@install -t "$(INSTALL_PATH)" barebox.bin
else
	@install -t "$(INSTALL_PATH)" $(KBUILD_IMAGE)
endif

PHONY += install

# barebox image
barebox: $(BAREBOX_LDS) $(BAREBOX_OBJS) $(kallsyms.o) FORCE
	$(call if_changed_rule,barebox__)
	$(Q)rm -f .old_version

barebox.srec: barebox
	$(OBJCOPY) -O srec $< $@

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(BAREBOX_OBJS)) $(BAREBOX_LDS) $(BAREBOX_PBL_OBJS): $(barebox-dirs) ;

# Handle descending into subdirectories listed in $(barebox-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

PHONY += $(barebox-dirs)
$(barebox-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

# Store (new) KERNELRELASE string in include/config/kernel.release
localversion = $(shell $(srctree)/scripts/setlocalversion $(srctree))
include/config/kernel.release: FORCE
	$(Q)rm -f $@
	$(Q)echo $(KERNELVERSION)$(localversion) > $@

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic include/generated/utsrelease.h
	$(Q)$(MAKE) $(build)=$(@)

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

# Listed in dependency order
PHONY += prepare archprepare prepare0

archprepare: outputmakefile scripts_basic include/config/kernel.release \
	$(version_h) include/generated/utsrelease.h include/config.h \
	include/generated/autoconf.h

prepare0: archprepare FORCE
ifneq ($(KBUILD_MODULES),)
	$(Q)mkdir -p $(MODVERDIR)
	$(Q)rm -f $(MODVERDIR)/*
endif
	$(Q)$(MAKE) $(build)=.

# All the preparing..
prepare: prepare0

# Leave this as default for preprocessing barebox.lds.S, which is now
# done in arch/$(SRCARCH)/kernel/Makefile

export CPPFLAGS_barebox.lds += -C -U$(SRCARCH)

define symlink-config-h
	if [ -f $(srctree)/$(BOARD)/config.h ]; then		\
		$(kecho) '  SYMLINK $@ -> $(BOARD)/config.h';	\
		ln -fsn $(srctree)/$(BOARD)/config.h $@;	\
	else							\
		[ -h $@ ] && rm -f $@;				\
		$(kecho) '  CREATE  $@';			\
		touch -a $@;					\
	fi
endef

PHONY += include/config.h
include/config.h:
	$(Q)$(symlink-config-h)

# Generate some files
# ---------------------------------------------------------------------------

# KERNELRELEASE can change from a few different places, meaning version.h
# needs to be updated, so this check is forced on all builds

uts_len := 64
define filechk_utsrelease.h
	if [ `echo -n "$(KERNELRELEASE)" | wc -c ` -gt $(uts_len) ]; then \
	  echo '"$(KERNELRELEASE)" exceeds $(uts_len) characters' >&2;    \
	  exit 1;                                                         \
	fi;                                                               \
	echo \#define UTS_RELEASE \"$(KERNELRELEASE)\"
endef

define filechk_version.h
	echo \#define LINUX_VERSION_CODE $(shell                         \
	expr $(VERSION) \* 65536 + 0$(PATCHLEVEL) \* 256 + 0$(SUBLEVEL)); \
	echo '#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))'
endef

include/generated/version.h: FORCE
	$(call filechk,version.h)

include/generated/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

# ---------------------------------------------------------------------------
# Devicetree files

ifneq ($(wildcard $(srctree)/arch/$(SRCARCH)/dts/),)
dtstree := arch/$(SRCARCH)/dts
endif

ifneq ($(dtstree),)

PHONY += dtbs
all_dtbs += $(patsubst $(srctree)/%.dts,$(objtree)/%.dtb,$(wildcard $(srctree)/$(dtstree)/*.dts))
targets += $(all_dtbs)
dtbs: $(all_dtbs)

endif

# ---------------------------------------------------------------------------
# Modules

ifdef CONFIG_MODULES

# By default, build modules as well

all: modules

#	Build modules

PHONY += modules
modules: $(barebox-dirs) $(if $(KBUILD_BUILTIN),barebox)
	@$(kecho) '  Building modules, stage 2.';
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost


# Target to prepare building external modules
PHONY += modules_prepare
modules_prepare: prepare scripts

# Target to install modules
PHONY += modules_install
modules_install: _modinst_ _modinst_post

PHONY += _modinst_
_modinst_:
	@if [ -z "`$(DEPMOD) -V 2>/dev/null | grep module-init-tools`" ]; then \
		echo "Warning: you may need to install module-init-tools"; \
		echo "See http://www.codemonkey.org.uk/docs/post-halloween-2.6.txt";\
		sleep 1; \
	fi
	@rm -rf $(MODLIB)/kernel
	@rm -f $(MODLIB)/source
	@mkdir -p $(MODLIB)/kernel
	@ln -s $(srctree) $(MODLIB)/source
	@if [ ! $(objtree) -ef  $(MODLIB)/build ]; then \
		rm -f $(MODLIB)/build ; \
		ln -s $(objtree) $(MODLIB)/build ; \
	fi
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modinst

# If System.map exists, run depmod.  This deliberately does not have a
# dependency on System.map since that would run the dependency tree on
# vmlinux.  This depmod is only for convenience to give the initial
# boot a modules.dep even before / is mounted read-write.  However the
# boot script depmod is the master version.
ifeq "$(strip $(INSTALL_MOD_PATH))" ""
depmod_opts	:=
else
depmod_opts	:= -b $(INSTALL_MOD_PATH) -r
endif
PHONY += _modinst_post
_modinst_post: _modinst_
	if [ -r System.map -a -x $(DEPMOD) ]; then $(DEPMOD) -ae -F System.map $(depmod_opts) $(KERNELRELEASE); fi

else # CONFIG_MODULES

# Modules not configured
# ---------------------------------------------------------------------------

modules modules_install: FORCE
	@echo
	@echo "The present kernel configuration has modules disabled."
	@echo "Type 'make config' and enable loadable module support."
	@echo "Then build a kernel with module support enabled."
	@echo
	@exit 1

endif # CONFIG_MODULES

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
#                Leave enough to build external modules
# make mrproper  Delete the current configuration, and all generated files
# make distclean Remove editor backup files, patch leftover files and the like

# Directories & files removed with 'make clean'
CLEAN_DIRS  += $(MODVERDIR)
CLEAN_FILES +=	barebox System.map include/generated/barebox_default_env.h \
                .tmp_version .tmp_barebox* barebox.bin barebox.map \
		.tmp_kallsyms* barebox.ldr compile_commands.json \
		barebox-flash-image \
		barebox.srec barebox.s5p barebox.ubl \
		barebox.uimage \
		barebox.efi barebox.canon-a1100.bin

CLEAN_FILES +=	scripts/bareboxenv-target scripts/kernel-install-target \
		scripts/bareboxcrc32-target scripts/bareboximd-target \
		scripts/omap3-usb-loader-target scripts/omap4_usbboot-target \
		scripts/imx-usb-loader-target scripts/kwboot-target

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += include/config usr/include include/generated Documentation/commands
MRPROPER_FILES += .config .config.old .version .old_version \
                  include/config.h           \
		  Module.symvers tags TAGS cscope*

# clean - Delete most, but leave enough to build external modules
#
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_,$(srctree) $(barebox-alldirs))

PHONY += $(clean-dirs) clean archclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=images
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: archclean $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*lex.c' -o -name '.tab.[ch]' \
		-o -name '*.symtypes' -o -name '*.bbenv.*' -o -name "*.bbenv" \) \
		-type f -print | xargs rm -f

# mrproper - Delete all generated files, including .config
#
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_,scripts)

PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)

# distclean
#
PHONY += distclean

distclean: mrproper
	@find $(srctree) $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '*%' \
		-o -name 'core' \) \
		-type f -print | xargs rm -f

# Clang Tooling
# ---------------------------------------------------------------------------

quiet_cmd_gen_compile_commands = GEN     $@
      cmd_gen_compile_commands = $(PYTHON3) $< -a $(AR) -o $@ $(filter-out $<, $(real-prereqs))

compile_commands.json: scripts/clang-tools/gen_compile_commands.py \
	$(BAREBOX_OBJS) $(if $(CONFIG_PBL_IMAGE),$(BAREBOX_PBL_OBJS),) FORCE
	$(call if_changed,gen_compile_commands)

PHONY += compile_commands.json

# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/arch/$(SRCARCH)/configs/*_defconfig)
boards := $(sort $(notdir $(boards)) $(generated_configs))

PHONY += $(generated_configs)

help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config and'
	@echo  '                    enough build support to build external modules'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  '  docs            - build documentation'
	@echo  ''
	@echo  'Configuration targets:'
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* barebox           - Build the bare kernel'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[ois]  - Build specified target only'
	@echo  '  dir/file.ko     - Build module including final link'
	@echo  '  tags/TAGS	  - Generate tags file for editors'
	@echo  '  cscope	  - Generate cscope index'
	@echo  '                    (default: $(INSTALL_HDR_PATH))'
	@echo  ''
	@echo  'Static analysers'
	@echo  '  checkstack      - Generate a list of stack hogs'
	@echo  '  namespacecheck  - Name space analysis on compiled kernel'
	@echo  ''
	@echo  'Architecture specific targets ($(SRCARCH)):'
	@$(if $(archhelp),$(archhelp),\
		echo '  No architecture specific help defined for $(SRCARCH)')
	@echo  ''
	@$(if $(dtstree), \
		echo '  Devicetree:'; \
		echo '    * dtbs             - Build device tree blobs for all boards'; \
		echo '')
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-24s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')

	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check all c source with $$CHECK (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '
	@echo  'For further info see the ./README file'

# Generate tags for editors
# ---------------------------------------------------------------------------
quiet_cmd_tags = GEN     $@
      cmd_tags = $(BASH) $(srctree)/scripts/tags.sh $@

tags TAGS cscope gtags: FORCE
	$(call cmd,tags)

SPHINXBUILD   = sphinx-build
ALLSPHINXOPTS   =  source

docs: FORCE
	@mkdir -p $(srctree)/Documentation/commands
	@$(srctree)/Documentation/gen_commands.py $(srctree) $(srctree)/Documentation/commands
	@$(SPHINXBUILD) -b html -d $(objtree)/doctrees $(srctree)/Documentation \
		$(objtree)/Documentation/html

bareboxversion:
	@echo $(KERNELVERSION)

# Single targets
# ---------------------------------------------------------------------------
# Single targets are compatible with:
# - build with mixed source and output
# - build with separate output dir 'make O=...'
# - external modules
#
#  target-dir => where to store outputfile
#  build-dir  => directory in kernel source tree to use

build-dir  = $(patsubst %/,%,$(dir $@))
target-dir = $(dir $@)

%.s: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.i: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.lst: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.s: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.symtypes: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# Modules
%/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir)
%.ko: prepare scripts FORCE
	$(Q)$(MAKE) KBUILD_MODULES=$(if $(CONFIG_MODULES),1)   \
	$(build)=$(build-dir) $(@:.ko=.o)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost

# FIXME Should go into a make.lib or something
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)


a_flags = -Wp,-MD,$(depfile) $(KBUILD_AFLAGS) $(AFLAGS_KERNEL) \
	  $(NOSTDINC_FLAGS) $(KBUILD_CPPFLAGS) \
	  $(modkern_aflags) $(EXTRA_AFLAGS) $(AFLAGS_$(basetarget).o)

quiet_cmd_as_o_S = AS      $@
cmd_as_o_S       = $(CC) $(a_flags) -c -o $@ $<

# read all saved command lines

targets := $(wildcard $(sort $(targets)))
cmd_files := $(wildcard .*.cmd $(foreach f,$(targets),$(dir $(f)).$(notdir $(f)).cmd))

ifneq ($(cmd_files),)
  $(cmd_files): ;	# Do not try to update included dependency files
  include $(cmd_files)
endif

endif # config-build
endif # mixed-build
endif # need-sub-make

PHONY += FORCE
FORCE:

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

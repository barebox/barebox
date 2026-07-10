# SPDX-License-Identifier: GPL-2.0
VERSION = 2026
PATCHLEVEL = 06
SUBLEVEL = 0
EXTRAVERSION =
NAME = None

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in the documentation.
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

ifeq ($(filter output-sync,$(.FEATURES)),)
$(error GNU Make >= 4.0 is required. Your Make version is $(MAKE_VERSION))
endif

$(if $(filter __%, $(MAKECMDGOALS)), \
	$(error targets prefixed with '__' are only for internal use))

# That's our default target when none is given on the command line
PHONY := __all
__all:

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

this-makefile := $(lastword $(MAKEFILE_LIST))
abs_srctree := $(realpath $(dir $(this-makefile)))
abs_output := $(CURDIR)

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
# Most of build commands in Kbuild start with "cmd_". You can optionally define
# "quiet_cmd_*". If defined, the short log is printed. Otherwise, no log from
# that command is printed by default.
#
# e.g.)
#    quiet_cmd_depmod = DEPMOD  $(MODLIB)
#          cmd_depmod = $(srctree)/scripts/depmod.sh $(DEPMOD) $(KERNELRELEASE)
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#    $(Q)$(MAKE) $(build)=scripts/basic
#
# If KBUILD_VERBOSE contains 1, the whole command is echoed.
# If KBUILD_VERBOSE contains 2, the reason for rebuilding is printed.
#
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif

quiet = quiet_
Q = @

ifneq ($(findstring 1, $(KBUILD_VERBOSE)),)
  quiet =
  Q =
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands
ifneq ($(findstring s,$(firstword -$(MAKEFLAGS))),)
quiet=silent_
override KBUILD_VERBOSE :=
endif

export quiet Q KBUILD_VERBOSE

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

export KBUILD_CHECKSRC

export quiet Q KBUILD_VERBOSE KPOLICY_TMPUPDATE

ifeq ("$(origin W)", "command line")
  KBUILD_EXTRA_WARN := $(W)
endif

export KBUILD_EXTRA_WARN

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

ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

objtree := .
output := $(KBUILD_OUTPUT)

export objtree srcroot

# Do we want to change the working directory?
ifneq ($(output),)
# $(realpath ...) gets empty if the path does not exist. Run 'mkdir -p' first.
$(shell mkdir -p "$(output)")
# $(realpath ...) resolves symlinks
abs_output := $(realpath $(output))
$(if $(abs_output),,$(error failed to create output directory "$(output)"))
endif

ifneq ($(words $(subst :, ,$(abs_srctree))), 1)
$(error source directory cannot contain spaces or colons)
endif

export sub_make_done := 1

endif # sub_make_done

ifeq ($(abs_output),$(CURDIR))
# Suppress "Entering directory ..." if we are at the final work directory.
no-print-directory := --no-print-directory
else
# Recursion to show "Entering directory ..."
need-sub-make := 1
endif

ifeq ($(filter --no-print-directory, $(MAKEFLAGS)),)
# If --no-print-directory is unset, recurse once again to set it.
# You may end up recursing into __sub-make twice. This is needed due to the
# behavior change in GNU Make 4.4.1.
need-sub-make := 1
endif

ifeq ($(need-sub-make),1)

PHONY += $(MAKECMDGOALS) __sub-make

$(filter-out $(this-makefile), $(MAKECMDGOALS)) __all: __sub-make
	@:

# Invoke a second make in the output directory, passing relevant variables
__sub-make:
	$(Q)$(MAKE) $(no-print-directory) -C $(abs_output) \
	-f $(abs_srctree)/Makefile $(MAKECMDGOALS)

else # need-sub-make

# We process the rest of the Makefile if this is the final invocation of make

srcroot := $(abs_srctree)

ifeq ($(srcroot),$(CURDIR))
building_out_of_srctree :=
else
export building_out_of_srctree := 1
endif

ifdef KBUILD_ABS_SRCTREE
    # Do nothing. Use the absolute path.
else ifeq ($(srcroot),$(CURDIR))
    # Building in the source.
    srcroot := .
else ifeq ($(srcroot)/,$(dir $(CURDIR)))
    # Building in a subdirectory of the source.
    srcroot := ..
endif

export srctree := $(if $(KBUILD_EXTMOD),$(abs_srctree),$(srcroot))

ifdef building_out_of_srctree
export VPATH := $(srcroot)
else
VPATH :=
endif

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

version_h := include/generated/version.h

clean-targets := %clean mrproper cleandocs
# barebox-specific: we have a check target, so check% is removed below
no-dot-config-targets := $(clean-targets) \
			 cscope gtags TAGS tags help% %docs \
			 $(version_h) \
			 bareboxversion \
			 outputmakefile
no-sync-config-targets := $(no-dot-config-targets) %install bareboxrelease \
			  image_name
single-targets := %.a %.i %.ko %.lds %.ll %.lst %.mod %.o %.rsi %.s %/

config-build	:=
mixed-build	:=
need-config	:= 1
may-sync-config	:= 1
single-build	:=

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

need-compiler := $(may-sync-config)

ifeq ($(KBUILD_EXTMOD),)
    ifneq ($(filter %config,$(MAKECMDGOALS)),)
        config-build := 1
        ifneq ($(words $(MAKECMDGOALS)),1)
            mixed-build := 1
        endif
    endif
endif

# We cannot build single targets and the others at the same time
ifneq ($(filter $(single-targets), $(MAKECMDGOALS)),)
    single-build := 1
    ifneq ($(filter-out $(single-targets), $(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

# For "make -j clean all", "make -j mrproper defconfig all", etc.
ifneq ($(filter $(clean-targets),$(MAKECMDGOALS)),)
    ifneq ($(filter-out $(clean-targets),$(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

# install and modules_install need also be processed one by one
ifneq ($(filter install,$(MAKECMDGOALS)),)
    ifneq ($(filter modules_install,$(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

ifdef mixed-build
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

PHONY += $(MAKECMDGOALS) __build_one_by_one

$(MAKECMDGOALS): __build_one_by_one
	@:

__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

else # !mixed-build

include $(srctree)/scripts/Kbuild.include

# Read KERNELRELEASE from include/config/kernel.release (if it exists)
KERNELRELEASE = $(call read-file, $(objtree)/include/config/kernel.release)
KERNELVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)
export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION

BUILDSYSTEM_VERSION =
export BUILDSYSTEM_VERSION

include $(srctree)/scripts/subarch.include

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=arm64
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=aarch64-linux-gnu-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables

ARCH            ?= sandbox

# Architecture as present in compile.h
UTS_MACHINE 	:= $(ARCH)
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

export cross_compiling :=
ifneq ($(SRCARCH),$(SUBARCH))
cross_compiling := 1
endif

KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := sh

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

PKG_CONFIG ?= pkg-config
HOSTPKG_CONFIG = $(PKG_CONFIG)
CROSS_PKG_CONFIG ?= $(CROSS_COMPILE)pkg-config

export KCONFIG_CONFIG


# the KERNELDOC macro needs to be exported, as scripts/Makefile.build
# has a logic to call it
KERNELDOC       = $(srctree)/scripts/kernel-doc.py
export KERNELDOC

KBUILD_USERHOSTCFLAGS := -Wall -Wmissing-prototypes -Wstrict-prototypes \
			      -O2 -fomit-frame-pointer -std=gnu11 \
			      -include $(srctree)/scripts/include/defines.h
KBUILD_USERCFLAGS  := $(KBUILD_USERHOSTCFLAGS) $(USERCFLAGS)
KBUILD_USERLDFLAGS := $(USERLDFLAGS)

KBUILD_HOSTCFLAGS   := $(KBUILD_USERHOSTCFLAGS) $(HOST_LFS_CFLAGS) \
		       $(HOSTCFLAGS) -I $(srctree)/scripts/include
KBUILD_HOSTCXXFLAGS := -Wall -O2 $(HOST_LFS_CFLAGS) $(HOSTCXXFLAGS) \
		       -I $(srctree)/scripts/include
KBUILD_HOSTLDFLAGS  := $(HOST_LFS_LDFLAGS) $(HOSTLDFLAGS)
KBUILD_HOSTLDLIBS   := $(HOST_LFS_LIBS) $(HOSTLDLIBS)

ENVCC		:= $(CC)
KBUILD_PROCMACROLDFLAGS := $(or $(PROCMACROLDFLAGS),$(KBUILD_HOSTLDFLAGS))

# Make variables (CC, etc...)
CPP		= $(CC) -E
ifneq ($(LLVM),)
CC		= $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
CXX		= $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)
LD		= $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
AR		= $(LLVM_PREFIX)llvm-ar$(LLVM_SUFFIX)
NM		= $(LLVM_PREFIX)llvm-nm$(LLVM_SUFFIX)
OBJCOPY		= $(LLVM_PREFIX)llvm-objcopy$(LLVM_SUFFIX)
OBJDUMP		= $(LLVM_PREFIX)llvm-objdump$(LLVM_SUFFIX)
READELF		= $(LLVM_PREFIX)llvm-readelf$(LLVM_SUFFIX)
STRIP		= $(LLVM_PREFIX)llvm-strip$(LLVM_SUFFIX)
PROFDATA	= $(LLVM_PREFIX)llvm-profdata$(LLVM_SUFFIX)
COV		= $(LLVM_PREFIX)llvm-cov$(LLVM_SUFFIX)
else
CC		= $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
LD		= $(CROSS_COMPILE)ld
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
READELF		= $(CROSS_COMPILE)readelf
STRIP		= $(CROSS_COMPILE)strip
endif
PAHOLE		= pahole
LEX		= flex
YACC		= bison
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
KALLSYMS	= scripts/kallsyms
SCONFIGPOST	= scripts/sconfig/sconfigpost
INSTALLKERNEL  := installkernel
PERL		= perl
PYTHON3		= python3
CHECK		= sparse
MKIMAGE		= mkimage
GENHTML		= genhtml
BASH		= bash
KGZIP		= gzip
KBZIP2		= bzip2
KLZOP		= lzop
LZMA		= lzma
LZ4		= lz4
XZ		= xz
ZSTD		= zstd
TAR		= tar

PYTEST		= $(if $(shell command -v labgrid-pytest 2>/dev/null),labgrid-pytest,pytest)


CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ \
		  -Wbitwise -Wno-return-void -Wno-unknown-attribute $(CF)
NOSTDINC_FLAGS :=
CFLAGS_MODULE   =
AFLAGS_MODULE   =
LDFLAGS_MODULE  =
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=
CFLAGS_MODULE	= -fshort-wchar -std=gnu11
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
                -include $(srctree)/include/linux/compiler-version.h \
                -include $(srctree)/include/linux/kconfig.h

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    := \
		-I$(srctree)/arch/$(SRCARCH)/include \
		-I$(objtree)/arch/$(SRCARCH)/include/generated \
		-I$(srctree)/include \
		-I$(objtree)/include \
		-I$(srctree)/dts/include \
		$(USERINCLUDE)

KBUILD_AFLAGS   := -D__ASSEMBLY__

KBUILD_CFLAGS :=
KBUILD_CFLAGS += -std=gnu11
KBUILD_CFLAGS += -fshort-wchar
KBUILD_CFLAGS += -funsigned-char
KBUILD_CFLAGS += -fno-common
KBUILD_CFLAGS += -fno-strict-aliasing

KBUILD_CPPFLAGS := -D__KERNEL__
KBUILD_CPPFLAGS += -D__BAREBOX__ -fno-builtin -ffreestanding -Ulinux -Uunix

KBUILD_AFLAGS_KERNEL :=
KBUILD_CFLAGS_KERNEL :=
KBUILD_AFLAGS_MODULE  := -DMODULE
KBUILD_CFLAGS_MODULE  := -DMODULE
KBUILD_LDFLAGS_MODULE :=
KBUILD_LDFLAGS :=
CLANG_FLAGS :=

export ARCH SRCARCH CONFIG_SHELL BASH HOSTCC KBUILD_HOSTCFLAGS CROSS_COMPILE LD CC HOSTPKG_CONFIG
export CXX CROSS_PKG_CONFIG PKG_CONFIG GENKSYMS MKIMAGE SCONFIGPOST
export PROFDATA COV GENHTML
export CPP AR NM STRIP OBJCOPY OBJDUMP READELF PAHOLE LEX YACC AWK INSTALLKERNEL
export PERL PYTHON3 CHECK CHECKFLAGS MAKE UTS_MACHINE HOSTCXX
export KGZIP KBZIP2 KLZOP LZMA LZ4 XZ ZSTD TAR
export KBUILD_HOSTCXXFLAGS KBUILD_HOSTLDFLAGS KBUILD_HOSTLDLIBS KBUILD_PROCMACROLDFLAGS LDFLAGS_MODULE
export KBUILD_USERCFLAGS KBUILD_USERLDFLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS KBUILD_LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL CFLAGS_MODULE
export KBUILD_AFLAGS AFLAGS_KERNEL AFLAGS_MODULE
export KBUILD_AFLAGS_MODULE KBUILD_CFLAGS_MODULE
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export LDFLAGS_barebox LDFLAGS_pbl LDFLAGS_elf

# Files to ignore in find ... statements

export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/basic/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

PHONY += scripts_sconfig
scripts_sconfig: scripts_basic
	$(Q)$(MAKE) $(build)=scripts/sconfig

PHONY += scripts_kconfig
scripts_kconfig: scripts_basic
	$(Q)$(MAKE) $(build)=scripts/kconfig build_config

PHONY += outputmakefile
ifdef building_out_of_srctree
# Before starting out-of-tree build, make sure the source tree is clean.
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
# At the same time when output Makefile generated, generate .gitignore to
# ignore whole output directory

print_env_for_makefile = \
	echo "export KBUILD_OUTPUT = $(CURDIR)"

quiet_cmd_makefile = GEN     Makefile
      cmd_makefile = { \
	echo "\# Automatically generated by $(abs_srctree)/Makefile: don't edit"; \
	$(print_env_for_makefile); \
	echo "include $(abs_srctree)/Makefile"; \
	} > Makefile

outputmakefile:
ifeq ($(KBUILD_EXTMOD),)
	@if [ -f $(srctree)/.config -o \
		 -d $(srctree)/include/config -o \
		 -d $(srctree)/arch/$(SRCARCH)/include/generated ]; then \
		echo >&2 "***"; \
		echo >&2 "*** The source tree is not clean, please run 'make$(if $(findstring command line, $(origin ARCH)), ARCH=$(ARCH)) mrproper'"; \
		echo >&2 "*** in $(abs_srctree)";\
		echo >&2 "***"; \
		false; \
	fi
endif
	$(Q)ln -fsn $(srcroot) source
	$(call cmd,makefile)
	$(Q)test -e .gitignore || \
	{ echo "# this is build directory, ignore it"; echo "*"; } > .gitignore
endif # building_out_of_srctree

# The expansion should be delayed until arch/$(SRCARCH)/Makefile is included.
# Some architectures define CROSS_COMPILE in arch/$(SRCARCH)/Makefile.
# CC_VERSION_TEXT is referenced from Kconfig (so they
# need export), and from include/config/auto.conf.cmd to detect the compiler
# upgrade.
CC_VERSION_TEXT = $(subst $(pound),,$(shell LC_ALL=C $(CC) --version 2>/dev/null | head -n 1))

ifneq ($(findstring clang,$(CC_VERSION_TEXT)),)
include $(srctree)/scripts/Makefile.clang
endif

# allow scan-build to override the used compiler
ifneq ($(CCC_ANALYZER_HTML),)
ifneq ($(ENVCC),)
CC = $(ENVCC)
endif
endif

# Include this also for config targets because some architectures need
# cc-cross-prefix to determine CROSS_COMPILE.
ifdef need-compiler
include $(srctree)/scripts/Makefile.compiler
endif

ifdef config-build
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# KCONFIG_CONFIG_ORIG is only set for policy kconfig processing
ifndef KCONFIG_CONFIG_ORIG
include $(srctree)/scripts/Makefile.defconf

# Read arch-specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
include $(srctree)/arch/$(SRCARCH)/Makefile
export KBUILD_DEFCONFIG KBUILD_KCONFIG CC_VERSION_TEXT
endif

%_efiloader_defconfig: FORCE
	$(call merge_into_defconfig_named,$*_defconfig,efi-loader,$@)
%_efi_defconfig: FORCE
	$(call merge_into_defconfig_named,$*_defconfig,efi-loader efi-payload,$@)

config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig KCONFIG_DEFCONFIG_LIST= $@

%config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig KCONFIG_DEFCONFIG_LIST= $@

else #!config-build
# ===========================================================================
# Build targets only - this includes barebox, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# If building an external module we do not care about the all: rule
# but instead __all depend on modules
PHONY += all
__all: all

targets :=

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_MODULES :=
KBUILD_BUILTIN := y

# If we have only "make modules", don't compile built-in objects.
ifeq ($(MAKECMDGOALS),modules)
  KBUILD_BUILTIN :=
endif

# If we have "make <whatever> modules", compile modules
# in addition to whatever we do anyway.
# Just "make" or "make all" shall build modules as well

ifneq ($(filter all modules compile_commands.json clang-%,$(MAKECMDGOALS)),)
  KBUILD_MODULES := y
endif

ifeq ($(MAKECMDGOALS),)
  KBUILD_MODULES := y
endif

export KBUILD_MODULES KBUILD_BUILTIN

ifdef need-config
include $(objtree)/include/config/auto.conf
endif

ifeq ($(CONFIG_RELR),y)
# ld.lld before 15 did not support -z pack-relative-relocs.
LDFLAGS_barebox += $(call ld-option,--pack-dyn-relocs=relr,-z pack-relative-relocs)
LDFLAGS_pbl += $(call ld-option,--pack-dyn-relocs=relr,-z pack-relative-relocs)
endif

ifdef CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
KBUILD_CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += --gc-sections
LDFLAGS_pbl += --gc-sections
endif

# FIXME: We still need some generic definitions.
include $(srctree)/scripts/Makefile.lib

# Objects we will link into vmlinux / subdirs we need to visit
core-y		:=

CFLAGS_GCOV	:= -fprofile-arcs -ftest-coverage
ifdef CONFIG_CC_IS_GCC
CFLAGS_GCOV	+= -fno-tree-loop-im
endif
export CFLAGS_GCOV

include $(srctree)/arch/$(SRCARCH)/Makefile

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
#
# Do not use $(call cmd,...) here. That would suppress prompts from syncconfig,
# so you cannot notice that Kconfig is waiting for the user input.
%/config/auto.conf %/config/auto.conf.cmd %/generated/autoconf.h: $(KCONFIG_CONFIG)
	$(Q)$(kecho) "  SYNC    $@"
	$(Q)$(MAKE) -f $(srctree)/Makefile syncconfig
else # !may-sync-config
# External modules and some install targets need include/generated/autoconf.h
# and include/config/auto.conf but do not care if they are up-to-date.
# Use auto.conf to show the error message

checked-configs := $(addprefix $(objtree)/, include/generated/autoconf.h include/config/auto.conf)
missing-configs := $(filter-out $(wildcard $(checked-configs)), $(checked-configs))

ifdef missing-configs
PHONY += $(objtree)/include/config/auto.conf

$(objtree)/include/config/auto.conf:
	@echo   >&2 '***'
	@echo   >&2 '***  ERROR: Kernel configuration is invalid. The following files are missing:'
	@printf >&2 '***    - %s\n' $(missing-configs)
	@echo   >&2 '***  Run "make oldconfig && make prepare" on kernel source to fix it.'
	@echo   >&2 '***'
	@/bin/false
endif

endif # may-sync-config
endif # need-config

KBUILD_CFLAGS	+= -fno-delete-null-pointer-checks

ifdef CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE
KBUILD_CFLAGS += -O2
else ifdef CONFIG_CC_OPTIMIZE_FOR_SIZE
KBUILD_CFLAGS += -Os
endif

# Tell gcc to never replace conditional load with a non-conditional one
ifdef CONFIG_CC_IS_GCC
# gcc-10 renamed --param=allow-store-data-races=0 to
# -fno-allow-store-data-races.
KBUILD_CFLAGS	+= $(call cc-option,--param=allow-store-data-races=0)
KBUILD_CFLAGS	+= $(call cc-option,-fno-allow-store-data-races)
endif

stackp-flags-y                                    := -fno-stack-protector
stackp-flags-$(CONFIG_STACKPROTECTOR)             := -fstack-protector
stackp-flags-$(CONFIG_STACKPROTECTOR_STRONG)      := -fstack-protector-strong

KBUILD_CFLAGS += $(stackp-flags-y)

ifdef CONFIG_FRAME_POINTER
KBUILD_CFLAGS	+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

# Initialize all stack variables with a 0xAA pattern.
ifdef CONFIG_INIT_STACK_ALL_PATTERN
KBUILD_CFLAGS	+= -ftrivial-auto-var-init=pattern
endif

# Initialize all stack variables with a zero value.
ifdef CONFIG_INIT_STACK_ALL_ZERO
KBUILD_CFLAGS	+= -ftrivial-auto-var-init=zero
ifdef CONFIG_CC_HAS_AUTO_VAR_INIT_ZERO_ENABLER
# https://github.com/llvm/llvm-project/issues/44842
CC_AUTO_VAR_INIT_ZERO_ENABLER := -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang
export CC_AUTO_VAR_INIT_ZERO_ENABLER
KBUILD_CFLAGS	+= $(CC_AUTO_VAR_INIT_ZERO_ENABLER)
endif
endif

# Clear used registers at func exit (to reduce data lifetime and ROP gadgets).
KBUILD_CFLAGS-$(CONFIG_ZERO_CALL_USED_REGS)	+= -fzero-call-used-regs=used-gpr

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

# Explicitly clear padding bits during variable initialization
KBUILD_CFLAGS += $(call cc-option,-fzero-init-padding-bits=all)

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

# warn about e.g. (unsigned)x < 0
KBUILD_CFLAGS += $(call cc-option,-Wtype-limits)

KBUILD_CFLAGS += $(KBUILD_CFLAGS-y)

KBUILD_CFLAGS	+= $(call cc-option, -fno-stack-clash-protection)

# Clear used registers at func exit (to reduce data lifetime and ROP gadgets).
ifdef CONFIG_ZERO_CALL_USED_REGS
KBUILD_CFLAGS	+= -fzero-call-used-regs=used-gpr
endif

ifdef CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
KBUILD_CFLAGS_KERNEL += -ffunction-sections -fdata-sections
LDFLAGS_vmlinux += --gc-sections
endif

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc

# disable invalid "can't wrap" optimizations for signed / pointers
KBUILD_CFLAGS	+= -fno-strict-overflow

# Make sure -fstack-check isn't enabled (like gentoo apparently did)
KBUILD_CFLAGS  += -fno-stack-check

# change __FILE__ to the relative path to the source directory
ifdef building_out_of_srctree
KBUILD_CPPFLAGS += $(call cc-option,-fmacro-prefix-map=$(srcroot)/=)
endif

# include additional Makefiles when needed
include-y			:= scripts/Makefile.warn
include-$(CONFIG_DEBUG_INFO)	+= scripts/Makefile.debug
include-$(CONFIG_KASAN)		+= scripts/Makefile.kasan
include-$(CONFIG_UBSAN)		+= scripts/Makefile.ubsan

include $(addprefix $(srctree)/, $(include-y))

# Add user supplied CPPFLAGS, AFLAGS and CFLAGS as the last assignments
KBUILD_CPPFLAGS += $(KCPPFLAGS)
KBUILD_AFLAGS   += $(KAFLAGS)
KBUILD_CFLAGS   += $(KCFLAGS)

LDFLAGS_barebox	+= -Map barebox.map

# Avoid 'Not enough room for program headers' error on binutils 2.28 onwards.
LDFLAGS_common += $(call ld-option, --no-dynamic-linker)
# Avoid 'missing .note.GNU-stack section implies executable stack' warnings on binutils 2.39+
LDFLAGS_common += -z noexecstack
# Avoid '... has a LOAD segment with RWX permissions' warnings on binutils 2.39+
LDFLAGS_common += $(call ld-option,--no-warn-rwx-segments)

LDFLAGS_barebox += $(LDFLAGS_common)
LDFLAGS_pbl += $(LDFLAGS_common)
LDFLAGS_elf += $(LDFLAGS_common) --nmagic -s

# Align the bit size of userspace programs with the kernel
USERFLAGS_FROM_KERNEL := -m32 -m64 --target=%
KBUILD_USERCFLAGS  += $(filter $(USERFLAGS_FROM_KERNEL), $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS))
KBUILD_USERLDFLAGS += $(filter $(USERFLAGS_FROM_KERNEL), $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS))

# userspace programs are linked via the compiler, use the correct linker
ifdef CONFIG_CC_IS_CLANG
KBUILD_USERLDFLAGS += --ld-path=$(LD)
endif

# make the checker run with the right architecture
CHECKFLAGS += --arch=$(ARCH)

# insure the checker run with the right endianness
CHECKFLAGS += $(if $(CONFIG_CPU_BIG_ENDIAN),-mbig-endian,-mlittle-endian)

# the checker needs the correct machine size
CHECKFLAGS += $(if $(CONFIG_64BIT),-m64,-m32)

PHONY += prepare0

build-dir	:= .
clean-dirs	:= $(sort . images \
		     $(patsubst %/,%,$(filter %/, $(core-) \
		        )))

export ARCH_CORE	:= $(core-y)
# Externally visible symbols (used by link-vmlinux.sh)

BAREBOX_PBL_OBJS := built-in.pbl.a
BAREBOX_OBJS := built-in.a

export BAREBOX_PBL_OBJS
export BAREBOX_LDS          := $(lds-y)

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
	$(MAKE) $(build)=common need-builtin=1

# Generate System.map
quiet_cmd_sysmap = SYSMAP  System.map
      cmd_sysmap = $(CONFIG_SHELL) $(srctree)/scripts/mksysmap $@ System.map

# Link of barebox
# If CONFIG_KALLSYMS is set .version is already updated
# Generate System.map and verify that the content is consistent
# Use + in front of the barebox_version rule to silent warning with make -j2
ifndef rule_barebox__
define rule_barebox__
	$(if $(CONFIG_KALLSYMS),,+$(call cmd,barebox_version))
	$(call cmd,barebox__)
	$(Q)echo 'savedcmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd
	$(call cmd,prelink__)
	$(call cmd,sysmap)
endef
endif

# Old kallsyms support
# ---------------------------------------------------------------------------

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

kallsyms_a_flags = -Wp,-MD,$(depfile) $(KBUILD_AFLAGS) $(AFLAGS_KERNEL) \
	  $(NOSTDINC_FLAGS) $(KBUILD_CPPFLAGS) \
	  $(modkern_aflags) $(LINUXINCLUDE) $(EXTRA_AFLAGS) $(AFLAGS_$(basetarget).o)

quiet_cmd_kallsyms_as_o_S = AS      $@
      cmd_kallsyms_as_o_S = $(CC) $(kallsyms_a_flags) -c -o $@ $<

# Generate .S file with all kernel symbols
quiet_cmd_kallsyms = KSYM    $@
      cmd_kallsyms = $(NM) -n $< | $(KALLSYMS) --all-symbols > $@

.tmp_kallsyms1.o .tmp_kallsyms2.o .tmp_kallsyms3.o: %.o: %.S scripts FORCE
	$(call if_changed_dep,kallsyms_as_o_S)

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

# Image and Symlink Handling
# ---------------------------------------------------------------------------

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

ifeq ($(CONFIG_PBL_IMAGE_ELF),y)
export BAREBOX_PROPER ?= vmbarebox
else
export BAREBOX_PROPER ?= barebox.bin
endif

barebox-flash-images: $(KBUILD_IMAGE)
	@echo $^ > $@

images: $(BAREBOX_PROPER) FORCE
	$(Q)$(MAKE) $(build)=images $@
images/%: $(BAREBOX_PROPER) FORCE
	$(Q)$(MAKE) $(build)=images $@

ifdef CONFIG_PBL_IMAGE
SYMLINK_TARGET_barebox.efi = images/barebox-dt-2nd.img
SYMLINK_DEP_barebox.efi = images
symlink-$(CONFIG_EFI_STUB) += barebox.efi
all: $(BAREBOX_PROPER) images
else
SYMLINK_TARGET_barebox-flash-image = $(KBUILD_IMAGE)
symlink-y += barebox-flash-image
all: barebox-flash-images
endif

all: $(symlink-y)

.SECONDEXPANSION:
$(symlink-y): $$(or $$(SYMLINK_DEP_$$(@F)),$$(SYMLINK_TARGET_$$(@F))) FORCE
	@ln -fsn --relative $(SYMLINK_TARGET_$(@F)) $@

# barebox image
# ---------------------------------------------------------------------------

barebox: $(BAREBOX_LDS) $(BAREBOX_OBJS) $(kallsyms.o) FORCE
	$(call if_changed_rule,barebox__)
ifeq ($(BAREBOX_PROPER),barebox)
	$(Q)rm -f .old_version
endif
barebox.fit: images/barebox-$(CONFIG_ARCH_LINUX_NAME).fit
	$(Q)ln -fsn $< $@

barebox.srec: barebox
	$(OBJCOPY) -O srec $< $@

OBJCOPYFLAGS_vmbarebox = $(call objcopy-option,--strip-section-headers,--strip-all)  \
			 --remove-section=.comment \
			 --remove-section=.note* \
			 --remove-section=.gnu.hash

vmbarebox: barebox FORCE
	$(call if_changed,objcopy)

OBJCOPYFLAGS_barebox.bin = -O binary

barebox.bin: barebox FORCE
	$(call if_changed,objcopy)
ifndef CONFIG_PBL_IMAGE
	$(call cmd,check_file_size,$@,$(CONFIG_BAREBOX_MAX_IMAGE_SIZE))
endif

quiet_cmd_barebox_proper__ = CC      $@
      cmd_barebox_proper__ = $(CC) -r -o $@ -Wl,--whole-archive $(BAREBOX_OBJS)

.tmp_barebox.o: $(BAREBOX_OBJS) $(kallsyms.o) FORCE
	$(if $(CONFIG_KALLSYMS),,+$(call cmd,barebox_version))
	$(call cmd,barebox_proper__)
	$(Q)echo 'savedcmd_$@ := $(cmd_barebox_proper__)' > $(@D)/.$(@F).cmd
	$(Q)rm -f .old_version

barebox.o: .tmp_barebox.o FORCE
	$(call if_changed,objcopy)

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(BAREBOX_LDS) $(BAREBOX_OBJS) $(BAREBOX_PBL_OBJS)): . ;

ifeq ($(origin KERNELRELEASE),file)
filechk_kernel.release = $(srctree)/scripts/setlocalversion $(srctree)
else
filechk_kernel.release = echo $(KERNELRELEASE)
endif

# Store (new) KERNELRELEASE string in include/config/kernel.release
include/config/kernel.release: FORCE
	$(call filechk,kernel.release)

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic scripts_dtc include/generated/utsrelease.h
	$(Q)$(MAKE) $(build)=$(@)

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

PHONY += prepare archprepare

archprepare: outputmakefile scripts include/config/kernel.release \
	$(version_h) include/generated/utsrelease.h \
	include/generated/compile.h include/generated/autoconf.h \
	remove-stale-files

prepare0: archprepare
	$(Q)$(MAKE) $(build)=scripts/mod
	$(Q)$(MAKE) $(build)=. prepare

# All the preparing..
prepare: prepare0
	@:

# Leave this as default for preprocessing barebox.lds.S, which is now
# done in arch/$(SRCARCH)/kernel/Makefile

export CPPFLAGS_barebox.lds += -C -U$(SRCARCH)

# Create $(FIRMWARE_DIR) from $(CONFIG_EXTRA_FIRMWARE_DIR) -- if it doesn't have a
# leading /, it's relative to $(srctree).
FIRMWARE_DIR := $(CONFIG_EXTRA_FIRMWARE_DIR)
FIRMWARE_DIR := $(addprefix $(srctree)/,$(filter-out /%,$(FIRMWARE_DIR)))$(filter /%,$(FIRMWARE_DIR))
export FIRMWARE_DIR

PHONY += remove-stale-files
remove-stale-files:
	$(Q)$(srctree)/scripts/remove-stale-files

# Testing barebox
# ---------------------------------------------------------------------------
# This target has pytest select the YAML env matching CONFIG_NAME if available.
# Use pytest --lg-env $labgrid_env_yaml directly if you need more than that.

labgrid-env := $(srctree)/test/$(SRCARCH)/$(CONFIG_NAME).yaml

check:
ifeq ($(CONFIG_NAME),)
	@echo "error: can't autoload labgrid env with CONFIG_NAME unset!" >&2
	@exit 1
endif
	@if [ ! -r "$(labgrid-env)" ]; then \
		echo "error: No Labgrid environment at $(labgrid-env)!" >&2; \
		echo "Choose a different config (or change CONFIG_NAME)" >&2; \
		echo "that can be automatically tested" >&2; \
		exit 1; \
	fi
	@echo
	@# This is intentionally not @suppressed, to make it easier to reproduce
	(cd $(srctree); KBUILD_OUTPUT=$(abs_output) $(PYTEST))

PHONY += check

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

$(version_h): private PATCHLEVEL := $(or $(PATCHLEVEL), 0)
$(version_h): private SUBLEVEL := $(or $(SUBLEVEL), 0)
$(version_h): FORCE
	$(call filechk,version.h)

include/generated/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

filechk_compile.h = $(srctree)/scripts/mkcompile_h \
	"$(UTS_MACHINE)" "$(CONFIG_CC_VERSION_TEXT)" "$(LD)"

include/generated/compile.h: FORCE
	$(call filechk,compile.h)

PHONY += headerdep
headerdep:
	$(Q)find $(srctree)/include/ -name '*.h' | xargs --max-args 1 \
	$(srctree)/scripts/headerdep.pl -I$(srctree)/include

# ---------------------------------------------------------------------------
# Security policies

ifdef CONFIG_SECURITY_POLICY

.security_config: $(KCONFIG_CONFIG) scripts_kconfig FORCE
	+$(call cmd,sconfig,allyesconfig,$@.tmp,$@)
	$(Q)if [ ! -r $@ ] || ! cmp -s $@.tmp $@; then	\
		mv -f $@.tmp $@;			\
	else						\
		rm -f $@.tmp;				\
	fi

targets	+= .security_config
targets += include/generated/security_autoconf.h
targets += include/generated/sconfig_names.h

KPOLICY = $(shell find $(objtree)/ -name policy-list -exec cat {} \;)

collect-dirs    := $(addprefix _policy_collect_,$(barebox-alldirs))

PHONY += _policy_collect_clean $(collect-dirs) collect-policies
_policy_collect_clean:
	$(Q)find $(objtree)/ -name policy-list -delete 2>/dev/null || true

$(collect-policy-dirs): | _policy_collect_clean
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.policy obj=$(patsubst _policy_collect_%,%,$@)

collect-policies: $(collect-policy-dirs)

PHONY += security_listconfigs
security_listconfigs: collect-policies FORCE
	@echo policies:
	@$(foreach p, $(KPOLICY), echo $p ;)

PHONY += security_checkconfigs
security_checkconfigs: collect-policies FORCE
	+$(Q)$(foreach p, $(KPOLICY), \
		$(MAKE) $(build)=$(patsubst %/,%,$(dir $p)) $p.tmp ;)
	+$(Q)$(foreach p, $(KPOLICY), \
		$(call loop_cmd,security_checkconfig,$p.tmp))

security_%config: collect-policies FORCE
	+$(Q)$(foreach p, $(KPOLICY), \
		$(MAKE) $(build)=$(patsubst %/,%,$(dir $p)) $p.tmp ;)
	+$(Q)$(foreach p, $(KPOLICY), $(call loop_cmd,sconfig, \
		$(@:security_%=%),$p.tmp))
ifeq ($(KPOLICY_TMPUPDATE),)
	+$(Q)$(foreach p, $(KPOLICY), \
		cp 2>/dev/null $p.tmp $(call resolve-external,$p) || true;)
endif

quiet_cmd_sconfigpost = SCONFPP $@
      cmd_sconfigpost = $(SCONFIGPOST) $2 -D $(depfile) -o $@ $<

include/generated/security_autoconf.h: .security_config scripts_sconfig FORCE
	$(call if_changed_dep,sconfigpost,-e)

include/generated/sconfig_names.h: .security_config scripts_sconfig include/generated/security_autoconf.h FORCE
	$(call if_changed_dep,sconfigpost,-s)

archprepare: include/generated/security_autoconf.h include/generated/sconfig_names.h

else

PHONY += security_%configs security_%config security_disabled
security_disabled: FORCE
	@echo "Security policies are disabled. Set CONFIG_SECURITY_POLICY to enable them"
	@false

security_%configs: security_disabled FORCE
	@false

security_%config: security_disabled FORCE
	@false

endif

PHONY += scripts_unifdef
scripts_unifdef: scripts_basic
	$(Q)$(MAKE) $(build)=scripts scripts/unifdef

PHONY += scripts_gen_packed_field_checks
scripts_gen_packed_field_checks: scripts_basic
	$(Q)$(MAKE) $(build)=scripts scripts/gen_packed_field_checks

# ---------------------------------------------------------------------------
# Install

install:
ifeq ($(INSTALL_PATH),)
	@echo 'error: INSTALL_PATH undefined' >&2
	@exit 1
endif
ifdef CONFIG_PBL_IMAGE
	$(Q)$(MAKE) $(build)=images __images_install
	@install -t "$(INSTALL_PATH)" $(BAREBOX_PROPER)
else
	@install -t "$(INSTALL_PATH)" $(KBUILD_IMAGE)
endif

PHONY += install

# ---------------------------------------------------------------------------
# Devicetree files

ifneq ($(wildcard $(srctree)/arch/$(SRCARCH)/dts/),)
dtstree := arch/$(SRCARCH)/dts
export dtstree
endif

ifneq ($(dtstree),)

%.dtb: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) $(dtstree)/$@

%.dtbo: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) $(dtstree)/$@

PHONY += dtbs dtbs_prepare dtbs_install
dtbs: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) need-dtbslist=1

# include/config/kernel.release is actually needed when installing DTBs because
# INSTALL_DTBS_PATH contains $(KERNELRELEASE). However, we do not want to make
# dtbs_install depend on it as dtbs_install may run as root.
dtbs_prepare: include/config/kernel.release scripts_dtc

dtbs_install:
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.dtbinst obj=$(dtstree)

endif # dtstree

PHONY += scripts_dtc
scripts_dtc: scripts_basic
	$(Q)$(MAKE) $(build)=scripts/dtc

# ---------------------------------------------------------------------------
# Modules

ifdef CONFIG_MODULES

# By default, build modules as well

all: modules

# Target to prepare building external modules
PHONY += modules_prepare
modules_prepare: prepare scripts

# Target to install modules
PHONY += modules_install
modules_install: _modinst_

PHONY += _modinst_
_modinst_:
	@rm -rf $(MODLIB)/barebox
	@rm -f $(MODLIB)/source
	@mkdir -p $(MODLIB)/barebox
	@ln -s $(srctree) $(MODLIB)/source
	@if [ ! $(objtree) -ef  $(MODLIB)/build ]; then \
		rm -f $(MODLIB)/build ; \
		ln -s $(objtree) $(MODLIB)/build ; \
	fi
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modinst

# Target to build modules environment
MODULES_ENV_DIR := $(objtree)/.tmp_barebox_modules_env
CLEAN_DIRS += $(MODULES_ENV_DIR)

barebox_modules_env: modules FORCE
	$(Q)rm -rf $(MODULES_ENV_DIR)
	$(Q)$(MAKE) -f $(srctree)/Makefile modules_install MODLIB=$(MODULES_ENV_DIR)
	$(Q)$(objtree)/scripts/bareboxenv -s $(MODULES_ENV_DIR)/barebox $@
	$(Q)mv barebox_modules_env $(objtree)/defaultenv/

ifdef CONFIG_MODULES_ENVIRONMENT
all: barebox_modules_env
endif

else # CONFIG_MODULES

# Modules not configured
# ---------------------------------------------------------------------------

modules_install: FORCE
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
CLEAN_FILES += Module.symvers \
	       compile_commands.json rust/test \
	       rust-project.json \
               .builtin-dtbs-list .builtin-dtb.S
CLEAN_FILES += barebox.o include/generated/barebox_default_env.h \
	       barebox-flash-image

# Directories & files removed with 'make mrproper'
MRPROPER_FILES += include/config include/generated          \
		  arch/$(SRCARCH)/include/generated .objdiff \
		  debian snap tar-install PKGBUILD pacman \
		  .config .config.old .version .old_version \
		  Module.symvers \
		  certs/signing_key.pem \
		  certs/x509.genkey

MRPROPER_FILES += .security_config *.sconfig.old Documentation/commands

# clean - Delete most, but leave enough to build external modules
#
clean: private rm-files := $(CLEAN_FILES)

PHONY += archclean

clean: archclean

# mrproper - Delete all generated files, including .config
#
mrproper: private rm-files := $(MRPROPER_FILES)
mrproper-dirs      := $(addprefix _mrproper_,scripts)

PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean $(mrproper-dirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.rmeta' \) \
		-type f -print | xargs rm -f

# distclean
#
PHONY += distclean

distclean: mrproper
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '*%' \
		-o -name 'core' -o -name tags -o -name TAGS -o -name 'cscope*' \
		-o -name GPATH -o -name GRTAGS -o -name GSYMS -o -name GTAGS \) \
		-type f -print | xargs rm -f

# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/arch/$(SRCARCH)/configs/*_defconfig)
boards := $(sort $(notdir $(boards)))
boards := $(sort $(boards) $(generated_configs))
board-dirs := $(dir $(wildcard $(srctree)/arch/$(SRCARCH)/configs/*/*_defconfig))
board-dirs := $(sort $(notdir $(board-dirs:/=)))

PHONY += $(generated_configs)

PHONY += help
help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config and'
	@echo  '                    enough build support to build external modules'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  '  docs            - build documentation'
	@echo  ''
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* barebox         - Build the barebox proper binary'
ifdef CONFIG_PBL_IMAGE
	@echo  '* images          - Build final prebootloader-prefixed images'
	@echo  '* barebox.fit     - Build 2nd stage barebox with device trees FIT image'
endif
	@echo  '* modules	  - Build all modules'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[ois]  - Build specified target only'
	@echo  '  dir/file.ll     - Build the LLVM assembly file'
	@echo  '                    (requires compiler support for LLVM assembly generation)'
	@echo  '  dir/file.lst    - Build specified mixed source/assembly target only'
	@echo  '                    (requires a recent binutils and recent build (System.map))'
	@echo  '  dir/file.ko     - Build module including final link'
	@echo  '  compile_commands.json'
	@echo  '                  - Generate compilation database for IDEs/LSP'
	@echo  '  modules_prepare - Set up for building external modules'
	@echo  '  tags/TAGS	  - Generate tags file for editors'
	@echo  '  cscope	  - Generate cscope index'
	@echo  '                    (default: $(INSTALL_HDR_PATH))'
	@echo  ''
	@echo  'Documentation targets:'
	@$(MAKE) -f $(srctree)/Documentation/Makefile dochelp
	@echo  ''
	@echo  '		    (requires kernel .config)'
	@echo  '  dir/file.[os]   - Build specified target only'
	@echo  '  dir/file.ll     - Build the LLVM assembly file'
	@echo  ''
	@$(if $(dtstree), \
		echo 'Devicetree:'; \
		echo '* dtbs               - Build device tree blobs for enabled boards'; \
		echo '  dtbs_install       - Install dtbs to $(INSTALL_DTBS_PATH)'; \
		echo '')
	@echo  'Architecture-specific targets ($(SRCARCH)):'
	@$(or $(archhelp),\
		echo '  No architecture-specific help defined for $(SRCARCH)')
	@echo  ''
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-27s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')
	@$(if $(board-dirs), \
		$(foreach b, $(board-dirs), \
		printf "  %-16s - Show %s-specific targets\\n" help-$(b) $(b);) \
		printf "  %-16s - Show all of the above\\n" help-boards; \
		echo '')

	@echo  '  make V=n   [targets] 1: verbose build'
	@echo  '                       2: give reason for rebuild of target'
	@echo  '                       V=1 and V=2 can be combined with V=12'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check re-compiled c source with $$CHECK'
	@echo  '                       (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  '  make W=n   [targets] Enable extra build checks, n=1,2,3,c,e where'
	@echo  '		1: warnings which may be relevant and do not occur too often'
	@echo  '		2: warnings which occur quite often but may still be relevant'
	@echo  '		3: more obscure warnings, can most likely be ignored'
	@echo  '		c: extra checks in the configuration stage (Kconfig)'
	@echo  '		e: warnings are being treated as errors'
	@echo  '		Multiple levels can be combined with W=12 or W=123'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '
	@echo  'For further info see the documentation'


help-board-dirs := $(addprefix help-,$(board-dirs))

help-boards: $(help-board-dirs)

boards-per-dir = $(sort $(notdir $(wildcard $(srctree)/arch/$(SRCARCH)/configs/$*/*_defconfig)))

$(help-board-dirs): help-%:
	@echo  'Architecture-specific targets ($(SRCARCH) $*):'
	@$(if $(boards-per-dir), \
		$(foreach b, $(boards-per-dir), \
		printf "  %-24s - Build for %s\\n" $*/$(b) $(subst _defconfig,,$(b));) \
		echo '')


# Documentation targets
# ---------------------------------------------------------------------------
DOC_TARGETS := docs htmldocs dochelp

PHONY += $(DOC_TARGETS)
$(DOC_TARGETS):
	$(Q)$(MAKE) -f $(srctree)/Documentation/Makefile $@

# Code Coverage
# ---------------------------------------------------------------------------

barebox.coverage_html: barebox.coverage-info
	genhtml -o $@ $<

barebox.coverage-info: default.profdata
	$(COV) export --format=lcov -instr-profile $< $(objtree)/barebox >$@

default.profdata: $(srctree)/default.profraw
	$(PROFDATA) merge -sparse $< -o $@

# We intentionally don't depend on barebox being built as that can take >10
# minutes when coverage is enabled
PHONY += coverage-html
coverage-html: barebox.coverage_html
	@echo "HTML coverage generated to $(objtree)/$<"

# Misc
# ---------------------------------------------------------------------------

PHONY += misc-check
misc-check:
	$(Q)$(srctree)/scripts/misc-check

all: misc-check

# ---------------------------------------------------------------------------
# Modules

PHONY += modules modules_prepare

ifdef CONFIG_MODULES

modules.order: $(build-dir)
	@:

# KBUILD_MODPOST_NOFINAL can be set to skip the final link of modules.
# This is solely useful to speed up test compiles.
modules: modpost
ifneq ($(KBUILD_MODPOST_NOFINAL),1)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modfinal
endif

PHONY += modules_check
modules_check: modules.order
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/modules-check.sh $<

else # CONFIG_MODULES

modules:
	@:

KBUILD_MODULES :=

endif # CONFIG_MODULES

PHONY += modpost
modpost: $(if $(single-build),, $(if $(KBUILD_BUILTIN), barebox.o)) \
	 $(if $(KBUILD_MODULES), modules_check)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost

# Single targets
# ---------------------------------------------------------------------------
# To build individual files in subdirectories, you can do like this:
#
#   make foo/bar/baz.s
#
# The supported suffixes for single-target are listed in 'single-targets'
#
# To build only under specific subdirectories, you can do like this:
#
#   make foo/bar/baz/

ifdef single-build

# .ko is special because modpost is needed
single-ko := $(sort $(filter %.ko, $(MAKECMDGOALS)))
single-no-ko := $(filter-out $(single-ko), $(MAKECMDGOALS)) \
		$(foreach x, o mod, $(patsubst %.ko, %.$x, $(single-ko)))

$(single-ko): single_modules
	@:
$(single-no-ko): $(build-dir)
	@:

# Remove modules.order when done because it is not the real one.
PHONY += single_modules
single_modules: $(single-no-ko) modules_prepare
	$(Q){ $(foreach m, $(single-ko), echo $(m:%.ko=%.o);) } > modules.order
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost
ifneq ($(KBUILD_MODPOST_NOFINAL),1)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modfinal
endif
	$(Q)rm -f modules.order

single-goals := $(addprefix $(build-dir)/, $(single-no-ko))

KBUILD_MODULES := y

endif

prepare: outputmakefile

# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language
PHONY += $(build-dir)
$(build-dir): prepare
	@find $(objtree)/$@ -name policy-list -exec rm -f {} \; 2>/dev/null || true
	$(Q)$(MAKE) $(build)=$@ need-builtin=1 need-modorder=1 $(single-goals) \
		$(if $(CONFIG_PBL_IMAGE),need-pbl=1)

clean-dirs := $(addprefix _clean_, $(clean-dirs))
PHONY += $(clean-dirs) clean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: $(clean-dirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[aios]' -o -name '*.rsi' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.ko.*' \
		-o -name '*.dtb' -o -name '*.dtbo' \
		-o -name '*.dtb.S' -o -name '*.dtbo.S' \
		-o -name '*.dt.yaml' -o -name 'dtbs-list' \
		-o -name '*.dwo' -o -name '*.lst' \
		-o -name '*.su' -o -name '*.mod' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.lex.c' -o -name '*.tab.[ch]' \
		-o -name '*.asn1.[ch]' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name '*.c.[012]*.*' \
		-o -name '*.ll' \
		-o -name '*.gcno' \
		-o -name 'policy-list' \
		-o -name '*.sconfig.tmp' -o -name '*.sconfig.[co]' \
		-o -name '*.bbenv.*' -o -name "*.bbenv" \
		-o -name '*.symtypes' \
		\) -type f -print \
		-o -name '.tmp_*' -print \
		| xargs rm -rf

# Generate tags for editors
# ---------------------------------------------------------------------------
quiet_cmd_tags = GEN     $@
      cmd_tags = $(BASH) $(srctree)/scripts/tags.sh $@

tags TAGS cscope gtags: FORCE
	$(call cmd,tags)

# Single targets
# ---------------------------------------------------------------------------

# Clang Tooling
# ---------------------------------------------------------------------------

quiet_cmd_gen_compile_commands = GEN     $@
      cmd_gen_compile_commands = $(PYTHON3) $< -a $(AR) -o $@ $(filter-out $<, $(real-prereqs))

compile_commands.json: scripts/clang-tools/gen_compile_commands.py \
	$(BAREBOX_OBJS) $(if $(CONFIG_PBL_IMAGE),$(BAREBOX_PBL_OBJS),) scripts/ FORCE
	$(call if_changed,gen_compile_commands)

PHONY += compile_commands.json

PHONY += clang-tidy clang-analyzer

ifdef CONFIG_CC_IS_CLANG
quiet_cmd_clang_tools = CHECK   $<
      cmd_clang_tools = $(PYTHON3) $(srctree)/scripts/clang-tools/run-clang-tools.py $@ $<

clang-tidy clang-analyzer: compile_commands.json
	$(call cmd,clang_tools)
else
clang-tidy clang-analyzer:
	@echo "$@ requires CC=clang" >&2
	@false
endif

# Scripts to check various things for consistency
# ---------------------------------------------------------------------------

PHONY += includecheck

includecheck:
	find $(srctree)/* $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w $(srctree)/scripts/checkincludes.pl

PHONY += bareboxrelease bareboxversion

bareboxrelease:
	@$(filechk_kernel.release)

bareboxversion:
	@echo $(KERNELVERSION)

PHONY += run-command
run-command:
	$(Q)$(KBUILD_RUN_COMMAND)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -rf $(rm-files)

# read saved command lines for existing targets
existing-targets := $(wildcard $(sort $(targets)))

-include $(foreach f,$(existing-targets),$(dir $(f)).$(notdir $(f)).cmd)

endif # config-build
endif # mixed-build
endif # need-sub-make

PHONY += FORCE
FORCE:

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

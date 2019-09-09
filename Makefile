VERSION = 2019
PATCHLEVEL = 09
SUBLEVEL = 0
EXTRAVERSION =
NAME = None

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoids hard-to-debug behaviour);
# o  print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.o targets which finally
# turn into barebox), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/sparse.txt" for more details, including
# where to get the "sparse" utility.

ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# Use make M=dir to specify directory of external module to build
# Old syntax make ... SUBDIRS=$PWD is still supported
# Setting the environment variable KBUILD_EXTMOD take precedence
ifdef SUBDIRS
  KBUILD_EXTMOD ?= $(SUBDIRS)
endif
ifeq ("$(origin M)", "command line")
  KBUILD_EXTMOD := $(M)
endif


# kbuild supports saving output files in a separate directory.
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.


# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# Do we want to locate output files in a separate directory?
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

# That's our default target when none is given on the command line
PHONY := _all
_all:

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) \
								&& /bin/pwd)
$(if $(KBUILD_OUTPUT),, \
     $(error failed to create output directory "$(saved-output)"))

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make,$(MAKECMDGOALS)) _all: sub-make
	@:

sub-make: FORCE
	$(if $(KBUILD_VERBOSE:1=),@)$(MAKE) -C $(KBUILD_OUTPUT) \
	KBUILD_SRC=$(CURDIR) \
	KBUILD_EXTMOD="$(KBUILD_EXTMOD)" -f $(CURDIR)/Makefile \
	$(filter-out _all sub-make,$(MAKECMDGOALS))

# Leave processing to above invocation of make
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(skip-makefile),)

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
_all: all

# CDPATH can have sideeffects; disable, since we do know where we want to cd to
export CDPATH=

srctree		:= $(if $(KBUILD_SRC),$(KBUILD_SRC),$(CURDIR))
objtree		:= $(CURDIR)
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)$(if $(KBUILD_EXTMOD),:$(KBUILD_EXTMOD))

export srctree objtree VPATH

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

KCONFIG_CONFIG	?= .config

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOST_LFS_CFLAGS := $(shell getconf LFS_CFLAGS 2>/dev/null)
HOST_LFS_LDFLAGS := $(shell getconf LFS_LDFLAGS 2>/dev/null)
HOST_LFS_LIBS := $(shell getconf LFS_LIBS 2>/dev/null)

HOSTCC       = gcc
HOSTCXX      = g++
HOSTCFLAGS   = -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer $(HOST_LFS_CFLAGS)
HOSTCXXFLAGS = -O2 $(HOST_LFS_CFLAGS)
HOSTLDFLAGS = $(HOST_LFS_LDFLAGS)
HOST_LOADLIBES = $(HOST_LFS_LIBS)

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_MODULES :=
KBUILD_BUILTIN := 1

#	If we have only "make modules", don't compile built-in objects.
#	When we're building modules with modversions, we need to consider
#	the built-in objects during the descend as well, in order to
#	make sure the checksums are up to date before we record them.

ifeq ($(MAKECMDGOALS),modules)
  KBUILD_BUILTIN := $(if $(CONFIG_MODVERSIONS),1)
endif

#	If we have "make <whatever> modules", compile modules
#	in addition to whatever we do anyway.
#	Just "make" or "make all" shall build modules as well

ifneq ($(filter all _all modules,$(MAKECMDGOALS)),)
  KBUILD_MODULES := 1
endif

export KBUILD_MODULES KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

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

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE


# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

# We need some generic definitions.
include $(srctree)/scripts/Kbuild.include
include $(srctree)/scripts/Makefile.lib

# Make variables (CC, etc...)

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
LEX		= flex
YACC		= bison
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
DEPMOD		= /sbin/depmod
KALLSYMS	= scripts/kallsyms
PERL		= perl
PYTHON3		= python3
CHECK		= sparse

CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ -Wbitwise $(CF)
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=

LDFLAGS_MODULE  = -T common/module.lds

# When compiling out-of-tree modules, put MODVERDIR in the module
# tree rather than in the kernel tree. The kernel tree might
# even be read-only.
export MODVERDIR := $(if $(KBUILD_EXTMOD),$(firstword $(KBUILD_EXTMOD))/).tmp_versions

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    := -Iinclude -I$(srctree)/dts/include \
                   $(if $(KBUILD_SRC), -I$(srctree)/include) \
		   -I$(srctree)/arch/$(ARCH)/include \
		   -I$(objtree)/arch/$(ARCH)/include \
                   -include $(srctree)/include/linux/kconfig.h

CPPFLAGS        := -D__KERNEL__ -D__BAREBOX__ $(LINUXINCLUDE) -fno-builtin -ffreestanding

CFLAGS          := -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
                   -Werror-implicit-function-declaration \
                   -fno-strict-aliasing -fno-common -Os -pipe -Wmissing-prototypes
AFLAGS          := -D__ASSEMBLY__

LDFLAGS_barebox	:= -Map barebox.map

# Avoid 'Not enough room for program headers' error on binutils 2.28 onwards.
LDFLAGS_barebox += $(call ld-option, --no-dynamic-linker)

# Read KERNELRELEASE from include/config/kernel.release (if it exists)
KERNELRELEASE = $(shell cat include/config/kernel.release 2> /dev/null)
KERNELVERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION
export ARCH SRCARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP MAKE AWK GENKSYMS PERL PYTHON3 UTS_MACHINE
export LEX YACC
export HOSTCXX HOSTCXXFLAGS HOSTLDFLAGS HOST_LOADLIBES LDFLAGS_MODULE CHECK CHECKFLAGS

export CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS LDFLAGS
export CFLAGS CFLAGS_KERNEL
export AFLAGS AFLAGS_KERNEL
export LDFLAGS_barebox

export CFLAGS_UBSAN

# Files to ignore in find ... statements

RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o -name CVS -o -name .pc -o -name .hg -o -name .git \) -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn --exclude CVS --exclude .pc --exclude .hg --exclude .git

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION) $(PATCHLEVEL)
endif

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

no-dot-config-targets := clean mrproper distclean \
			 cscope TAGS tags help %docs check% \
			 include/generated/version.h headers_% \
			 kernelrelease kernelversion

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
	ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
		mixed-targets := 1
	endif
endif

ifeq ($(mixed-targets),1)
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

else
ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# Read arch specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
include $(srctree)/arch/$(ARCH)/Makefile
export KBUILD_DEFCONFIG

config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# ===========================================================================
# Build targets only - this includes barebox, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic
	$(Q)$(MAKE) $(build)=$(@)

# Objects we will link into barebox / subdirs we need to visit
common-y		:= common/ drivers/ commands/ lib/ crypto/ net/ fs/ firmware/

ifeq ($(dot-config),1)
include include/config/auto.conf

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
%/auto.conf %/auto.conf.cmd %/tristate.conf: $(KCONFIG_CONFIG)
	$(Q)$(MAKE) -f $(srctree)/Makefile syncconfig
endif # $(dot-config)

include $(srctree)/arch/$(ARCH)/Makefile

CFLAGS		+= -ggdb3

# Force gcc to behave correct even for buggy distributions
CFLAGS          += $(call cc-option, -fno-stack-protector)

# This warning generated too much noise in a regular build.
# Use make W=1 to enable this warning (see scripts/Makefile.build)
CFLAGS += $(call cc-disable-warning, unused-but-set-variable)

CFLAGS += $(call cc-disable-warning, trampolines)

CFLAGS += $(call cc-option, -fno-delete-null-pointer-checks,)

CFLAGS   += $(call cc-disable-warning, address-of-packed-member)

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CHECKFLAGS     += $(NOSTDINC_FLAGS)

# warn about C99 declaration after statement
CFLAGS += $(call cc-option,-Wdeclaration-after-statement,)

# disable pointer signed / unsigned warnings in gcc 4.0
CFLAGS += $(call cc-option,-Wno-pointer-sign,)

# change __FILE__ to the relative path from the srctree
CFLAGS += $(call cc-option,-fmacro-prefix-map=$(srctree)/=)

include $(srctree)/scripts/Makefile.ubsan

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
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# the default value.

barebox-flash-image: $(KBUILD_IMAGE) FORCE
	$(call if_changed,ln)

barebox-flash-images: $(KBUILD_IMAGE)
	@echo $^ > $@

images: barebox.bin FORCE
	$(Q)$(MAKE) $(build)=images $@
images/%.s: barebox.bin FORCE
	$(Q)$(MAKE) $(build)=images $@

ifdef CONFIG_PBL_IMAGE
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

pbl-common-y	:= $(patsubst %/, %/built-in-pbl.o, $(common-y))
common-y	:= $(patsubst %/, %/built-in.o, $(common-y))

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
# $(barebox-main). Most are built-in.o files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)Makefile.
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
#   |    +--< driver/built-in.o mm/built-in.o + more
#   |
#   +-< kallsyms.o (see description in CONFIG_KALLSYMS section)
#
# barebox version cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update barebox.
#
# System.map is generated to document addresses of all kernel symbols

barebox-common := $(common-y)
barebox-pbl-common := $(pbl-common-y)
export barebox-pbl-common
barebox-all    := $(barebox-common)
barebox-lds    := $(lds-y)

# Rule to link barebox
# May be overridden by arch/$(ARCH)/Makefile
quiet_cmd_barebox__ ?= LD      $@
      cmd_barebox__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_barebox) -o $@ \
      -T $(barebox-lds)                         \
      --start-group $(barebox-common) --end-group                  \
      $(filter-out $(barebox-lds) $(barebox-common) FORCE ,$^)

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
quiet_cmd_sysmap = SYSMAP
      cmd_sysmap = $(CONFIG_SHELL) $(srctree)/scripts/mksysmap

# Link of barebox
# If CONFIG_KALLSYMS is set .version is already updated
# Generate System.map and verify that the content is consistent
# Use + in front of the barebox_version rule to silent warning with make -j2
# First command is ':' to allow us to use + in front of the rule
define rule_barebox__
	:
	$(if $(CONFIG_KALLSYMS),,+$(call cmd,barebox_version))
	$(call cmd,barebox__)

	$(Q)echo 'cmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd

	$(Q)$(if $($(quiet)cmd_sysmap),                                      \
	  echo '  $($(quiet)cmd_sysmap)  System.map' &&)                     \
	$(cmd_sysmap) $@ System.map;                                         \
	if [ $$? -ne 0 ]; then                                               \
		rm -f $@;                                                    \
		false;                                                       \
	fi;
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
# First command is ':' to allow us to use + in front of this rule
cmd_ksym_ld = $(cmd_barebox__)
define rule_ksym_ld
	:
	+$(call cmd,barebox_version)
	$(call cmd,barebox__)
	$(Q)echo 'cmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd
endef

# Generate .S file with all kernel symbols
quiet_cmd_kallsyms = KSYM    $@
      cmd_kallsyms = $(NM) -n $< | $(KALLSYMS) --all-symbols > $@

.tmp_kallsyms1.o .tmp_kallsyms2.o .tmp_kallsyms3.o: %.o: %.S scripts FORCE
	$(call if_changed_dep,as_o_S)

.tmp_kallsyms%.S: .tmp_barebox% $(KALLSYMS)
	$(call cmd,kallsyms)

# .tmp_barebox1 must be complete except kallsyms, so update barebox version
.tmp_barebox1: $(barebox-lds) $(barebox-all) FORCE
	$(call if_changed_rule,ksym_ld)

.tmp_barebox2: $(barebox-lds) $(barebox-all) .tmp_kallsyms1.o FORCE
	$(call if_changed,barebox__)

.tmp_barebox3: $(barebox-lds) $(barebox-all) .tmp_kallsyms2.o FORCE
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

# Do modpost on a prelinked vmlinux. The finally linked vmlinux has
# relevant sections renamed as per the linker script.
quiet_cmd_barebox-modpost = LD      $@
      cmd_barebox-modpost = $(LD) $(LDFLAGS) -r -o $@                          \
	 $(vmlinux-init) --start-group $(barebox-main) --end-group             \
	 $(filter-out $(barebox-init) $(barebox-main) $(barebox-lds) FORCE ,$^)
define rule_barebox-modpost
	:
	+$(call cmd,barebox-modpost)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost $@
	$(Q)echo 'cmd_$@ := $(cmd_barebox-modpost)' > $(dot-target).cmd
endef

OBJCOPYFLAGS_barebox.bin = -O binary

barebox.bin: barebox FORCE
	$(call if_changed,objcopy)
ifndef CONFIG_PBL_IMAGE
	$(call cmd,check_file_size,$@,$(CONFIG_BAREBOX_MAX_IMAGE_SIZE))
endif

# By default the uImage load address is 2MB below CONFIG_TEXT_BASE,
# leaving space for the compressed PBL image at 1MB below CONFIG_TEXT_BASE.
UIMAGE_BASE ?= $(shell printf "0x%08x" $$(($(CONFIG_TEXT_BASE) - 0x200000)))

# For development provide a target which makes barebox loadable by an
# unmodified u-boot
quiet_cmd_barebox_mkimage = MKIMAGE $@
      cmd_barebox_mkimage = $(srctree)/scripts/mkimage -A $(ARCH) -T firmware -C none \
       -O barebox -a $(UIMAGE_BASE) -e $(UIMAGE_BASE) \
       -n "barebox $(KERNELRELEASE)" -d $< $@

# barebox.uimage is build from the raw barebox binary, without any other
# headers.
barebox.uimage: $(KBUILD_BINARY) FORCE
	$(call if_changed,barebox_mkimage)

ifdef CONFIG_X86
barebox.S barebox.s: barebox
ifdef CONFIG_X86_HDBOOT
	@echo "-------------------------------------------------" > barebox.S
	@echo " * MBR content" >> barebox.S
	$(Q)$(OBJDUMP) -j .bootsector -mi8086 -d barebox >> barebox.S
	@echo "-------------------------------------------------" >> barebox.S
	@echo " * Boot loader content" >> barebox.S
	$(Q)$(OBJDUMP) -j .bootstrapping -mi8086 -d barebox >> barebox.S
endif
	@echo "-------------------------------------------------" >> barebox.S
	@echo " * Regular Text content" >> barebox.S
	$(Q)$(OBJDUMP) -j .text -d barebox >> barebox.S
	@echo "-------------------------------------------------" >> barebox.S
	@echo " * Regular Data content" >> barebox.S
	$(Q)$(OBJDUMP) -j .data -d barebox >> barebox.S
	@echo "-------------------------------------------------" >> barebox.S
	@echo " * Commands content" >> barebox.S
	$(Q)$(OBJDUMP) -j .barebox_cmd -d barebox >> barebox.S
	@echo "-------------------------------------------------" >> barebox.S
	@echo " * Init Calls content" >> barebox.S
	$(Q)$(OBJDUMP) -j .barebox_initcalls -d barebox >> barebox.S
else
barebox.S barebox.s: barebox FORCE
	$(call if_changed,disasm)
endif

# barebox image
barebox: $(barebox-lds) $(barebox-head) $(barebox-common) $(kallsyms.o) FORCE
	$(call barebox-modpost)
	$(call if_changed_rule,barebox__)
	$(Q)rm -f .old_version

barebox.srec: barebox
	$(OBJCOPY) -O srec $< $@

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(barebox-head) $(barebox-common) ) $(barebox-lds) $(barebox-pbl-common): $(barebox-dirs) ;

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

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

# Listed in dependency order
PHONY += prepare archprepare prepare0 prepare1 prepare2 prepare3

# prepare-all is deprecated, use prepare as valid replacement
PHONY += prepare-all

# prepare3 is used to check if we are building in a separate output directory,
# and if so do:
# 1) Check that make has not been executed in the kernel src $(srctree)
prepare3: include/config/kernel.release
ifneq ($(KBUILD_SRC),)
	@echo '  Using $(srctree) as source for barebox'
	$(Q)if [ -f $(srctree)/.config -o -d $(srctree)/include/config ]; then \
		echo "  $(srctree) is not clean, please run 'make mrproper'";\
		echo "  in the '$(srctree)' directory.";\
		false; \
	fi;
endif

# prepare2 creates a makefile if using a separate output directory
prepare2: prepare3 outputmakefile

prepare1: prepare2 include/generated/version.h include/generated/utsrelease.h \
                   include/config.h

ifneq ($(KBUILD_MODULES),)
	$(Q)mkdir -p $(MODVERDIR)
	$(Q)rm -f $(MODVERDIR)/*
endif

archprepare: prepare1 scripts_basic

prepare0: archprepare FORCE
	$(Q)$(MAKE) $(build)=.

# All the preparing..
prepare prepare-all: prepare0

# Leave this as default for preprocessing barebox.lds.S, which is now
# done in arch/$(ARCH)/kernel/Makefile

export CPPFLAGS_barebox.lds += -C -U$(ARCH)

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
	(echo \#define UTS_RELEASE \"$(KERNELRELEASE)\";)
endef

define filechk_version.h
	(echo \#define LINUX_VERSION_CODE $(shell                             \
	expr $(VERSION) \* 65536 + $(PATCHLEVEL) \* 256 + $(SUBLEVEL));     \
	echo '#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))';)
endef

include/generated/version.h: $(srctree)/Makefile FORCE
	$(call filechk,version.h)

include/generated/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

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
                .tmp_version .tmp_barebox* barebox.bin barebox.map barebox.S \
		.tmp_kallsyms* barebox.ldr \
		scripts/bareboxenv-target barebox-flash-image \
		barebox.srec barebox.s5p barebox.ubl barebox.zynq \
		barebox.uimage barebox.spi barebox.kwb barebox.kwbuart \
		barebox.efi barebox.canon-a1100.bin

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


# Packaging of the kernel to various formats
# ---------------------------------------------------------------------------
# rpm target kept for backward compatibility
package-dir	:= $(srctree)/scripts/package

%pkg: include/config/kernel.release FORCE
	$(Q)$(MAKE) $(build)=$(package-dir) $@
rpm: include/config/kernel.release FORCE
	$(Q)$(MAKE) $(build)=$(package-dir) $@


# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/arch/$(ARCH)/configs/*_defconfig)
boards := $(sort $(notdir $(boards)))

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
	@if [ -r include/asm-$(ARCH)/Kbuild ]; then \
	 echo  '  headers_check   - Sanity check on exported headers'; \
	 fi
	@echo  ''
	@echo  'Architecture specific targets ($(ARCH)):'
	@$(if $(archhelp),$(archhelp),\
		echo '  No architecture specific help defined for $(ARCH)')
	@echo  ''
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
      cmd_tags = $(CONFIG_SHELL) $(srctree)/scripts/tags.sh $@

tags TAGS cscope: FORCE
	$(call cmd,tags)

SPHINXBUILD   = sphinx-build
ALLSPHINXOPTS   =  source

docs: FORCE
	@mkdir -p $(srctree)/Documentation/commands
	@$(srctree)/Documentation/gen_commands.py $(srctree) $(srctree)/Documentation/commands
	@$(SPHINXBUILD) -b html -d $(objtree)/doctrees $(srctree)/Documentation \
		$(objtree)/Documentation/html

endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

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


a_flags = -Wp,-MD,$(depfile) $(AFLAGS) $(AFLAGS_KERNEL) \
	  $(NOSTDINC_FLAGS) $(CPPFLAGS) \
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

endif	# skip-makefile

PHONY += FORCE
FORCE:

# Cancel implicit rules on top Makefile, `-rR' will apply to sub-makes.
Makefile: ;

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

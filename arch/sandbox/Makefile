# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := sandbox_defconfig

generated_configs += headless_defconfig noshell_defconfig lockdown_defconfig \
		     simpleconsole_defconfig libfuzzer_defconfig
headless_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless)
noshell_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,noshell)
lockdown_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless noshell)
simpleconsole_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,simpleconsole)
libfuzzer_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,libfuzzer)

KBUILD_CPPFLAGS += -D__SANDBOX__ -fno-strict-aliasing -fvisibility=hidden


machine-y := sandbox

board-y := arch/sandbox/board
BOARD	:= $(board-y)/

HEADER_ARCH 	:= $(SUBARCH)
HOST_DIR := arch/$(HEADER_ARCH)

-include $(srctree)/$(HOST_DIR)/Makefile.um

common-y += $(HOST_DIR)/um/

KBUILD_CPPFLAGS += -I$(srctree)/$(HOST_DIR)/include

TEXT_BASE = $(CONFIG_TEXT_BASE)

SANDBOX_PBL2PROPER_GLUE_SYMS := \
	putchar errno setjmp longjmp \
	malloc_stats memalign malloc free realloc calloc brk sbrk

KBUILD_CFLAGS += $(foreach s,$(SANDBOX_PBL2PROPER_GLUE_SYMS),-D$(s)=barebox_$(s))

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

archprepare: maketools
	$(Q)$(MAKE) $(build)=$(HOST_DIR)/um

PHONY += maketools

export SANDBOX_LIBS BAREBOX_LDFLAGS

SANDBOX_LIBS-$(CONFIG_SDL) += $(shell $(CROSS_PKG_CONFIG) sdl2 --libs)
SANDBOX_LIBS-$(CONFIG_GPIO_LIBFTDI1) += $(shell $(CROSS_PKG_CONFIG) libftdi1 --libs)

ifeq ($(CONFIG_ASAN),y)
KBUILD_CPPFLAGS += -fsanitize=address
SANDBOX_LIBS-y += -fsanitize=address
ifeq ($(CONFIG_CC_IS_CLANG),y)
KBUILD_CPPFLAGS += -fno-sanitize-address-globals-dead-stripping
SANDBOX_LIBS-y += -fno-sanitize-address-globals-dead-stripping
endif
endif

SANDBOX_LIBS-$(CONFIG_UBSAN) += -fsanitize=undefined

ifeq ($(CONFIG_FUZZ_EXTERNAL),y)
LIBARCH-y = $(UNAME_M)
LIBARCH-$(CONFIG_SANDBOX_LINUX_I386) = i386

KBUILD_CPPFLAGS += -fsanitize=fuzzer-no-link
SANDBOX_LIBS-y += -Wl,-Bstatic -L"$(CONFIG_CLANG_RUNTIME_DIR)" \
		-lclang_rt.fuzzer_no_main-$(LIBARCH-y) -Wl,-Bdynamic
endif

ifeq ($(CONFIG_GCOV),y)
GCOV_OPT-$(CONFIG_CC_IS_CLANG) = -fprofile-instr-generate -fcoverage-mapping
GCOV_OPT-$(CONFIG_CC_IS_GCC) = -fprofile-arcs -ftest-coverage
KBUILD_CFLAGS += $(GCOV_OPT-y)
BAREBOX_LDFLAGS += $(GCOV_OPT-y)
endif

ifeq ($(CONFIG_SANDBOX_LINUX_I386),y)
KBUILD_CFLAGS += -m32
KBUILD_LDFLAGS += -m elf_i386
KBUILD_AFLAGS += -m32
BAREBOX_LDFLAGS += -m32
endif

SANDBOX_LIBS += $(SANDBOX_LIBS-y)

common-y += $(BOARD) arch/sandbox/os/ arch/sandbox/lib/

SANDBOX_PROPER2PBL_GLUE_SYMS := \
	strsep_unescaped start_barebox linux_get_stickypage_path \
	stickypage mem_malloc_init \
	barebox_register_filedev barebox_register_dtb barebox_register_console \
	barebox_errno barebox_loglevel list_fuzz_tests setup_external_fuzz

OBJCOPYFLAGS_barebox.o := $(addprefix --keep-global-symbol=, $(SANDBOX_PROPER2PBL_GLUE_SYMS))

quiet_cmd_barebox__ = $(quiet_cmd_symlink)
      cmd_barebox__ = ln -fsn --relative $(objtree)/images/barebox $(objtree)/barebox

define rule_barebox__
	$(call cmd,barebox__)
	$(Q)echo 'savedcmd_$@ := $(cmd_barebox__)' > $(@D)/.$(@F).cmd
endef

all: barebox

KBUILD_IMAGE := barebox.o
BAREBOX_PROPER := $(KBUILD_IMAGE)

common-$(CONFIG_OFTREE) += arch/sandbox/dts/

CLEAN_FILES += $(BOARD)/barebox.lds

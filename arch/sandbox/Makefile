# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := sandbox_defconfig

generated_configs += headless_defconfig noshell_defconfig lockdown_defconfig \
		     libfuzzer_defconfig
headless_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless)
noshell_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,noshell)
lockdown_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless noshell)
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

ifeq ($(CONFIG_SDL),y)
SANDBOX_LIBS += $(shell $(CROSS_PKG_CONFIG) sdl2 --libs)
endif

ifeq ($(CONFIG_GPIO_LIBFTDI1),y)
SANDBOX_LIBS += $(shell $(CROSS_PKG_CONFIG) libftdi1 --libs)
endif

ifeq ($(CONFIG_ASAN),y)
KBUILD_CPPFLAGS += -fsanitize=address
SANDBOX_LIBS += -fsanitize=address
ifeq ($(CONFIG_CC_IS_CLANG),y)
KBUILD_CPPFLAGS += -fno-sanitize-address-globals-dead-stripping
SANDBOX_LIBS += -fno-sanitize-address-globals-dead-stripping
endif
endif

ifeq ($(CONFIG_UBSAN),y)
SANDBOX_LIBS += -fsanitize=undefined
endif

ifeq ($(CONFIG_FUZZ_EXTERNAL),y)
LIBARCH-y = $(UNAME_M)
LIBARCH-$(CONFIG_SANDBOX_LINUX_I386) = i386

KBUILD_CPPFLAGS += -fsanitize=fuzzer-no-link
SANDBOX_LIBS += -Wl,-Bstatic -L"$(CONFIG_CLANG_RUNTIME_DIR)" \
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

common-y += $(BOARD) arch/sandbox/os/ arch/sandbox/lib/

SANDBOX_PROPER2PBL_GLUE_SYMS := \
	strsep_unescaped start_barebox linux_get_stickypage_path \
	stickypage mem_malloc_init \
	barebox_register_filedev barebox_register_dtb barebox_register_console \
	barebox_errno barebox_loglevel call_for_each_fuzz_test setup_external_fuzz

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

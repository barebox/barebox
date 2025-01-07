# SPDX-License-Identifier: GPL-2.0-only

machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

KBUILD_CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))

cppflags-$(CONFIG_CONSOLE_NONE) += -DCONFIG_CONSOLE_NONE=$(CONFIG_CONSOLE_NONE)
cppflags-$(CONFIG_ASAN) += -DCONFIG_ASAN=$(CONFIG_ASAN)

KBUILD_CPPFLAGS += -DCONFIG_MALLOC_SIZE=$(CONFIG_MALLOC_SIZE) -D_FILE_OFFSET_BITS=64 \
		   -DCONFIG_STACK_SIZE=$(CONFIG_STACK_SIZE) $(cppflags-y)

KBUILD_CFLAGS := -Wall

NOSTDINC_FLAGS :=

ifeq ($(CONFIG_SANDBOX_LINUX_I386),y)
KBUILD_CFLAGS += -m32
endif

obj-y = common.o tap.o setjmp.o
obj-$(CONFIG_MALLOC_LIBC) += libc_malloc.o

CFLAGS_sdl.o = $(shell $(PKG_CONFIG) sdl2 --cflags)
obj-$(CONFIG_SDL) += sdl.o

CFLAGS_ftdi.o = $(shell $(PKG_CONFIG) libftdi1 --cflags)
obj-$(CONFIG_GPIO_LIBFTDI1) += ftdi.o

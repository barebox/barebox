machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

KBUILD_CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))

KBUILD_CPPFLAGS += -DCONFIG_MALLOC_SIZE=$(CONFIG_MALLOC_SIZE)

ifeq ($(CONFIG_KASAN),y)
KBUILD_CPPFLAGS += -DCONFIG_KASAN=1
endif

KBUILD_CFLAGS += -Wall
NOSTDINC_FLAGS :=

obj-y = common.o tap.o
obj-$(CONFIG_MALLOC_LIBC) += libc_malloc.o

CFLAGS_sdl.o = $(shell pkg-config sdl --cflags)
obj-$(CONFIG_DRIVER_VIDEO_SDL) += sdl.o

CFLAGS_ftdi.o = $(shell pkg-config libftdi1 --cflags)
obj-$(CONFIG_GPIO_LIBFTDI1) += ftdi.o

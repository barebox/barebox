machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

KBUILD_CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))

KBUILD_CPPFLAGS += -DCONFIG_MALLOC_SIZE=$(CONFIG_MALLOC_SIZE)

KBUILD_CFLAGS := -Wall
NOSTDINC_FLAGS :=

obj-y = common.o tap.o

CFLAGS_sdl.o = $(shell pkg-config sdl --cflags)
obj-$(CONFIG_DRIVER_VIDEO_SDL) += sdl.o

CFLAGS_ftdi.o = $(shell pkg-config libftdi1 --cflags)
obj-$(CONFIG_GPIO_LIBFTDI1) += ftdi.o

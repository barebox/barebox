machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS := $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

CPPFLAGS += -DCONFIG_MALLOC_SIZE=$(CONFIG_MALLOC_SIZE)

CFLAGS := -Wall
NOSTDINC_FLAGS :=

obj-y = common.o tap.o

CFLAGS_sdl.o = $(shell pkg-config sdl --cflags)
obj-$(CONFIG_DRIVER_VIDEO_SDL) += sdl.o

CFLAGS_ftdi.o = $(shell pkg-config libftdi1 --cflags)
obj-$(CONFIG_GPIO_LIBFTDI1) += ftdi.o

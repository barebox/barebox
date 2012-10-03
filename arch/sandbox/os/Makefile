machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS := $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

CFLAGS := -Wall
NOSTDINC_FLAGS :=

obj-y = common.o tap.o

CFLAGS_sdl.o = $(shell pkg-config sdl --cflags)
obj-$(CONFIG_DRIVER_VIDEO_SDL) += sdl.o

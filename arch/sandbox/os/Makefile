machine-y := sandbox

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS := $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS = $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

CPPFLAGS += -P
CFLAGS := -Wall
NOSTDINC_FLAGS :=

obj-y = common.o tap.o


CPPFLAGS_u-boot.lds = -DSTART=$(START) -U$(SUBARCH) -DELF_ARCH=$(ELF_ARCH) \
	-DELF_FORMAT="$(ELF_FORMAT)" -DKERNEL_STACK_SIZE=8192

extra-y += u-boot.lds

START := 0x8048000

export START

CPPFLAGS := -I/usr/include -Iinclude -P
CFLAGS := -Wall
#NOSTDINC_FLAGS :=

obj-y = common.o tap.o

USER_OBJS := common.o

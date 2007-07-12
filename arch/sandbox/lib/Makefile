CPPFLAGS_u-boot.lds = -U$(SUBARCH) -DELF_ARCH=$(ELF_ARCH) \
	-DELF_FORMAT="$(ELF_FORMAT)"

extra-y += u-boot.lds

CPPFLAGS := -I/usr/include -Iinclude -P
CFLAGS := -Wall
#NOSTDINC_FLAGS :=

obj-y = common.o tap.o

USER_OBJS := common.o

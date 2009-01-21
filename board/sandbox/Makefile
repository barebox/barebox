obj-y += board.o
obj-y += clock.o
obj-y += hostfile.o
obj-y += console.o

CPPFLAGS_u-boot.lds = -U$(SUBARCH) -DELF_ARCH=$(ELF_ARCH) \
        -DELF_FORMAT="$(ELF_FORMAT)"
extra-y += u-boot.lds


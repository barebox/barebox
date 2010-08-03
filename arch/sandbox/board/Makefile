obj-y += board.o
obj-y += clock.o
obj-y += hostfile.o
obj-y += console.o

CPPFLAGS_barebox.lds = -U$(SUBARCH) -DELF_ARCH=$(ELF_ARCH) \
        -DELF_FORMAT="$(ELF_FORMAT)"
extra-y += barebox.lds


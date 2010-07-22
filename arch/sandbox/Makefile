
CPPFLAGS += -fno-strict-aliasing

machine-y := sandbox

board-y := arch/sandbox/board
BOARD	:= $(board-y)/
lds-y   := $(BOARD)/barebox.lds


TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -P
CFLAGS += -Dmalloc=barebox_malloc \
		-Dfree=barebox_free -Drealloc=barebox_realloc \
		-Dread=barebox_read -Dwrite=barebox_write \
		-Dopen=barebox_open -Dclose=barebox_close \
		-Dlseek=barebox_lseek -Dperror=barebox_perror \
		-Derrno=barebox_errno -Dgetc=barebox_getc \
		-Dputc=barebox_putc -Dfgetc=barebox_fgetc \
		-Dfputc=barebox_fputc -Dfgets=barebox_fgets \
		-Dfputs=barebox_fputs -Dsetenv=barebox_setenv \
		-Dgetenv=barebox_getenv -Dprintf=barebox_printf \
		-Dglob=barebox_glob -Dglobfree=barebox_globfree \
		-Dioctl=barebox_ioctl

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

SUBARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
				  -e s/arm.*/arm/ -e s/sa110/arm/ \
				  -e s/s390x/s390/ -e s/parisc64/parisc/ \
				  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ )

ifeq ($(SUBARCH),x86_64)
ELF_ARCH   := i386:x86-64
ELF_FORMAT := elf64-x86-64
ELF_CLASS := ELFCLASS64
endif
ifeq ($(SUBARCH),i386)
ELF_ARCH   := $(SUBARCH)
ELF_FORMAT := elf32-$(SUBARCH)
ELF_CLASS  := ELFCLASS32
endif
ifeq ($(SUBARCH),powerpc)
ELF_ARCH   := powerpc
ELF_FORMAT := elf32-powerpc
ELF_CLASS  := ELFCLASS32
endif

CPPFLAGS += -DELF_CLASS="$(ELF_CLASS)"
export ELF_FORMAT ELF_ARCH SUBARCH ELF_CLASS

archprepare: maketools

PHONY += maketools

cmd_barebox__ = $(CC) -o $@ -Wl,-T,$(barebox-lds) \
	-Wl,--start-group $(barebox-common) -Wl,--end-group \
	-lrt -lpthread

common-y += $(BOARD) arch/sandbox/os/

CLEAN_FILES += $(BOARD)/barebox.lds


CPPFLAGS += -fno-strict-aliasing

machine-y := sandbox

board-y := sandbox
lds-y   := board/sandbox/u-boot.lds

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -P
CFLAGS += -Dmalloc=u_boot_malloc \
		-Dfree=u_boot_free -Drealloc=u_boot_realloc \
		-Dread=u_boot_read -Dwrite=u_boot_write \
		-Dopen=u_boot_open -Dclose=u_boot_close \
		-Dlseek=u_boot_lseek -Dperror=u_boot_perror \
		-Derrno=u_boot_errno -Dgetc=u_boot_getc \
		-Dputc=u_boot_putc -Dfgetc=u_boot_fgetc \
		-Dfputc=u_boot_fputc -Dfgets=u_boot_fgets \
		-Dfputs=u_boot_fputs -Dsetenv=u_boot_setenv \
		-Dgetenv=u_boot_getenv -Dprintf=u_boot_printf \
		-Dglob=u_boot_glob -Dglobfree=u_boot_globfree \
		-Dioctl=u_boot_ioctl

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

cmd_uboot__ = $(CC) -o $@ -Wl,-T,$(uboot-lds) \
	-Wl,--start-group $(uboot-common) -Wl,--end-group \
	-lrt -lpthread

common-y += board/sandbox/ arch/sandbox/os/

CLEAN_FILES += board/sandbox/u-boot.lds

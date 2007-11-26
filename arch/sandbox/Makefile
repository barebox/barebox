
CPPFLAGS += -fno-builtin -ffreestanding -nostdinc -Wall \
	-isystem $(gccincdir) -pipe \
	-fno-strict-aliasing


machine-y	   := sandbox

BOARD:= arch/sandbox/lib
board-y := sandbox
lds-y   := arch/sandbox/lib/u-boot.lds

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -P
CFLAGS := -fno-common -Os -Dmalloc=u_boot_malloc \
		-Dfree=u_boot_free -Drealloc=u_boot_realloc \
		-Dread=u_boot_read -Dwrite=u_boot_write \
		-Dopen=u_boot_open -Dclose=u_boot_close \
		-Dlseek=u_boot_lseek -Dperror=u_boot_perror \
		-Derrno=u_boot_errno -Dgetc=u_boot_getc \
		-Dputc=u_boot_putc -Dfgetc=u_boot_fgetc \
		-Dfputc=u_boot_fputc -Dfgets=u_boot_fgets \
		-Dfputs=u_boot_fputs -Dsetenv=u_boot_setenv \
		-Dgetenv=u_boot_getenv -Dprintf=u_boot_printf

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

SUBARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
				  -e s/arm.*/arm/ -e s/sa110/arm/ \
				  -e s/s390x/s390/ -e s/parisc64/parisc/ \
				  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ )

ifeq ($(SUBARCH),x86_64)
ELF_ARCH   := i386:x86-64
ELF_FORMAT := elf64-x86-64
endif
ifeq ($(SUBARCH),i386)
ELF_ARCH   := $(SUBARCH)
ELF_FORMAT := elf32-$(SUBARCH)
endif

export ELF_FORMAT ELF_ARCH SUBARCH
#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-sandbox/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-sandbox/arch -> include/asm-sandbox/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-sandbox
	$(Q)ln -fsn $(srctree)/include/asm-sandbox/$(INCDIR) include/asm-sandbox/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-sandbox/arch
endif
	@touch $@

archprepare: maketools

PHONY += maketools
maketools: include/asm-sandbox/.arch

cmd_uboot__ = $(CC) -o $@ -Wl,-T,$(uboot-lds) \
	-Wl,--start-group $(uboot-common) -Wl,--end-group \
	-lrt -lpthread

common-y += arch/sandbox/lib/ arch/sandbox/board/

MRPROPER_FILES += include/asm-sandbox/arch

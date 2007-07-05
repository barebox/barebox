
CPPFLAGS += -fno-builtin -ffreestanding -nostdinc -Wall \
	-isystem $(gccincdir) -pipe \
	-fno-strict-aliasing


machine-y	   := linux

BOARD:= arch/linux/lib
board-y := linux

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -P
CFLAGS := -fno-common -Os -Dmalloc=u_boot_malloc \
		-Dfree=u_boot_free -Drealloc=u_boot_realloc \
		-Dread=u_boot_read -Dwrite=u_boot_write \
		-Dopen=u_boot_open -Dclose=u_boot_close \
		-Dlseek=u_boot_lseek -Dperror=u_boot_perror \
		-Derrno=u_boot_errno

LDFLAGS_vmlinux  :=-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := arch-$(incdir-y)

SUBARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
				  -e s/arm.*/arm/ -e s/sa110/arm/ \
				  -e s/s390x/s390/ -e s/parisc64/parisc/ \
				  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ )

ELF_ARCH   := $(SUBARCH)
ELF_FORMAT := elf32-$(SUBARCH)

export ELF_FORMAT ELF_ARCH SUBARCH
#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm-linux/.arch: $(wildcard include/config/arch/*.h) include/config/auto.conf
	@echo '  SYMLINK include/asm-linux/arch -> include/asm-linux/$(INCDIR)'
ifneq ($(KBUILD_SRC),)
	$(Q)mkdir -p include/asm-linux
	$(Q)ln -fsn $(srctree)/include/asm-linux/$(INCDIR) include/asm-linux/arch
else
	$(Q)ln -fsn $(INCDIR) include/asm-linux/arch
endif
	@touch $@

archprepare: maketools

PHONY += maketools
maketools: include/asm-linux/.arch

cmd_vmlinux__ = $(CC) -o $@ -Wl,-T,$(vmlinux-lds) \
	-Wl,--start-group $(vmlinux-common) -Wl,--end-group \
	-lrt -Wl,-rpath,/lib -lrt -lpthread

common-y += arch/linux/lib/ arch/linux/board/

MRPROPER_FILES += include/asm-linux/arch

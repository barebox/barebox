# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := sandbox_defconfig

generated_configs += headless_defconfig noshell_defconfig lockdown_defconfig
headless_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless)
noshell_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,noshell)
lockdown_defconfig:
	$(call merge_into_defconfig,sandbox_defconfig,headless noshell)

KBUILD_CPPFLAGS += -D__SANDBOX__ -fno-strict-aliasing -fvisibility=hidden


machine-y := sandbox

board-y := arch/sandbox/board
BOARD	:= $(board-y)/
lds-y   := $(BOARD)/barebox.lds


TEXT_BASE = $(CONFIG_TEXT_BASE)
KBUILD_CFLAGS += -Dmalloc=barebox_malloc -Dcalloc=barebox_calloc \
		-Dmalloc_stats=barebox_malloc_stats -Dmemalign=barebox_memalign \
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
		-Dioctl=barebox_ioctl -Dfstat=barebox_fstat \
		-Dftruncate=barebox_ftruncate -Dasprintf=barebox_asprintf \
		-Dopendir=barebox_opendir -Dreaddir=barebox_readdir \
		-Dclosedir=barebox_closedir -Dreadlink=barebox_readlink \
		-Doptarg=barebox_optarg -Doptind=barebox_optind \
		-Dsetjmp=barebox_setjmp -Dlongjmp=barebox_longjmp \
		-Dmkdir=barebox_mkdir -Ddirname=barebox_dirname \
		-Dremove=barebox_remove -Dputchar=barebox_putchar

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

archprepare: maketools

PHONY += maketools

ifeq ($(CONFIG_SDL),y)
SDL_LIBS := $(shell $(PKG_CONFIG) sdl2 --libs)
endif

ifeq ($(CONFIG_GPIO_LIBFTDI1),y)
FTDI1_LIBS := $(shell $(PKG_CONFIG) libftdi1 --libs)
endif

ifeq ($(CONFIG_ASAN),y)
KBUILD_CPPFLAGS += -fsanitize=address
SANITIZER_LIBS += -fsanitize=address
endif

ifeq ($(CONFIG_UBSAN),y)
SANITIZER_LIBS += -fsanitize=undefined
endif

ifeq ($(CONFIG_SANDBOX_LINUX_I386),y)
KBUILD_CFLAGS += -m32
KBUILD_LDFLAGS += -m elf_i386
KBUILD_AFLAGS += -m32
BAREBOX_LDFLAGS += -m32
endif

BAREBOX_LDFLAGS += \
	-Wl,-T,$(BAREBOX_LDS) \
	-Wl,--whole-archive $(BAREBOX_OBJS) -Wl,--no-whole-archive \
	-lrt -pthread $(SDL_LIBS) $(FTDI1_LIBS) \
	$(SANITIZER_LIBS)

cmd_barebox__ = $(CC) -o $@ $(BAREBOX_LDFLAGS)

common-y += $(BOARD) arch/sandbox/os/ arch/sandbox/lib/

KBUILD_IMAGE := barebox

common-$(CONFIG_OFTREE) += arch/sandbox/dts/

CLEAN_FILES += $(BOARD)/barebox.lds

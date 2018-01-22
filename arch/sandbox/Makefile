KBUILD_DEFCONFIG := sandbox_defconfig

CPPFLAGS += -D__SANDBOX__ -fno-strict-aliasing

machine-y := sandbox

board-y := arch/sandbox/board
BOARD	:= $(board-y)/
lds-y   := $(BOARD)/barebox.lds


TEXT_BASE = $(CONFIG_TEXT_BASE)
CFLAGS += -Dmalloc=barebox_malloc -Dcalloc=barebox_calloc \
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
		-Dopendir=barebox_opendir -Dreaddir=barebox_readdir \
		-Dclosedir=barebox_closedir

machdirs := $(patsubst %,arch/sandbox/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

archprepare: maketools

PHONY += maketools

ifeq ($(CONFIG_DRIVER_VIDEO_SDL),y)
SDL_LIBS := $(shell pkg-config sdl --libs)
endif

ifeq ($(CONFIG_GPIO_LIBFTDI1),y)
FTDI1_LIBS := $(shell pkg-config libftdi1 --libs)
endif

cmd_barebox__ = $(CC) -o $@ -Wl,-T,$(barebox-lds) \
	-Wl,--start-group $(barebox-common) -Wl,--end-group \
	-lrt -lpthread $(SDL_LIBS) $(FTDI1_LIBS)

common-y += $(BOARD) arch/sandbox/os/

common-$(CONFIG_OFTREE) += arch/sandbox/dts/

CLEAN_FILES += $(BOARD)/barebox.lds


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

archprepare: maketools

PHONY += maketools

ifeq ($(CONFIG_DRIVER_VIDEO_SDL),y)
SDL_LIBS := $(shell pkg-config sdl --libs)
endif

cmd_barebox__ = $(CC) -o $@ -Wl,-T,$(barebox-lds) \
	-Wl,--start-group $(barebox-common) -Wl,--end-group \
	-lrt -lpthread $(SDL_LIBS)

common-y += $(BOARD) arch/sandbox/os/

CLEAN_FILES += $(BOARD)/barebox.lds

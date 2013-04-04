
/*
 * Align to a 32 byte boundary equal to the
 * alignment gcc 4.5 uses for a struct
 */
#define STRUCT_ALIGNMENT 32
#define STRUCT_ALIGN() . = ALIGN(STRUCT_ALIGNMENT)

#if defined CONFIG_ARCH_IMX25 || \
	defined CONFIG_ARCH_IMX35 || \
	defined CONFIG_ARCH_IMX51 || \
	defined CONFIG_ARCH_IMX53 || \
	defined CONFIG_ARCH_IMX6 || \
	defined CONFIG_X86 || \
	defined CONFIG_ARCH_EP93XX || \
	defined CONFIG_ARCH_ZYNQ
#include <mach/barebox.lds.h>
#endif

#ifndef PRE_IMAGE
#define PRE_IMAGE
#endif

#define INITCALLS			\
	KEEP(*(.initcall.0))			\
	KEEP(*(.initcall.1))			\
	KEEP(*(.initcall.2))			\
	KEEP(*(.initcall.3))			\
	KEEP(*(.initcall.4))			\
	KEEP(*(.initcall.5))			\
	KEEP(*(.initcall.6))			\
	KEEP(*(.initcall.7))			\
	KEEP(*(.initcall.8))			\
	KEEP(*(.initcall.9))			\
	KEEP(*(.initcall.10))			\
	KEEP(*(.initcall.11))

#define BAREBOX_CMDS	KEEP(*(SORT_BY_NAME(.barebox_cmd*)))

#define BAREBOX_SYMS	KEEP(*(__usymtab))

#define BAREBOX_MAGICVARS	KEEP(*(SORT_BY_NAME(.barebox_magicvar*)))

#define BAREBOX_DTB()				\
	__dtb_start = .;			\
	KEEP(*(.dtb.rodata.*));				\
	__dtb_end = .;

#if defined(CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE) && \
CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE < CONFIG_BAREBOX_MAX_BARE_INIT_SIZE
#define MAX_BARE_INIT_SIZE CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE
#else
#define MAX_BARE_INIT_SIZE CONFIG_BAREBOX_MAX_BARE_INIT_SIZE
#endif

#include <linux/stringify.h>
/* use 2 ASSERT because ld can not accept '"size" "10"' format */
#define BAREBOX_BARE_INIT_SIZE					\
	_barebox_bare_init_size = __bare_init_end - _text;	\
	ASSERT(_barebox_bare_init_size < MAX_BARE_INIT_SIZE, "Barebox bare_init size > ") \
	ASSERT(_barebox_bare_init_size < MAX_BARE_INIT_SIZE, __stringify(MAX_BARE_INIT_SIZE)) \


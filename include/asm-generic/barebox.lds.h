
/*
 * Align to a 32 byte boundary equal to the
 * alignment gcc 4.5 uses for a struct
 */
#define STRUCT_ALIGNMENT 32
#define STRUCT_ALIGN() . = ALIGN(STRUCT_ALIGNMENT)

#if defined CONFIG_X86 || \
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
	KEEP(*(.initcall.11))			\
	KEEP(*(.initcall.12))			\
	KEEP(*(.initcall.13))			\
	KEEP(*(.initcall.14))

#define EXITCALLS			\
	KEEP(*(.exitcall.0))			\
	KEEP(*(.exitcall.1))			\
	KEEP(*(.exitcall.2))			\
	KEEP(*(.exitcall.3))			\
	KEEP(*(.exitcall.4))			\
	KEEP(*(.exitcall.5))			\
	KEEP(*(.exitcall.6))

#define BAREBOX_CMDS	KEEP(*(SORT_BY_NAME(.barebox_cmd*)))

#define BAREBOX_RATP_CMDS	KEEP(*(SORT_BY_NAME(.barebox_ratp_cmd*)))

#define BAREBOX_SYMS	KEEP(*(__usymtab))

#define BAREBOX_MAGICVARS	KEEP(*(SORT_BY_NAME(.barebox_magicvar*)))

#define BAREBOX_CLK_TABLE()			\
	. = ALIGN(8);				\
	__clk_of_table_start = .;		\
	KEEP(*(.__clk_of_table));		\
	KEEP(*(.__clk_of_table_end));		\
	__clk_of_table_end = .;

#define BAREBOX_DTB()				\
	. = ALIGN(8);				\
	__dtb_start = .;			\
	KEEP(*(.dtb.rodata.*));			\
	__dtb_end = .;

#define BAREBOX_IMD				\
	KEEP(*(.barebox_imd_start))		\
	KEEP(*(.barebox_imd_1*))		\
	*(.barebox_imd_0*)			\
	KEEP(*(.barebox_imd_end))

#if defined(CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE) && \
CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE < CONFIG_BAREBOX_MAX_BARE_INIT_SIZE
#define MAX_BARE_INIT_SIZE CONFIG_ARCH_BAREBOX_MAX_BARE_INIT_SIZE
#else
#define MAX_BARE_INIT_SIZE CONFIG_BAREBOX_MAX_BARE_INIT_SIZE
#endif

#if defined(CONFIG_ARCH_BAREBOX_MAX_PBL_SIZE) && \
CONFIG_ARCH_BAREBOX_MAX_PBL_SIZE < CONFIG_BAREBOX_MAX_PBL_SIZE
#define MAX_PBL_SIZE CONFIG_ARCH_BAREBOX_MAX_PBL_SIZE
#else
#define MAX_PBL_SIZE CONFIG_BAREBOX_MAX_PBL_SIZE
#endif

#include <linux/stringify.h>
/* use 2 ASSERT because ld can not accept '"size" "10"' format */
#define BAREBOX_BARE_INIT_SIZE					\
	_barebox_bare_init_size = __bare_init_end - _text;	\
	ASSERT(_barebox_bare_init_size < MAX_BARE_INIT_SIZE, "Barebox bare_init size > ") \
	ASSERT(_barebox_bare_init_size < MAX_BARE_INIT_SIZE, __stringify(MAX_BARE_INIT_SIZE)) \

#define BAREBOX_PBL_SIZE					\
	_barebox_pbl_size = __bss_start - _text;		\
	ASSERT(MAX_BARE_INIT_SIZE <= MAX_PBL_SIZE, "bare_init cannot be bigger than pbl") \
	ASSERT(_barebox_pbl_size < MAX_PBL_SIZE, "Barebox pbl size > ") \
	ASSERT(_barebox_pbl_size < MAX_PBL_SIZE, __stringify(MAX_PBL_SIZE)) \



#if defined CONFIG_ARCH_IMX25 || defined CONFIG_ARCH_IMX35 || defined CONFIG_ARCH_IMX51 || defined CONFIG_X86
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
	KEEP(*(.initcall.8))

#define BAREBOX_CMDS	KEEP(*(SORT_BY_NAME(.barebox_cmd*)))

#define BAREBOX_SYMS	KEEP(*(__usymtab))

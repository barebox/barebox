/* SPDX-License-Identifier: GPL-2.0-only */


/*
 * Align to a 32 byte boundary equal to the
 * alignment gcc 4.5 uses for a struct
 */
#define STRUCT_ALIGNMENT 32
#define STRUCT_ALIGN() . = ALIGN(STRUCT_ALIGNMENT)

#ifndef PRE_IMAGE
#define PRE_IMAGE
#endif

#define BAREBOX_INITCALLS			\
	STRUCT_ALIGN();				\
	__barebox_initcalls_start = .;		\
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
	KEEP(*(.initcall.14))			\
	KEEP(*(.initcall.15))			\
	KEEP(*(.initcall.16))			\
	__barebox_initcalls_end = .;

#define BAREBOX_EXITCALLS			\
	STRUCT_ALIGN();				\
	__barebox_exitcalls_start = .;		\
	KEEP(*(.exitcall.0))			\
	KEEP(*(.exitcall.1))			\
	KEEP(*(.exitcall.2))			\
	KEEP(*(.exitcall.3))			\
	KEEP(*(.exitcall.4))			\
	KEEP(*(.exitcall.5))			\
	KEEP(*(.exitcall.6))			\
	__barebox_exitcalls_end = .;

#define BAREBOX_CMDS				\
	STRUCT_ALIGN();				\
	__barebox_cmd_start = .;		\
	KEEP(*(SORT_BY_NAME(.barebox_cmd*)))	\
	__barebox_cmd_end = .;

#define BAREBOX_RATP_CMDS			\
	STRUCT_ALIGN();				\
	__barebox_ratp_cmd_start = .;		\
	KEEP(*(SORT_BY_NAME(.barebox_ratp_cmd*)))	\
	__barebox_ratp_cmd_end = .;

#define BAREBOX_SYMS				\
	STRUCT_ALIGN();				\
	__usymtab_start = .;			\
	KEEP(*(__usymtab))			\
	__usymtab_end = .;

#define BAREBOX_MAGICVARS			\
	STRUCT_ALIGN();				\
	__barebox_magicvar_start = .;		\
	KEEP(*(SORT_BY_NAME(.barebox_magicvar*)))	\
	__barebox_magicvar_end = .;

#define BAREBOX_CLK_TABLE			\
	STRUCT_ALIGN();				\
	__clk_of_table_start = .;		\
	KEEP(*(.__clk_of_table));		\
	KEEP(*(.__clk_of_table_end));		\
	__clk_of_table_end = .;

#define BAREBOX_DTB				\
	STRUCT_ALIGN();				\
	__dtb_start = .;			\
	KEEP(*(.dtb.rodata.*));			\
	__dtb_end = .;

#define BAREBOX_IMD				\
	STRUCT_ALIGN();				\
	KEEP(*(.barebox_imd_start))		\
	KEEP(*(.barebox_imd_1*))		\
	*(.barebox_imd_0*)			\
	KEEP(*(.barebox_imd_end))

#ifdef CONFIG_PCI
#define BAREBOX_PCI_FIXUP			\
	STRUCT_ALIGN();				\
	__start_pci_fixups_early = .;		\
	KEEP(*(.pci_fixup_early))		\
	__end_pci_fixups_early = .;		\
	__start_pci_fixups_header = .;		\
	KEEP(*(.pci_fixup_header))		\
	__end_pci_fixups_header = .;		\
	__start_pci_fixups_enable = .;		\
	KEEP(*(.pci_fixup_enable))		\
	__end_pci_fixups_enable = .;
#else
#define BAREBOX_PCI_FIXUP
#endif

#define BAREBOX_PUBLIC_KEYS			\
	STRUCT_ALIGN();				\
	__public_keys_start = .;		\
	KEEP(*(.public_keys.rodata.*));		\
	__public_keys_end = .;			\

#define BAREBOX_DEEP_PROBE			\
	STRUCT_ALIGN();				\
	__barebox_deep_probe_start = .;		\
	KEEP(*(SORT_BY_NAME(.barebox_deep_probe*)))	\
	__barebox_deep_probe_end = .;

#ifdef CONFIG_FUZZ
#define BAREBOX_FUZZ_TESTS			\
	STRUCT_ALIGN();				\
	__barebox_fuzz_tests_start = .;		\
	KEEP(*(SORT_BY_NAME(.barebox_fuzz_test*)))	\
	__barebox_fuzz_tests_end = .;
#else
#define BAREBOX_FUZZ_TESTS
#endif


#ifdef CONFIG_CONSTRUCTORS
#define KERNEL_CTORS()  . = ALIGN(8);                      \
			__ctors_start = .;                 \
			KEEP(*(.ctors))                    \
			KEEP(*(SORT(.init_array.*)))       \
			KEEP(*(.init_array))               \
			__ctors_end = .;
#else
#define KERNEL_CTORS()
#endif

#define RO_DATA_SECTION				\
	BAREBOX_INITCALLS			\
	BAREBOX_EXITCALLS			\
	BAREBOX_CMDS				\
	BAREBOX_RATP_CMDS			\
	BAREBOX_SYMS				\
	KERNEL_CTORS()				\
	BAREBOX_MAGICVARS			\
	BAREBOX_CLK_TABLE			\
	BAREBOX_DTB				\
	BAREBOX_PUBLIC_KEYS			\
	BAREBOX_PCI_FIXUP			\
	BAREBOX_DEEP_PROBE			\
	BAREBOX_FUZZ_TESTS

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

#if defined(__GNUC__) && __GNUC__ >= 11
#define READONLY READONLY
#else
#define READONLY
#endif

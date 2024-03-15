/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  Portable Executable binary format structures
 *
 *  Copyright (c) 2016 Alexander Graf
 *
 *  Based on wine code, adjusted to use <linux/pe.h> defines
 */

#ifndef _PE_H
#define _PE_H

#include <linux/pe.h>

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_NT_HEADERS64 {
	struct pe_hdr FileHeader;
	struct pe32plus_opt_hdr OptionalHeader;
	struct data_dirent DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS {
	struct pe_hdr FileHeader;
	struct pe32_opt_hdr OptionalHeader;
	struct data_dirent DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_NT_HEADERS32;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
	uint8_t	Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		uint32_t PhysicalAddress;
		uint32_t VirtualSize;
	} Misc;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

/* Indices for Optional Header Data Directories */
#define IMAGE_DIRECTORY_ENTRY_SECURITY		4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC         5

typedef struct _IMAGE_BASE_RELOCATION
{
        uint32_t VirtualAddress;
        uint32_t SizeOfBlock;
        /* WORD TypeOffset[1]; */
} IMAGE_BASE_RELOCATION;

#define IMAGE_SIZEOF_RELOCATION 10

/* generic relocation types */
#define IMAGE_REL_BASED_ABSOLUTE                0
#define IMAGE_REL_BASED_HIGH                    1
#define IMAGE_REL_BASED_LOW                     2
#define IMAGE_REL_BASED_HIGHLOW                 3
#define IMAGE_REL_BASED_HIGHADJ                 4
#define IMAGE_REL_BASED_MIPS_JMPADDR            5
#define IMAGE_REL_BASED_ARM_MOV32A              5 /* yes, 5 too */
#define IMAGE_REL_BASED_ARM_MOV32               5 /* yes, 5 too */
#define IMAGE_REL_BASED_RISCV_HI20		5 /* yes, 5 too */
#define IMAGE_REL_BASED_SECTION                 6
#define IMAGE_REL_BASED_REL                     7
#define IMAGE_REL_BASED_ARM_MOV32T              7 /* yes, 7 too */
#define IMAGE_REL_BASED_THUMB_MOV32             7 /* yes, 7 too */
#define IMAGE_REL_BASED_RISCV_LOW12I		7 /* yes, 7 too */
#define IMAGE_REL_BASED_RISCV_LOW12S		8
#define IMAGE_REL_BASED_MIPS_JMPADDR16          9
#define IMAGE_REL_BASED_IA64_IMM64              9 /* yes, 9 too */
#define IMAGE_REL_BASED_DIR64                   10
#define IMAGE_REL_BASED_HIGH3ADJ                11

struct pe_image {
	u64 entry;
	struct resource *code;
	void *bin;

	u16 image_type;
	IMAGE_SECTION_HEADER *sections;
	IMAGE_NT_HEADERS32 *nt;
	int num_sections;
};

#ifdef CONFIG_PE
struct pe_image *pe_open(const char *filename);
unsigned long pe_get_mem_size(struct pe_image *pe);
struct pe_image *pe_open_buf(void *bin, size_t pe_size);
int pe_load(struct pe_image *pe);
void pe_close(struct pe_image *pe);
#else
static inline struct pe_image *pe_open(const char *filename)
{
	return ERR_PTR(-ENOSYS);
}

static inline unsigned long pe_get_mem_size(struct pe_image *pe)
{
	return 0;
}

static inline struct pe_image *pe_open_buf(void *bin, size_t pe_size)
{
	return ERR_PTR(-ENOSYS);
}

static inline int pe_load(struct pe_image *pe)
{
	return -ENOSYS;
}

static inline void pe_close(struct pe_image *pe)
{
}
#endif

#endif /* _PE_H */

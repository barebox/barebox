// SPDX-License-Identifier: GPL-2.0+
/*
 *  PE image loader
 *
 *  based partly on wine code
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define pr_fmt(fmt) "pe: " fmt

#include <common.h>
#include <pe.h>
#include <linux/sizes.h>
#include <libfile.h>
#include <memory.h>
#include <linux/err.h>

static int machines[] = {
#if defined(__aarch64__)
	IMAGE_FILE_MACHINE_ARM64,
#elif defined(__arm__)
	IMAGE_FILE_MACHINE_ARM,
	IMAGE_FILE_MACHINE_THUMB,
	IMAGE_FILE_MACHINE_ARMV7,
#endif

#if defined(__x86_64__)
	IMAGE_FILE_MACHINE_AMD64,
#elif defined(__i386__)
	IMAGE_FILE_MACHINE_I386,
#endif

#if defined(__riscv) && (__riscv_xlen == 32)
	IMAGE_FILE_MACHINE_RISCV32,
#endif

#if defined(__riscv) && (__riscv_xlen == 64)
	IMAGE_FILE_MACHINE_RISCV64,
#endif
	0 };

/**
 * pe_loader_relocate() - relocate PE binary
 *
 * @rel:		pointer to the relocation table
 * @rel_size:		size of the relocation table in bytes
 * @pe_reloc:		actual load address of the image
 * @pref_address:	preferred load address of the image
 * Return:		status code
 */
static int pe_loader_relocate(const IMAGE_BASE_RELOCATION *rel,
			unsigned long rel_size, void *pe_reloc,
			unsigned long pref_address)
{
	unsigned long delta = (unsigned long)pe_reloc - pref_address;
	const IMAGE_BASE_RELOCATION *end;
	int i;

	if (delta == 0)
		return 0;

	end = (const IMAGE_BASE_RELOCATION *)((const char *)rel + rel_size);
	while (rel + 1 < end && rel->SizeOfBlock) {
		const uint16_t *relocs = (const uint16_t *)(rel + 1);
		i = (rel->SizeOfBlock - sizeof(*rel)) / sizeof(uint16_t);
		while (i--) {
			uint32_t offset = (uint32_t)(*relocs & 0xfff) +
					  rel->VirtualAddress;
			int type = *relocs >> PAGE_SHIFT;
			uint64_t *x64 = pe_reloc + offset;
			uint32_t *x32 = pe_reloc + offset;
			uint16_t *x16 = pe_reloc + offset;

			switch (type) {
			case IMAGE_REL_BASED_ABSOLUTE:
				break;
			case IMAGE_REL_BASED_HIGH:
				*x16 += ((uint32_t)delta) >> 16;
				break;
			case IMAGE_REL_BASED_LOW:
				*x16 += (uint16_t)delta;
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				*x32 += (uint32_t)delta;
				break;
			case IMAGE_REL_BASED_DIR64:
				*x64 += (uint64_t)delta;
				break;
#ifdef __riscv
			case IMAGE_REL_BASED_RISCV_HI20:
				*x32 = ((*x32 & 0xfffff000) + (uint32_t)delta) |
					(*x32 & 0x00000fff);
				break;
			case IMAGE_REL_BASED_RISCV_LOW12I:
			case IMAGE_REL_BASED_RISCV_LOW12S:
				/* We know that we're 4k aligned */
				if (delta & 0xfff) {
					pr_err("Unsupported reloc offset\n");
					return -ENOEXEC;
				}
				break;
#endif
			default:
				pr_err("Unknown Relocation off %x type %x\n",
					offset, type);
				return -ENOEXEC;
			}
			relocs++;
		}
		rel = (const IMAGE_BASE_RELOCATION *)relocs;
	}
	return 0;
}

/**
 * pe_check_header() - check if a memory buffer contains a PE-COFF image
 *
 * @buffer:	buffer to check
 * @size:	size of buffer
 * @nt_header:	on return pointer to NT header of PE-COFF image
 * Return:	0 if the buffer contains a PE-COFF image
 */
static int pe_check_header(void *buffer, size_t size, void **nt_header)
{
	struct mz_hdr *dos = buffer;
	IMAGE_NT_HEADERS32 *nt;

	if (size < sizeof(*dos))
		return -EINVAL;

	/* Check for DOS magix */
	if (dos->magic != MZ_MAGIC)
		return -EINVAL;

	/*
	 * Check if the image section header fits into the file. Knowing that at
	 * least one section header follows we only need to check for the length
	 * of the 64bit header which is longer than the 32bit header.
	 */
	if (size < dos->peaddr + sizeof(IMAGE_NT_HEADERS32))
		return -EINVAL;
	nt = (IMAGE_NT_HEADERS32 *)((u8 *)buffer + dos->peaddr);

	/* Check for PE-COFF magic */
	if (nt->FileHeader.magic != PE_MAGIC)
		return -EINVAL;

	if (nt_header)
		*nt_header = nt;

	return 0;
}

/**
 * section_size() - determine size of section
 *
 * The size of a section in memory if normally given by VirtualSize.
 * If VirtualSize is not provided, use SizeOfRawData.
 *
 * @sec:	section header
 * Return:	size of section in memory
 */
static u32 section_size(IMAGE_SECTION_HEADER *sec)
{
	if (sec->Misc.VirtualSize)
		return sec->Misc.VirtualSize;
	else
		return sec->SizeOfRawData;
}

struct pe_image *pe_open_buf(void *bin, size_t pe_size)
{
	struct pe_image *pe;
	int i;
	int supported = 0;
	int ret;

	pe = calloc(1, sizeof(*pe));
	if (!pe)
		return ERR_PTR(-ENOMEM);

	ret = pe_check_header(bin, pe_size, (void **)&pe->nt);
	if (ret) {
		pr_err("Not a PE-COFF file\n");
		ret = -ENOEXEC;
		goto err;
	}

	for (i = 0; machines[i]; i++)
		if (machines[i] == pe->nt->FileHeader.machine) {
			supported = 1;
			break;
		}

	if (!supported) {
		pr_err("Machine type 0x%04x is not supported\n",
			pe->nt->FileHeader.machine);
		ret = -ENOEXEC;
		goto err;
	}

	pe->num_sections = pe->nt->FileHeader.sections;
	pe->sections = (void *)&pe->nt->OptionalHeader +
			    pe->nt->FileHeader.opt_hdr_size;

	if (pe_size < ((void *)pe->sections + sizeof(pe->sections[0]) * pe->num_sections - bin)) {
		pr_err("Invalid number of sections: %d\n", pe->num_sections);
		ret = -ENOEXEC;
		goto err;
	}

	pe->bin = bin;

	return pe;
err:
	return ERR_PTR(ret);
}

struct pe_image *pe_open(const char *filename)
{
	struct pe_image *pe;
	size_t size;
	void *bin;

	bin = read_file(filename, &size);
	if (!bin)
		return ERR_PTR(-errno);

	pe = pe_open_buf(bin, size);
	if (IS_ERR(pe))
		free(bin);

	return pe;
}

static struct resource *pe_alloc(size_t virt_size)
{
	resource_size_t start, end;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return NULL;

	start = ALIGN(start, SZ_64K);

	if (start + virt_size > end)
		return NULL;

	return request_sdram_region("pe-code", start, virt_size,
				    MEMTYPE_LOADER_CODE,
				    MEMATTRS_RWX);
}

unsigned long pe_get_mem_size(struct pe_image *pe)
{
	unsigned long virt_size = 0;
	int i;

	/* Calculate upper virtual address boundary */
	for (i = pe->num_sections - 1; i >= 0; i--) {
		IMAGE_SECTION_HEADER *sec = &pe->sections[i];

		virt_size = max_t(unsigned long, virt_size,
				  sec->VirtualAddress + section_size(sec));
	}

	/* Read 32/64bit specific header bits */
	if (pe->nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32PLUS) {
		IMAGE_NT_HEADERS64 *nt64 = (void *)pe->nt;
		struct pe32plus_opt_hdr  *opt = &nt64->OptionalHeader;

		virt_size = ALIGN(virt_size, opt->section_align);
	} else if (pe->nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32) {
		struct pe32_opt_hdr *opt = &pe->nt->OptionalHeader;

		virt_size = ALIGN(virt_size, opt->section_align);
	} else {
		pr_err("Invalid optional header magic %x\n",
		       pe->nt->OptionalHeader.magic);
		return 0;
	}

	return virt_size;
}

int pe_load(struct pe_image *pe)
{
	int rel_idx = IMAGE_DIRECTORY_ENTRY_BASERELOC;
	uint64_t image_base;
	unsigned long virt_size;
	unsigned long rel_size;
	const IMAGE_BASE_RELOCATION *rel;
	struct mz_hdr *dos;
	struct resource *code;
	void *pe_reloc;
	int i;

	virt_size = pe_get_mem_size(pe);
	if (!virt_size)
		return -ENOEXEC;

	/* Read 32/64bit specific header bits */
	if (pe->nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32PLUS) {
		IMAGE_NT_HEADERS64 *nt64 = (void *)pe->nt;
		struct pe32plus_opt_hdr *opt = &nt64->OptionalHeader;
		image_base = opt->image_base;
		pe->image_type = opt->subsys;

		code = pe_alloc(virt_size);
		if (!code)
			return -ENOMEM;

		pe_reloc = (void *)code->start;

		pe->entry = code->start + opt->entry_point;
		rel_size = nt64->DataDirectory[rel_idx].size;
		rel = pe_reloc + nt64->DataDirectory[rel_idx].virtual_address;
	} else if (pe->nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32) {
		struct pe32_opt_hdr *opt = &pe->nt->OptionalHeader;
		image_base = opt->image_base;
		pe->image_type = opt->subsys;
		virt_size = ALIGN(virt_size, opt->section_align);

		code = pe_alloc(virt_size);
		if (!code)
			return -ENOMEM;

		pe_reloc = (void *)code->start;

		pe->entry = code->start + opt->entry_point;
		rel_size = pe->nt->DataDirectory[rel_idx].size;
		rel = pe_reloc + pe->nt->DataDirectory[rel_idx].virtual_address;
	} else {
		pr_err("Invalid optional header magic %x\n",
		       pe->nt->OptionalHeader.magic);
		return -ENOEXEC;
	}

	/* Copy PE headers */
	memcpy(pe_reloc, pe->bin,
	       sizeof(*dos)
		 + sizeof(*pe->nt)
		 + pe->nt->FileHeader.opt_hdr_size
		 + pe->num_sections * sizeof(IMAGE_SECTION_HEADER));

	/* Load sections into RAM */
	for (i = pe->num_sections - 1; i >= 0; i--) {
		IMAGE_SECTION_HEADER *sec = &pe->sections[i];
		u32 copy_size = section_size(sec);

		if (copy_size > sec->SizeOfRawData) {
			copy_size = sec->SizeOfRawData;
			memset(pe_reloc + sec->VirtualAddress, 0,
			       sec->Misc.VirtualSize);
		}

		memcpy(pe_reloc + sec->VirtualAddress,
		       pe->bin + sec->PointerToRawData,
		       copy_size);
	}

	/* Run through relocations */
	if (pe_loader_relocate(rel, rel_size, pe_reloc,
				(unsigned long)image_base) != 0) {
		release_sdram_region(code);
		return -EINVAL;
	}

	pe->code = code;

	return 0;
}

void pe_close(struct pe_image *pe)
{
	if (pe->code)
		release_sdram_region(pe->code);
	free(pe->bin);
	free(pe);
}

// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/463e4e6476299b32452a8a9e57374241cca26292/lib/efi_loader/efi_image_loader.c
/*
 *  EFI image loader
 *
 *  based partly on wine code
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define pr_fmt(fmt) "efi-loader: pe: " fmt

#include <stdio.h>
#include <memory.h>
#include <linux/align.h>
#include <linux/sizes.h>
#include <efi/services.h>
#include <efi/memory.h>
#include <efi/loader.h>
#include <efi/loader/pe.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <pe.h>
#include <qsort.h>
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
 * efi_print_image_info() - print information about a loaded image
 *
 * If the program counter is located within the image the offset to the base
 * address is shown.
 *
 * @obj:	EFI object
 * @image:	loaded image
 * @pc:		program counter (use NULL to suppress offset output)
 * Return:	status code
 */
static efi_status_t efi_print_image_info(struct efi_loaded_image_obj *obj,
					 struct efi_loaded_image *image,
					 void *pc)
{
	printf("UEFI image");
	printf(" [0x%p:0x%p]",
	       image->image_base, image->image_base + image->image_size - 1);
	if (pc && pc >= image->image_base &&
	    pc < image->image_base + image->image_size)
		printf(" pc=0x%zx", pc - image->image_base);
	if (image->file_path)
		printf(" '%pD'", image->file_path);
	printf("\n");
	return EFI_SUCCESS;
}

/**
 * efi_print_image_infos() - print information about all loaded images
 *
 * @pc:		program counter (use NULL to suppress offset output)
 */
void efi_print_image_infos(void *pc)
{
	struct efi_object *efiobj;
	struct efi_handler *handler;

	list_for_each_entry(efiobj, &efi_obj_list, link) {
		list_for_each_entry(handler, &efiobj->protocols, link) {
			if (!efi_guidcmp(handler->guid, efi_loaded_image_protocol_guid)) {
				efi_print_image_info(
					(struct efi_loaded_image_obj *)efiobj,
					handler->protocol_interface, pc);
			}
		}
	}
}

/**
 * efi_loader_relocate() - relocate UEFI binary
 *
 * @rel:		pointer to the relocation table
 * @rel_size:		size of the relocation table in bytes
 * @efi_reloc:		actual load address of the image
 * @pref_address:	preferred load address of the image
 * Return:		status code
 */
static efi_status_t efi_loader_relocate(const IMAGE_BASE_RELOCATION *rel,
			unsigned long rel_size, void *efi_reloc,
			unsigned long pref_address)
{
	unsigned long delta = (unsigned long)efi_reloc - pref_address;
	const IMAGE_BASE_RELOCATION *end;
	int i;

	if (delta == 0)
		return EFI_SUCCESS;

	end = (const IMAGE_BASE_RELOCATION *)((const char *)rel + rel_size);
	while (rel + 1 < end && rel->SizeOfBlock) {
		const uint16_t *relocs = (const uint16_t *)(rel + 1);
		i = (rel->SizeOfBlock - sizeof(*rel)) / sizeof(uint16_t);
		while (i--) {
			uint32_t offset = (uint32_t)(*relocs & 0xfff) +
					  rel->VirtualAddress;
			int type = *relocs >> EFI_PAGE_SHIFT;
			uint64_t *x64 = efi_reloc + offset;
			uint32_t *x32 = efi_reloc + offset;
			uint16_t *x16 = efi_reloc + offset;

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
					return EFI_LOAD_ERROR;
				}
				break;
#endif
			default:
				pr_err("Unknown Relocation off %x type %x\n",
					offset, type);
				return EFI_LOAD_ERROR;
			}
			relocs++;
		}
		rel = (const IMAGE_BASE_RELOCATION *)relocs;
	}
	return EFI_SUCCESS;
}

/**
 * efi_set_code_and_data_type() - determine the memory types to be used for code
 *				  and data.
 *
 * @loaded_image_info:	image descriptor
 * @image_type:		field Subsystem of the optional header for
 *			Windows specific field
 */
static void efi_set_code_and_data_type(
			struct efi_loaded_image *loaded_image_info,
			uint16_t image_type)
{
	switch (image_type) {
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		loaded_image_info->image_code_type = EFI_LOADER_CODE;
		loaded_image_info->image_data_type = EFI_LOADER_DATA;
		break;
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		loaded_image_info->image_code_type = EFI_BOOT_SERVICES_CODE;
		loaded_image_info->image_data_type = EFI_BOOT_SERVICES_DATA;
		break;
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
	case IMAGE_SUBSYSTEM_EFI_ROM_IMAGE:
		loaded_image_info->image_code_type = EFI_RUNTIME_SERVICES_CODE;
		loaded_image_info->image_data_type = EFI_RUNTIME_SERVICES_DATA;
		break;
	default:
		pr_err("invalid image type: %u\n", image_type);
		/* Let's assume it is an application */
		loaded_image_info->image_code_type = EFI_LOADER_CODE;
		loaded_image_info->image_data_type = EFI_LOADER_DATA;
		break;
	}
}

/**
 * efi_image_region_add() - add an entry of region
 * @regs:	Pointer to array of regions
 * @start:	Start address of region (included)
 * @end:	End address of region (excluded)
 * @nocheck:	flag against overlapped regions
 *
 * Take one entry of region \[@start, @end\[ and insert it into the list.
 *
 * * If @nocheck is false, the list will be sorted ascending by address.
 *   Overlapping entries will not be allowed.
 *
 * * If @nocheck is true, the list will be sorted ascending by sequence
 *   of adding the entries. Overlapping is allowed.
 *
 * Return:	status code
 */
efi_status_t efi_image_region_add(struct efi_image_regions *regs,
				  const void *start, const void *end,
				  int nocheck)
{
	struct image_region *reg;
	int i, j;

	if (regs->num >= regs->max) {
		pr_err("%s: no more room for regions\n", __func__);
		return EFI_OUT_OF_RESOURCES;
	}

	if (end < start)
		return EFI_INVALID_PARAMETER;

	for (i = 0; i < regs->num; i++) {
		reg = &regs->reg[i];
		if (nocheck)
			continue;

		/* new data after registered region */
		if (start >= reg->data + reg->size)
			continue;

		/* new data preceding registered region */
		if (end <= reg->data) {
			for (j = regs->num - 1; j >= i; j--)
				memcpy(&regs->reg[j + 1], &regs->reg[j],
				       sizeof(*reg));
			break;
		}

		/* new data overlapping registered region */
		pr_err("%s: new region already part of another\n", __func__);
		return EFI_INVALID_PARAMETER;
	}

	reg = &regs->reg[i];
	reg->data = start;
	reg->size = end - start;
	regs->num++;

	return EFI_SUCCESS;
}

/**
 * cmp_pe_section() - compare virtual addresses of two PE image sections
 * @arg1:	pointer to pointer to first section header
 * @arg2:	pointer to pointer to second section header
 *
 * Compare the virtual addresses of two sections of an portable executable.
 * The arguments are defined as const void * to allow usage with qsort().
 *
 * Return:	-1 if the virtual address of arg1 is less than that of arg2,
 *		0 if the virtual addresses are equal, 1 if the virtual address
 *		of arg1 is greater than that of arg2.
 */
static int cmp_pe_section(const void *arg1, const void *arg2)
{
	const IMAGE_SECTION_HEADER *section1, *section2;

	section1 = *((const IMAGE_SECTION_HEADER **)arg1);
	section2 = *((const IMAGE_SECTION_HEADER **)arg2);

	if (section1->VirtualAddress < section2->VirtualAddress)
		return -1;
	else if (section1->VirtualAddress == section2->VirtualAddress)
		return 0;
	else
		return 1;
}

/**
 * efi_prepare_aligned_image() - prepare 8-byte aligned image
 * @efi:		pointer to the EFI binary
 * @efi_size:		size of @efi binary
 *
 * If @efi is not 8-byte aligned, this function newly allocates
 * the image buffer.
 *
 * Return:	valid pointer to a image, return NULL if allocation fails.
 */
void *efi_prepare_aligned_image(void *efi, u64 *efi_size)
{
	size_t new_efi_size;
	void *new_efi;

	/*
	 * Size must be 8-byte aligned and the trailing bytes must be
	 * zero'ed. Otherwise hash value may be incorrect.
	 */
	if (!IS_ALIGNED(*efi_size, 8)) {
		new_efi_size = ALIGN(*efi_size, 8);
		new_efi = calloc(new_efi_size, 1);
		if (!new_efi)
			return NULL;
		memcpy(new_efi, efi, *efi_size);
		*efi_size = new_efi_size;
		return new_efi;
	} else {
		return efi;
	}
}

/**
 * efi_image_parse() - parse a PE image
 * @efi:	Pointer to image
 * @len:	Size of @efi
 * @regp:	Pointer to a list of regions
 * @auth:	Pointer to a pointer to authentication data in PE
 * @auth_len:	Size of @auth
 *
 * Parse image binary in PE32(+) format, assuming that sanity of PE image
 * has been checked by a caller.
 * On success, an address of authentication data in @efi and its size will
 * be returned in @auth and @auth_len, respectively.
 *
 * Return:	true on success, false on error
 */
bool efi_image_parse(void *efi, size_t len, struct efi_image_regions **regp,
		     WIN_CERTIFICATE **auth, size_t *auth_len)
{
	struct efi_image_regions *regs;
	struct mz_hdr *dos;
	IMAGE_NT_HEADERS32 *nt;
	IMAGE_SECTION_HEADER *sections, **sorted;
	int num_regions, num_sections, i;
	int ctidx = IMAGE_DIRECTORY_ENTRY_SECURITY;
	u32 align, size, authsz, authoff;
	size_t bytes_hashed;

	dos = (void *)efi;
	nt = (void *)(efi + dos->peaddr);
	authoff = 0;
	authsz = 0;

	/*
	 * Count maximum number of regions to be digested.
	 * We don't have to have an exact number here.
	 * See efi_image_region_add()'s in parsing below.
	 */
	num_regions = 3; /* for header */
	num_regions += nt->FileHeader.sections;
	num_regions++; /* for extra */

	regs = calloc(sizeof(*regs) + sizeof(struct image_region) * num_regions,
		      1);
	if (!regs)
		goto err;
	regs->max = num_regions;

	/*
	 * Collect data regions for hash calculation
	 * 1. File headers
	 */
	if (nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32PLUS) {
		IMAGE_NT_HEADERS64  *nt64 = (void *)nt;
		struct pe32plus_opt_hdr *opt = &nt64->OptionalHeader;

		/* Skip CheckSum */
		efi_image_region_add(regs, efi, &opt->csum, 0);
		if (nt64->OptionalHeader.data_dirs <= ctidx) {
			efi_image_region_add(regs,
					     &opt->subsys,
					     efi + opt->header_size, 0);
		} else {
			/* Skip Certificates Table */
			efi_image_region_add(regs,
					     &opt->subsys,
					     &nt64->DataDirectory[ctidx], 0);
			efi_image_region_add(regs,
					     &nt64->DataDirectory[ctidx] + 1,
					     efi + opt->header_size, 0);

			authoff = nt64->DataDirectory[ctidx].virtual_address;
			authsz = nt64->DataDirectory[ctidx].size;
		}

		bytes_hashed = opt->header_size;
		align = opt->file_align;
	} else if (nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32) {
		struct pe32_opt_hdr *opt = &nt->OptionalHeader;

		/* Skip CheckSum */
		efi_image_region_add(regs, efi, &opt->csum, 0);
		if (nt->OptionalHeader.data_dirs <= ctidx) {
			efi_image_region_add(regs,
					     &opt->subsys,
					     efi + opt->header_size, 0);
		} else {
			/* Skip Certificates Table */
			efi_image_region_add(regs, &opt->subsys,
					     &nt->DataDirectory[ctidx], 0);
			efi_image_region_add(regs,
					     &nt->DataDirectory[ctidx] + 1,
					     efi + opt->header_size, 0);

			authoff = nt->DataDirectory[ctidx].virtual_address;
			authsz = nt->DataDirectory[ctidx].size;
		}

		bytes_hashed = opt->header_size;
		align = opt->file_align;
	} else {
		pr_err("%s: Invalid optional header magic %x\n", __func__,
			nt->OptionalHeader.magic);
		goto err;
	}

	/* 2. Sections */
	num_sections = nt->FileHeader.sections;
	sections = (void *)((uint8_t *)&nt->OptionalHeader +
			    nt->FileHeader.opt_hdr_size);
	sorted = calloc(sizeof(IMAGE_SECTION_HEADER *), num_sections);
	if (!sorted) {
		pr_err("%s: Out of memory\n", __func__);
		goto err;
	}

	/*
	 * Make sure the section list is in ascending order.
	 */
	for (i = 0; i < num_sections; i++)
		sorted[i] = &sections[i];
	qsort(sorted, num_sections, sizeof(sorted[0]), cmp_pe_section);

	for (i = 0; i < num_sections; i++) {
		if (!sorted[i]->SizeOfRawData)
			continue;

		size = (sorted[i]->SizeOfRawData + align - 1) & ~(align - 1);
		efi_image_region_add(regs, efi + sorted[i]->PointerToRawData,
				     efi + sorted[i]->PointerToRawData + size,
				     0);
		pr_debug("section[%d](%s): raw: 0x%x-0x%x, virt: %x-%x\n",
			  i, sorted[i]->Name,
			  sorted[i]->PointerToRawData,
			  sorted[i]->PointerToRawData + size,
			  sorted[i]->VirtualAddress,
			  sorted[i]->VirtualAddress
			    + sorted[i]->Misc.VirtualSize);

		bytes_hashed += size;
	}
	free(sorted);

	/* 3. Extra data excluding Certificates Table */
	if (bytes_hashed + authsz < len) {
		pr_debug("extra data for hash: %zu\n",
			  len - (bytes_hashed + authsz));
		efi_image_region_add(regs, efi + bytes_hashed,
				     efi + len - authsz, 0);
	}

	/* Return Certificates Table */
	if (authsz) {
		if (len < authoff + authsz) {
			pr_err("%s: Size for auth too large: %u >= %zu\n",
				__func__, authsz, len - authoff);
			goto err;
		}
		if (authsz < sizeof(*auth)) {
			pr_err("%s: Size for auth too small: %u < %zu\n",
				__func__, authsz, sizeof(*auth));
			goto err;
		}
		*auth = efi + authoff;
		*auth_len = authsz;
		pr_debug("WIN_CERTIFICATE: 0x%x, size: 0x%x\n", authoff,
			  authsz);
	} else {
		*auth = NULL;
		*auth_len = 0;
	}

	*regp = regs;

	return true;

err:
	free(regs);

	return false;
}

static bool efi_image_authenticate(void *efi, size_t efi_size)
{
	return true;
}

/**
 * efi_check_pe() - check if a memory buffer contains a PE-COFF image
 *
 * @buffer:	buffer to check
 * @size:	size of buffer
 * @nt_header:	on return pointer to NT header of PE-COFF image
 * Return:	EFI_SUCCESS if the buffer contains a PE-COFF image
 */
efi_status_t efi_check_pe(void *buffer, size_t size, void **nt_header)
{
	struct mz_hdr *dos = buffer;
	IMAGE_NT_HEADERS32 *nt;

	if (size < sizeof(*dos))
		return EFI_INVALID_PARAMETER;

	/* Check for DOS magix */
	if (dos->magic != MZ_MAGIC)
		return EFI_INVALID_PARAMETER;

	/*
	 * Check if the image section header fits into the file. Knowing that at
	 * least one section header follows we only need to check for the length
	 * of the 64bit header which is longer than the 32bit header.
	 */
	if (size < dos->peaddr + sizeof(IMAGE_NT_HEADERS32))
		return EFI_INVALID_PARAMETER;
	nt = (IMAGE_NT_HEADERS32 *)((u8 *)buffer + dos->peaddr);

	/* Check for PE-COFF magic */
	if (nt->FileHeader.magic != PE_MAGIC)
		return EFI_INVALID_PARAMETER;

	if (nt_header)
		*nt_header = nt;

	return EFI_SUCCESS;
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

/**
 * efi_load_pe() - relocate EFI binary
 *
 * This function loads all sections from a PE binary into a newly reserved
 * piece of memory. On success the entry point is returned as handle->entry.
 *
 * @handle:		loaded image handle
 * @efi:		pointer to the EFI binary
 * @efi_size:		size of @efi binary
 * @loaded_image_info:	loaded image protocol
 * Return:		status code
 */
efi_status_t efi_load_pe(struct efi_loaded_image_obj *handle,
			 void *efi, size_t efi_size,
			 struct efi_loaded_image *loaded_image_info)
{
	IMAGE_NT_HEADERS32 *nt;
	struct mz_hdr *dos;
	IMAGE_SECTION_HEADER *sections;
	int num_sections;
	void *efi_reloc;
	int i;
	const IMAGE_BASE_RELOCATION *rel;
	unsigned long rel_size;
	int rel_idx = IMAGE_DIRECTORY_ENTRY_BASERELOC;
	uint64_t image_base;
	unsigned long virt_size = 0;
	int supported = 0;
	efi_status_t ret;

	ret = efi_check_pe(efi, efi_size, (void **)&nt);
	if (ret != EFI_SUCCESS) {
		pr_err("Not a PE-COFF file\n");
		return EFI_LOAD_ERROR;
	}

	for (i = 0; machines[i]; i++)
		if (machines[i] == nt->FileHeader.machine) {
			supported = 1;
			break;
		}

	if (!supported) {
		pr_err("Machine type 0x%04x is not supported\n",
			nt->FileHeader.machine);
		return EFI_LOAD_ERROR;
	}

	num_sections = nt->FileHeader.sections;
	sections = (void *)&nt->OptionalHeader +
			    nt->FileHeader.opt_hdr_size;

	if (efi_size < ((void *)sections + sizeof(sections[0]) * num_sections
			- efi)) {
		pr_err("Invalid number of sections: %d\n", num_sections);
		return EFI_LOAD_ERROR;
	}

	/* Authenticate an image */
	if (efi_image_authenticate(efi, efi_size)) {
		handle->auth_status = EFI_IMAGE_AUTH_PASSED;
	} else {
		handle->auth_status = EFI_IMAGE_AUTH_FAILED;
		pr_err("Image not authenticated\n");
	}

	/* Calculate upper virtual address boundary */
	for (i = num_sections - 1; i >= 0; i--) {
		IMAGE_SECTION_HEADER *sec = &sections[i];

		virt_size = max_t(unsigned long, virt_size,
				  sec->VirtualAddress + section_size(sec));
	}

	/* Read 32/64bit specific header bits */
	if (nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32PLUS) {
		IMAGE_NT_HEADERS64 *nt64 = (void *)nt;
		struct pe32plus_opt_hdr *opt = &nt64->OptionalHeader;
		image_base = opt->image_base;
		efi_set_code_and_data_type(loaded_image_info, opt->subsys);
		handle->image_type = opt->subsys;
		efi_reloc = efi_alloc_aligned_pages(virt_size,
						   loaded_image_info->image_code_type,
						   opt->section_align, "pe64");
		if (!efi_reloc) {
			pr_err("Out of memory\n");
			ret = EFI_OUT_OF_RESOURCES;
			goto err;
		}
		handle->entry = efi_reloc + opt->entry_point;
		rel_size = nt64->DataDirectory[rel_idx].size;
		rel = efi_reloc + nt64->DataDirectory[rel_idx].virtual_address;
	} else if (nt->OptionalHeader.magic == PE_OPT_MAGIC_PE32) {
		struct pe32_opt_hdr *opt = &nt->OptionalHeader;
		image_base = opt->image_base;
		efi_set_code_and_data_type(loaded_image_info, opt->subsys);
		handle->image_type = opt->subsys;
		efi_reloc = efi_alloc_aligned_pages(virt_size,
						    loaded_image_info->image_code_type,
						    opt->section_align, "pe32");
		if (!efi_reloc) {
			pr_err("Out of memory\n");
			ret = EFI_OUT_OF_RESOURCES;
			goto err;
		}
		handle->entry = efi_reloc + opt->entry_point;
		rel_size = nt->DataDirectory[rel_idx].size;
		rel = efi_reloc + nt->DataDirectory[rel_idx].virtual_address;
	} else {
		pr_err("Invalid optional header magic %x\n",
			nt->OptionalHeader.magic);
		ret = EFI_LOAD_ERROR;
		goto err;
	}

	/* Copy PE headers */
	memcpy(efi_reloc, efi,
	       sizeof(*dos)
		 + sizeof(*nt)
		 + nt->FileHeader.opt_hdr_size
		 + num_sections * sizeof(IMAGE_SECTION_HEADER));

	/* Load sections into RAM */
	for (i = num_sections - 1; i >= 0; i--) {
		IMAGE_SECTION_HEADER *sec = &sections[i];
		u32 copy_size = section_size(sec);

		if (copy_size > sec->SizeOfRawData) {
			copy_size = sec->SizeOfRawData;
			memset(efi_reloc + sec->VirtualAddress, 0,
			       sec->Misc.VirtualSize);
		}
		memcpy(efi_reloc + sec->VirtualAddress,
		       efi + sec->PointerToRawData,
		       copy_size);
	}

	/* Run through relocations */
	if (efi_loader_relocate(rel, rel_size, efi_reloc,
				(unsigned long)image_base) != EFI_SUCCESS) {
		efi_free_pages((uintptr_t) efi_reloc,
			       (virt_size + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT);
		ret = EFI_LOAD_ERROR;
		goto err;
	}

	/* Populate the loaded image interface bits */
	loaded_image_info->image_base = efi_reloc;
	loaded_image_info->image_size = virt_size;

	if (handle->auth_status == EFI_IMAGE_AUTH_PASSED)
		return EFI_SUCCESS;
	else
		return EFI_SECURITY_VIOLATION;

err:
	return ret;
}

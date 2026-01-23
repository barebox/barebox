// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 *
 * ELF parts that need no extra allocations for use both in barebox PBL and proper.
 */

#include <lib/elf.h>
#include <linux/elf.h>
#include <errno.h>
#include <linux/string.h>
#include <linux/printk.h>

int elf_check_image(struct elf_image *elf, void *buf)
{
	u16 e_type;

	if (memcmp(buf, ELFMAG, SELFMAG)) {
		pr_err("ELF magic not found.\n");
		return -EINVAL;
	}

	elf->class = ((char *) buf)[EI_CLASS];

	e_type = elf_hdr_e_type(elf, buf);
	if (e_type != ET_EXEC && e_type != ET_DYN) {
		pr_err("Unsupported ELF type: %u (only ET_EXEC and ET_DYN supported)\n", e_type);
		return -ENOEXEC;
	}

	if (elf->class != ELF_CLASS)
		return -EINVAL;

	elf->type = e_type;

	if (!elf_hdr_e_phnum(elf, buf)) {
		pr_err("No phdr found.\n");
		return -ENOEXEC;
	}

	return 0;
}

int elf_open_binary_into(struct elf_image *elf, void *buf)
{
	int ret;

	memset(elf, 0, sizeof(*elf));
	elf_init_struct(elf);

	elf->hdr_buf = buf;
	ret = elf_check_image(elf, buf);
	if (ret)
		return ret;

	elf->entry = elf_hdr_e_entry(elf, elf->hdr_buf);

	return 0;
}

void elf_set_load_address(struct elf_image *elf, void *addr)
{
	elf->load_address = addr;
}

void *elf_find_dynamic_segment(struct elf_image *elf)
{
	void *buf = elf->hdr_buf;
	void *phdr = buf + elf_hdr_e_phoff(elf, buf);

	elf_for_each_segment(phdr, elf, buf) {
		if (elf_phdr_p_type(elf, phdr) == PT_DYNAMIC)
			return elf_phdr_relocated_paddr(elf, phdr);
	}

	return NULL;  /* No PT_DYNAMIC segment */
}

/**
 * elf_parse_dynamic_section - Parse the dynamic section and extract relocation info
 * @elf: ELF image structure
 * @dyn_seg: Pointer to the PT_DYNAMIC segment
 * @rel_out: Output pointer to the relocation table (either REL or RELA)
 * @relsz_out: Output size of the relocation table in bytes
 * @is_rela: flag indicating RELA (true) vs REL (false) format is expected
 *
 * This is a generic function that works for both 32-bit and 64-bit ELF files,
 * and handles both REL and RELA relocation formats.
 *
 * Returns: 0 on success, -EINVAL on error
 */
static int elf_parse_dynamic_section(struct elf_image *elf, const void *dyn_seg,
				     void **rel_out, u64 *relsz_out, void **symtab,
				     bool is_rela)
{
	const void *dyn = dyn_seg;
	void *rel = NULL, *rela = NULL;
	u64 relsz = 0, relasz = 0;
	u64 relent = 0, relaent = 0;
	phys_addr_t base = (phys_addr_t)elf->reloc_offset;
	size_t expected_rel_size, expected_rela_size;

	/* Calculate expected sizes based on ELF class */
	if (ELF_CLASS == ELFCLASS32) {
		expected_rel_size = sizeof(Elf32_Rel);
		expected_rela_size = sizeof(Elf32_Rela);
	} else {
		expected_rel_size = sizeof(Elf64_Rel);
		expected_rela_size = sizeof(Elf64_Rela);
	}

	/* Iterate through dynamic entries until DT_NULL */
	while (elf_dyn_d_tag(elf, dyn) != DT_NULL) {
		unsigned long tag = elf_dyn_d_tag(elf, dyn);

		switch (tag) {
		case DT_REL:
			/* REL table address - needs to be adjusted by load offset */
			rel = (void *)(unsigned long)(base + elf_dyn_d_ptr(elf, dyn));
			break;
		case DT_RELSZ:
			relsz = elf_dyn_d_val(elf, dyn);
			break;
		case DT_RELENT:
			relent = elf_dyn_d_val(elf, dyn);
			break;
		case DT_RELA:
			/* RELA table address - needs to be adjusted by load offset */
			rela = (void *)(unsigned long)(base + elf_dyn_d_ptr(elf, dyn));
			break;
		case DT_RELASZ:
			relasz = elf_dyn_d_val(elf, dyn);
			break;
		case DT_RELAENT:
			relaent = elf_dyn_d_val(elf, dyn);
			break;
		case DT_SYMTAB:
			*symtab = (void *)(unsigned long)(base + elf_dyn_d_val(elf, dyn));
			break;
		default:
			break;
		}

		dyn += elf_size_of_dyn(elf);
	}

	/* Check that we found exactly one relocation type */
	if (rel && rela) {
		pr_err("ELF has both REL and RELA relocations\n");
		return -EINVAL;
	}

	if (rel && !is_rela) {
		/* REL relocations */
		if (!relsz || relent != expected_rel_size) {
			pr_debug("No REL relocations or invalid relocation info\n");
			return -EINVAL;
		}
		*rel_out = rel;
		*relsz_out = relsz;

		return 0;
	} else if (rela && is_rela) {
		/* RELA relocations */
		if (!relasz || relaent != expected_rela_size) {
			pr_debug("No RELA relocations or invalid relocation info\n");
			return -EINVAL;
		}
		*rel_out = rela;
		*relsz_out = relasz;

		return 0;
	}

	pr_debug("No relocations found in dynamic section\n");

	return -EINVAL;
}

int elf_parse_dynamic_section_rel(struct elf_image *elf, const void *dyn_seg,
				     void **rel_out, u64 *relsz_out, void **symtab)
{
	return elf_parse_dynamic_section(elf, dyn_seg, rel_out, relsz_out, symtab,
					 false);
}

int elf_parse_dynamic_section_rela(struct elf_image *elf, const void *dyn_seg,
				     void **rel_out, u64 *relsz_out, void **symtab)
{
	return elf_parse_dynamic_section(elf, dyn_seg, rel_out, relsz_out, symtab,
					 true);
}

/*
 * Weak default implementation for architectures that don't support
 * ELF relocations yet. Can be overridden by arch-specific implementation.
 */
int __weak elf_apply_relocations(struct elf_image *elf, const void *dyn_seg)
{
	pr_warn("ELF relocations not supported for this architecture\n");
	return -ENOSYS;
}

/**
 * elf_load_inplace() - Apply dynamic relocations to an ELF binary in place
 * @elf: ELF image previously opened with elf_open_binary()
 *
 * This function applies dynamic relocations to an ELF binary that is already
 * loaded at its target address in memory. Unlike elf_load(), this does not
 * allocate memory or copy segments - it only modifies the existing image.
 *
 * This is useful for self-relocating loaders or when the ELF has been loaded
 * by external means (e.g., loaded by firmware or another bootloader).
 *
 * The ELF image must have been previously opened with elf_open_binary().
 *
 * For ET_DYN (position-independent) binaries, the relocation offset is
 * calculated relative to the first executable PT_LOAD segment (.text section).
 *
 * For ET_EXEC binaries, no relocation is applied as they are expected to
 * be at their link-time addresses.
 *
 * Returns: 0 on success, negative error code on failure
 */
int elf_load_inplace(struct elf_image *elf)
{
	const void *dyn_seg;
	void *buf, *phdr;
	void *elf_buf;
	int ret;
	u64 text_vaddr = U64_MAX;
	u64 text_vaddr_min = U64_MAX;
	u64 text_offset = U64_MAX;

	buf = elf->hdr_buf;
	elf_buf = elf->hdr_buf;

	/*
	 * First pass: Clear BSS segments (p_memsz > p_filesz) and find lowest
	 * virtual address.
	 * BSS clearing must be done before relocations as uninitialized data
	 * must be zeroed per C standard.
	 */
	phdr = buf + elf_hdr_e_phoff(elf, buf);
	elf_for_each_segment(phdr, elf, buf) {
		if (elf_phdr_p_type(elf, phdr) == PT_LOAD) {
			u64 p_offset = elf_phdr_p_offset(elf, phdr);
			u64 p_filesz = elf_phdr_p_filesz(elf, phdr);
			u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

			/* Clear BSS (uninitialized data) */
			if (p_filesz < p_memsz) {
				void *bss_start = elf_buf + p_offset + p_filesz;
				size_t bss_size = p_memsz - p_filesz;
				memset(bss_start, 0x00, bss_size);
			}

			text_vaddr = elf_phdr_p_vaddr(elf, phdr);

			if (text_vaddr < text_vaddr_min) {
				text_vaddr_min = text_vaddr;
				text_offset = p_offset;
			}
		}
	}

	text_vaddr = text_vaddr_min;

	/*
	 * Calculate relocation offset for the in-place binary.
	 * For ET_DYN, we need to find the first PT_LOAD segment
	 * and use it as the relocation base.
	 */
	if (elf->type == ET_DYN) {

		if (text_vaddr == U64_MAX) {
			pr_err("No PT_LOAD segment found\n");
			ret = -EINVAL;
			goto out;
		}

		/*
		 * Calculate relocation offset relative to .text section:
		 * - .text is at file offset text_offset, so in memory at: elf_buf + text_offset
		 * - .text has virtual address text_vaddr
		 * - reloc_offset = (actual .text address) - (virtual .text address)
		 */
		elf->reloc_offset = ((unsigned long)elf_buf + text_offset) - text_vaddr;

		pr_debug("In-place ELF relocation: text_vaddr=0x%llx, text_offset=0x%llx, "
			 "load_addr=%p, offset=0x%08lx\n",
			 text_vaddr, text_offset, elf_buf, elf->reloc_offset);

		/* Adjust entry point to point to relocated image */
		elf->entry += elf->reloc_offset;
	} else {
		/*
		 * ET_EXEC binaries are at their link-time addresses,
		 * no relocation needed
		 */
		elf->reloc_offset = 0;
	}

	/* Find PT_DYNAMIC segment */
	dyn_seg = elf_find_dynamic_segment(elf);
	if (!dyn_seg) {
		/*
		 * No PT_DYNAMIC segment found.
		 * This is fine for statically-linked binaries or
		 * binaries without relocations.
		 */
		pr_debug("No PT_DYNAMIC segment found\n");
		ret = 0;
		goto out;
	}

	/* Apply architecture-specific relocations */
	ret = elf_apply_relocations(elf, dyn_seg);
	if (ret) {
		pr_err("In-place relocation failed: %d\n", ret);
		goto out;
	}

	pr_debug("In-place ELF relocation completed successfully\n");
	return 0;

out:
	return ret;
}

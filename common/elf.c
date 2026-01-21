// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libfile.h>
#include <memory.h>
#include <unistd.h>
#include <zero_page.h>
#include <linux/fs.h>
#include <linux/list_sort.h>

struct elf_segment {
	struct list_head list;
	struct resource *r;
	void *phdr;
	bool is_iomem_region;
};

static void *elf_phdr_relocated_paddr(struct elf_image *elf, void *phdr)
{
	void *dst;

	if (elf->reloc_offset)
		dst = (void *)(unsigned long)(elf->reloc_offset + elf_phdr_p_vaddr(elf, phdr));
	else
		dst = (void *)(unsigned long)elf_phdr_p_paddr(elf, phdr);

	return dst;
}

static int elf_request_region(struct elf_image *elf, resource_size_t start,
			      resource_size_t size, void *phdr)
{
	struct list_head *list = &elf->list;
	struct resource *r_new;
	struct elf_segment *r;

	r = calloc(1, sizeof(*r));
	if (!r)
		return -ENOMEM;

	r_new = request_sdram_region("elf_segment", start, size,
				     MEMTYPE_LOADER_CODE, MEMATTRS_RWX);
	if (!r_new) {
		r_new = request_iomem_region("elf_segment", start, size);
		if (!r_new) {
			pr_err("Failed to request region: %pa %pa\n", &start, &size);
			return -EINVAL;
		}
		r->is_iomem_region = true;
	}

	r->r = r_new;
	r->phdr = phdr;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct elf_image *elf)
{
	struct list_head *list = &elf->list;
	struct elf_segment *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		if (r->is_iomem_region)
			release_region(r->r);
		else
			release_sdram_region(r->r);
		list_del(&r->list);
		free(r);
	}
}

static int elf_compute_load_offset(struct elf_image *elf)
{
	void *buf = elf->hdr_buf;
	void *phdr = buf + elf_hdr_e_phoff(elf, buf);
	u64 min_vaddr = (u64)-1;
	u64 min_paddr = (u64)-1;

	/* Find lowest p_vaddr and p_paddr in PT_LOAD segments */
	elf_for_each_segment(phdr, elf, buf) {
		if (elf_phdr_p_type(elf, phdr) == PT_LOAD) {
			u64 vaddr = elf_phdr_p_vaddr(elf, phdr);
			u64 paddr = elf_phdr_p_paddr(elf, phdr);

			if (vaddr < min_vaddr)
				min_vaddr = vaddr;
			if (paddr < min_paddr)
				min_paddr = paddr;
		}
	}

	/*
	 * Determine base load address:
	 * 1. If user specified load_address, use it
	 * 2. Otherwise for ET_EXEC, use NULL (segments use p_paddr directly)
	 * 3. For ET_DYN, use lowest p_paddr
	 */
	if (elf->load_address)
		elf->base_load_addr = elf->load_address;
	else if (elf->type == ET_EXEC)
		elf->base_load_addr = NULL;
	else
		elf->base_load_addr = (void *)(phys_addr_t)min_paddr;

	/*
	 * Calculate relocation offset:
	 * - For ET_EXEC with no custom load address: no offset needed
	 * - Otherwise: offset = base_load_addr - lowest_vaddr
	 */
	if (elf->type == ET_EXEC && !elf->load_address)
		elf->reloc_offset = 0;
	else
		elf->reloc_offset = ((unsigned long)elf->base_load_addr - min_vaddr);

	pr_debug("ELF load: type=%s, base=%p, offset=%08lx\n",
		 elf->type == ET_EXEC ? "ET_EXEC" : "ET_DYN",
		 elf->base_load_addr, elf->reloc_offset);

	return 0;
}

static int request_elf_segment(struct elf_image *elf, void *phdr)
{
	void *dst;
	int ret;
	u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

	/* we care only about PT_LOAD segments */
	if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
		return 0;

	if (!p_memsz)
		return 0;

	/*
	 * Calculate destination address:
	 * - If reloc_offset is set (custom load address or ET_DYN):
	 *   dst = reloc_offset + p_vaddr
	 * - Otherwise (ET_EXEC, no custom address):
	 *   dst = p_paddr (original behavior)
	 */
	dst = elf_phdr_relocated_paddr(elf, phdr);

	if (dst < elf->low_addr)
		elf->low_addr = dst;
	if (dst + p_memsz > elf->high_addr)
		elf->high_addr = dst + p_memsz;

	pr_debug("Requesting segment 0x%p (%llu bytes)\n", dst, p_memsz);

	ret = elf_request_region(elf, (resource_size_t)dst, p_memsz, phdr);
	if (ret)
		return ret;

	return 0;
}

static int elf_segment_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct elf_image *elf = priv;
	struct elf_segment *elf_a, *elf_b;

	if (a == b)
		return 0;

	elf_a = list_entry(a, struct elf_segment, list);
	elf_b = list_entry(b, struct elf_segment, list);

	return elf_phdr_p_offset(elf, elf_a->phdr) >
	       elf_phdr_p_offset(elf, elf_b->phdr);
}

static int load_elf_to_memory(struct elf_image *elf)
{
	void *dst;
	int ret = 0, fd = -1;
	u64 p_filesz, p_memsz, p_offset;
	struct elf_segment *r;
	struct list_head *list = &elf->list;

	if (elf->filename) {
		fd = open(elf->filename, O_RDONLY);
		if (fd < 0) {
			pr_err("could not open: %m\n");
			return -errno;
		}
	}

	zero_page_access();

	list_for_each_entry(r, list, list) {
		p_offset = elf_phdr_p_offset(elf, r->phdr);
		p_filesz = elf_phdr_p_filesz(elf, r->phdr);
		p_memsz = elf_phdr_p_memsz(elf, r->phdr);

		dst = elf_phdr_relocated_paddr(elf, r->phdr);

		pr_debug("Loading phdr offset 0x%llx to 0x%p (%llu bytes)\n",
			 p_offset, dst, p_filesz);

		if (fd >= 0) {
			ret = lseek(fd, p_offset, SEEK_SET);
			if (ret == -1) {
				pr_err("lseek at offset 0x%llx failed\n",
				       p_offset);
				goto out;
			}

			if (read_full(fd, dst, p_filesz) < 0) {
				pr_err("could not read elf segment: %m\n");
				ret = -errno;
				goto out;
			}
		} else {
			memcpy(dst, elf->hdr_buf + p_offset, p_filesz);
		}

		if (p_filesz < p_memsz)
			memset(dst + p_filesz, 0x00, p_memsz - p_filesz);
	}

out:
	zero_page_faulting();

	close(fd);

	return ret >= 0 ? 0 : ret;
}

static int load_elf_image_segments(struct elf_image *elf)
{
	void *buf = elf->hdr_buf;
	void *phdr;
	int ret;

	/* File as already been loaded */
	if (!list_empty(&elf->list))
		return -EINVAL;

	/* Calculate load offset for ET_DYN */
	ret = elf_compute_load_offset(elf);
	if (ret)
		return ret;

	elf_for_each_segment(phdr, elf, buf) {
		ret = request_elf_segment(elf, phdr);
		if (ret)
			goto elf_release_regions;
	}

	/*
	 * Sort the list to avoid doing backward lseek while loading the elf
	 * segments from file to memory(some filesystems don't support it)
	 */
	list_sort(elf, &elf->list, elf_segment_cmp);

	ret = load_elf_to_memory(elf);
	if (ret)
		goto elf_release_regions;

	return 0;

elf_release_regions:
	elf_release_regions(elf);

	return ret;
}

static int elf_check_image(struct elf_image *elf, void *buf)
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

static void elf_init_struct(struct elf_image *elf)
{
	INIT_LIST_HEAD(&elf->list);
	elf->low_addr = (void *) (unsigned long) -1;
	elf->high_addr = 0;
	elf->filename = NULL;
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

struct elf_image *elf_open_binary(void *buf)
{
	int ret;
	struct elf_image *elf;

	elf = calloc(1, sizeof(*elf));
	if (!elf)
		return ERR_PTR(-ENOMEM);

	ret = elf_open_binary_into(elf, buf);
	if (ret) {
		free(elf);
		return ERR_PTR(ret);
	}

	return elf;
}

static struct elf_image *elf_check_init(const char *filename)
{
	int ret, fd;
	int hdr_size;
	struct elf64_hdr hdr;
	struct elf_image *elf;

	elf = calloc(1, sizeof(*elf));
	if (!elf)
		return ERR_PTR(-ENOMEM);

	elf_init_struct(elf);

	/* First pass is to read elf header only */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		pr_err("could not open: %m\n");
		ret = -errno;
		goto err_free_elf;
	}

	if (read_full(fd, &hdr, sizeof(hdr)) < 0) {
		pr_err("could not read elf header: %m\n");
		close(fd);
		ret = -errno;
		goto err_free_elf;
	}
	close(fd);

	ret = elf_check_image(elf, &hdr);
	if (ret)
		goto err_free_elf;

	hdr_size = elf_hdr_e_phoff(elf, &hdr) +
		   elf_hdr_e_phnum(elf, &hdr) *
		   elf_hdr_e_phentsize(elf, &hdr);

	/* Second pass is to read the whole elf header and program headers */
	elf->hdr_buf = malloc(hdr_size);
	if (!elf->hdr_buf) {
		ret = -ENOMEM;
		goto err_free_elf;
	}

	/*
	 * We must open the file again since some fs (tftp) do not support
	 * backward lseek operations
	 */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		pr_err("could not open: %m\n");
		ret = -errno;
		goto err_free_hdr_buf;
	}

	if (read_full(fd, elf->hdr_buf, hdr_size) < 0) {
		pr_err("could not read elf program headers: %m\n");
		ret = -errno;
		close(fd);
		goto err_free_hdr_buf;
	}
	close(fd);

	elf->filename = strdup_const(filename);
	if (!elf->filename) {
		ret = -ENOMEM;
		goto err_free_hdr_buf;
	}

	elf->entry = elf_hdr_e_entry(elf, elf->hdr_buf);

	return elf;

err_free_hdr_buf:
	free(elf->hdr_buf);
err_free_elf:
	free(elf);

	return ERR_PTR(ret);
}

struct elf_image *elf_open(const char *filename)
{
	return elf_check_init(filename);
}

void elf_set_load_address(struct elf_image *elf, void *addr)
{
	elf->load_address = addr;
}

static void *elf_find_dynamic_segment(struct elf_image *elf)
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

static int elf_relocate(struct elf_image *elf)
{
	void *dyn_seg;

	/*
	 * Relocations needed if:
	 * - ET_DYN (position-independent), OR
	 * - ET_EXEC with custom load address
	 */
	if (elf->type == ET_EXEC && !elf->load_address)
		return 0;

	/* Find PT_DYNAMIC segment */
	dyn_seg = elf_find_dynamic_segment(elf);
	if (!dyn_seg) {
		/*
		 * No PT_DYNAMIC segment found.
		 * For ET_DYN this is unusual but legal.
		 * For ET_EXEC with custom load address, this means no relocations
		 * can be applied - warn the user.
		 */
		if (elf->type == ET_EXEC && elf->load_address) {
			pr_warn("ET_EXEC loaded at custom address but no PT_DYNAMIC segment - "
				"relocations cannot be applied\n");
		} else {
			pr_debug("No PT_DYNAMIC segment found\n");
		}
		return 0;
	}

	/* Call architecture-specific relocation handler */
	return elf_apply_relocations(elf, dyn_seg);
}

int elf_load(struct elf_image *elf)
{
	int ret;

	ret = load_elf_image_segments(elf);
	if (ret)
		return ret;

	/* Apply relocations if needed */
	ret = elf_relocate(elf);
	if (ret) {
		pr_err("Relocation failed: %d\n", ret);
		return ret;
	}

	return 0;
}

void elf_close(struct elf_image *elf)
{
	elf_release_regions(elf);

	if (elf->filename) {
		free(elf->hdr_buf);
		free_const(elf->filename);
	}

	free(elf);
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

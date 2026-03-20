// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <firmware.h>
#include <asm/system.h>
#include <mach/rockchip/atf.h>
#include <elf.h>
#include <tee/optee.h>
#include <asm/atf_common.h>
#include <asm/barebox-arm.h>
#include <asm/mmu.h>
#include <asm-generic/memory_layout.h>
#include <asm-generic/sections.h>
#include <mach/rockchip/dmc.h>
#include <mach/rockchip/rockchip.h>
#include <mach/rockchip/bootrom.h>
#include <mach/rockchip/rk3562-regs.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/rk3576-regs.h>
#include <mach/rockchip/rk3588-regs.h>

struct rockchip_scratch_space *rk_scratch;

static void rk_scratch_save_optee_hdr(const struct optee_header *hdr)
{
	if (!rk_scratch) {
		pr_err("No scratch area initialized, skip saving optee-hdr\n");
		return;
	}

	pr_debug("Saving optee-hdr to scratch area 0x%p\n", &rk_scratch->optee_hdr);
	rk_scratch->optee_hdr = *hdr;
}

static void *free_mem(void)
{
	return (void *)PTR_ALIGN(&__image_end, SZ_1M);
}

static unsigned long load_elf64_image_phdr(struct fwobj *bl31)
{
	const Elf64_Ehdr *ehdr; /* Elf header structure pointer */
	const Elf64_Phdr *phdr; /* Program header structure pointer */
	int i, ret;
	void *bl31_image = free_mem();

	ret = fwobj_uncompress(bl31, bl31_image);
	if (ret)
		panic("Failed to uncompress TF-A\n");

	ehdr = bl31_image;
	phdr = bl31_image + ehdr->e_phoff;

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(ulong)phdr->p_paddr;
		const void *src = bl31_image + phdr->p_offset;

		pr_debug("Loading phdr %i to 0x%p (%lu bytes)\n",
			 i, dst, (ulong)phdr->p_filesz);
		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);
		if (phdr->p_filesz != phdr->p_memsz)
			memset(dst + phdr->p_filesz, 0x00,
				phdr->p_memsz - phdr->p_filesz);
		++phdr;
	}

	return ehdr->e_entry;
}

static uintptr_t rk_load_optee(uintptr_t bl32, struct fwobj *bl32_fw)
{
	const struct optee_header *hdr;
	struct optee_header dummy_hdr;
	int ret;
	void *bl32_image = free_mem();
	size_t bl32_size = bl32_fw->uncompressed_size;

	ret = fwobj_uncompress(bl32_fw, bl32_image);
	if (ret)
		panic("Failed to uncompress OP-TEE\n");

	/* We already have ELF support for BL31, but adding it for BL32,
	 * would require us to identify a range that fits all ELF
	 * sections and fake a dummy OP-TEE header that describes it.
	 * This is doable, but let's postpone that until there is an
	 * actual user interested in this.
	 */
	BUG_ON(memcmp(bl32_image, ELFMAG, 4) == 0);

	hdr = bl32_image;

	if (optee_verify_header(hdr) == 0) {
		bl32_size -= sizeof(*hdr);
		bl32_image += sizeof(*hdr);

		bl32 = (u64)hdr->init_load_addr_hi << 32;
		bl32 |= hdr->init_load_addr_lo;

		pr_debug("optee: adjusting address to 0x%lx\n", bl32);
	} else if (bl32 != ROCKCHIP_OPTEE_HEADER_REQUIRED) {
		dummy_hdr.magic = OPTEE_MAGIC;
		dummy_hdr.version = OPTEE_VERSION_V1;
		dummy_hdr.arch = OPTEE_ARCH_ARM64;
		dummy_hdr.flags = 0;
		dummy_hdr.init_size = bl32_size;
		dummy_hdr.init_load_addr_hi = upper_32_bits(bl32);
		dummy_hdr.init_load_addr_lo = lower_32_bits(bl32);
		dummy_hdr.init_mem_usage = 0;
		dummy_hdr.paged_size = 0;

		hdr = &dummy_hdr;

		pr_debug("optee: assuming load address is 0x%lx\n", bl32);

	} else {
		/* If we have neither a header, nor a defined load address
		 * there is really nothing we can do here.
		 */
		pr_err("optee: skipping. No header and no hardcoded load address\n");
		return 0;
	}

	rk_scratch_save_optee_hdr(hdr);

	memcpy((void *)bl32, bl32_image, bl32_size);

	return bl32;
}

static phys_addr_t membase[ROCKCHIP_MAX_DRAM_RESOURCES];
static resource_size_t memsize[ROCKCHIP_MAX_DRAM_RESOURCES];
static int n_mem_resources;
static uintptr_t barebox_load_address; /* where barebox is loaded and started */
static uintptr_t optee_load_address; /* standard SoC specific OP-TEE load address */
static struct fwobj bl31; /* TF-A in barebox image */
static struct fwobj bl32; /* OP-TEE in barebox image */

#define ROCKCHIP_GET_ADDRESSES(SOC, atf_bin, tee_bin)				\
	do {									\
		barebox_load_address = SOC##_BAREBOX_LOAD_ADDRESS;		\
		optee_load_address = SOC##_OPTEE_LOAD_ADDRESS;			\
		get_builtin_firmware_compressed(atf_bin, &bl31);		\
		if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_OPTEE))			\
			get_builtin_firmware_compressed(tee_bin, &bl32);	\
	} while (0)

static int rockchip_create_optee_fdt(void *buf, int bufsize)
{
	unsigned long base[ROCKCHIP_MAX_DRAM_RESOURCES];
	unsigned long size[ARRAY_SIZE(base)];
	int i, root;

	if (fdt_create_empty_tree(buf, bufsize) != 0)
		return -EINVAL;

	root = fdt_path_offset(buf, "/");

	fdt_setprop_u32(buf, root, "#address-cells", 2);
	fdt_setprop_u32(buf, root, "#size-cells", 2);

	for (i = 0; i < n_mem_resources; i++) {
		base[i] = membase[i];
		size[i] = memsize[i];
	}

	return fdt_fixup_mem(buf, base, size, n_mem_resources);
}

static void rockchip_atf_load_bl31(void *fdt)
{
	unsigned long bl31_ep;

	mmu_early_enable(membase[0], membase[0] + memsize[0]);

	bl31_ep = load_elf64_image_phdr(&bl31);

	if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_OPTEE))
		optee_load_address = rk_load_optee(optee_load_address, &bl32);

	/* Setup an initial stack for EL2 */
	asm volatile("msr sp_el2, %0" : :
			"r" ((ulong)barebox_load_address - 16) :
			"cc");

	mmu_disable();

	bl31_entry(bl31_ep, optee_load_address,
		   barebox_load_address, (uintptr_t)fdt);
}

void __noreturn rk3562_barebox_entry(void *fdt)
{
	phys_addr_t memend;

	n_mem_resources = rk3562_ram_sizes(membase, memsize, ROCKCHIP_MAX_DRAM_RESOURCES);

	memend = membase[0] + memsize[0];

	rk_scratch = (void *)arm_mem_scratch(memend);

	if (current_el() == 3) {
		rk3562_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3562_IRAM_BASE));
		ROCKCHIP_GET_ADDRESSES(RK3562, rk3562_bl31_bin, rk3562_bl32_bin);

		/*
		 * The downstream TF-A doesn't cope with our device tree when
		 * CONFIG_OF_OVERLAY_LIVE is enabled, supposedly because it is
		 * too big for some reason. Otherwise it doesn't have any visible
		 * effect if we pass a device tree or not, except that the TF-A
		 * fills in the ethernet MAC address into the device tree.
		 * The upstream TF-A doesn't use the device tree at all.
		 *
		 * Pass NULL for now until we have a good reason to pass a real
		 * device tree.
		 */
		rockchip_atf_load_bl31(NULL);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase[0], memsize[0], fdt);
}

void __noreturn rk3568_barebox_entry(void *fdt)
{
	phys_addr_t memend;

	n_mem_resources = rk3568_ram_sizes(membase, memsize, ROCKCHIP_MAX_DRAM_RESOURCES);

	memend = membase[0] + memsize[0];

	rk_scratch = (void *)arm_mem_scratch(memend);

	if (current_el() == 3) {
		rk3568_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3568_IRAM_BASE));
		ROCKCHIP_GET_ADDRESSES(RK3568, rk3568_bl31_bin, rk3568_bl32_bin);

		/*
		 * The downstream TF-A doesn't cope with our device tree when
		 * CONFIG_OF_OVERLAY_LIVE is enabled, supposedly because it is
		 * too big for some reason. Otherwise it doesn't have any visible
		 * effect if we pass a device tree or not, except that the TF-A
		 * fills in the ethernet MAC address into the device tree.
		 * The upstream TF-A doesn't use the device tree at all.
		 *
		 * Pass NULL for now until we have a good reason to pass a real
		 * device tree.
		 */
		rockchip_atf_load_bl31(NULL);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase[0], memsize[0], fdt);
}

void __noreturn rk3588_barebox_entry(void *fdt)
{
	phys_addr_t memend;
	int ret;

	n_mem_resources = rk3588_ram_sizes(membase, memsize, ROCKCHIP_MAX_DRAM_RESOURCES);

	memend = membase[0] + memsize[0];

	rk_scratch = (void *)arm_mem_scratch(memend);

	if (current_el() == 3) {
		rk3588_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3588_IRAM_BASE));
		ROCKCHIP_GET_ADDRESSES(RK3588, rk3588_bl31_bin, rk3588_bl32_bin);

		if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_ATF_PASS_FDT)) {
			pr_debug("Copy fdt to scratch area 0x%p (%zu bytes)\n",
				 rk_scratch->fdt, sizeof(rk_scratch->fdt));
			ret = rockchip_create_optee_fdt(rk_scratch->fdt, sizeof(rk_scratch->fdt));
			if (ret)
				pr_warn("Failed to create OP-TEE Device tree\n");
		}

		rockchip_atf_load_bl31(rk_scratch->fdt);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase[0], memsize[0], fdt);
}

void __noreturn rk3576_barebox_entry(void *fdt)
{
	phys_addr_t memend;

	n_mem_resources = rk3576_ram_sizes(membase, memsize, ROCKCHIP_MAX_DRAM_RESOURCES);

	memend = membase[0] + memsize[0];

	rk_scratch = (void *)arm_mem_scratch(memend);

	if (current_el() == 3) {
		void *fdt_scratch = NULL;

		rk3576_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3576_IRAM_BASE));
		ROCKCHIP_GET_ADDRESSES(RK3576, rk3576_bl31_bin, rk3576_bl32_bin);

		if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_ATF_PASS_FDT)) {
			pr_debug("Copy fdt to scratch area 0x%p (%zu bytes)\n",
				 rk_scratch->fdt, sizeof(rk_scratch->fdt));
			if (fdt_open_into(fdt, rk_scratch->fdt, sizeof(rk_scratch->fdt)) == 0)
				fdt_scratch = rk_scratch->fdt;
			else
				pr_warn("Failed to copy fdt to scratch: Continue without fdt\n");
		}

		rockchip_atf_load_bl31(fdt_scratch);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase[0], memsize[0], fdt);
}

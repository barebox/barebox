// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <firmware.h>
#include <asm/system.h>
#include <mach/rockchip/atf.h>
#include <elf.h>
#include <tee/optee.h>
#include <asm/atf_common.h>
#include <asm/barebox-arm.h>
#include <asm-generic/memory_layout.h>
#include <mach/rockchip/dmc.h>
#include <mach/rockchip/rockchip.h>
#include <mach/rockchip/bootrom.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/rk3588-regs.h>

struct rockchip_scratch_space *rk_scratch;

static void rk_scratch_save_optee_hdr(const struct optee_header *hdr)
{
	if (!rk_scratch) {
		pr_err("No scratch area initialized, skip saving optee-hdr");
		return;
	}

	pr_debug("Saving optee-hdr to scratch area 0x%p\n", &rk_scratch->optee_hdr);
	rk_scratch->optee_hdr = *hdr;
}

static unsigned long load_elf64_image_phdr(const void *elf)
{
	const Elf64_Ehdr *ehdr; /* Elf header structure pointer */
	const Elf64_Phdr *phdr; /* Program header structure pointer */
	int i;

	ehdr = elf;
	phdr = elf + ehdr->e_phoff;

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(ulong)phdr->p_paddr;
		const void *src = elf + phdr->p_offset;

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

static uintptr_t rk_load_optee(uintptr_t bl32, const void *bl32_image,
			       size_t bl32_size)
{
	const struct optee_header *hdr = bl32_image;
	struct optee_header dummy_hdr;

	/* We already have ELF support for BL31, but adding it for BL32,
	 * would require us to identify a range that fits all ELF
	 * sections and fake a dummy OP-TEE header that describes it.
	 * This is doable, but let's postpone that until there is an
	 * actual user interested in this.
	 */
	BUG_ON(memcmp(bl32_image, ELFMAG, 4) == 0);

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

#define rockchip_atf_load_bl31(SOC, atf_bin, tee_bin, fdt) do {                 \
	const void *bl31_elf, *optee;                                           \
	unsigned long bl31;                                                     \
	size_t bl31_elf_size, optee_size;                                       \
	uintptr_t optee_load_address = 0;                                       \
										\
	get_builtin_firmware(atf_bin, &bl31_elf, &bl31_elf_size);               \
										\
	bl31 = load_elf64_image_phdr(bl31_elf);                                 \
										\
	if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_OPTEE)) {                           \
		get_builtin_firmware(tee_bin, &optee, &optee_size);             \
		optee_load_address = rk_load_optee(SOC##_OPTEE_LOAD_ADDRESS,	\
						   optee, optee_size);		\
	}                                                                       \
										\
	/* Setup an initial stack for EL2 */                                    \
	asm volatile("msr sp_el2, %0" : :                                       \
			"r" ((ulong)SOC##_BAREBOX_LOAD_ADDRESS - 16) :		\
			"cc");                                                  \
										\
	bl31_entry(bl31, optee_load_address,                                    \
		   SOC##_BAREBOX_LOAD_ADDRESS, (uintptr_t)fdt);                 \
} while (0)                                                                     \

void rk3568_atf_load_bl31(void *fdt)
{
	rockchip_atf_load_bl31(RK3568, rk3568_bl31_bin, rk3568_bl32_bin, fdt);
}

void __noreturn rk3568_barebox_entry(void *fdt)
{
	unsigned long membase, endmem;

	membase = RK3568_DRAM_BOTTOM;
	endmem = rk3568_ram0_size();

	rk_scratch = (void *)arm_mem_scratch(endmem);

	if (current_el() == 3) {
		rk3568_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3568_IRAM_BASE));

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
		rk3568_atf_load_bl31(NULL);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase, endmem - membase, fdt);
}

void rk3588_atf_load_bl31(void *fdt)
{
	rockchip_atf_load_bl31(RK3588, rk3588_bl31_bin, rk3588_bl32_bin, fdt);
}

static int rk3588_fixup_mem(void *fdt)
{
	/* Use 4 blocks since rk3588 has 3 gaps in the address space */
	unsigned long base[4];
	unsigned long size[ARRAY_SIZE(base)];
	phys_addr_t base_tmp[ARRAY_SIZE(base)];
	resource_size_t size_tmp[ARRAY_SIZE(base_tmp)];
	int i, n;

	n = rk3588_ram_sizes(base_tmp, size_tmp, ARRAY_SIZE(base_tmp));
	for (i = 0; i < n; i++) {
		base[i] = base_tmp[i];
		size[i] = size_tmp[i];
	}

	return fdt_fixup_mem(fdt, base, size, i);
}

void __noreturn rk3588_barebox_entry(void *fdt)
{
	unsigned long membase, endmem;

	membase = RK3588_DRAM_BOTTOM;
	endmem = rk3588_ram0_size();

	rk_scratch = (void *)arm_mem_scratch(endmem);

	if (current_el() == 3) {
		void *fdt_scratch = NULL;

		rk3588_lowlevel_init();
		rockchip_store_bootrom_iram(IOMEM(RK3588_IRAM_BASE));

		if (IS_ENABLED(CONFIG_ARCH_ROCKCHIP_ATF_PASS_FDT)) {
			pr_debug("Copy fdt to scratch area 0x%p (%zu bytes)\n",
				 rk_scratch->fdt, sizeof(rk_scratch->fdt));
			if (fdt_open_into(fdt, rk_scratch->fdt, sizeof(rk_scratch->fdt)) == 0)
				fdt_scratch = rk_scratch->fdt;
			else
				pr_warn("Failed to copy fdt to scratch: Continue without fdt\n");
			if (fdt_scratch && rk3588_fixup_mem(fdt_scratch) != 0)
				pr_warn("Failed to fixup memory nodes\n");
		}

		rk3588_atf_load_bl31(fdt_scratch);
		/* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
	}

	optee_set_membase(rk_scratch_get_optee_hdr());
	barebox_arm_entry(membase, endmem - membase, fdt);
}

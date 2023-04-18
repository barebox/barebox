// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <firmware.h>
#include <asm/system.h>
#include <mach/rockchip/atf.h>
#include <elf.h>
#include <asm/atf_common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/dmc.h>
#include <mach/rockchip/rockchip.h>
#include <mach/rockchip/bootrom.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/rk3588-regs.h>

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
	if (IS_ENABLED(CONFIG_ARCH_##SOC##_OPTEE)) {                            \
		optee_load_address = SOC##_OPTEE_LOAD_ADDRESS;                  \
										\
		get_builtin_firmware(tee_bin, &optee, &optee_size);             \
										\
		memcpy((void *)optee_load_address, optee, optee_size);          \
	}                                                                       \
										\
	/* Setup an initial stack for EL2 */                                    \
	asm volatile("msr sp_el2, %0" : :                                       \
			"r" (SOC##_BAREBOX_LOAD_ADDRESS - 16) :                 \
			"cc");                                                  \
										\
	bl31_entry(bl31, optee_load_address,                                    \
		   SOC##_BAREBOX_LOAD_ADDRESS, (uintptr_t)fdt);                 \
} while (0)                                                                     \

void rk3399_atf_load_bl31(void *fdt)
{
	rockchip_atf_load_bl31(RK3399, rk3399_bl31_bin, rk3399_op_tee_bin, fdt);
}

void rk3568_atf_load_bl31(void *fdt)
{
	rockchip_atf_load_bl31(RK3568, rk3568_bl31_bin, rk3568_op_tee_bin, fdt);
}

void __noreturn rk3568_barebox_entry(void *fdt)
{
	unsigned long membase, memsize;

	membase = RK3568_DRAM_BOTTOM;
	memsize = rk3568_ram0_size() - RK3568_DRAM_BOTTOM;

	if (current_el() == 3) {
		rk3568_lowlevel_init();
		rockchip_store_bootrom_iram(membase, memsize, IOMEM(RK3568_IRAM_BASE));

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

	barebox_arm_entry(membase, memsize, fdt);
}

void rk3588_atf_load_bl31(void *fdt)
{
	rockchip_atf_load_bl31(RK3588, rk3588_bl31_bin, rk3588_op_tee_bin, fdt);
}

void __noreturn rk3588_barebox_entry(void *fdt)
{
       unsigned long membase, memsize;

       membase = RK3588_DRAM_BOTTOM;
       memsize = rk3588_ram0_size() - RK3588_DRAM_BOTTOM;

       if (current_el() == 3) {
               rk3588_lowlevel_init();
               rockchip_store_bootrom_iram(membase, memsize, IOMEM(RK3588_IRAM_BASE));

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
               rk3588_atf_load_bl31(NULL);
               /* not reached when CONFIG_ARCH_ROCKCHIP_ATF */
       }

       barebox_arm_entry(membase, memsize, fdt);
}

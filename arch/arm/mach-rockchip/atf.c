// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <firmware.h>
#include <asm/system.h>
#include <mach/atf.h>
#include <elf.h>
#include <asm/atf_common.h>

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

void rk3568_atf_load_bl31(void *fdt)
{
	const void *bl31_elf, *optee;
	unsigned long bl31;
	size_t bl31_elf_size, optee_size;
	uintptr_t optee_load_address = 0;

	get_builtin_firmware(rk3568_bl31_bin, &bl31_elf, &bl31_elf_size);

	bl31 = load_elf64_image_phdr(bl31_elf);

	if (IS_ENABLED(CONFIG_ARCH_RK3568_OPTEE)) {
		optee_load_address = RK3568_OPTEE_LOAD_ADDRESS;

		get_builtin_firmware(rk3568_op_tee_bin, &optee, &optee_size);

		memcpy((void *)optee_load_address, optee, optee_size);
	}

	/* Setup an initial stack for EL2 */
	asm volatile("msr sp_el2, %0" : :
			"r" (RK3568_BAREBOX_LOAD_ADDRESS - 16) :
			"cc");

	bl31_entry(bl31, optee_load_address,
		   RK3568_BAREBOX_LOAD_ADDRESS, (uintptr_t)fdt);
}

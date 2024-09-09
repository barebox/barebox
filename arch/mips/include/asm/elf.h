/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Much of this is taken from binutils and GNU libc ...
 */

/**
 * @file
 * @brief mips specific elf information
 *
 */

#ifndef _ASM_MIPS_ELF_H
#define _ASM_MIPS_ELF_H

#ifndef ELF_ARCH

#define EM_MIPS		 8	/* MIPS R3000 (officially, big-endian only) */

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == EM_MIPS)

#ifdef CONFIG_32BIT
/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32

#endif /* CONFIG_32BIT */

#ifdef CONFIG_64BIT
/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS64

#endif /* CONFIG_64BIT */

/*
 * These are used to set parameters in the core dumps.
 */
#ifdef __MIPSEB__
#define ELF_DATA	ELFDATA2MSB
#elif defined(__MIPSEL__)
#define ELF_DATA	ELFDATA2LSB
#endif
#define ELF_ARCH	EM_MIPS

#endif /* !defined(ELF_ARCH) */
#endif /* _ASM_MIPS_ELF_H */

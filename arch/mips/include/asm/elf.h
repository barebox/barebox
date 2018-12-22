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

#ifdef CONFIG_32BIT

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(hdr)						\
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

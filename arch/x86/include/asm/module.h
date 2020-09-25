/* SPDX-License-Identifier: GPL-2.0-or-later */

/**
 * @file
 * @brief x86 module support
 *
 */

#ifndef _ASM_X86_MODULE_H_
#define _ASM_X86_MODULE_H_

/** currently nothing special */
struct mod_arch_specific
{
        int foo;
};

#define Elf_Shdr        Elf32_Shdr
#define Elf_Ehdr        Elf32_Ehdr

#endif /* _ASM_X86_MODULE_H_ */

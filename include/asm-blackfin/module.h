#ifndef _ASM_BFIN_MODULE_H
#define _ASM_BFIN_MODULE_H

#define MODULE_SYMBOL_PREFIX "_"

#define Elf_Shdr        Elf32_Shdr
#define Elf_Sym         Elf32_Sym
#define Elf_Ehdr        Elf32_Ehdr
#define FLG_CODE_IN_L1	0x10
#define FLG_DATA_IN_L1	0x20

struct mod_arch_specific {
};
#endif				/* _ASM_BFIN_MODULE_H */

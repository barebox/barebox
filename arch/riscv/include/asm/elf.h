#ifndef __ASM_RISCV_ELF_H__
#define __ASM_RISCV_ELF_H__

#if __SIZEOF_POINTER__ == 8
#define ELF_CLASS	ELFCLASS64
#define CONFIG_PHYS_ADDR_T_64BIT
#else
#define ELF_CLASS	ELFCLASS32
#endif

#endif /* __ASM_RISCV_ELF_H__ */


/**/
struct mod_arch_specific {
	/* Indices of PLT sections within module. */
	unsigned int core_plt_section;
	unsigned int init_plt_section;
};

#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym	Elf32_Sym
#define Elf_Ehdr	Elf32_Ehdr

struct ppc_plt_entry {
	/* 16 byte jump instruction sequence (4 instructions) */
	unsigned int jump[4];
};

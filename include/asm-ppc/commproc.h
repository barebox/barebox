#ifndef __INCLUDE_ASM_ARCH_COMMPROC_H
#define __INCLUDE_ASM_ARCH_COMMPROC_H

int	dpram_init (void);
uint	dpram_base(void);
uint	dpram_base_align(uint align);
uint	dpram_alloc(uint size);
uint	dpram_alloc_align(uint size,uint align);
void	post_word_store (ulong);
ulong	post_word_load (void);
void	bootcount_store (ulong);
ulong	bootcount_load (void);
#define BOOTCOUNT_MAGIC		0xB001C041

#endif  /* __INCLUDE_ASM_ARCH_COMMPROC_H */

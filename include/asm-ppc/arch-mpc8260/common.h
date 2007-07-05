#ifndef __INCLUDE_ASM_ARCH_COMMON_H
#define __INCLUDE_ASM_ARCH_COMMON_H

int	prt_8260_clks (void);
void	cpu_init_f    (volatile immap_t *immr);
int	prt_8260_rsr  (void);

#endif  /* __INCLUDE_ASM_ARCH_COMMON_H */

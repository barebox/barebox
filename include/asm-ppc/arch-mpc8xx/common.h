#ifndef __INCLUDE_ASM_ARCH_COMMON_H
#define __INCLUDE_ASM_ARCH_COMMON_H
uint	rd_ic_cst     (void);
void	wr_ic_cst     (uint);
void	wr_ic_adr     (uint);

uint	rd_dc_cst     (void);
void	wr_dc_cst     (uint);
void	wr_dc_adr     (uint);

int	adjust_sdram_tbs_8xx (void);

void	cpu_init_f    (volatile immap_t *immr);

#endif  /* __INCLUDE_ASM_ARCH_COMMON_H */

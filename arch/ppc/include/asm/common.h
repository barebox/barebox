#ifndef __ASM_COMMON_H
#define __ASM_COMMON_H

#include <asm/barebox.h>

void	upmconfig     (unsigned int, unsigned int *, unsigned int);
ulong	get_tbclk     (void);

unsigned long long get_ticks(void);

int	get_clocks (void);
ulong	get_bus_freq  (ulong);

int	cpu_init      (void);

uint	get_pvr	      (void);
uint	get_svr	      (void);

void	trap_init     (ulong);

int cpu_init_board_data(bd_t *bd);
int init_board_data(bd_t *bd);

#endif /* __ASM_COMMON_H */

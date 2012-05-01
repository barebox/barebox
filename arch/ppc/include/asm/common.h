#ifndef __ASM_COMMON_H
#define __ASM_COMMON_H

#include <asm/barebox.h>

extern unsigned long _text_base;

unsigned long long get_ticks(void);

int	cpu_init      (void);

uint	get_pvr	      (void);
uint	get_svr	      (void);

void	trap_init     (ulong);

static inline unsigned long get_pc(void)
{
	unsigned long pc;

	__asm__ __volatile__(
		"	mflr    0\n"
		"	bl	1f\n"
		"1:\n"
		"	mflr    %0\n"
		"	mtlr    0\n"
		: "=r" (pc)
                :
                : "0", "memory");

	return pc;
}

extern unsigned long search_exception_table(unsigned long);
#endif /* __ASM_COMMON_H */

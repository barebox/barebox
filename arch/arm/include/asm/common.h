#ifndef __ASM_ARM_COMMON_H
#define __ASM_ARM_COMMON_H

#define ARCH_SHUTDOWN

static inline unsigned long get_pc(void)
{
	unsigned long pc;

	__asm__ __volatile__(
                "mov    %0, pc\n"
                : "=r" (pc)
                :
                : "memory");

	return pc;
}

#endif /* __ASM_ARM_COMMON_H */

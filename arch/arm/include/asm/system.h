#ifndef __ASM_ARM_SYSTEM_H
#define __ASM_ARM_SYSTEM_H

#if __LINUX_ARM_ARCH__ >= 7
#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#ifdef CONFIG_CPU_64v8
#define dsb() __asm__ __volatile__ ("dsb sy" : : : "memory")
#else
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#endif
#define dmb() __asm__ __volatile__ ("dmb" : : : "memory")
#elif defined(CONFIG_CPU_XSC3) || __LINUX_ARM_ARCH__ == 6
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
                                    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" \
                                    : : "r" (0) : "memory")
#elif defined(CONFIG_CPU_FA526)
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
                                    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#else
#define isb() __asm__ __volatile__ ("" : : : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#endif

/*
 * CR1 bits (CP#15 CR1)
 */
#define CR_M    (1 << 0)	/* MMU enable				*/
#define CR_A    (1 << 1)	/* Alignment abort enable		*/
#define CR_C    (1 << 2)	/* Dcache enable			*/
#define CR_W    (1 << 3)	/* Write buffer enable			*/
#define CR_P    (1 << 4)	/* 32-bit exception handler		*/
#define CR_D    (1 << 5)	/* 32-bit data address range		*/
#define CR_L    (1 << 6)	/* Implementation defined		*/
#define CR_B    (1 << 7)	/* Big endian				*/
#define CR_S    (1 << 8)	/* System MMU protection		*/
#define CR_R    (1 << 9)	/* ROM MMU protection			*/
#define CR_F    (1 << 10)	/* Implementation defined		*/
#define CR_Z    (1 << 11)	/* Implementation defined		*/
#define CR_I    (1 << 12)	/* Icache enable			*/
#define CR_V    (1 << 13)	/* Vectors relocated to 0xffff0000	*/
#define CR_RR   (1 << 14)	/* Round Robin cache replacement	*/
#define CR_L4   (1 << 15)	/* LDR pc can set T bit			*/
#define CR_DT   (1 << 16)
#define CR_IT   (1 << 18)
#define CR_ST   (1 << 19)
#define CR_FI   (1 << 21)	/* Fast interrupt (lower latency mode)	*/
#define CR_U    (1 << 22)	/* Unaligned access operation		*/
#define CR_XP   (1 << 23)	/* Extended page tables			*/
#define CR_VE   (1 << 24)	/* Vectored interrupts			*/
#define CR_EE   (1 << 25)	/* Exception (Big) Endian		*/
#define CR_TRE  (1 << 28)	/* TEX remap enable			*/
#define CR_AFE  (1 << 29)	/* Access flag enable			*/
#define CR_TE   (1 << 30)	/* Thumb exception enable		*/

#ifndef __ASSEMBLY__
#if __LINUX_ARM_ARCH__ >= 7
static inline unsigned int current_el(void)
{
	unsigned int el;
	asm volatile("mrs %0, CurrentEL" : "=r" (el) : : "cc");
	return el >> 2;
}

static inline unsigned long read_mpidr(void)
{
	unsigned long val;

	asm volatile("mrs %0, mpidr_el1" : "=r" (val));

	return val;
}

static inline void set_cntfrq(unsigned long cntfrq)
{
	asm volatile("msr cntfrq_el0, %0" : : "r" (cntfrq) : "memory");
}

static inline unsigned long get_cntfrq(void)
{
	unsigned long cntfrq;

	asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
	return cntfrq;
}

static inline unsigned long get_cntpct(void)
{
	unsigned long cntpct;

	isb();
	asm volatile("mrs %0, cntpct_el0" : "=r" (cntpct));

	return cntpct;
}

#endif
static inline unsigned int get_cr(void)
{
	unsigned int val;

#ifdef CONFIG_CPU_64v8
	unsigned int el = current_el();
	if (el == 1)
		asm volatile("mrs %0, sctlr_el1" : "=r" (val) : : "cc");
	else if (el == 2)
		asm volatile("mrs %0, sctlr_el2" : "=r" (val) : : "cc");
	else
		asm volatile("mrs %0, sctlr_el3" : "=r" (val) : : "cc");
#else
	asm volatile ("mrc p15, 0, %0, c1, c0, 0  @ get CR" : "=r" (val) : : "cc");
#endif

	return val;
}

static inline void set_cr(unsigned int val)
{
#ifdef CONFIG_CPU_64v8
	unsigned int el;

	el = current_el();
	if (el == 1)
		asm volatile("msr sctlr_el1, %0" : : "r" (val) : "cc");
	else if (el == 2)
		asm volatile("msr sctlr_el2, %0" : : "r" (val) : "cc");
	else
		asm volatile("msr sctlr_el3, %0" : : "r" (val) : "cc");
#else
	asm volatile("mcr p15, 0, %0, c1, c0, 0 @ set CR"
	  : : "r" (val) : "cc");
#endif
	isb();
}

#ifdef CONFIG_CPU_32v7
static inline unsigned int get_vbar(void)
{
	unsigned int vbar;
	asm volatile("mrc p15, 0, %0, c12, c0, 0 @ get VBAR"
		     : "=r" (vbar) : : "cc");
	return vbar;
}

static inline void set_vbar(unsigned int vbar)
{
	asm volatile("mcr p15, 0, %0, c12, c0, 0 @ set VBAR"
		     : : "r" (vbar) : "cc");
	isb();
}
#else
static inline unsigned int get_vbar(void) { return 0; }
static inline void set_vbar(unsigned int vbar) {}
#endif
#endif

#endif /* __ASM_ARM_SYSTEM_H */

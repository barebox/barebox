/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_SYSTEM_H
#define __ASM_ARM_SYSTEM_H

#include <linux/const.h>

/* Common SCTLR_ELx flags. */
#define SCTLR_ELx_DSSBS	(_BITUL(44))
#define SCTLR_ELx_ENIA	(_BITUL(31))
#define SCTLR_ELx_ENIB	(_BITUL(30))
#define SCTLR_ELx_ENDA	(_BITUL(27))
#define SCTLR_ELx_EE    (_BITUL(25))
#define SCTLR_ELx_IESB	(_BITUL(21))
#define SCTLR_ELx_WXN	(_BITUL(19))
#define SCTLR_ELx_ENDB	(_BITUL(13))
#define SCTLR_ELx_I	(_BITUL(12))
#define SCTLR_ELx_SA	(_BITUL(3))
#define SCTLR_ELx_C	(_BITUL(2))
#define SCTLR_ELx_A	(_BITUL(1))
#define SCTLR_ELx_M	(_BITUL(0))

#define SCTLR_ELx_FLAGS	(SCTLR_ELx_M  | SCTLR_ELx_A | SCTLR_ELx_C | \
			 SCTLR_ELx_SA | SCTLR_ELx_I | SCTLR_ELx_IESB)

#if __LINUX_ARM_ARCH__ >= 7
#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#define dsb() __asm__ __volatile__ ("dsb sy" : : : "memory")
#define dmb() __asm__ __volatile__ ("dmb sy" : : : "memory")
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

/*
 * PSR bits
 */
#define USR_MODE	0x00000010
#define FIQ_MODE	0x00000011
#define IRQ_MODE	0x00000012
#define SVC_MODE	0x00000013
#define ABT_MODE	0x00000017
#define HYP_MODE	0x0000001a
#define UND_MODE	0x0000001b
#define SYSTEM_MODE	0x0000001f
#define MODE32_BIT	0x00000010
#define MODE_MASK	0x0000001f

#define PSR_T_BIT	0x00000020
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_E_BIT	0x00000200
#define PSR_J_BIT	0x01000000
#define PSR_Q_BIT	0x08000000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

#ifndef __ASSEMBLY__
#if __LINUX_ARM_ARCH__ > 7
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
#else
static inline void set_cntfrq(unsigned long cntfrq)
{
	asm("mcr p15, 0, %0, c14, c0, 0" : : "r" (cntfrq));
}

static inline unsigned int get_cntfrq(void)
{
	unsigned int val;

	asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (val));

	return val;
}

static inline unsigned long long get_cntpct(void)
{
	unsigned long long cval;

	isb();
	asm volatile("mrrc p15, 0, %Q0, %R0, c14" : "=r" (cval));

	return cval;
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

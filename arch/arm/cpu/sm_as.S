#include <linux/linkage.h>
#include <asm/ptrace.h>
#include <asm-generic/memory_layout.h>
#include <asm/secure.h>
#include <asm/system.h>

.arch_extension sec
.arch_extension virt

	.section ".text","ax"
	.arm

	.align	5
.globl secure_monitor_init_vectors
secure_monitor_init_vectors:
1:	b 1b			/* reset */
1:	b 1b			/* undefined instruction */
	b secure_monitor_init	/* software interrupt (SWI) */
1:	b 1b			/* prefetch abort */
1:	b 1b			/* data abort */
1:	b 1b			/* (reserved) */
1:	b 1b			/* irq (interrupt) */
1:	b 1b			/* fiq (fast interrupt) */

#define CPUID_ARM_GENTIMER_MASK		(0xF << CPUID_ARM_GENTIMER_SHIFT)
#define CPUID_ARM_GENTIMER_SHIFT	16

#define CPUID_ARM_VIRT_MASK		(0xF << CPUID_ARM_VIRT_SHIFT)
#define CPUID_ARM_VIRT_SHIFT		12

.macro is_cpu_virt_capable	tmp
	mrc	p15, 0, \tmp, c0, c1, 1		@ read ID_PFR1
	and	\tmp, \tmp, #CPUID_ARM_VIRT_MASK	@ mask virtualization bits
	cmp	\tmp, #(1 << CPUID_ARM_VIRT_SHIFT)
.endm

@ Requires dense and single-cluster CPU ID space
ENTRY(psci_get_cpu_id)
	mrc	p15, 0, r0, c0, c0, 5   /* read MPIDR */
	and	r0, r0, #0xff           /* return CPU ID in cluster */
	bx	lr
ENDPROC(psci_get_cpu_id)

ENTRY(secure_monitor_stack_setup)
	mrc	p15, 0, r0, c0, c0, 5   /* read MPIDR */
	and	r0, r0, #0xff           /* CPU ID => r0 */

	@ stack top = __secure_stack_end - (cpuid << ARM_PSCI_STACK_SHIFT)
	ldr	r1, =__secure_stack_end
	sub	r0, r1, r0, LSL #ARM_SECURE_STACK_SHIFT
	sub	r0, r0, #4              @ Save space for target PC

	mov	sp, r0
	bx	lr
ENDPROC(secure_monitor_stack_setup)

secure_monitor_init:
	mov	r3, lr

	bl	secure_monitor_stack_setup

	push	{r4-r7}
	mov	r7, r3
	ldr	r5, =secure_monitor_vectors	@ Switch MVBAR to secure_monitor_vectors
	mcr	p15, 0, r5, c12, c0, 1
	isb

#ifdef CONFIG_MMU
	mrc	p15, 0, r5, c1, c0, 0
	tst	r5, #CR_M
	beq	1f
	bl	__mmu_cache_off
1:
#endif
	mrc	p15, 0, r5, c1, c1, 0		@ read SCR
	bic	r5, r5, #0x4a			@ clear IRQ, EA, nET bits
	orr	r5, r5, #0x31			@ enable NS, AW, FW bits
						@ FIQ preserved for secure mode
	mov	r6, #SVC_MODE			@ default mode is SVC

	is_cpu_virt_capable r4

	orreq   r5, r5, #0x100			@ allow HVC instruction

	mcr	p15, 0, r5, c1, c1, 0		@ write SCR (with NS bit set)
	isb

	mrceq	p15, 0, r0, c12, c0, 1          @ get MVBAR value
	mcreq	p15, 4, r0, c12, c0, 0          @ write HVBAR

	bne	1f

	@ Reset CNTVOFF to 0 before leaving monitor mode
	mrc	p15, 0, r4, c0, c1, 1		@ read ID_PFR1
	ands	r4, r4, #CPUID_ARM_GENTIMER_MASK	@ test arch timer bits
	movne	r4, #0
	mcrrne	p15, 4, r4, r4, c14		@ Reset CNTVOFF to zero
1:
	mov	lr, r7
	mov	ip, #(PSR_F_BIT | PSR_I_BIT | PSR_A_BIT)	@ Set A, I and F
	tst	lr, #1				@ Check for Thumb PC
	orrne	ip, ip, #PSR_T_BIT		@ Set T if Thumb
	orr	ip, ip, r6			@ Slot target mode in
	msr	spsr_cxfs, ip			@ Set full SPSR
	pop	{r4-r7}
	movs	pc, lr				@ ERET to non-secure

	.align	5
secure_monitor_vectors:
1:	b 1b			/* reset */
1:	b 1b			/* undefined instruction */
	b secure_monitor	/* software interrupt (SWI) */
1:	b 1b			/* prefetch abort */
1:	b 1b			/* data abort */
1:	b hyp_trap		/* (reserved) */
1:	b 1b			/* irq (interrupt) */
1:	b 1b			/* fiq (fast interrupt) */

secure_monitor:
	push    {r4-r7,lr}

	@ Switch to secure mode
	mrc     p15, 0, r7, c1, c1, 0
	bic     r4, r7, #1
	mcr     p15, 0, r4, c1, c1, 0
        isb

	/* r0-r6: Arguments */
	sub	sp, sp, #4*4			@ allocate result structure on stack
	mov	r12, sp
	push	{r4-r6, r12}
#ifdef CONFIG_ARM_PSCI
	bl	psci_entry
#endif
	pop	{r4-r6, r12}
	ldm	r12, {r0-r3}
	add	sp, sp, #4*4
	/* r0-r3: results, r4-r14: preserved */

	@ back to non-secure
	mcr     p15, 0, r7, c1, c1, 0

	pop     {r4-r7, lr}
	movs	pc, lr

hyp_trap:
        mrs     lr, elr_hyp     @ for older asm: .byte 0x00, 0xe3, 0x0e, 0xe1
        mov pc, lr                              @ do no switch modes, but
                                                @ return to caller

ENTRY(psci_cpu_entry)
	mrc	p15, 0, r0, c1, c0, 1	@ ACTLR
	orr	r0, r0, #(1 << 6)	@ Set SMP bit
	mcr	p15, 0, r0, c1, c0, 1	@ ACTLR

	bl	secure_monitor_stack_setup
#ifdef CONFIG_ARM_PSCI
	bl	psci_cpu_entry_c
#endif

ENDPROC(psci_cpu_entry)

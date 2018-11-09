#include <linux/linkage.h>
#include <asm/system.h>
#include <asm/opcodes-virt.h>
#include <init.h>

.arch_extension sec
.arch_extension virt

__BARE_INIT

.data
	.align  2
ENTRY(__boot_cpu_mode)
	.long   0
.text

ENTRY(__hyp_install)
	mrs	r12, cpsr
	and	r12, r12, #MODE_MASK

	@ Save the initial CPU state
	adr	r0, .L__boot_cpu_mode_offset
	ldr	r1, [r0]
	str	r12, [r0, r1]

	cmp	r12, #HYP_MODE
	movne	pc, lr			@ give up if the CPU is not in HYP mode

	@ Now install the hypervisor stub:
	adr	r12, __hyp_vectors
	mcr	p15, 4, r12, c12, c0, 0	@ set hypervisor vector base (HVBAR)

	@ Disable all traps, so we don't get any nasty surprise
	mov	r12, #0
	mcr	p15, 4, r12, c1, c1, 0	@ HCR
	mcr	p15, 4, r12, c1, c1, 2	@ HCPTR
	mcr	p15, 4, r12, c1, c1, 3	@ HSTR

THUMB(	orr	r12, #(1 << 30)	)	@ HSCTLR.TE
	mcr	p15, 4, r12, c1, c0, 0	@ HSCTLR

	mrc	p15, 4, r12, c1, c1, 1	@ HDCR
	and	r12, #0x1f		@ Preserve HPMN
	mcr	p15, 4, r12, c1, c1, 1	@ HDCR

	@ Make sure NS-SVC is initialised appropriately
	mrc	p15, 0, r12, c1, c0, 0	@ SCTLR
	orr	r12, #(1 << 5)		@ CP15 barriers enabled
	bic	r12, #(3 << 7)		@ Clear SED/ITD for v8 (RES0 for v7)
	bic	r12, #(3 << 19)		@ WXN and UWXN disabled
	mcr	p15, 0, r12, c1, c0, 0	@ SCTLR

	mrc	p15, 0, r12, c0, c0, 0	@ MIDR
	mcr	p15, 4, r12, c0, c0, 0	@ VPIDR

	mrc	p15, 0, r12, c0, c0, 5	@ MPIDR
	mcr	p15, 4, r12, c0, c0, 5	@ VMPIDR
	bx	lr
ENDPROC(__hyp_install)

ENTRY(armv7_hyp_install)
	mov	r2, lr

	bl	__hyp_install

	/* set the cpu to SVC32 mode, mask irq and fiq */
	mrs	r12, cpsr
	eor	r12, r12, #HYP_MODE
	tst	r12, #MODE_MASK
	bic	r12, r12, #MODE_MASK
	orr	r12, r12, #(PSR_I_BIT | PSR_F_BIT | SVC_MODE)
THUMB(	orr	r12, r12, #PSR_T_BIT	)
	bne	1f
	orr	r12, r12, #PSR_A_BIT
	adr	lr, 2f
	msr	spsr_cxsf, r12
	__MSR_ELR_HYP(14)
	__ERET
1:	msr	cpsr_c, r12
2:
	mov	pc, r2
ENDPROC(armv7_hyp_install)

ENTRY(armv7_switch_to_hyp)
	mov	r0, lr
	mov	r1, sp		@ save SVC copy of LR and SP
	isb
	hvc #0			@ for older asm: .byte 0x70, 0x00, 0x40, 0xe1
	mov	sp, r1
	mov	lr, r0		@ restore SVC copy of LR and SP

	bx	lr
ENDPROC(armv7_switch_to_hyp)

.align 2
.L__boot_cpu_mode_offset:
	.long	__boot_cpu_mode - .

/* The HYP trap is crafted to match armv7_switch_to_hyp() */
__hyp_do_trap:
	mov lr, r0
	mov sp, r1
	bx  lr
ENDPROC(__hyp_do_trap)

.align 5
__hyp_vectors:
__hyp_reset:	W(b)	.
__hyp_und:	W(b)	.
__hyp_svc:	W(b)	.
__hyp_pabort:	W(b)	.
__hyp_dabort:	W(b)	.
__hyp_trap:	W(b)	__hyp_do_trap
__hyp_irq:	W(b)	.
__hyp_fiq:	W(b)	.
ENDPROC(__hyp_vectors)

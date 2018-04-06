#ifndef __ASM_ARCH_ASSEMBLY_H
#define __ASM_ARCH_ASSEMBLY_H

#ifndef __ASSEMBLY__
#error "Only include this from assembly code"
#endif

/*
 * Branch according to exception level
 */
.macro  switch_el, xreg, el3_label, el2_label, el1_label
	mrs	\xreg, CurrentEL
	cmp	\xreg, 0xc
	b.eq	\el3_label
	cmp	\xreg, 0x8
	b.eq	\el2_label
	cmp	\xreg, 0x4
	b.eq	\el1_label
.endm

#endif /* __ASM_ARCH_ASSEMBLY_H */
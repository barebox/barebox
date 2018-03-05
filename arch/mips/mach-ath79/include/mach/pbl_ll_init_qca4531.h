#ifndef __ASM_MACH_ATH79_PBL_LL_INIT_QCA4531_H
#define __ASM_MACH_ATH79_PBL_LL_INIT_QCA4531_H

#include <asm/addrspace.h>
#include <asm/regdef.h>


.macro	pbl_qca4531_ddr2_550_550_init
	.set	push
	.set	noreorder

	pbl_reg_writel 0xfeceffff , 0xb806001c
	pbl_reg_writel 0xeeceffff , 0xb806001c
	pbl_reg_writel 0xe6ceffff , 0xb806001c
	pbl_reg_writel 0x633c8176 , 0xb8116c40
	pbl_reg_writel 0x10200000 , 0xb8116c44
	pbl_reg_writel 0x4b962100 , 0xb81162c0
	pbl_reg_writel 0x480      , 0xb81162c4
	pbl_reg_writel 0x04000144 , 0xb81162c8
	pbl_reg_writel 0x54086000 , 0xb81161c4
	pbl_reg_writel 0x54086000 , 0xb8116244
	pbl_reg_writel 0x0131001c , 0xb8050008
	pbl_reg_writel 0x40001580 , 0xb8050000
	pbl_reg_writel 0x40015800 , 0xb8050004
	pbl_reg_writel 0x0131001c , 0xb8050008
	pbl_reg_writel 0x00001580 , 0xb8050000
	pbl_reg_writel 0x00015800 , 0xb8050004
	pbl_reg_writel 0x01310000 , 0xb8050008
	pbl_reg_writel 0x781003ff , 0xb8050044
	pbl_reg_writel 0x003c103f , 0xb8050048
	pbl_reg_writel 0x401f0042 , 0xb8000108
	pbl_reg_writel 0x0000166d , 0xb80000b8
	pbl_reg_writel 0xcfaaf33b , 0xb8000000
	pbl_reg_writel 0x0000000f , 0xb800015c
	pbl_reg_writel 0xa272efa8 , 0xb8000004
	pbl_reg_writel 0x000ffff  , 0xb8000018
	pbl_reg_writel 0x74444444 , 0xb80000c4
	pbl_reg_writel 0x00000444 , 0xb80000c8
	pbl_reg_writel 0xa210ee28 , 0xb8000004
	pbl_reg_writel 0xa2b2e1a8 , 0xb8000004
	pbl_reg_writel 0x8        , 0xb8000010
	pbl_reg_writel 0x0        , 0xb80000bc
	pbl_reg_writel 0x10       , 0xb8000010
	pbl_reg_writel 0x0        , 0xb80000c0
	pbl_reg_writel 0x40       , 0xb8000010
	pbl_reg_writel 0x2        , 0xb800000c
	pbl_reg_writel 0x2        , 0xb8000010
	pbl_reg_writel 0xb43      , 0xb8000008
	pbl_reg_writel 0x1        , 0xb8000010
	pbl_reg_writel 0x8        , 0xb8000010
	pbl_reg_writel 0x4        , 0xb8000010
	pbl_reg_writel 0x4        , 0xb8000010
	pbl_reg_writel 0xa43      , 0xb8000008
	pbl_reg_writel 0x1        , 0xb8000010
	pbl_reg_writel 0x382      , 0xb800000c
	pbl_reg_writel 0x2        , 0xb8000010
	pbl_reg_writel 0x402      , 0xb800000c
	pbl_reg_writel 0x2        , 0xb8000010
	pbl_reg_writel 0x40be     , 0xb8000014
	pbl_reg_writel 0x20       , 0xb800001C
	pbl_reg_writel 0x20       , 0xb8000020
	pbl_reg_writel 0xfffff    , 0xb80000cc
	pbl_reg_writel 0xff30b    , 0xb8040000
	pbl_reg_writel 0x908      , 0xb8040044
	pbl_reg_writel 0x160000   , 0xb8040034

	.set	pop
.endm

#endif /* __ASM_MACH_ATH79_PBL_LL_INIT_QCA4531_H */

/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 *
 */

/*
 * The actual number of banks implemented depends on the IFC version
 *    - IFC version 1.0 implements 4 banks.
 *    - IFC version 1.1 onward implements 8 banks.
 */
#define FSL_IFC_BANK_COUNT 8

#define FSL_IFC_REV		0x0000
#define		FSL_IFC_V1_1_0	0x01010000
#define		FSL_IFC_V2_0_0	0x02000000

/*
 * Version 1.1.0 adds offset 0x1000
 * Version 2.0.0 adds offset 0x10000
 */
#define FSL_IFC_NCFGR		0x000

#define		IFC_NAND_SRAM_INIT_EN           0x20000000

/*
 * NAND Flash Command Registers (NAND_FCR0/NAND_FCR1)
 */
#define FSL_IFC_FCR0		0x014
/* General purpose FCM flash command bytes CMD0-CMD7 */
#define		IFC_NAND_FCR0_CMD0_SHIFT	24
#define		IFC_NAND_FCR0_CMD1_SHIFT	16
#define		IFC_NAND_FCR0_CMD2_SHIFT	8
#define		IFC_NAND_FCR0_CMD3_SHIFT	0
#define FSL_IFC_ROW0		0x03c
#define		IFC_NAND_COL_MS			0x80000000
#define FSL_IFC_COL0		0x044
#define FSL_IFC_ROW3		0x06c
#define FSL_IFC_NAND_BC		0x108
/*
 * NAND Flash Instruction Registers (NAND_FIR0/NAND_FIR1/NAND_FIR2)
 */
#define FSL_IFC_FIR0		0x110
/* NAND Machine specific opcodes OP0-OP14*/
#define		IFC_NAND_FIR0_OP0_SHIFT		26
#define		IFC_NAND_FIR0_OP1_SHIFT		20
#define		IFC_NAND_FIR0_OP2_SHIFT		14
#define		IFC_NAND_FIR0_OP3_SHIFT		8
#define		IFC_NAND_FIR0_OP4_SHIFT		2
#define FSL_IFC_FIR1		0x114
#define		IFC_NAND_FIR1_OP5_SHIFT		26
#define		IFC_NAND_FIR1_OP6_SHIFT		20
#define		IFC_NAND_FIR1_OP7_SHIFT		14
#define		IFC_NAND_FIR1_OP8_SHIFT		8
#define FSL_IFC_NAND_CSEL	0x15c
#define		IFC_NAND_CSEL_SHIFT		26
#define FSL_IFC_NANDSEQ_STRT	0x164
#define		IFC_NAND_SEQ_STRT_FIR_STRT	0x80000000
/* NAND Event and Error Status Register */
#define FSL_IFC_NAND_EVTER_STAT	0x16c
#define		IFC_NAND_EVTER_STAT_OPC		0x80000000
#define		IFC_NAND_EVTER_STAT_FTOER	0x08000000
#define		IFC_NAND_EVTER_STAT_WPER	0x04000000
/* NAND Flash Page Read Completion Event Status Register */
#define FSL_IFC_PGRDCMPL_EVT_STAT 0x174
/* NAND Event and Error Enable Register (NAND_EVTER_EN) */
#define FSL_IFC_EVTER_EN	0x180
#define		IFC_NAND_EVTER_EN_OPC_EN	0x80000000
#define		IFC_NAND_EVTER_EN_PGRDCMPL_EN	0x20000000
#define		IFC_NAND_EVTER_EN_FTOER_EN	0x08000000
#define		IFC_NAND_EVTER_EN_WPER_EN	0x04000000

#define FSL_IFC_NAND_FSR	0x1e0
#define FSL_IFC_ECCSTAT(v)	(0x1e8 + (4 * v))
#define		IFC_NAND_EVTER_STAT_ECCER	0x02000000

/*
 * Instruction opcodes to be programmed
 * in FIR registers- 6bits
 */
enum ifc_nand_fir_opcodes {
	IFC_FIR_OP_NOP,
	IFC_FIR_OP_CA0,
	IFC_FIR_OP_CA1,
	IFC_FIR_OP_CA2,
	IFC_FIR_OP_CA3,
	IFC_FIR_OP_RA0,
	IFC_FIR_OP_RA1,
	IFC_FIR_OP_RA2,
	IFC_FIR_OP_RA3,
	IFC_FIR_OP_CMD0,
	IFC_FIR_OP_CMD1,
	IFC_FIR_OP_CMD2,
	IFC_FIR_OP_CMD3,
	IFC_FIR_OP_CMD4,
	IFC_FIR_OP_CMD5,
	IFC_FIR_OP_CMD6,
	IFC_FIR_OP_CMD7,
	IFC_FIR_OP_CW0,
	IFC_FIR_OP_CW1,
	IFC_FIR_OP_CW2,
	IFC_FIR_OP_CW3,
	IFC_FIR_OP_CW4,
	IFC_FIR_OP_CW5,
	IFC_FIR_OP_CW6,
	IFC_FIR_OP_CW7,
	IFC_FIR_OP_WBCD,
	IFC_FIR_OP_RBCD,
	IFC_FIR_OP_BTRD,
	IFC_FIR_OP_RDSTAT,
	IFC_FIR_OP_NWAIT,
	IFC_FIR_OP_WFR,
	IFC_FIR_OP_SBRD,
	IFC_FIR_OP_UA,
	IFC_FIR_OP_RB,
};

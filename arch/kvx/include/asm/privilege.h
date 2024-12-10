/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2024 Kalray Inc.
 */

#ifndef _ASM_KVX_PRIVILEGE_H
#define _ASM_KVX_PRIVILEGE_H

#include <asm/sys_arch.h>

/**
 * Privilege level stuff
 */

/*
 * When manipulating ring levels, we always use relative values. This means that
 * settings a resource owner to value 1 means "Current privilege level + 1.
 * Setting it to 0 means "Current privilege level"
 */
#define PL_CUR_PLUS_1	1
#define PL_CUR		0

/**
 * Syscall owner configuration
 */
#define SYO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(SYO, __field, __pl)

#define SYO_WFXL_VALUE(__pl) (SYO_WFXL_OWN(Q0, __pl) | \
			      SYO_WFXL_OWN(Q1, __pl) | \
			      SYO_WFXL_OWN(Q2, __pl) | \
			      SYO_WFXL_OWN(Q3, __pl))

#define SYO_WFXL_VALUE_PL_CUR_PLUS_1	SYO_WFXL_VALUE(PL_CUR_PLUS_1)
#define SYO_WFXL_VALUE_PL_CUR		SYO_WFXL_VALUE(PL_CUR)

/**
 * hardware trap owner configuration
 */
#define HTO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(HTO, __field, __pl)


#define HTO_WFXL_VALUE_BASE(__pl)	(HTO_WFXL_OWN(OPC, __pl) | \
					 HTO_WFXL_OWN(DMIS, __pl) | \
					 HTO_WFXL_OWN(PSYS, __pl) | \
					 HTO_WFXL_OWN(DSYS, __pl) | \
					 HTO_WFXL_OWN(NOMAP, __pl) | \
					 HTO_WFXL_OWN(PROT, __pl) | \
					 HTO_WFXL_OWN(W2CL, __pl) | \
					 HTO_WFXL_OWN(A2CL, __pl) | \
					 HTO_WFXL_OWN(VSFR, __pl) | \
					 HTO_WFXL_OWN(PLO, __pl))

/*
 * When alone on the processor, we need to request all traps or the processor
 * will die badly without any information at all by jumping to the more
 * privilege level even if nobody is there.
 */
#define HTO_WFXL_VALUE_PL_CUR_PLUS_1	(HTO_WFXL_VALUE_BASE(PL_CUR_PLUS_1) | \
					 HTO_WFXL_OWN(DECCG, PL_CUR_PLUS_1) | \
					 HTO_WFXL_OWN(SECCG, PL_CUR_PLUS_1) | \
					 HTO_WFXL_OWN(DE, PL_CUR_PLUS_1))

#define HTO_WFXL_VALUE_PL_CUR		HTO_WFXL_VALUE_BASE(PL_CUR)

/**
 * Interrupt owner configuration
 */
#define ITO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(ITO, __field, __pl)

#define ITO_WFXL_VALUE(__pl)	(ITO_WFXL_OWN(IT0, __pl) | \
				 ITO_WFXL_OWN(IT1, __pl) | \
				 ITO_WFXL_OWN(IT2, __pl) | \
				 ITO_WFXL_OWN(IT3, __pl) | \
				 ITO_WFXL_OWN(IT4, __pl) | \
				 ITO_WFXL_OWN(IT5, __pl) | \
				 ITO_WFXL_OWN(IT6, __pl) | \
				 ITO_WFXL_OWN(IT7, __pl) | \
				 ITO_WFXL_OWN(IT8, __pl) | \
				 ITO_WFXL_OWN(IT9, __pl) | \
				 ITO_WFXL_OWN(IT10, __pl) | \
				 ITO_WFXL_OWN(IT11, __pl) | \
				 ITO_WFXL_OWN(IT12, __pl) | \
				 ITO_WFXL_OWN(IT13, __pl) | \
				 ITO_WFXL_OWN(IT14, __pl) | \
				 ITO_WFXL_OWN(IT15, __pl))

#define ITO_WFXL_VALUE_PL_CUR_PLUS_1	ITO_WFXL_VALUE(PL_CUR_PLUS_1)
#define ITO_WFXL_VALUE_PL_CUR		ITO_WFXL_VALUE(PL_CUR)

#define ITO_WFXM_OWN(__field, __pl) \
	SFR_SET_VAL_WFXM(ITO, __field, __pl)

#define ITO_WFXM_VALUE(__pl) (ITO_WFXM_OWN(IT16, __pl) | \
			      ITO_WFXM_OWN(IT17, __pl) | \
			      ITO_WFXM_OWN(IT18, __pl) | \
			      ITO_WFXM_OWN(IT19, __pl) | \
			      ITO_WFXM_OWN(IT20, __pl) | \
			      ITO_WFXM_OWN(IT21, __pl) | \
			      ITO_WFXM_OWN(IT22, __pl) | \
			      ITO_WFXM_OWN(IT23, __pl) | \
			      ITO_WFXM_OWN(IT24, __pl) | \
			      ITO_WFXM_OWN(IT25, __pl) | \
			      ITO_WFXM_OWN(IT26, __pl) | \
			      ITO_WFXM_OWN(IT27, __pl) | \
			      ITO_WFXM_OWN(IT28, __pl) | \
			      ITO_WFXM_OWN(IT29, __pl) | \
			      ITO_WFXM_OWN(IT30, __pl) | \
			      ITO_WFXM_OWN(IT31, __pl))

#define ITO_WFXM_VALUE_PL_CUR_PLUS_1	ITO_WFXM_VALUE(PL_CUR_PLUS_1)
#define ITO_WFXM_VALUE_PL_CUR		ITO_WFXM_VALUE(PL_CUR)

/**
 * Debug Owner configuration
 */

#define DO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(DO, __field, __pl)

#if defined(CONFIG_ARCH_COOLIDGE_V1)
#define DO_WFXL_VALUE(__pl)	(DO_WFXL_OWN(B0, __pl) | \
				 DO_WFXL_OWN(B1, __pl) | \
				 DO_WFXL_OWN(W0, __pl) | \
				 DO_WFXL_OWN(W1, __pl))

#define DO_WFXL_VALUE_PL_CUR_PLUS_1	DO_WFXL_VALUE(PL_CUR_PLUS_1)
#elif defined(CONFIG_ARCH_COOLIDGE_V2)
#define DO_WFXL_VALUE(__pl)	(DO_WFXL_OWN(B2, __pl) | \
				 DO_WFXL_OWN(B3, __pl) | \
				 DO_WFXL_OWN(W2, __pl) | \
				 DO_WFXL_OWN(W3, __pl))

#define DO_WFXL_VALUE_PL_CUR_PLUS_1	(DO_WFXL_VALUE(PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(B0, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(B1, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(W0, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(W1, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(BI0, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(BI1, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(BI2, PL_CUR_PLUS_1) | \
					 DO_WFXL_OWN(BI3, PL_CUR_PLUS_1))


#else
#error Unsupported arch
#endif

#define DO_WFXL_VALUE_PL_CUR		DO_WFXL_VALUE(PL_CUR)

/**
 * Misc owner configuration
 */
#define MO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(MO, __field, __pl)

#define MO_WFXL_VALUE(__pl)	(MO_WFXL_OWN(MMI, __pl) | \
				 MO_WFXL_OWN(RFE, __pl) | \
				 MO_WFXL_OWN(STOP, __pl) | \
				 MO_WFXL_OWN(SYNC, __pl) | \
				 MO_WFXL_OWN(PCR, __pl) | \
				 MO_WFXL_OWN(MSG, __pl) | \
				 MO_WFXL_OWN(MEN, __pl) | \
				 MO_WFXL_OWN(MES, __pl) | \
				 MO_WFXL_OWN(CSIT, __pl) | \
				 MO_WFXL_OWN(T0, __pl) | \
				 MO_WFXL_OWN(T1, __pl) | \
				 MO_WFXL_OWN(WD, __pl) | \
				 MO_WFXL_OWN(PM0, __pl) | \
				 MO_WFXL_OWN(PM1, __pl) | \
				 MO_WFXL_OWN(PM2, __pl) | \
				 MO_WFXL_OWN(PM3, __pl))

#define MO_WFXL_VALUE_PL_CUR_PLUS_1	MO_WFXL_VALUE(PL_CUR_PLUS_1)
#define MO_WFXL_VALUE_PL_CUR		MO_WFXL_VALUE(PL_CUR)

#define MO_WFXM_OWN(__field, __pl) \
	SFR_SET_VAL_WFXM(MO, __field, __pl)

#if defined(CONFIG_ARCH_COOLIDGE_V1)
#define MO_WFXM_VALUE(__pl)	(MO_WFXM_OWN(PMIT, __pl))
#elif defined(CONFIG_ARCH_COOLIDGE_V2)
#define MO_WFXM_VALUE(__pl)	(MO_WFXM_OWN(PMIT, __pl) | \
				 MO_WFXM_OWN(COMM, __pl) | \
				 MO_WFXM_OWN(TPCM, __pl) | \
				 MO_WFXM_OWN(DISW, __pl) | \
				 MO_WFXM_OWN(PM4, __pl) | \
				 MO_WFXM_OWN(PM5, __pl) | \
				 MO_WFXM_OWN(PM6, __pl) | \
				 MO_WFXM_OWN(PM7, __pl) | \
				 MO_WFXM_OWN(SRHPC, __pl))
#else
#error Unsupported arch
#endif

#define MO_WFXM_VALUE_PL_CUR_PLUS_1	MO_WFXM_VALUE(PL_CUR_PLUS_1)
#define MO_WFXM_VALUE_PL_CUR		MO_WFXM_VALUE(PL_CUR)

/**
 * $ps owner configuration
 */
#define PSO_WFXL_OWN(__field, __pl) \
	SFR_SET_VAL_WFXL(PSO, __field, __pl)

#define PSO_WFXL_BASE_VALUE(__pl)	(PSO_WFXL_OWN(PL0, __pl) | \
					 PSO_WFXL_OWN(PL1, __pl) | \
					 PSO_WFXL_OWN(ET, __pl) | \
					 PSO_WFXL_OWN(HTD, __pl) | \
					 PSO_WFXL_OWN(IE, __pl) | \
					 PSO_WFXL_OWN(HLE, __pl) | \
					 PSO_WFXL_OWN(SRE, __pl) | \
					 PSO_WFXL_OWN(DAUS, __pl) | \
					 PSO_WFXL_OWN(ICE, __pl) | \
					 PSO_WFXL_OWN(USE, __pl) | \
					 PSO_WFXL_OWN(DCE, __pl) | \
					 PSO_WFXL_OWN(MME, __pl) | \
					 PSO_WFXL_OWN(IL0, __pl) | \
					 PSO_WFXL_OWN(IL1, __pl) | \
					 PSO_WFXL_OWN(VS0, __pl))
/* Request additionnal VS1 when alone */
#define PSO_WFXL_VALUE_PL_CUR_PLUS_1	(PSO_WFXL_BASE_VALUE(PL_CUR_PLUS_1) | \
					 PSO_WFXL_OWN(VS1, PL_CUR_PLUS_1))
#define PSO_WFXL_VALUE_PL_CUR		PSO_WFXL_BASE_VALUE(PL_CUR)

#define PSO_WFXM_OWN(__field, __pl) \
	SFR_SET_VAL_WFXM(PSO, __field, __pl)

#define PSO_WFXM_VALUE(__pl)	(PSO_WFXM_OWN(V64, __pl) | \
				 PSO_WFXM_OWN(L2E, __pl)  | \
				 PSO_WFXM_OWN(SME, __pl)  | \
				 PSO_WFXM_OWN(SMR, __pl)  | \
				 PSO_WFXM_OWN(PMJ0, __pl) | \
				 PSO_WFXM_OWN(PMJ1, __pl) | \
				 PSO_WFXM_OWN(PMJ2, __pl) | \
				 PSO_WFXM_OWN(PMJ3, __pl) | \
				 PSO_WFXM_OWN(MMUP, __pl))

/* Request additionnal VS1 */
#define PSO_WFXM_VALUE_PL_CUR_PLUS_1	PSO_WFXM_VALUE(PL_CUR_PLUS_1)
#define PSO_WFXM_VALUE_PL_CUR		PSO_WFXM_VALUE(PL_CUR)

#endif /* _ASM_KVX_PRIVILEGE_H */

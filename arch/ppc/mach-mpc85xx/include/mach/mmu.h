/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef _MPC85XX_MMU_H_
#define _MPC85XX_MMU_H_

#ifdef CONFIG_E500
#include <asm/mmu.h>

#ifndef __ASSEMBLY__
extern void e500_set_tlb(u8 tlb, u32 epn, u64 rpn, u8 perms, u8 wimge,
		u8 ts, u8 esel, u8 tsize, u8 iprot);
extern void e500_disable_tlb(u8 esel);
extern void e500_invalidate_tlb(u8 tlb);
extern void e500_init_tlbs(void);
extern int e500_find_tlb_idx(void *addr, u8 tlbsel);
extern void e500_init_used_tlb_cams(void);

extern unsigned int e500_setup_ddr_tlbs(unsigned int memsize_in_meg);
extern void e500_write_tlb(u32 _mas0, u32 _mas1, u32 _mas2, u32 _mas3,
			u32 _mas7);

#define FSL_SET_TLB_ENTRY(_tlb, _epn, _rpn, _perms, _wimge, _ts, _esel, _sz,\
			_iprot) \
	{ .mas0 = FSL_BOOKE_MAS0(_tlb, _esel, 0), \
	  .mas1 = FSL_BOOKE_MAS1(1, _iprot, 0, _ts, _sz), \
	  .mas2 = FSL_BOOKE_MAS2(_epn, _wimge), \
	  .mas3 = FSL_BOOKE_MAS3(_rpn, 0, _perms), \
	  .mas7 = FSL_BOOKE_MAS7(_rpn), }

struct fsl_e_tlb_entry {
	u32	mas0;
	u32	mas1;
	u32	mas2;
	u32	mas3;
	u32	mas7;
};
extern struct fsl_e_tlb_entry tlb_table[];
extern int num_tlb_entries;
#endif
#endif
#endif /* _MPC85XX_MMU_H_ */

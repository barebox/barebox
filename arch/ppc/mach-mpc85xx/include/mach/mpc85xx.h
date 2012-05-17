/*
 * Copyright 2004, 2007 Freescale Semiconductor.
 * Copyright(c) 2003 Motorola Inc.
 */

#ifndef __MPC85xx_H__
#define __MPC85xx_H__

/* define for common ppc_asm.tmpl */
#define EXC_OFF_SYS_RESET	0x100	/* System reset */
#define _START_OFFSET		0

#ifndef __ASSEMBLY__
int fsl_l2_cache_init(void);
int fsl_cpu_numcores(void);

phys_size_t fsl_get_effective_memsize(void);

#endif /* __ASSEMBLY__ */

#define END_OF_MEM (fsl_get_effective_memsize())

#endif	/* __MPC85xx_H__ */

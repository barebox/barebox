/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_COMMON_H
#define _ASM_KVX_COMMON_H

#ifndef __ASSEMBLY__

/*
 * Magic value passed in r0 to indicate valid parameters from FSBL when booting
 * If $r0 contains this value, then $r1 contains dtb pointer.
 */
#define FSBL_PARAM_MAGIC	0x31564752414C414BULL /* KALARGV1 */
#define LINUX_BOOT_PARAM_MAGIC	0x31564752414E494CULL /* LINARGV1 */

extern char _exception_start;
extern char __end;
extern void *boot_dtb;

void kvx_start_barebox(void);
void kvx_lowlevel_setup(unsigned long r0, void *dtb_ptr);

#endif

#endif	/* _ASM_KVX_COMMON_H */


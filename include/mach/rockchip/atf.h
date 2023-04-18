/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ATF_H
#define __MACH_ATF_H

/* First usable DRAM address. Lower mem is used for ATF and OP-TEE */
#define RK3399_DRAM_BOTTOM		0xa00000
#define RK3568_DRAM_BOTTOM		0xa00000
#define RK3588_DRAM_BOTTOM		0xa00000

/* OP-TEE expects to be loaded here */
#define RK3399_OPTEE_LOAD_ADDRESS	0x200000
#define RK3568_OPTEE_LOAD_ADDRESS	0x200000
#define RK3588_OPTEE_LOAD_ADDRESS	0x200000

/*
 * board lowlevel code should relocate barebox here. This is where
 * OP-TEE jumps to after initialization.
 */
#define RK3399_BAREBOX_LOAD_ADDRESS	(RK3399_DRAM_BOTTOM + 1024*1024)
#define RK3568_BAREBOX_LOAD_ADDRESS	(RK3568_DRAM_BOTTOM + 1024*1024)
#define RK3588_BAREBOX_LOAD_ADDRESS	(RK3588_DRAM_BOTTOM + 1024*1024)

#ifndef __ASSEMBLY__
#ifdef CONFIG_ARCH_ROCKCHIP_ATF
void rk3399_atf_load_bl31(void *fdt);
void rk3568_atf_load_bl31(void *fdt);
void rk3588_atf_load_bl31(void *fdt);
#else
static inline void rk3399_atf_load_bl31(void *fdt) { }
static inline void rk3568_atf_load_bl31(void *fdt) { }
static inline void rk3588_atf_load_bl31(void *fdt) { }
#endif
#endif

void __noreturn rk3568_barebox_entry(void *fdt);
void __noreturn rk3588_barebox_entry(void *fdt);

#endif /* __MACH_ATF_H */

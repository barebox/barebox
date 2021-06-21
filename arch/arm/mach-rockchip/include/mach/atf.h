#ifndef __MACH_ATF_H
#define __MACH_ATF_H

/* First usable DRAM address. Lower mem is used for ATF and OP-TEE */
#define RK3568_DRAM_BOTTOM		0xa00000

/* OP-TEE expects to be loaded here */
#define RK3568_OPTEE_LOAD_ADDRESS	0x200000

/*
 * board lowlevel code should relocate barebox here. This is where
 * OP-TEE jumps to after initialization.
 */
#define RK3568_BAREBOX_LOAD_ADDRESS	(RK3568_DRAM_BOTTOM + 1024*1024)

void rk3568_atf_load_bl31(void *fdt);

#endif /* __MACH_ATF_H */

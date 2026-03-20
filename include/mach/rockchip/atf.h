/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ATF_H
#define __MACH_ATF_H

/* The first 10MiB of DRAM are used by the TF-A */
#define ROCKCHIP_DRAM_TFA_CARVE_OUT	0xa00000

#define RK3399_DRAM_START		0x0
#define RK3562_DRAM_START		0x0
#define RK3568_DRAM_START		0x0
#define RK3576_DRAM_START		0x40000000
#define RK3588_DRAM_START		0x0

/*
 * The tee.bin image has an OP-TEE specific header that describes the
 * initial load address and size. Unfortunately, the vendor blobs are in the
 * tee-raw.bin format, which omits the header. We thus hardcode here the
 * fallback addresses that should be used when barebox encounters
 * tee-raw.bin instead of tee.bin.
 *
 * The values are taken from rkbin/RKTRUST/RK3*.ini: [BL32_OPTION] ADDR
 */
#define RK3399_OPTEE_LOAD_ADDRESS	0x8400000
#define RK3562_OPTEE_LOAD_ADDRESS	0x8400000
#define RK3568_OPTEE_LOAD_ADDRESS	0x8400000
#define RK3576_OPTEE_LOAD_ADDRESS	0x8400000
#define RK3588_OPTEE_LOAD_ADDRESS	0x8400000

/*
 * Hopefully for future platforms, the vendor binaries would use the image
 * with an OP-TEE header and we can just set the load address for new SoCs
 * to below macro to enforce that only tee.bin is used.
 */
#define ROCKCHIP_OPTEE_HEADER_REQUIRED	0

/*
 * board lowlevel code should relocate barebox here. This is where
 * OP-TEE jumps to after initialization.
 */
#define RK3399_BAREBOX_LOAD_ADDRESS	(RK3399_DRAM_START + ROCKCHIP_DRAM_TFA_CARVE_OUT + 1024*1024)
#define RK3562_BAREBOX_LOAD_ADDRESS	(RK3562_DRAM_START + ROCKCHIP_DRAM_TFA_CARVE_OUT + 1024*1024)
#define RK3568_BAREBOX_LOAD_ADDRESS	(RK3568_DRAM_START + ROCKCHIP_DRAM_TFA_CARVE_OUT + 1024*1024)
#define RK3576_BAREBOX_LOAD_ADDRESS	(RK3576_DRAM_START + ROCKCHIP_DRAM_TFA_CARVE_OUT + 1024*1024)
#define RK3588_BAREBOX_LOAD_ADDRESS	(RK3588_DRAM_START + ROCKCHIP_DRAM_TFA_CARVE_OUT + 1024*1024)

void __noreturn rk3562_barebox_entry(void *fdt);
void __noreturn rk3568_barebox_entry(void *fdt);
void __noreturn rk3576_barebox_entry(void *fdt);
void __noreturn rk3588_barebox_entry(void *fdt);

#endif /* __MACH_ATF_H */

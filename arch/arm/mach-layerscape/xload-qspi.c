// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm-generic/sections.h>
#include <asm/cache.h>
#include <mach/xload.h>
#include <mach/layerscape.h>

/*
 * The offset of the 2nd stage image in the output file. This must match with the
 * offset the pblimage tool puts barebox to.
 */
#define BAREBOX_START	(128 * 1024)

int ls1046a_qspi_start_image(unsigned long r0, unsigned long r1,
					     unsigned long r2)
{
	void *qspi_reg_base = IOMEM(LSCH2_QSPI0_BASE_ADDR);
	void *membase = (void *)LS1046A_DDR_SDRAM_BASE;
	void *qspi_mem_base = IOMEM(0x40000000);
	void (*barebox)(unsigned long, unsigned long, unsigned long) = membase;

	/* Switch controller into little endian mode */
	out_be32(qspi_reg_base, 0x000f400c);

	memcpy(membase, qspi_mem_base + BAREBOX_START, barebox_image_size);

	sync_caches_for_execution();

	printf("Starting barebox\n");

	barebox(r0, r1, r2);

	printf("failed\n");

	return -EIO;
}

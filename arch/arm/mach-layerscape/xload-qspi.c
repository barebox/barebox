// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm-generic/sections.h>
#include <asm/cache.h>
#include <mach/layerscape/xload.h>
#include <mach/layerscape/layerscape.h>

/*
 * The offset of the 2nd stage image in the output file. This must match with the
 * offset the pblimage tool puts barebox to.
 */
#define BAREBOX_START	(128 * 1024)

struct layerscape_base_addr {
	void *qspi_reg_base;
	void *membase;
	void *qspi_mem_base;
};

static int layerscape_qspi_start_image(struct layerscape_base_addr *base)
{
	void (*barebox)(void) = base->membase;

	/* Switch controller into little endian mode */
	out_be32(base->qspi_reg_base, 0x000f400c);

	memcpy(base->membase, base->qspi_mem_base + BAREBOX_START, barebox_image_size);

	sync_caches_for_execution();

	printf("Starting barebox\n");

	barebox();

	printf("failed\n");

	return -EIO;
}

int ls1046a_qspi_start_image(void)
{
	struct layerscape_base_addr base;

	base.qspi_reg_base = IOMEM(LSCH2_QSPI0_BASE_ADDR);
	base.membase = IOMEM(LS1046A_DDR_SDRAM_BASE);
	base.qspi_mem_base = IOMEM(0x40000000);

	return layerscape_qspi_start_image(&base);
}

int ls1021a_qspi_start_image(void)
{
	struct layerscape_base_addr base;

	base.qspi_reg_base = IOMEM(LSCH2_QSPI0_BASE_ADDR);
	base.membase = IOMEM(LS1021A_DDR_SDRAM_BASE);
	base.qspi_mem_base = IOMEM(0x40000000);

	return layerscape_qspi_start_image(&base);
}

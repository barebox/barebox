// SPDX-License-Identifier: GPL-2.0-only

#include <asm/barebox-arm.h>
#include <init.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/imx9-regs.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/scratch.h>
#include <memory.h>
#include <tee/optee.h>
#include <pbl.h>

struct imx_scratch_space {
	union {
		/* Internal buffer, never use directly! */
		u8 __buf[CONFIG_SCRATCH_SIZE];
		struct {
			u32 bootrom_log[128];
			/* reserve for bootrom log */
			u32 reserved[128];
			struct optee_header optee_hdr;
			/* FDT needs an 8 byte alignment, always last element! */
			u8 fdt[] __aligned(8);
		};
	};
};
/* Ensure that 'fdt' size fits into OP-TEE DTB_MAX_SIZE size */
static_assert(sizeof(struct imx_scratch_space) -
	      offsetof(struct imx_scratch_space, fdt) >= PBL_OPTEE_DTB_MAX_SIZE);

static struct imx_scratch_space *scratch;

void imx_init_scratch_space(ulong endmem, bool zero_init)
{
	scratch = (void *)arm_mem_scratch(endmem);

	if (zero_init)
		memset(scratch, 0, sizeof(*scratch));
}

void imx8m_init_scratch_space(int ddr_buswidth, bool zero_init)
{
	ulong endmem = MX8M_DDR_CSD1_BASE_ADDR +
		imx8m_barebox_earlymem_size(ddr_buswidth);
	imx_init_scratch_space(endmem, zero_init);
}

void imx93_init_scratch_space(bool zero_init)
{
	ulong endmem = MX9_DDR_CSD1_BASE_ADDR + imx9_ddrc_sdram_size();
	imx_init_scratch_space(endmem, zero_init);
}

void imx8m_scratch_save_bootrom_log(const u32 *rom_log)
{
	size_t sz = sizeof(scratch->bootrom_log);

	if (!scratch) {
		pr_err("No scratch area initialized, skip saving bootrom log");
		return;
	}

	pr_debug("Saving bootrom log to scratch area 0x%p\n", &scratch->bootrom_log);

	memcpy(scratch->bootrom_log, rom_log, sz);
}

const u32 *imx8m_scratch_get_bootrom_log(void)
{
	if (!scratch) {
		if (IN_PBL)
			return ERR_PTR(-EINVAL);
		else
			scratch = (void *)arm_mem_scratch_get();
	}

	return scratch->bootrom_log;
}

void imx_scratch_save_optee_hdr(const struct optee_header *hdr)
{
	size_t sz = sizeof(*hdr);

	if (!scratch) {
		pr_err("No scratch area initialized, skip saving optee-hdr");
		return;
	}

	pr_debug("Saving optee-hdr to scratch area 0x%p\n", &scratch->optee_hdr);

	memcpy(&scratch->optee_hdr, hdr, sz);
}

const struct optee_header *imx_scratch_get_optee_hdr(void)
{
	if (!scratch) {
		if (IN_PBL)
			return ERR_PTR(-EINVAL);
		else
			scratch = (void *)arm_mem_scratch_get();
	}

	return &scratch->optee_hdr;
}

u8 *imx_scratch_get_fdt(unsigned int *fdt_sz)
{
	unsigned int sz;

	if (!scratch) {
		if (IN_PBL)
			return ERR_PTR(-EINVAL);
		else
			scratch = (void *)arm_mem_scratch_get();
	}

	if (PBL_OPTEE_DTB_MAX_SIZE == 0)
		return NULL;

	sz = sizeof(struct imx_scratch_space) -
	     offsetof(struct imx_scratch_space, fdt);

	if (sz > PBL_OPTEE_DTB_MAX_SIZE)
		sz = PBL_OPTEE_DTB_MAX_SIZE;

	if (fdt_sz)
		*fdt_sz = sz;

	return scratch->fdt;
}

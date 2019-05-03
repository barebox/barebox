// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <init.h>
#include <envfs.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <soc/fsl/immap_lsch2.h>

static int tqmls1046a_mem_init(void)
{
	if (!of_machine_is_compatible("tqc,tqmls1046a"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, SZ_2G);

        return 0;
}
mem_initcall(tqmls1046a_mem_init);

static int tqmls1046a_postcore_init(void)
{
	struct ccsr_scfg *scfg = IOMEM(LSCH2_SCFG_ADDR);

	if (!of_machine_is_compatible("tqc,tqmls1046a"))
		return 0;

	defaultenv_append_directory(defaultenv_tqmls1046a);

	/* Configure iomux for i2c4 */
	out_be32(&scfg->rcwpmuxcr0, 0x3300);

	return 0;
}

postcore_initcall(tqmls1046a_postcore_init);

// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <init.h>
#include <restart.h>
#include <asm/io.h>
#include <soc/fsl/immap_lsch2.h>
#include <soc/fsl/fsl_immap.h>
#include <mach/layerscape/layerscape.h>

static void ls102xa_restart(struct restart_handler *rst)
{
	void __iomem *rcr = IOMEM(LSCH2_RST_ADDR);

	/* Set RESET_REQ bit */
	setbits_be32(rcr, 0x2);

	mdelay(100);

	hang();
}

void ls1021a_restart_register_feature(void)
{
	restart_handler_register_fn("soc-reset", ls102xa_restart);
}

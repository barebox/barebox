// SPDX-License-Identifier: GPL-2.0-only

#include <mach/imx/tzasc.h>
#include <linux/bitops.h>
#include <mach/imx/imx8m-regs.h>
#include <io.h>

#define GPR_TZASC_EN					BIT(0)
#define GPR_TZASC_ID_SWAP_BYPASS		BIT(1)
#define GPR_TZASC_EN_LOCK				BIT(16)
#define GPR_TZASC_ID_SWAP_BYPASS_LOCK	BIT(17)

#define MX8M_TZASC_REGION_ATTRIBUTES_0		(MX8M_TZASC_BASE_ADDR + 0x108)
#define MX8M_TZASC_REGION_ATTRIBUTES_0_SP	GENMASK(31, 28)

static void enable_tzc380(bool bypass_id_swap, bool bypass_id_swap_lock)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	/* Enable TZASC and lock setting */
	setbits_le32(&gpr[10], GPR_TZASC_EN);
	setbits_le32(&gpr[10], GPR_TZASC_EN_LOCK);

	/*
	 * According to TRM, TZASC_ID_SWAP_BYPASS should be set in
	 * order to avoid AXI Bus errors when GPU is in use
	 */
	if (bypass_id_swap)
		setbits_le32(&gpr[10], GPR_TZASC_ID_SWAP_BYPASS);

	/*
	 * imx8mn and imx8mp implements the lock bit for
	 * TZASC_ID_SWAP_BYPASS, enable it to lock settings
	 */
	if (bypass_id_swap_lock)
		setbits_le32(&gpr[10], GPR_TZASC_ID_SWAP_BYPASS_LOCK);

	/*
	 * set Region 0 attribute to allow secure and non-secure
	 * read/write permission. Found some masters like usb dwc3
	 * controllers can't work with secure memory.
	 */
	writel(MX8M_TZASC_REGION_ATTRIBUTES_0_SP,
		   MX8M_TZASC_REGION_ATTRIBUTES_0);
}

void imx8mq_tzc380_init(void)
{
	enable_tzc380(false, false);
}

void imx8mm_tzc380_init(void)
{
	enable_tzc380(true, false);
}

void imx8mn_tzc380_init(void) __alias(imx8mp_tzc380_init);
void imx8mp_tzc380_init(void)
{
	enable_tzc380(true, true);
}

bool tzc380_is_enabled(void)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	return (readl(&gpr[10]) & (GPR_TZASC_EN | GPR_TZASC_EN_LOCK))
		== (GPR_TZASC_EN | GPR_TZASC_EN_LOCK);
}

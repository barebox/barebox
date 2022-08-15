// SPDX-License-Identifier: GPL-2.0-only

#include <mach/tzasc.h>
#include <linux/bitops.h>
#include <mach/imx8m-regs.h>
#include <io.h>

#define GPR_TZASC_EN		BIT(0)
#define GPR_TZASC_SWAP_ID	BIT(1)
#define GPR_TZASC_EN_LOCK	BIT(16)

static void enable_tzc380(bool bypass_id_swap)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	/* Enable TZASC and lock setting */
	setbits_le32(&gpr[10], GPR_TZASC_EN);
	setbits_le32(&gpr[10], GPR_TZASC_EN_LOCK);
	if (bypass_id_swap)
		setbits_le32(&gpr[10], BIT(1));
	/*
	 * set Region 0 attribute to allow secure and non-secure
	 * read/write permission. Found some masters like usb dwc3
	 * controllers can't work with secure memory.
	 */
	writel(0xf0000000, MX8M_TZASC_BASE_ADDR + 0x108);
}

void imx8mq_tzc380_init(void)
{
	enable_tzc380(false);
}

void imx8mn_tzc380_init(void) __alias(imx8mm_tzc380_init);
void imx8mp_tzc380_init(void) __alias(imx8mm_tzc380_init);
void imx8mm_tzc380_init(void)
{
	enable_tzc380(true);
}

bool tzc380_is_enabled(void)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	return (readl(&gpr[10]) & (GPR_TZASC_EN | GPR_TZASC_EN_LOCK))
		== (GPR_TZASC_EN | GPR_TZASC_EN_LOCK);
}

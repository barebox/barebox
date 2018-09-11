#ifndef __MACH_IMX23_H
#define __MACH_IMX23_H

#include <linux/bitfield.h>
#include <io.h>

#define DRAM_CTL14_CS0_EN	BIT(0)
#define DRAM_CTL14_CS1_EN	BIT(1)
#define DRAM_CTL11_COLUMNS_DIFF	GENMASK(10, 8)
#define DRAM_CTL10_ROWS_DIFF	GENMASK(18, 16)

#define DRAM_CTL(n)	(IMX_SDRAMC_BASE + 4 * (n))

static inline u32 imx23_get_memsize(void)
{
	u32 ctl10 = readl(DRAM_CTL(10));
	u32 ctl11 = readl(DRAM_CTL(11));
	u32 ctl14 = readl(DRAM_CTL(14));
	int rows, columns, banks = 4, cs0, cs1;

	columns = 12 - FIELD_GET(DRAM_CTL11_COLUMNS_DIFF, ctl11);
	rows = 13 - FIELD_GET(DRAM_CTL10_ROWS_DIFF, ctl10);
	cs0 = FIELD_GET(DRAM_CTL14_CS0_EN, ctl14);
	cs1 = FIELD_GET(DRAM_CTL14_CS1_EN, ctl14);

	return (1 << columns) * (1 << rows) * banks * (cs0 + cs1);
}

#endif /* __MACH_IMX23_H */
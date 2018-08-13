#ifndef __MACH_IMX28_H
#define __MACH_IMX28_H

#include <linux/bitfield.h>
#include <io.h>

#define DRAM_CTL29_CS0_EN	BIT(24)
#define DRAM_CTL29_CS1_EN	BIT(25)
#define DRAM_CTL29_COLUMNS_DIFF	GENMASK(18, 16)
#define DRAM_CTL29_ROWS_DIFF	GENMASK(10, 8)
#define DRAM_CTL31_EIGHT_BANKS	BIT(16)

#define DRAM_CTL(n)	(IMX_SDRAMC_BASE + 4 * (n))

static inline u32 imx28_get_memsize(void)
{
	u32 ctl29 = readl(DRAM_CTL(29));
	u32 ctl31 = readl(DRAM_CTL(31));
	int rows, columns, banks, cs0, cs1;

	columns = 12 - FIELD_GET(DRAM_CTL29_COLUMNS_DIFF, ctl29);
	rows = 15 - FIELD_GET(DRAM_CTL29_ROWS_DIFF, ctl29);
	banks = FIELD_GET(DRAM_CTL31_EIGHT_BANKS, ctl31) ? 8 : 4;
	cs0 = FIELD_GET(DRAM_CTL29_CS0_EN, ctl29);
	cs1 = FIELD_GET(DRAM_CTL29_CS1_EN, ctl29);

	return (1 << columns) * (1 << rows) * banks * (cs0 + cs1);
}

#endif /* __MACH_IMX28_H */
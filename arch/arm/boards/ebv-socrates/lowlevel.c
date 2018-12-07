#define SECT(name) __attribute__((section("ebv_socrates_" #name))) name

#include "sdram_config.h"
#include "pinmux_config.c"
#include "pll_config.h"
#include "sequencer_defines.h"
#include "sequencer_auto.h"
#include "sequencer_auto_inst_init.c"
#include "sequencer_auto_ac_init.c"
#include "iocsr_config_cyclone5.c"

#include <mach/lowlevel.h>

static inline void ledon(void)
{
	u32 val;

	val = readl(0xFF708000);
	val &= ~(1 << 28);
	writel(val, 0xFF708000);

	val = readl(0xFF708004);
	val |= 1 << 28;
	writel(val, 0xFF708004);
}

static inline void ledoff(void)
{
	u32 val;

	val = readl(0xFF708000);
	val |= 1 << 28;
	writel(val, 0xFF708000);

	val = readl(0xFF708004);
	val |= 1 << 28;
	writel(val, 0xFF708004);
}

SOCFPGA_C5_ENTRY(start_socfpga_socrates, socfpga_cyclone5_socrates, SZ_1G);
SOCFPGA_C5_XLOAD_ENTRY(start_socfpga_socrates_xload, SZ_1G);

#define S5PCXX_CLOCK_REFERENCE 24000000

#define set_pll(mdiv, pdiv, sdiv)	(1<<31 | mdiv<<16 | pdiv<<8 | sdiv)

#define BOARD_APLL_VAL	set_pll(0x7d, 0x3, 0x1)
#define BOARD_MPLL_VAL	set_pll(0x29b, 0xc, 0x1)
#define BOARD_EPLL_VAL	set_pll(0x60, 0x6, 0x2)
#define BOARD_VPLL_VAL	set_pll(0x6c, 0x6, 0x3)

#define BOARD_CLK_DIV0_MASK	0xFFFFFFFF
#define BOARD_CLK_DIV0_VAL	0x14131440
#define BOARD_APLL_LOCKTIME	0x2cf

#define S5P_DRAM_WR	3
#define S5P_DRAM_CAS	4
#define DMC_TIMING_AREF	0x00000618
#define DMC_TIMING_ROW	0x2B34438A
#define DMC_TIMING_DATA	0x24240000
#define DMC_TIMING_PWR	0x0BDC0343

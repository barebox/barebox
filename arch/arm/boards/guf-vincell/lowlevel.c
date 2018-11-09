#include <common.h>
#include <debug_ll.h>
#include <io.h>
#include <init.h>
#include <mach/imx53-regs.h>
#include <mach/clock-imx51_53.h>
#include <mach/imx5.h>
#include <mach/iomux-v3.h>
#include <mach/esdctl-v4.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/cache.h>
#include <mach/xload.h>

#define IOMUX_PADCTL_DDRI_DDR (1 << 9)

#define IOMUX_PADCTL_DDRDSE(x)	((x) << 19)
#define IOMUX_PADCTL_DDRSEL(x)	((x) << 25)

#define IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3		0x554
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3	0x558
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2		0x560
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1	0x564
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2	0x568
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1	0x570
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS		0x574
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0	0x578
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0	0x57c
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0	0x580
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS		0x588
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1	0x590
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1		0x594
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0		0x584
#define IOMUXC_SW_PAD_CTL_GRP_ADDDS		0x6f0
#define IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL	0x6f4
#define IOMUXC_SW_PAD_CTL_GRP_DDRPKE		0x6fc
#define IOMUXC_SW_PAD_CTL_GRP_DDRHYS		0x710
#define IOMUXC_SW_PAD_CTL_GRP_DDRMODE		0x714
#define IOMUXC_SW_PAD_CTL_GRP_B0DS		0x718
#define IOMUXC_SW_PAD_CTL_GRP_B1DS		0x71c
#define IOMUXC_SW_PAD_CTL_GRP_CTLDS		0x720
#define IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE		0x724
#define IOMUXC_SW_PAD_CTL_GRP_B2DS		0x728
#define IOMUXC_SW_PAD_CTL_GRP_B3DS		0x72c


static void configure_dram_iomux(void)
{
	void __iomem *iomux = (void *)MX53_IOMUXC_BASE_ADDR;
	u32 r1, r2, r4, r5, r6;

	/* define the INPUT mode for DRAM_D[31:0] */
	writel(0, iomux + IOMUXC_SW_PAD_CTL_GRP_DDRMODE);

	/*
	 * define the INPUT mode for SDQS[3:0]
	 * (Freescale's documentation suggests DDR-mode for the
	 * control line, but their source actually uses CMOS)
	 */
	writel(IOMUX_PADCTL_DDRI_DDR, iomux + IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL);

	r1 = IOMUX_PADCTL_DDRDSE(5);
	r2 = IOMUX_PADCTL_DDRDSE(5) | PAD_CTL_PUE;
	r4 = IOMUX_PADCTL_DDRSEL(2);
	r5 = IOMUX_PADCTL_DDRDSE(5) | PAD_CTL_PKE | PAD_CTL_PUE | IOMUX_PADCTL_DDRI_DDR | PAD_CTL_PUS_47K_UP;
	r6 = IOMUX_PADCTL_DDRDSE(4);

	/*
	 * this will adisable the Pull/Keeper for DRAM_x pins EXCEPT,
	 * for DRAM_SDQS[3:0] and DRAM_SDODT[1:0]
	 */
	writel(0, iomux + IOMUXC_SW_PAD_CTL_GRP_DDRPKE);

	/* set global drive strength for all DRAM_x pins */
	writel(r4, iomux + IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE);

	/* set data dqs dqm drive strength */
	writel(r6, iomux + IOMUXC_SW_PAD_CTL_GRP_B0DS);
	writel(r6, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0);
	writel(r5, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0);

	writel(r1, iomux + IOMUXC_SW_PAD_CTL_GRP_B1DS);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1);
	writel(r5, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1);

	writel(r6, iomux + IOMUXC_SW_PAD_CTL_GRP_B2DS);
	writel(r6, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2);
	writel(r5, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2);

	writel(r1, iomux + IOMUXC_SW_PAD_CTL_GRP_B3DS);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3);
	writel(r5, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3);

	/* SDCLK pad drive strength control options */
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1);

	/* Control and addr bus pad drive strength control options */
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_GRP_ADDDS);
	writel(r1, iomux + IOMUXC_SW_PAD_CTL_GRP_CTLDS);
	writel(r2, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0);
	writel(r2, iomux + IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1);

	/*
	 * enable hysteresis on input pins
	 * (Freescale's documentation suggests that enable hysteresis
	 * would be better, but their source-code doesn't)
	 */
	writel(PAD_CTL_HYS, iomux + IOMUXC_SW_PAD_CTL_GRP_DDRHYS);
}

static void disable_watchdog(void)
{
	/*
	 * configure WDOG to generate external reset on trigger
	 * and disable power down counter
	 */
	writew(0x38, MX53_WDOG1_BASE_ADDR);
	writew(0x0, MX53_WDOG1_BASE_ADDR + 8);
	writew(0x38, MX53_WDOG2_BASE_ADDR);
	writew(0x0, MX53_WDOG2_BASE_ADDR + 8);
}

extern char __dtb_imx53_guf_vincell_lt_start[];
extern char __dtb_imx53_guf_vincell_start[];

static noinline void imx53_guf_vincell_init(int is_lt)
{
	void __iomem *ccm = (void *)MX53_CCM_BASE_ADDR;
	void __iomem *uart = IOMEM(MX53_UART2_BASE_ADDR);
	void *fdt;
	u32 r;
	enum bootsource src;
	int instance;

	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	writel(0x0088494c, ccm + MX5_CCM_CBCDR);
	writel(0x02b12f0a, ccm + MX5_CCM_CSCMR2);
	imx53_ungate_all_peripherals();

	imx53_init_lowlevel_early(800);

	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x27c);
	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x278);
	imx53_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);
	pr_debug("GuF Vincell\n");

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (!(r > 0x70000000 && r < 0xf0000000)) {
		disable_watchdog();
		configure_dram_iomux();
		imx_esdctlv4_init();

		imx53_get_boot_source(&src, &instance);

		if (src == BOOTSOURCE_NAND &&
		    IS_ENABLED(CONFIG_MACH_GUF_VINCELL_XLOAD))
			imx53_nand_start_image();
	}

	if (is_lt)
		fdt = __dtb_imx53_guf_vincell_lt_start;
	else
		fdt = __dtb_imx53_guf_vincell_start;

	imx53_barebox_entry(fdt);
}

static void __imx53_guf_vincell_init(int is_lt)
{
	arm_early_mmu_cache_invalidate();
	imx5_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();
	barrier();

	imx53_guf_vincell_init(is_lt);
}

ENTRY_FUNCTION(start_imx53_guf_vincell_lt, r0, r1, r2)
{
	__imx53_guf_vincell_init(1);
}

ENTRY_FUNCTION(start_imx53_guf_vincell, r0, r1, r2)
{
	__imx53_guf_vincell_init(0);
}

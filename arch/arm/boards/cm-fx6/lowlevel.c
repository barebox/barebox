#define pr_fmt(fmt) "cm-fx6: " fmt

#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <debug_ll.h>
#include <io.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6-ddr-regs.h>
#include <mach/imx6.h>
#include <mach/xload.h>
#include <mach/esdctl.h>
#include <serial/imx-uart.h>

enum ddr_config {
	DDR_16BIT_256MB,
	DDR_32BIT_512MB,
	DDR_32BIT_1GB,
	DDR_64BIT_1GB,
	DDR_64BIT_2GB,
	DDR_64BIT_4GB,
	DDR_UNKNOWN,
};

static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 4; i++);
}

/*
 * Below DRAM_RESET[DDR_SEL] = 0 which is incorrect according to
 * Freescale QRM, but this is exactly the value used by the automatic
 * calibration script and it works also in all our tests, so we leave
 * it as is at this point.
 */
#define CM_FX6_DDR_IOMUX_CFG \
	.dram_sdqs0	= 0x00000038, \
	.dram_sdqs1	= 0x00000038, \
	.dram_sdqs2	= 0x00000038, \
	.dram_sdqs3	= 0x00000038, \
	.dram_sdqs4	= 0x00000038, \
	.dram_sdqs5	= 0x00000038, \
	.dram_sdqs6	= 0x00000038, \
	.dram_sdqs7	= 0x00000038, \
	.dram_dqm0	= 0x00000038, \
	.dram_dqm1	= 0x00000038, \
	.dram_dqm2	= 0x00000038, \
	.dram_dqm3	= 0x00000038, \
	.dram_dqm4	= 0x00000038, \
	.dram_dqm5	= 0x00000038, \
	.dram_dqm6	= 0x00000038, \
	.dram_dqm7	= 0x00000038, \
	.dram_cas	= 0x00000038, \
	.dram_ras	= 0x00000038, \
	.dram_sdclk_0	= 0x00000038, \
	.dram_sdclk_1	= 0x00000038, \
	.dram_sdcke0	= 0x00003000, \
	.dram_sdcke1	= 0x00003000, \
	.dram_reset	= 0x00000038, \
	.dram_sdba2	= 0x00000000, \
	.dram_sdodt0	= 0x00000038, \
	.dram_sdodt1	= 0x00000038,

#define CM_FX6_GPR_IOMUX_CFG \
	.grp_b0ds	= 0x00000038, \
	.grp_b1ds	= 0x00000038, \
	.grp_b2ds	= 0x00000038, \
	.grp_b3ds	= 0x00000038, \
	.grp_b4ds	= 0x00000038, \
	.grp_b5ds	= 0x00000038, \
	.grp_b6ds	= 0x00000038, \
	.grp_b7ds	= 0x00000038, \
	.grp_addds	= 0x00000038, \
	.grp_ddrmode_ctl = 0x00020000, \
	.grp_ddrpke	= 0x00000000, \
	.grp_ddrmode	= 0x00020000, \
	.grp_ctlds	= 0x00000038, \
	.grp_ddr_type	= 0x000C0000,

static struct mx6sdl_iomux_ddr_regs ddr_iomux_s = { CM_FX6_DDR_IOMUX_CFG };
static struct mx6sdl_iomux_grp_regs grp_iomux_s = { CM_FX6_GPR_IOMUX_CFG };
static struct mx6dq_iomux_ddr_regs ddr_iomux_q = { CM_FX6_DDR_IOMUX_CFG };
static struct mx6dq_iomux_grp_regs grp_iomux_q = { CM_FX6_GPR_IOMUX_CFG };

static struct mx6_mmdc_calibration cm_fx6_calib_s = {
	.p0_mpwldectrl0	= 0x005B0061,
	.p0_mpwldectrl1	= 0x004F0055,
	.p0_mpdgctrl0	= 0x0314030C,
	.p0_mpdgctrl1	= 0x025C0268,
	.p0_mprddlctl	= 0x42464646,
	.p0_mpwrdlctl	= 0x36322C34,
};

static struct mx6_ddr_sysinfo cm_fx6_sysinfo_s = {
	.cs1_mirror	= 1,
	.cs_density	= 16,
	.bi_on		= 1,
	.rtt_nom	= 1,
	.rtt_wr		= 0,
	.ralat		= 5,
	.walat		= 1,
	.mif3_mode	= 3,
	.rst_to_cke	= 0x23,
	.sde_to_rst	= 0x10,
};

static struct mx6_ddr3_cfg cm_fx6_ddr3_cfg_s = {
	.mem_speed	= 800,
	.density	= 4,
	.rowaddr	= 14,
	.coladdr	= 10,
	.pagesz		= 2,
	.trcd		= 1800,
	.trcmin		= 5200,
	.trasmin	= 3600,
	.SRT		= 0,
};

static void spl_mx6s_dram_init(enum ddr_config dram_config, bool reset)
{
	if (reset)
		((struct mmdc_p_regs *)MX6_MMDC_P0_MDCTL)->mdmisc = 2;

	switch (dram_config) {
	case DDR_16BIT_256MB:
		cm_fx6_sysinfo_s.dsize = 0;
		cm_fx6_sysinfo_s.ncs = 1;
		break;
	case DDR_32BIT_512MB:
		cm_fx6_sysinfo_s.dsize = 1;
		cm_fx6_sysinfo_s.ncs = 1;
		break;
	case DDR_32BIT_1GB:
		cm_fx6_sysinfo_s.dsize = 1;
		cm_fx6_sysinfo_s.ncs = 2;
		break;
	default:
		pr_err("Tried to setup invalid DDR configuration\n");
		hang();
	}

	mx6_dram_cfg(&cm_fx6_sysinfo_s, &cm_fx6_calib_s, &cm_fx6_ddr3_cfg_s);
	__udelay(100);
}

static struct mx6_mmdc_calibration cm_fx6_calib_q = {
	.p0_mpwldectrl0	= 0x00630068,
	.p0_mpwldectrl1	= 0x0068005D,
	.p0_mpdgctrl0	= 0x04140428,
	.p0_mpdgctrl1	= 0x037C037C,
	.p0_mprddlctl	= 0x3C30303A,
	.p0_mpwrdlctl	= 0x3A344038,
	.p1_mpwldectrl0	= 0x0035004C,
	.p1_mpwldectrl1	= 0x00170026,
	.p1_mpdgctrl0	= 0x0374037C,
	.p1_mpdgctrl1	= 0x0350032C,
	.p1_mprddlctl	= 0x30322A3C,
	.p1_mpwrdlctl	= 0x48304A3E,
};

static struct mx6_ddr_sysinfo cm_fx6_sysinfo_q = {
	.cs_density	= 16,
	.cs1_mirror	= 1,
	.bi_on		= 1,
	.rtt_nom	= 1,
	.rtt_wr		= 0,
	.ralat		= 5,
	.walat		= 1,
	.mif3_mode	= 3,
	.rst_to_cke	= 0x23,
	.sde_to_rst	= 0x10,
};

static struct mx6_ddr3_cfg cm_fx6_ddr3_cfg_q = {
	.mem_speed	= 1066,
	.density	= 4,
	.rowaddr	= 14,
	.coladdr	= 10,
	.pagesz		= 2,
	.trcd		= 1324,
	.trcmin		= 59500,
	.trasmin	= 9750,
	.SRT		= 0,
};

static void spl_mx6q_dram_init(enum ddr_config dram_config, bool reset)
{
	if (reset)
		((struct mmdc_p_regs *)MX6_MMDC_P0_MDCTL)->mdmisc = 2;

	cm_fx6_ddr3_cfg_q.rowaddr = 14;
	switch (dram_config) {
	case DDR_16BIT_256MB:
		cm_fx6_sysinfo_q.dsize = 0;
		cm_fx6_sysinfo_q.ncs = 1;
		break;
	case DDR_32BIT_512MB:
		cm_fx6_sysinfo_q.dsize = 1;
		cm_fx6_sysinfo_q.ncs = 1;
		break;
	case DDR_64BIT_1GB:
		cm_fx6_sysinfo_q.dsize = 2;
		cm_fx6_sysinfo_q.ncs = 1;
		break;
	case DDR_64BIT_2GB:
		cm_fx6_sysinfo_q.cs_density = 8;
		cm_fx6_ddr3_cfg_q.density = 2;
		cm_fx6_sysinfo_q.dsize = 2;
		cm_fx6_sysinfo_q.ncs = 2;
		break;
	case DDR_64BIT_4GB:
		cm_fx6_sysinfo_q.dsize = 2;
		cm_fx6_sysinfo_q.ncs = 2;
		cm_fx6_ddr3_cfg_q.rowaddr = 15;
		break;
	default:
		pr_err("Tried to setup invalid DDR configuration\n");
		hang();
	}

	mx6_dram_cfg(&cm_fx6_sysinfo_q, &cm_fx6_calib_q, &cm_fx6_ddr3_cfg_q);
	__udelay(100);
}

static unsigned long cm_fx6_spl_dram_init(void)
{
	unsigned long bank1_size, bank2_size;
	int cpu_type = __imx6_cpu_type();

	if (cpu_type == IMX6_CPUTYPE_IMX6S) {
		mx6sdl_dram_iocfg(64, &ddr_iomux_s, &grp_iomux_s);

		spl_mx6s_dram_init(DDR_32BIT_1GB, false);
		bank1_size = get_ram_size((long int *)0x10000000, 0x80000000);
		bank2_size = get_ram_size((long int *)0x80000000, 0x80000000);
		if (bank1_size == 0x20000000) {
			if (bank2_size == 0x20000000)
				return SZ_1G;

			spl_mx6s_dram_init(DDR_32BIT_512MB, true);
			return SZ_512M;
		}

		spl_mx6s_dram_init(DDR_16BIT_256MB, true);
		bank1_size = get_ram_size((long int *)0x10000000, 0x80000000);
		if (bank1_size == 0x10000000)
			return SZ_256M;
	} else if (cpu_type == IMX6_CPUTYPE_IMX6D || cpu_type == IMX6_CPUTYPE_IMX6Q) {
		mx6dq_dram_iocfg(64, &ddr_iomux_q, &grp_iomux_q);

		spl_mx6q_dram_init(DDR_64BIT_4GB, false);
		bank1_size = get_ram_size((long int *)0x10000000, 0x80000000);
		if (bank1_size == 0x80000000)
			return SZ_2G;

		if (bank1_size == 0x40000000) {
			bank2_size = get_ram_size((long int *)0x90000000,
								0x40000000);
			if (bank2_size == 0x40000000) {
				/* Don't do a full reset here */
				spl_mx6q_dram_init(DDR_64BIT_2GB, false);
				return SZ_2G;
			} else {
				spl_mx6q_dram_init(DDR_64BIT_1GB, true);
				return SZ_1G;
			}
		}

		spl_mx6q_dram_init(DDR_32BIT_512MB, true);
		bank1_size = get_ram_size((long int *)0x10000000, 0x80000000);
		if (bank1_size == 0x20000000)
			return SZ_512M;

		spl_mx6q_dram_init(DDR_16BIT_256MB, true);
		bank1_size = get_ram_size((long int *)0x10000000, 0x80000000);
		if (bank1_size == 0x10000000)
			return SZ_256M;
	}

	return 0;
}

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x4, iomuxbase + 0x01f8);

	imx6_ungate_all_peripherals();
	imx6_uart_setup((void *)MX6_UART4_BASE_ADDR);
	pbl_set_putc(imx_uart_putc, (void *)MX6_UART4_BASE_ADDR);

	pr_debug("\n");
}

static void cm_fx6_init(void)
{
	unsigned long sdram_size;

	setup_uart();

	if (get_pc() > 0x10000000)
		return;

	sdram_size = cm_fx6_spl_dram_init();

	pr_debug("SDRAM init finished. SDRAM size 0x%08lx\n", sdram_size);

	imx6_esdhc_start_image(2);
	pr_info("Loading image from SPI flash\n");
	imx6_spi_start_image(0);
}

extern char __dtb_imx6q_cm_fx6_start[];
extern char __dtb_imx6dl_cm_fx6_start[];

static noinline void cm_fx6_start(void)
{
	int cpu_type = __imx6_cpu_type();

	cm_fx6_init();

	if (cpu_type == IMX6_CPUTYPE_IMX6S)
		imx6q_barebox_entry(__dtb_imx6dl_cm_fx6_start);
	else
		imx6q_barebox_entry(__dtb_imx6q_cm_fx6_start);
}

ENTRY_FUNCTION(start_imx6_cm_fx6, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();
	barrier();

	cm_fx6_start();
}

extern char __dtb_imx6q_utilite_start[];
extern char __dtb_imx6dl_utilite_value_start[];

static noinline void utilite_start(void)
{
	int cpu_type = __imx6_cpu_type();

	cm_fx6_init();

	if (cpu_type == IMX6_CPUTYPE_IMX6S)
		/* FIXME: This needs a specialized utilite value dts */
		imx6q_barebox_entry(__dtb_imx6dl_cm_fx6_start);
	else
		imx6q_barebox_entry(__dtb_imx6q_utilite_start);
}

ENTRY_FUNCTION(start_imx6_utilite, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();
	barrier();

	utilite_start();
}

// SPDX-License-Identifier:     GPL-2.0+
#include <common.h>
#include <io.h>
#include <bootsource.h>
#include <mach/rockchip/bootrom.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/rockchip.h>

#define GRF_BASE		0xfdc60000
#define GRF_GPIO1B_DS_2		0x218
#define GRF_GPIO1B_DS_3		0x21c
#define GRF_GPIO1C_DS_0		0x220
#define GRF_GPIO1C_DS_1		0x224
#define GRF_GPIO1C_DS_2		0x228
#define GRF_GPIO1C_DS_3		0x22c
#define GRF_GPIO1D_DS_0		0x230
#define GRF_GPIO1D_DS_1		0x234
#define GRF_SOC_CON4		0x510
#define EDP_PHY_GRF_BASE	0xfdcb0000
#define EDP_PHY_GRF_CON0	(EDP_PHY_GRF_BASE + 0x00)
#define EDP_PHY_GRF_CON10	(EDP_PHY_GRF_BASE + 0x28)
#define PMU_BASE_ADDR		0xfdd90000
#define PMU_NOC_AUTO_CON0	0x70
#define PMU_NOC_AUTO_CON1	0x74
#define CRU_BASE		0xfdd20000
#define CRU_SOFTRST_CON26	0x468
#define CRU_SOFTRST_CON28	0x470
#define SGRF_BASE		0xFDD18000
#define SGRF_SOC_CON3		0xC
#define SGRF_SOC_CON4		0x10
#define PMUGRF_SOC_CON15	0xfdc20100
#define CPU_GRF_BASE		0xfdc30000
#define GRF_CORE_PVTPLL_CON0	0x10
#define USBPHY_U3_GRF		0xfdca0000
#define USBPHY_U3_GRF_CON1	(USBPHY_U3_GRF + 0x04)
#define USBPHY_U2_GRF		0xfdca8000
#define USBPHY_U2_GRF_CON0	(USBPHY_U2_GRF + 0x00)
#define USBPHY_U2_GRF_CON1	(USBPHY_U2_GRF + 0x04)

#define PMU_PWR_GATE_SFTCON	0xA0
#define PMU_PWR_DWN_ST		0x98
#define PMU_BUS_IDLE_SFTCON0	0x50
#define PMU_BUS_IDLE_ST		0x68
#define PMU_BUS_IDLE_ACK	0x60

#define EBC_PRIORITY_REG	0xfe158008

static void qos_priority_init(void)
{
	u32 delay;

	/* enable all pd except npu and gpu */
	writel(0xffff0000 & ~(BIT(0 + 16) | BIT(1 + 16)),
	       PMU_BASE_ADDR + PMU_PWR_GATE_SFTCON);
	delay = 1000;
	do {
		udelay(1);
		delay--;
		if (delay == 0) {
			printf("Fail to set domain.");
			hang();
		}
	} while (readl(PMU_BASE_ADDR + PMU_PWR_DWN_ST) & ~(BIT(0) | BIT(1)));

	/* release all idle request except npu and gpu */
	writel(0xffff0000 & ~(BIT(1 + 16) | BIT(2 + 16)),
	       PMU_BASE_ADDR + PMU_BUS_IDLE_SFTCON0);

	delay = 1000;
	/* wait ack status */
	do {
		udelay(1);
		delay--;
		if (delay == 0) {
			printf("Fail to get ack on domain.\n");
			hang();
		}
	} while (readl(PMU_BASE_ADDR + PMU_BUS_IDLE_ACK) & ~(BIT(1) | BIT(2)));

	delay = 1000;
	/* wait idle status */
	do {
		udelay(1);
		delay--;
		if (delay == 0) {
			printf("Fail to set idle on domain.\n");
			hang();
		}
	} while (readl(PMU_BASE_ADDR + PMU_BUS_IDLE_ST) & ~(BIT(1) | BIT(2)));

	writel(0x303, EBC_PRIORITY_REG);
}

void rk3568_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	/*
	 * When perform idle operation, corresponding clock can
	 * be opened or gated automatically.
	 */
	writel(0xffffffff, PMU_BASE_ADDR + PMU_NOC_AUTO_CON0);
	writel(0x000f000f, PMU_BASE_ADDR + PMU_NOC_AUTO_CON1);

	/* Set the emmc sdmmc0 to secure */
	writel(((0x3 << 11 | 0x1 << 4) << 16), SGRF_BASE + SGRF_SOC_CON4);
	/* set the emmc ds to level 2 */
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1B_DS_2);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1B_DS_3);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_0);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_1);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_2);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_3);

	/* Set the fspi to secure */
	writel(((0x1 << 14) << 16) | (0x0 << 14), SGRF_BASE + SGRF_SOC_CON3);

	/* Disable eDP phy by default */
	writel(0x00070007, EDP_PHY_GRF_CON10);
	writel(0x0ff10ff1, EDP_PHY_GRF_CON0);

	/* Set core pvtpll ring length */
	writel(0x00ff002b, CPU_GRF_BASE + GRF_CORE_PVTPLL_CON0);

	/*
	 * Assert reset the pipephy0, pipephy1 and pipephy2,
	 * and de-assert reset them in Kernel combphy driver.
	 */
	writel(0x02a002a0, CRU_BASE + CRU_SOFTRST_CON28);

	/*
	 * Set USB 2.0 PHY0 port1 and PHY1 port0 and port1
	 * enter suspend mode to to save power. And USB 2.0
	 * PHY0 port0 for OTG interface still in normal mode.
	 */
	writel(0x01ff01d1, USBPHY_U3_GRF_CON1);
	writel(0x01ff01d1, USBPHY_U2_GRF_CON0);
	writel(0x01ff01d1, USBPHY_U2_GRF_CON1);

	qos_priority_init();
}

int rk3568_init(void)
{
	rockchip_parse_bootrom_iram(rockchip_scratch_space()->iram);

	return 0;
}

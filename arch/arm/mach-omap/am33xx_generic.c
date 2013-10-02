/*
 * (C) Copyright 2012 Teresa Gámez, Phytec Messtechnik GmbH
 * (C) Copyright 2012-2013 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/generic.h>
#include <mach/sys_info.h>
#include <mach/am33xx-generic.h>
#include <mach/gpmc.h>

void __noreturn reset_cpu(unsigned long addr)
{
	writel(AM33XX_PRM_RSTCTRL_RESET, AM33XX_PRM_RSTCTRL);

	while (1);
}

/**
 * @brief Extract the AM33xx ES revision
 *
 * The significance of the CPU revision depends upon the cpu type.
 * Latest known revision is considered default.
 *
 * This function is called before barebox_arm_entry(), so avoid switch
 * statements.
 *
 * @return silicon version
 */
u32 am33xx_get_cpu_rev(void)
{
	u32 version = (readl(AM33XX_IDCODE_REG) >> 28) & 0xF;

	if (version == 0)
		return AM335X_ES1_0;
	else if (version == 1)
		return AM335X_ES2_0;
	else
		return AM335X_ES2_1;
}

/**
 * @brief Get the upper address of current execution
 *
 * we can use this to figure out if we are running in SRAM /
 * XIP Flash or in SDRAM
 *
 * @return base address
 */
u32 get_base(void)
{
	u32 val;
	__asm__ __volatile__("mov %0, pc \n":"=r"(val)::"memory");
	val &= 0xF0000000;
	val >>= 28;
	return val;
}

/**
 * @brief Are we running in Flash XIP?
 *
 * If the base is in GPMC address space, we probably are!
 *
 * @return 1 if we are running in XIP mode, else return 0
 */
u32 running_in_flash(void)
{
	if (get_base() < 4)
		return 1;	/* in flash */
	return 0;		/* running in SRAM or SDRAM */
}

/**
 * @brief Are we running in OMAP internal SRAM?
 *
 * If in SRAM address, then yes!
 *
 * @return  1 if we are running in SRAM, else return 0
 */
u32 running_in_sram(void)
{
	if (get_base() == 4)
		return 1;	/* in SRAM */
	return 0;		/* running in FLASH or SDRAM */
}

/**
 * @brief Are we running in SDRAM?
 *
 * if we are not in GPMC nor in SRAM address space,
 * we are in SDRAM execution area
 *
 * @return 1 if we are running from SDRAM, else return 0
 */
u32 running_in_sdram(void)
{
	if (get_base() > 4)
		return 1;	/* in sdram */
	return 0;		/* running in SRAM or FLASH */
}

static int am33xx_bootsource(void)
{
	enum bootsource src;
	int instance = 0;

	switch (omap_bootinfo[2] & 0xFF) {
	case 0x05:
		src = BOOTSOURCE_NAND;
		break;
	case 0x08:
		src = BOOTSOURCE_MMC;
		instance = 0;
		break;
	case 0x09:
		src = BOOTSOURCE_MMC;
		instance = 1;
		break;
	case 0x0b:
		src = BOOTSOURCE_SPI;
		break;
	default:
		src = BOOTSOURCE_UNKNOWN;
	}
	bootsource_set(src);
	bootsource_set_instance(instance);
	return 0;
}
postcore_initcall(am33xx_bootsource);

int am33xx_register_ethaddr(int eth_id, int mac_id)
{
	void __iomem *mac_id_low = (void *)AM33XX_MAC_ID0_LO + mac_id * 8;
	void __iomem *mac_id_high = (void *)AM33XX_MAC_ID0_HI + mac_id * 8;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;

	mac_lo = readl(mac_id_low);
	mac_hi = readl(mac_id_high);
	mac_addr[0] = mac_hi & 0xff;
	mac_addr[1] = (mac_hi & 0xff00) >> 8;
	mac_addr[2] = (mac_hi & 0xff0000) >> 16;
	mac_addr[3] = (mac_hi & 0xff000000) >> 24;
	mac_addr[4] = mac_lo & 0xff;
	mac_addr[5] = (mac_lo & 0xff00) >> 8;

	if (is_valid_ether_addr(mac_addr)) {
		eth_register_ethaddr(eth_id, mac_addr);
		return 0;
	}

	return -ENODEV;
}

/* GPMC timing for AM33XX nand device */
const struct gpmc_config am33xx_nand_cfg = {
	.cfg = {
		0x00000800,	/* CONF1 */
		0x001e1e00,	/* CONF2 */
		0x001e1e00,	/* CONF3 */
		0x16051807,	/* CONF4 */
		0x00151e1e,	/* CONF5 */
		0x16000f80,	/* CONF6 */
	},
	.base = 0x08000000,
	.size = GPMC_SIZE_16M,
};

static int am33xx_gpio_init(void)
{
	add_generic_device("omap-gpio", 0, NULL, AM33XX_GPIO0_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 1, NULL, AM33XX_GPIO1_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 2, NULL, AM33XX_GPIO2_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 3, NULL, AM33XX_GPIO3_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	return 0;
}
coredevice_initcall(am33xx_gpio_init);

/* UART Defines */
#define UART_SYSCFG_OFFSET	0x54
#define UART_SYSSTS_OFFSET	0x58

#define UART_CLK_RUNNING_MASK	0x1
#define UART_RESET		(0x1 << 1)
#define UART_SMART_IDLE_EN	(0x1 << 0x3)

void am33xx_uart0_soft_reset(void)
{
	int reg;

	reg = readl(AM33XX_UART0_BASE + UART_SYSCFG_OFFSET);
	reg |= UART_RESET;
	writel(reg, (AM33XX_UART0_BASE + UART_SYSCFG_OFFSET));

	while ((readl(AM33XX_UART0_BASE + UART_SYSSTS_OFFSET) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK)
		;

	/* Disable smart idle */
	reg = readl((AM33XX_UART0_BASE + UART_SYSCFG_OFFSET));
	reg |= UART_SMART_IDLE_EN;
	writel(reg, (AM33XX_UART0_BASE + UART_SYSCFG_OFFSET));
}


#define VTP_CTRL_READY		(0x1 << 5)
#define VTP_CTRL_ENABLE		(0x1 << 6)
#define VTP_CTRL_START_EN	(0x1)

void am33xx_config_vtp(void)
{
	uint32_t val;

	val = readl(AM33XX_VTP0_CTRL_REG);
	val |= VTP_CTRL_ENABLE;
	writel(val, AM33XX_VTP0_CTRL_REG);

	val = readl(AM33XX_VTP0_CTRL_REG);
	val &= ~VTP_CTRL_START_EN;
	writel(val, AM33XX_VTP0_CTRL_REG);

	val = readl(AM33XX_VTP0_CTRL_REG);
	val |= VTP_CTRL_START_EN;
	writel(val, AM33XX_VTP0_CTRL_REG);

	/* Poll for READY */
	while ((readl(AM33XX_VTP0_CTRL_REG) &
				VTP_CTRL_READY) != VTP_CTRL_READY)
		;
}

void am33xx_ddr_phydata_cmd_macro(const struct am33xx_cmd_control *cmd_ctrl)
{
	writel(cmd_ctrl->slave_ratio0, AM33XX_CMD0_CTRL_SLAVE_RATIO_0);
	writel(cmd_ctrl->dll_lock_diff0, AM33XX_CMD0_DLL_LOCK_DIFF_0);
	writel(cmd_ctrl->invert_clkout0, AM33XX_CMD0_INVERT_CLKOUT_0);

	writel(cmd_ctrl->slave_ratio1, AM33XX_CMD1_CTRL_SLAVE_RATIO_0);
	writel(cmd_ctrl->dll_lock_diff1, AM33XX_CMD1_DLL_LOCK_DIFF_0);
	writel(cmd_ctrl->invert_clkout1, AM33XX_CMD1_INVERT_CLKOUT_0);

	writel(cmd_ctrl->slave_ratio2, AM33XX_CMD2_CTRL_SLAVE_RATIO_0);
	writel(cmd_ctrl->dll_lock_diff2, AM33XX_CMD2_DLL_LOCK_DIFF_0);
	writel(cmd_ctrl->invert_clkout2, AM33XX_CMD2_INVERT_CLKOUT_0);
}

#define CM_EMIF_SDRAM_CONFIG	(AM33XX_CTRL_BASE + 0x110)

void am33xx_config_sdram(const struct am33xx_emif_regs *regs)
{
	writel(regs->emif_read_latency, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1));
	writel(regs->emif_read_latency, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1_SHADOW));
	writel(regs->emif_read_latency, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_2));
	writel(regs->emif_tim1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1));
	writel(regs->emif_tim1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1_SHADOW));
	writel(regs->emif_tim2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2));
	writel(regs->emif_tim2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2_SHADOW));
	writel(regs->emif_tim3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3));
	writel(regs->emif_tim3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3_SHADOW));

	if (regs->zq_config) {
		/*
		 * A value of 0x2800 for the REF CTRL will give us
		 * about 570us for a delay, which will be long enough
		 * to configure things.
		 */
		writel(0x2800, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
		writel(regs->zq_config, AM33XX_EMIF4_0_REG(ZQ_CONFIG));
		writel(regs->sdram_config, CM_EMIF_SDRAM_CONFIG);
		writel(regs->sdram_config, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
		writel(regs->sdram_ref_ctrl,
				AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
		writel(regs->sdram_ref_ctrl,
			AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	}

	writel(regs->sdram_ref_ctrl, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	writel(regs->sdram_ref_ctrl, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));
	writel(regs->sdram_config, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
}

void am33xx_config_io_ctrl(int ioctrl)
{
	writel(ioctrl, AM33XX_DDR_CMD0_IOCTRL);
	writel(ioctrl, AM33XX_DDR_CMD1_IOCTRL);
	writel(ioctrl, AM33XX_DDR_CMD2_IOCTRL);
	writel(ioctrl, AM33XX_DDR_DATA0_IOCTRL);
	writel(ioctrl, AM33XX_DDR_DATA1_IOCTRL);
}

void am33xx_config_ddr_data(const struct am33xx_ddr_data *data, int macronr)
{
	u32 base = 0x0;

	if (macronr)
		base = 0xA4;

	writel(data->rd_slave_ratio0, AM33XX_DATA0_RD_DQS_SLAVE_RATIO_0 + base);
	writel(data->wr_dqs_slave_ratio0, AM33XX_DATA0_WR_DQS_SLAVE_RATIO_0 + base);
	writel(data->wrlvl_init_ratio0, AM33XX_DATA0_WRLVL_INIT_RATIO_0 + base);
	writel(data->gatelvl_init_ratio0, AM33XX_DATA0_GATELVL_INIT_RATIO_0 + base);
	writel(data->fifo_we_slave_ratio0, AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_0 + base);
	writel(data->wr_slave_ratio0, AM33XX_DATA0_WR_DATA_SLAVE_RATIO_0 + base);
	writel(data->use_rank0_delay, AM33XX_DATA0_RANK0_DELAYS_0 + base);
	writel(data->dll_lock_diff0, AM33XX_DATA0_DLL_LOCK_DIFF_0 + base);
}

void am335x_sdram_init(int ioctrl, const struct am33xx_cmd_control *cmd_ctrl,
		const struct am33xx_emif_regs *emif_regs,
		const struct am33xx_ddr_data *ddr_data)
{
	uint32_t val;

	enable_ddr_clocks();

	am33xx_config_vtp();

	am33xx_ddr_phydata_cmd_macro(cmd_ctrl);
	am33xx_config_ddr_data(ddr_data, 0);
	am33xx_config_ddr_data(ddr_data, 1);

	am33xx_config_io_ctrl(ioctrl);

	val = readl(AM33XX_DDR_IO_CTRL);
	val &= 0xefffffff;
	writel(val, AM33XX_DDR_IO_CTRL);

	val = readl(AM33XX_DDR_CKE_CTRL);
	val |= 0x00000001;
	writel(val, AM33XX_DDR_CKE_CTRL);

	am33xx_config_sdram(emif_regs);
}

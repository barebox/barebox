#define pr_fmt(fmt) "babbage-power: " fmt

#include <common.h>
#include <init.h>
#include <notifier.h>
#include <mach/revision.h>
#include <mach/imx5.h>
#include <mfd/mc13xxx.h>

static void babbage_power_init(struct mc13xxx *mc13xxx)
{
	u32 val;

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_MISC, &val);
	val &= ~0x10000;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	/* Write needed to update Charger 0 */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_CHARGE, 0x0023807F);

	/* power up the system first */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, 0x00200000);

	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		/* Set core voltage to 1.1V */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_0, &val);
		val &= ~0x1f;
		val |= 0x14;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_0, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x19;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x18;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	}

	if (mc13xxx_revision(mc13xxx) < MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x1405;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x505;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode
		 * for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x2008;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x808;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_0, &val);
	val &= ~0x34030;
	val |= 0x10020;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_0, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_1, &val);
	val &= ~0x1FC;
	val |= 0x1F4;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	pr_info("initialized PMIC\n");

	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();
}

void imx51_babbage_power_init(void)
{
	mc13xxx_register_init_callback(babbage_power_init);
}

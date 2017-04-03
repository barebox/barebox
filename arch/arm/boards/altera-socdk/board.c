#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <linux/micrel_phy.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <fcntl.h>
#include <fs.h>
#include <mach/cyclone5-regs.h>

static int ksz9021rn_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x09, 0x0f00);

	/* rx skew */
	phy_write(dev, 0x0b, 0x8105);
	phy_write(dev, 0x0c, 0x0000);

	/* clk/ctrl skew */
	phy_write(dev, 0x0b, 0x8104);
	phy_write(dev, 0x0c, 0xa0d0);

	return 0;
}

static int socfpga_console_init(void)
{
	if (!of_machine_is_compatible("altr,socdk"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);

	return 0;
}
console_initcall(socfpga_console_init);

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

static int phy_fixup(struct phy_device *dev)
{
	/* min rx data delay */
	phy_write(dev, 0x0b, 0x8105);
	phy_write(dev, 0x0c, 0x0000);

	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(dev, 0x0b, 0x8104);
	phy_write(dev, 0x0c, 0xa0d0);
	phy_write(dev, 0x0b, 0x104);

	return 0;
}

static int socfpga_console_init(void)
{
	if (!of_machine_is_compatible("terasic,sockit"))
		return 0;

	if (IS_ENABLED(CONFIG_PHYLIB))
		phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK, phy_fixup);

	return 0;
}
console_initcall(socfpga_console_init);

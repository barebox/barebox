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

static int phy_fixup(struct phy_device *dev)
{
	/*
	 * min rx data delay, max rx/tx clock delay,
	 * min rx/tx control delay
	 */
	phy_write_mmd_indirect(dev, 4, 2, 0);
	phy_write_mmd_indirect(dev, 5, 2, 0);
	phy_write_mmd_indirect(dev, 8, 2, 0x003ff);
	return 0;
}

static int socfpga_init(void)
{
	if (!of_machine_is_compatible("terasic,de0-nano-soc"))
		return 0;

	if (IS_ENABLED(CONFIG_PHYLIB))
		phy_register_fixup_for_uid(PHY_ID_KSZ9031, MICREL_PHY_ID_MASK, phy_fixup);

	return 0;
}
console_initcall(socfpga_init);

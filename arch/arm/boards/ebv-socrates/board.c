#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <bbu.h>
#include <bootsource.h>
#include <filetype.h>
#include <asm/armlinux.h>
#include <linux/micrel_phy.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <fcntl.h>
#include <fs.h>
#include <mach/cyclone5-regs.h>

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

static int socrates_init(void)
{
	enum bootsource bootsource = bootsource_get();
	uint32_t flag_qspi = 0;
	uint32_t flag_mmc = 0;

	if (!of_machine_is_compatible("ebv,socrates"))
		return 0;

	if (IS_ENABLED(CONFIG_PHYLIB))
		phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK, phy_fixup);

	switch (bootsource) {
	case BOOTSOURCE_MMC:
		flag_mmc |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	case BOOTSOURCE_SPI:
		flag_qspi |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	default:
		break;
	}

	bbu_register_std_file_update("qspi-xload0", flag_qspi,
					"/dev/mtd0.prebootloader0",
					filetype_socfpga_xload);
	bbu_register_std_file_update("qspi-xload1", 0,
					"/dev/mtd0.prebootloader1",
					filetype_socfpga_xload);
	bbu_register_std_file_update("qspi-xload2", 0,
					"/dev/mtd0.prebootloader2",
					filetype_socfpga_xload);
	bbu_register_std_file_update("qspi-xload3", 0,
					"/dev/mtd0.prebootloader3",
					filetype_socfpga_xload);
	bbu_register_std_file_update("qspi", 0,	"/dev/mtd0.barebox",
					filetype_arm_barebox);

	bbu_register_std_file_update("mmc-xload", flag_mmc, "/dev/mmc0.0",
					filetype_socfpga_xload);

	return 0;
}
postcore_initcall(socrates_init);

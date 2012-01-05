/*
 * MCI pmic power.
 */

#include <mfd/twl6030.h>
#include <mci/twl6030.h>
#include <asm/io.h>

static int twl6030_mci_write(u8 address, u8 data)
{
	int ret;
	struct twl6030 *twl6030 = twl6030_get();

	ret = twl6030_reg_write(twl6030, address, data);
	if (ret != 0)
		printf("TWL6030:MCI:Write[0x%x] Error %d\n", address, ret);

	return ret;
}

void twl6030_mci_power_init(void)
{
	twl6030_mci_write(TWL6030_PMCS_VMMC_CFG_VOLTAGE,
			TWL6030_VMMC_VSEL_0 | TWL6030_VMMC_VSEL_2 |
			TWL6030_VMMC_VSEL_4);
	twl6030_mci_write(TWL6030_PMCS_VMMC_CFG_STATE,
			TWL6030_VMMC_STATE0 | TWL6030_VMMC_GRP_APP);
}


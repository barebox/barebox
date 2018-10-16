#include <linux/ethtool.h>
#include <linux/bitfield.h>

#include "global2.h"

int mv88e6xxx_g2_read(struct mv88e6xxx_chip *chip, int reg, u16 *val)
{
	return mv88e6xxx_read(chip, chip->info->global2_addr, reg, val);
}

int mv88e6xxx_g2_write(struct mv88e6xxx_chip *chip, int reg, u16 val)
{
	return mv88e6xxx_write(chip, chip->info->global2_addr, reg, val);
}

int mv88e6xxx_g2_wait(struct mv88e6xxx_chip *chip, int reg, u16 mask)
{
	return mv88e6xxx_wait(chip, chip->info->global2_addr, reg, mask);
}

static int mv88e6xxx_g2_smi_phy_wait(struct mv88e6xxx_chip *chip)
{
	return mv88e6xxx_g2_wait(chip, MV88E6XXX_G2_SMI_PHY_CMD,
				 MV88E6XXX_G2_SMI_PHY_CMD_BUSY);
}

static int mv88e6xxx_g2_smi_phy_cmd(struct mv88e6xxx_chip *chip, u16 cmd)
{
	int err;

	err = mv88e6xxx_g2_write(chip, MV88E6XXX_G2_SMI_PHY_CMD,
				 MV88E6XXX_G2_SMI_PHY_CMD_BUSY | cmd);
	if (err)
		return err;

	return mv88e6xxx_g2_smi_phy_wait(chip);
}

static int mv88e6xxx_g2_smi_phy_access(struct mv88e6xxx_chip *chip,
				       bool external, bool c45, u16 op, int dev,
				       int reg)
{
	u16 cmd = op;

	if (external)
		cmd |= MV88E6390_G2_SMI_PHY_CMD_FUNC_EXTERNAL;
	else
		cmd |= MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL; /* empty mask */

	if (c45)
		cmd |= MV88E6XXX_G2_SMI_PHY_CMD_MODE_45; /* empty mask */
	else
		cmd |= MV88E6XXX_G2_SMI_PHY_CMD_MODE_22;

	dev <<= __bf_shf(MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK);
	cmd |= dev & MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK;
	cmd |= reg & MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK;

	return mv88e6xxx_g2_smi_phy_cmd(chip, cmd);
}

static int mv88e6xxx_g2_smi_phy_access_c22(struct mv88e6xxx_chip *chip,
					   bool external, u16 op, int dev,
					   int reg)
{
	return mv88e6xxx_g2_smi_phy_access(chip, external, false, op, dev, reg);
}

/* IEEE 802.3 Clause 22 Read Data Register */
static int mv88e6xxx_g2_smi_phy_read_data_c22(struct mv88e6xxx_chip *chip,
					      bool external, int dev, int reg,
					      u16 *data)
{
	u16 op = MV88E6XXX_G2_SMI_PHY_CMD_OP_22_READ_DATA;
	int err;

	err = mv88e6xxx_g2_smi_phy_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_smi_phy_access_c22(chip, external, op, dev, reg);
	if (err)
		return err;

	return mv88e6xxx_g2_read(chip, MV88E6XXX_G2_SMI_PHY_DATA, data);
}

/* IEEE 802.3 Clause 22 Write Data Register */
static int mv88e6xxx_g2_smi_phy_write_data_c22(struct mv88e6xxx_chip *chip,
					       bool external, int dev, int reg,
					       u16 data)
{
	u16 op = MV88E6XXX_G2_SMI_PHY_CMD_OP_22_WRITE_DATA;
	int err;

	err = mv88e6xxx_g2_smi_phy_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_write(chip, MV88E6XXX_G2_SMI_PHY_DATA, data);
	if (err)
		return err;

	return mv88e6xxx_g2_smi_phy_access_c22(chip, external, op, dev, reg);
}


int mv88e6xxx_g2_smi_phy_read(struct mv88e6xxx_chip *chip, struct mii_bus *bus,
			      int addr, int reg, u16 *val)
{
	bool external = false;

	return mv88e6xxx_g2_smi_phy_read_data_c22(chip, external, addr, reg,
						  val);
}

int mv88e6xxx_g2_smi_phy_write(struct mv88e6xxx_chip *chip, struct mii_bus *bus,
			       int addr, int reg, u16 val)
{
	bool external = false;

	return mv88e6xxx_g2_smi_phy_write_data_c22(chip, external, addr, reg,
						   val);
}

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

/* Offset 0x14: EEPROM Command
 * Offset 0x15: EEPROM Data (for 16-bit data access)
 * Offset 0x15: EEPROM Addr (for 8-bit data access)
 */

static int mv88e6xxx_g2_eeprom_wait(struct mv88e6xxx_chip *chip)
{
	return mv88e6xxx_g2_wait(chip, MV88E6XXX_G2_EEPROM_CMD,
				 MV88E6XXX_G2_EEPROM_CMD_BUSY |
				 MV88E6XXX_G2_EEPROM_CMD_RUNNING);
}

static int mv88e6xxx_g2_eeprom_cmd(struct mv88e6xxx_chip *chip, u16 cmd)
{
	int err;

	err = mv88e6xxx_g2_write(chip, MV88E6XXX_G2_EEPROM_CMD,
				 MV88E6XXX_G2_EEPROM_CMD_BUSY | cmd);
	if (err)
		return err;

	return mv88e6xxx_g2_eeprom_wait(chip);
}

static int mv88e6xxx_g2_eeprom_read8(struct mv88e6xxx_chip *chip,
				     u16 addr, u8 *data)
{
	u16 cmd = MV88E6XXX_G2_EEPROM_CMD_OP_READ;
	int err;

	err = mv88e6xxx_g2_eeprom_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_write(chip, MV88E6390_G2_EEPROM_ADDR, addr);
	if (err)
		return err;

	err = mv88e6xxx_g2_eeprom_cmd(chip, cmd);
	if (err)
		return err;

	err = mv88e6xxx_g2_read(chip, MV88E6XXX_G2_EEPROM_CMD, &cmd);
	if (err)
		return err;

	*data = cmd & 0xff;

	return 0;
}

static int mv88e6xxx_g2_eeprom_write8(struct mv88e6xxx_chip *chip,
				      u16 addr, u8 data)
{
	u16 cmd = MV88E6XXX_G2_EEPROM_CMD_OP_WRITE |
		MV88E6XXX_G2_EEPROM_CMD_WRITE_EN;
	int err;

	err = mv88e6xxx_g2_eeprom_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_write(chip, MV88E6390_G2_EEPROM_ADDR, addr);
	if (err)
		return err;

	return mv88e6xxx_g2_eeprom_cmd(chip, cmd | data);
}

static int mv88e6xxx_g2_eeprom_read16(struct mv88e6xxx_chip *chip,
				      u8 addr, u16 *data)
{
	u16 cmd = MV88E6XXX_G2_EEPROM_CMD_OP_READ | addr;
	int err;

	err = mv88e6xxx_g2_eeprom_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_eeprom_cmd(chip, cmd);
	if (err)
		return err;

	return mv88e6xxx_g2_read(chip, MV88E6352_G2_EEPROM_DATA, data);
}

static int mv88e6xxx_g2_eeprom_write16(struct mv88e6xxx_chip *chip,
				       u8 addr, u16 data)
{
	u16 cmd = MV88E6XXX_G2_EEPROM_CMD_OP_WRITE | addr;
	int err;

	err = mv88e6xxx_g2_eeprom_wait(chip);
	if (err)
		return err;

	err = mv88e6xxx_g2_write(chip, MV88E6352_G2_EEPROM_DATA, data);
	if (err)
		return err;

	return mv88e6xxx_g2_eeprom_cmd(chip, cmd);
}

int mv88e6xxx_g2_get_eeprom8(struct mv88e6xxx_chip *chip,
			     struct ethtool_eeprom *eeprom, u8 *data)
{
	unsigned int offset = eeprom->offset;
	unsigned int len = eeprom->len;
	int err;

	eeprom->len = 0;

	while (len) {
		err = mv88e6xxx_g2_eeprom_read8(chip, offset, data);
		if (err)
			return err;

		eeprom->len++;
		offset++;
		data++;
		len--;
	}

	return 0;
}

int mv88e6xxx_g2_set_eeprom8(struct mv88e6xxx_chip *chip,
			     struct ethtool_eeprom *eeprom, u8 *data)
{
	unsigned int offset = eeprom->offset;
	unsigned int len = eeprom->len;
	int err;

	eeprom->len = 0;

	while (len) {
		err = mv88e6xxx_g2_eeprom_write8(chip, offset, *data);
		if (err)
			return err;

		eeprom->len++;
		offset++;
		data++;
		len--;
	}

	return 0;
}

int mv88e6xxx_g2_get_eeprom16(struct mv88e6xxx_chip *chip,
			      struct ethtool_eeprom *eeprom, u8 *data)
{
	unsigned int offset = eeprom->offset;
	unsigned int len = eeprom->len;
	u16 val;
	int err;

	eeprom->len = 0;

	if (offset & 1) {
		err = mv88e6xxx_g2_eeprom_read16(chip, offset >> 1, &val);
		if (err)
			return err;

		*data++ = (val >> 8) & 0xff;

		offset++;
		len--;
		eeprom->len++;
	}

	while (len >= 2) {
		err = mv88e6xxx_g2_eeprom_read16(chip, offset >> 1, &val);
		if (err)
			return err;

		*data++ = val & 0xff;
		*data++ = (val >> 8) & 0xff;

		offset += 2;
		len -= 2;
		eeprom->len += 2;
	}

	if (len) {
		err = mv88e6xxx_g2_eeprom_read16(chip, offset >> 1, &val);
		if (err)
			return err;

		*data++ = val & 0xff;

		offset++;
		len--;
		eeprom->len++;
	}

	return 0;
}

int mv88e6xxx_g2_set_eeprom16(struct mv88e6xxx_chip *chip,
			      struct ethtool_eeprom *eeprom, u8 *data)
{
	unsigned int offset = eeprom->offset;
	unsigned int len = eeprom->len;
	u16 val;
	int err;

	/* Ensure the RO WriteEn bit is set */
	err = mv88e6xxx_g2_read(chip, MV88E6XXX_G2_EEPROM_CMD, &val);
	if (err)
		return err;

	if (!(val & MV88E6XXX_G2_EEPROM_CMD_WRITE_EN))
		return -EROFS;

	eeprom->len = 0;

	if (offset & 1) {
		err = mv88e6xxx_g2_eeprom_read16(chip, offset >> 1, &val);
		if (err)
			return err;

		val = (*data++ << 8) | (val & 0xff);

		err = mv88e6xxx_g2_eeprom_write16(chip, offset >> 1, val);
		if (err)
			return err;

		offset++;
		len--;
		eeprom->len++;
	}

	while (len >= 2) {
		val = *data++;
		val |= *data++ << 8;

		err = mv88e6xxx_g2_eeprom_write16(chip, offset >> 1, val);
		if (err)
			return err;

		offset += 2;
		len -= 2;
		eeprom->len += 2;
	}

	if (len) {
		err = mv88e6xxx_g2_eeprom_read16(chip, offset >> 1, &val);
		if (err)
			return err;

		val = (val & 0xff00) | *data++;

		err = mv88e6xxx_g2_eeprom_write16(chip, offset >> 1, val);
		if (err)
			return err;

		offset++;
		len--;
		eeprom->len++;
	}

	return 0;
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

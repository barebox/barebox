#ifndef _MV88E6XXX_GLOBAL2_H
#define _MV88E6XXX_GLOBAL2_H

#include "chip.h"

/* Offset 0x18: SMI PHY Command Register */
#define MV88E6XXX_G2_SMI_PHY_CMD			0x18
#define MV88E6XXX_G2_SMI_PHY_CMD_BUSY			0x8000
#define MV88E6390_G2_SMI_PHY_CMD_FUNC_MASK		0x6000
#define MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL		0x0000
#define MV88E6390_G2_SMI_PHY_CMD_FUNC_EXTERNAL		0x2000
#define MV88E6390_G2_SMI_PHY_CMD_FUNC_SETUP		0x4000
#define MV88E6XXX_G2_SMI_PHY_CMD_MODE_MASK		0x1000
#define MV88E6XXX_G2_SMI_PHY_CMD_MODE_45		0x0000
#define MV88E6XXX_G2_SMI_PHY_CMD_MODE_22		0x1000
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_MASK		0x0c00
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_22_WRITE_DATA	0x0400
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_22_READ_DATA	0x0800
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_45_WRITE_ADDR	0x0000
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_45_WRITE_DATA	0x0400
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_45_READ_DATA_INC	0x0800
#define MV88E6XXX_G2_SMI_PHY_CMD_OP_45_READ_DATA	0x0c00
#define MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK		0x03e0
#define MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK		0x001f
#define MV88E6XXX_G2_SMI_PHY_CMD_SETUP_PTR_MASK		0x03ff

/* Offset 0x19: SMI PHY Data Register */
#define MV88E6XXX_G2_SMI_PHY_DATA	0x19

/* Offset 0x14: EEPROM Command */
#define MV88E6XXX_G2_EEPROM_CMD			0x14
#define MV88E6XXX_G2_EEPROM_CMD_BUSY		0x8000
#define MV88E6XXX_G2_EEPROM_CMD_OP_MASK		0x7000
#define MV88E6XXX_G2_EEPROM_CMD_OP_WRITE	0x3000
#define MV88E6XXX_G2_EEPROM_CMD_OP_READ		0x4000
#define MV88E6XXX_G2_EEPROM_CMD_OP_LOAD		0x6000
#define MV88E6XXX_G2_EEPROM_CMD_RUNNING		0x0800
#define MV88E6XXX_G2_EEPROM_CMD_WRITE_EN	0x0400
#define MV88E6352_G2_EEPROM_CMD_ADDR_MASK	0x00ff
#define MV88E6390_G2_EEPROM_CMD_DATA_MASK	0x00ff

/* Offset 0x15: EEPROM Data */
#define MV88E6352_G2_EEPROM_DATA	0x15
#define MV88E6352_G2_EEPROM_DATA_MASK	0xffff

/* Offset 0x15: EEPROM Addr */
#define MV88E6390_G2_EEPROM_ADDR	0x15
#define MV88E6390_G2_EEPROM_ADDR_MASK	0xffff

int mv88e6xxx_g2_read(struct mv88e6xxx_chip *chip, int reg, u16 *val);
int mv88e6xxx_g2_write(struct mv88e6xxx_chip *chip, int reg, u16 val);
int mv88e6xxx_g2_wait(struct mv88e6xxx_chip *chip, int reg, u16 mask);

int mv88e6xxx_g2_smi_phy_read(struct mv88e6xxx_chip *chip,
			      struct mii_bus *bus,
			      int addr, int reg, u16 *val);
int mv88e6xxx_g2_smi_phy_write(struct mv88e6xxx_chip *chip,
			       struct mii_bus *bus,
			       int addr, int reg, u16 val);

int mv88e6xxx_g2_get_eeprom8(struct mv88e6xxx_chip *chip,
			     struct ethtool_eeprom *eeprom, u8 *data);
int mv88e6xxx_g2_set_eeprom8(struct mv88e6xxx_chip *chip,
			     struct ethtool_eeprom *eeprom, u8 *data);
int mv88e6xxx_g2_get_eeprom16(struct mv88e6xxx_chip *chip,
			      struct ethtool_eeprom *eeprom, u8 *data);
int mv88e6xxx_g2_set_eeprom16(struct mv88e6xxx_chip *chip,
			      struct ethtool_eeprom *eeprom, u8 *data);

#endif /* _MV88E6XXX_GLOBAL2_H */

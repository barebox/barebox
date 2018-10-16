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

int mv88e6xxx_g2_read(struct mv88e6xxx_chip *chip, int reg, u16 *val);
int mv88e6xxx_g2_write(struct mv88e6xxx_chip *chip, int reg, u16 val);
int mv88e6xxx_g2_wait(struct mv88e6xxx_chip *chip, int reg, u16 mask);

int mv88e6xxx_g2_smi_phy_read(struct mv88e6xxx_chip *chip,
			      struct mii_bus *bus,
			      int addr, int reg, u16 *val);
int mv88e6xxx_g2_smi_phy_write(struct mv88e6xxx_chip *chip,
			       struct mii_bus *bus,
			       int addr, int reg, u16 val);

#endif /* _MV88E6XXX_GLOBAL2_H */

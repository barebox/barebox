#ifndef _MV88E6XXX_CHIP_H
#define _MV88E6XXX_CHIP_H

#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/phy.h>

/* sub-devices MDIO addresses */
#define MV88E6XXX_SWITCH_GLOBAL_REGS_1		0x1b
#define MV88E6XXX_SWITCH_GLOBAL_REGS_2		0x1c

enum mv88e6xxx_family {
	MV88E6XXX_FAMILY_NONE,
	MV88E6XXX_FAMILY_6065,	/* 6031 6035 6061 6065 */
	MV88E6XXX_FAMILY_6095,	/* 6092 6095 */
	MV88E6XXX_FAMILY_6097,	/* 6046 6085 6096 6097 */
	MV88E6XXX_FAMILY_6165,	/* 6123 6161 6165 */
	MV88E6XXX_FAMILY_6185,	/* 6108 6121 6122 6131 6152 6155 6182 6185 */
	MV88E6XXX_FAMILY_6320,	/* 6320 6321 */
	MV88E6XXX_FAMILY_6341,	/* 6141 6341 */
	MV88E6XXX_FAMILY_6351,	/* 6171 6175 6350 6351 */
	MV88E6XXX_FAMILY_6352,	/* 6172 6176 6240 6352 */
	MV88E6XXX_FAMILY_6390,  /* 6190 6190X 6191 6290 6390 6390X */
};

struct mv88e6xxx_ops;

struct mv88e6xxx_info {
	enum mv88e6xxx_family family;
	u16 prod_num;
	const char *name;
	unsigned int num_ports;
	unsigned int port_base_addr;
	unsigned int global2_addr;

	const struct mv88e6xxx_ops *ops;
};

struct mv88e6xxx_chip {
	const struct mv88e6xxx_info *info;
	struct mii_bus *parent_miibus;
	struct mii_bus miibus;
	struct device_d *dev;
	int reset;
};

struct mv88e6xxx_ops {
	int (*phy_read)(struct mv88e6xxx_chip *chip,
			struct mii_bus *bus,
			int addr, int reg, u16 *val);
	int (*phy_write)(struct mv88e6xxx_chip *chip,
			 struct mii_bus *bus,
			 int addr, int reg, u16 val);
};

int mv88e6xxx_write(struct mv88e6xxx_chip *chip, int addr, int reg, u16 val);
int mv88e6xxx_read(struct mv88e6xxx_chip *chip, int addr, int reg, u16 *val);
int mv88e6xxx_wait(struct mv88e6xxx_chip *chip, int addr, int reg, u16 mask);

#endif /* _MV88E6XXX_CHIP_H */

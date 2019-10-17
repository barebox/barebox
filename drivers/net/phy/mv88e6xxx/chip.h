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

#define DSA_MAX_PORTS		12

struct mv88e6xxx_info {
	enum mv88e6xxx_family family;
	u16 prod_num;
	const char *name;
	unsigned int num_ports;
	unsigned int port_base_addr;
	unsigned int global1_addr;
	unsigned int global2_addr;

	const struct mv88e6xxx_ops *ops;
};

struct mv88e6xxx_port {
	u8 cmode;
};

struct mv88e6xxx_chip {
	const struct mv88e6xxx_info *info;
	struct mii_bus *parent_miibus;
	struct mii_bus miibus;
	struct device_d *dev;
	int reset;

	/* Array of port structures. */
	struct mv88e6xxx_port ports[DSA_MAX_PORTS];
};

struct ethtool_eeprom {
	__u32	offset;
	__u32	len;
};

struct phylink_link_state {
	phy_interface_t interface;
	int speed;
	int duplex;
	int pause;
	unsigned int link:1;
	unsigned int an_enabled:1;
	unsigned int an_complete:1;
};

struct mv88e6xxx_ops {
	int (*phy_read)(struct mv88e6xxx_chip *chip,
			struct mii_bus *bus,
			int addr, int reg, u16 *val);
	int (*phy_write)(struct mv88e6xxx_chip *chip,
			 struct mii_bus *bus,
			 int addr, int reg, u16 val);

	int (*get_eeprom)(struct mv88e6xxx_chip *chip,
			  struct ethtool_eeprom *eeprom, u8 *data);
	int (*set_eeprom)(struct mv88e6xxx_chip *chip,
			  struct ethtool_eeprom *eeprom, u8 *data);

	/* RGMII Receive/Transmit Timing Control
	 * Add delay on PHY_INTERFACE_MODE_RGMII_*ID, no delay otherwise.
	 */
	int (*port_set_rgmii_delay)(struct mv88e6xxx_chip *chip, int port,
				    phy_interface_t mode);

#define LINK_FORCED_DOWN	0
#define LINK_FORCED_UP		1
#define LINK_UNFORCED		-2

	/* Port's MAC link state
	 * Use LINK_FORCED_UP or LINK_FORCED_DOWN to force link up or down,
	 * or LINK_UNFORCED for normal link detection.
	 */
	int (*port_set_link)(struct mv88e6xxx_chip *chip, int port, int link);

#define DUPLEX_UNFORCED		-2

	/* Port's MAC duplex mode
	 *
	 * Use DUPLEX_HALF or DUPLEX_FULL to force half or full duplex,
	 * or DUPLEX_UNFORCED for normal duplex detection.
	 */
	int (*port_set_duplex)(struct mv88e6xxx_chip *chip, int port, int dup);

#define PAUSE_ON		1
#define PAUSE_OFF		0

	/* Enable/disable sending Pause */
	int (*port_set_pause)(struct mv88e6xxx_chip *chip, int port,
			      int pause);

#define SPEED_MAX		INT_MAX
#define SPEED_UNFORCED		-2

	/* Port's MAC speed (in Mbps)
	 *
	 * Depending on the chip, 10, 100, 200, 1000, 2500, 10000 are valid.
	 * Use SPEED_UNFORCED for normal detection, SPEED_MAX for max value.
	 */
	int (*port_set_speed)(struct mv88e6xxx_chip *chip, int port, int speed);

	/* CMODE control what PHY mode the MAC will use, eg. SGMII, RGMII, etc.
	 * Some chips allow this to be configured on specific ports.
	 */
	int (*port_set_cmode)(struct mv88e6xxx_chip *chip, int port,
			      phy_interface_t mode);

	/* Return the port link state, as required by phylink */
	int (*port_link_state)(struct mv88e6xxx_chip *chip, int port,
			       struct phylink_link_state *state);
};

int mv88e6xxx_write(struct mv88e6xxx_chip *chip, int addr, int reg, u16 val);
int mv88e6xxx_read(struct mv88e6xxx_chip *chip, int addr, int reg, u16 *val);
int mv88e6xxx_wait(struct mv88e6xxx_chip *chip, int addr, int reg, u16 mask);

#endif /* _MV88E6XXX_CHIP_H */

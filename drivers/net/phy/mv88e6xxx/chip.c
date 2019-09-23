#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/bitfield.h>
#include <linux/nvmem-provider.h>

#include <gpio.h>
#include <of_device.h>
#include <of_gpio.h>

#include "chip.h"
#include "global2.h"
#include "port.h"


/* List of supported models */
enum mv88e6xxx_model {
	MV88E6085,
	MV88E6095,
	MV88E6097,
	MV88E6123,
	MV88E6131,
	MV88E6141,
	MV88E6161,
	MV88E6165,
	MV88E6171,
	MV88E6172,
	MV88E6175,
	MV88E6176,
	MV88E6185,
	MV88E6190,
	MV88E6190X,
	MV88E6191,
	MV88E6240,
	MV88E6290,
	MV88E6320,
	MV88E6321,
	MV88E6341,
	MV88E6350,
	MV88E6351,
	MV88E6352,
	MV88E6390,
	MV88E6390X,
};

static const struct mv88e6xxx_ops mv88e6085_ops = {
	/* MV88E6XXX_FAMILY_6097 */
	/* FIXME: Was not ported due to lack of HW */
	.phy_read = NULL,
	.phy_write = NULL,
};

static const struct mv88e6xxx_ops mv88e6095_ops = {
	/* MV88E6XXX_FAMILY_6095 */
	/* FIXME: Was not ported due to lack of HW */
	.phy_read = NULL,
	.phy_write = NULL,
};

static const struct mv88e6xxx_ops mv88e6097_ops = {
	/* MV88E6XXX_FAMILY_6097 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6123_ops = {
	/* MV88E6XXX_FAMILY_6165 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6131_ops = {
	/* MV88E6XXX_FAMILY_6185 */
	/* FIXME: Was not ported due to lack of HW */
	.phy_read = NULL,
	.phy_write = NULL,
};

static const struct mv88e6xxx_ops mv88e6141_ops = {
	/* MV88E6XXX_FAMILY_6341 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6161_ops = {
	/* MV88E6XXX_FAMILY_6165 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6165_ops = {
	/* MV88E6XXX_FAMILY_6165 */
	/* FIXME: Was not ported due to lack of HW */
	.phy_read = NULL,
	.phy_write = NULL,
};

static const struct mv88e6xxx_ops mv88e6171_ops = {
	/* MV88E6XXX_FAMILY_6351 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6172_ops = {
	/* MV88E6XXX_FAMILY_6352 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6352_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6175_ops = {
	/* MV88E6XXX_FAMILY_6351 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6176_ops = {
	/* MV88E6XXX_FAMILY_6352 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6352_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6185_ops = {
	/* MV88E6XXX_FAMILY_6185 */
	/* FIXME: Was not ported due to lack of HW */
	.phy_read = NULL,
	.phy_write = NULL,
};

static const struct mv88e6xxx_ops mv88e6190_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6190x_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390x_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6191_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6240_ops = {
	/* MV88E6XXX_FAMILY_6352 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6352_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6290_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390_port_set_speed,
	.port_set_cmode = mv88e6390x_port_set_cmode,
};

static const struct mv88e6xxx_ops mv88e6320_ops = {
	/* MV88E6XXX_FAMILY_6320 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6321_ops = {
	/* MV88E6XXX_FAMILY_6320 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6341_ops = {
	/* MV88E6XXX_FAMILY_6341 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6350_ops = {
	/* MV88E6XXX_FAMILY_6351 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6351_ops = {
	/* MV88E6XXX_FAMILY_6351 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6185_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6352_ops = {
	/* MV88E6XXX_FAMILY_6352 */
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.get_eeprom = mv88e6xxx_g2_get_eeprom16,
	.set_eeprom = mv88e6xxx_g2_set_eeprom16,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6352_port_set_rgmii_delay,
	.port_set_speed = mv88e6352_port_set_speed,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6390_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390_port_set_speed,
	.port_set_cmode = mv88e6390x_port_set_cmode,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_ops mv88e6390x_ops = {
	/* MV88E6XXX_FAMILY_6390 */
	.get_eeprom = mv88e6xxx_g2_get_eeprom8,
	.set_eeprom = mv88e6xxx_g2_set_eeprom8,
	.phy_read = mv88e6xxx_g2_smi_phy_read,
	.phy_write = mv88e6xxx_g2_smi_phy_write,
	.port_set_link = mv88e6xxx_port_set_link,
	.port_set_duplex = mv88e6xxx_port_set_duplex,
	.port_set_rgmii_delay = mv88e6390_port_set_rgmii_delay,
	.port_set_speed = mv88e6390x_port_set_speed,
	.port_set_cmode = mv88e6390x_port_set_cmode,
	.port_link_state = mv88e6352_port_link_state,
};

static const struct mv88e6xxx_info mv88e6xxx_table[] = {
	[MV88E6085] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6085,
		.family = MV88E6XXX_FAMILY_6097,
		.name = "Marvell 88E6085",
		.num_ports = 10,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6085_ops,
	},

	[MV88E6095] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6095,
		.family = MV88E6XXX_FAMILY_6095,
		.name = "Marvell 88E6095/88E6095F",
		.num_ports = 11,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6095_ops,
	},

	[MV88E6097] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6097,
		.family = MV88E6XXX_FAMILY_6097,
		.name = "Marvell 88E6097/88E6097F",
		.num_ports = 11,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6097_ops,
	},

	[MV88E6123] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6123,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6123",
		.num_ports = 3,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6123_ops,
	},

	[MV88E6131] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6131,
		.family = MV88E6XXX_FAMILY_6185,
		.name = "Marvell 88E6131",
		.num_ports = 8,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6131_ops,
	},

	[MV88E6141] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6141,
		.family = MV88E6XXX_FAMILY_6341,
		.name = "Marvell 88E6341",
		.num_ports = 6,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6141_ops,
	},

	[MV88E6161] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6161,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6161",
		.num_ports = 6,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6161_ops,
	},

	[MV88E6165] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6165,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6165",
		.num_ports = 6,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6165_ops,
	},

	[MV88E6171] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6171,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6171",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6171_ops,
	},

	[MV88E6172] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6172,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6172",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6172_ops,
	},

	[MV88E6175] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6175,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6175",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6175_ops,
	},

	[MV88E6176] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6176,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6176",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6176_ops,
	},

	[MV88E6185] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6185,
		.family = MV88E6XXX_FAMILY_6185,
		.name = "Marvell 88E6185",
		.num_ports = 10,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6185_ops,
	},

	[MV88E6190] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6190,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6190",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6190_ops,
	},

	[MV88E6190X] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6190X,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6190X",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6190x_ops,
	},

	[MV88E6191] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6191,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6191",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6191_ops,
	},

	[MV88E6240] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6240,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6240",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6240_ops,
	},

	[MV88E6290] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6290,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6290",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6290_ops,
	},

	[MV88E6320] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6320,
		.family = MV88E6XXX_FAMILY_6320,
		.name = "Marvell 88E6320",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6320_ops,
	},

	[MV88E6321] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6321,
		.family = MV88E6XXX_FAMILY_6320,
		.name = "Marvell 88E6321",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6321_ops,
	},

	[MV88E6341] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6341,
		.family = MV88E6XXX_FAMILY_6341,
		.name = "Marvell 88E6341",
		.num_ports = 6,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6341_ops,
	},

	[MV88E6350] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6350,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6350",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6350_ops,
	},

	[MV88E6351] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6351,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6351",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6351_ops,
	},

	[MV88E6352] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6352,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6352",
		.num_ports = 7,
		.port_base_addr = 0x10,
		.global2_addr = 0x1c,
		.ops = &mv88e6352_ops,
	},

	[MV88E6390] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6390,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6390",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6390_ops,
	},

	[MV88E6390X] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6390X,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6390X",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
		.global2_addr = 0x1c,
		.ops = &mv88e6390x_ops,
	},
};

int mv88e6xxx_write(struct mv88e6xxx_chip *chip, int addr, int reg, u16 val)
{
	int ret;
	ret = mdiobus_write(chip->parent_miibus, addr, reg, val);
	if (ret < 0)
		return ret;

	dev_dbg(chip->dev, "<- addr: 0x%.2x reg: 0x%.2x val: 0x%.4x\n",
		addr, reg, val);

	return 0;
}

int mv88e6xxx_read(struct mv88e6xxx_chip *chip, int addr, int reg, u16 *val)
{
	int ret;

	ret = mdiobus_read(chip->parent_miibus, addr, reg);
	if (ret < 0)
		return ret;

	*val = ret & 0xffff;

	dev_dbg(chip->dev, "-> addr: 0x%.2x reg: 0x%.2x val: 0x%.4x\n",
		addr, reg, *val);

	return 0;
}

int mv88e6xxx_wait(struct mv88e6xxx_chip *chip, int addr, int reg, u16 mask)
{
	int i;

	for (i = 0; i < 16; i++) {
		u16 val;
		int err;

		err = mv88e6xxx_read(chip, addr, reg, &val);
		if (err)
			return err;

		if (!(val & mask))
			return 0;

		udelay(2000);
	}

	dev_err(chip->dev, "Timeout while waiting for switch\n");
	return -ETIMEDOUT;
}

static int mv88e6xxx_mdio_read(struct mii_bus *bus, int phy, int reg)
{
	struct mv88e6xxx_chip *chip = bus->priv;
	u16 val;
	int err;

	if (!chip->info->ops->phy_read)
		return -EOPNOTSUPP;

	err = chip->info->ops->phy_read(chip, bus, phy, reg, &val);

	if (reg == MII_PHYSID2) {
		/* Some internal PHYS don't have a model number.  Use
		 * the mv88e6390 family model number instead.
		 */
		if (!(val & 0x3f0))
			val |= MV88E6XXX_PORT_SWITCH_ID_PROD_6390 >> 4;
	}

	return err ?: val;
}

static int mv88e6xxx_mdio_write(struct mii_bus *bus, int phy, int reg, u16 val)
{
	struct mv88e6xxx_chip *chip = bus->priv;
	int err;

	if (!chip->info->ops->phy_write)
		return -EOPNOTSUPP;

	err = chip->info->ops->phy_write(chip, bus, phy, reg, val);

	return err;
}

static const struct mv88e6xxx_info *
mv88e6xxx_lookup_info(unsigned int prod_num)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mv88e6xxx_table); ++i)
		if (mv88e6xxx_table[i].prod_num == prod_num)
			return &mv88e6xxx_table[i];

	return NULL;
}

static int mv88e6xxx_detect(struct mv88e6xxx_chip *chip)
{
	const struct mv88e6xxx_info *info;
	unsigned int prod_num, rev;
	u16 id;
	int err;

	err = mv88e6xxx_port_read(chip, 0, MV88E6XXX_PORT_SWITCH_ID, &id);
	if (err)
		return err;

	prod_num = id & MV88E6XXX_PORT_SWITCH_ID_PROD_MASK;
	rev = id & MV88E6XXX_PORT_SWITCH_ID_REV_MASK;

	info = mv88e6xxx_lookup_info(prod_num);
	if (!info)
		return -ENODEV;

	/* Update the compatible info with the probed one */
	chip->info = info;

	dev_info(chip->dev, "switch 0x%x detected: %s, revision %u\n",
		 chip->info->prod_num, chip->info->name, rev);

	return 0;
}

/*
 * Linux driver has this delay at 20ms, but it doesn't seem to be
 * enough in Barebox and trying to access switch registers immediately
 * after this function will return all F's on some platforms
 * tested. Increasing this to 50ms seem to resolve the issue.
 */
static void mv88e6xxx_hardware_reset_delay(void)
{
	udelay(50000);
}

static void mv88e6xxx_hardware_reset(struct mv88e6xxx_chip *chip)
{
	/* If there is a GPIO connected to the reset pin, toggle it */
	if (gpio_is_valid(chip->reset)) {
		gpio_set_active(chip->reset, 1);
		mv88e6xxx_hardware_reset_delay();
		gpio_set_active(chip->reset, 0);
		mv88e6xxx_hardware_reset_delay();
	}
}

static int mv88e6xxx_switch_reset(struct mv88e6xxx_chip *chip)
{
	mv88e6xxx_hardware_reset(chip);
	return 0;
}

static int mv88e6xxx_eeprom_read(struct device_d *dev, const int offset,
				 void *val, int bytes)
{
	struct mv88e6xxx_chip *chip = dev->parent->priv;
	struct ethtool_eeprom eeprom = {
		.offset = offset,
		.len = bytes,
	};

	if (!chip->info->ops->get_eeprom)
		return -ENOTSUPP;

	return chip->info->ops->get_eeprom(chip, &eeprom, val);
}

static int mv88e6xxx_eeprom_write(struct device_d *dev, const int offset,
				  const void *val, int bytes)
{
	struct mv88e6xxx_chip *chip = dev->parent->priv;
	struct ethtool_eeprom eeprom = {
		.offset = offset,
		.len = bytes,
	};

	if (!chip->info->ops->set_eeprom)
		return -ENOTSUPP;

	return chip->info->ops->set_eeprom(chip, &eeprom, (void *)val);
}

static const struct nvmem_bus mv88e6xxx_eeprom_nvmem_bus = {
	.write = mv88e6xxx_eeprom_write,
	.read  = mv88e6xxx_eeprom_read,
};

static int mv88e6xxx_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct device_node *mdio_node;
	struct mv88e6xxx_chip *chip;
	enum of_gpio_flags of_flags;
	u32 eeprom_len = 0;
	int err;
	u32 reg;

	err = of_property_read_u32(np, "reg", &reg);
	if (err) {
		dev_err(dev, "Couldn't determine switch MDIO address\n");
		return err;
	}

	if (reg) {
		dev_err(dev, "Only single-chip address mode is supported\n");
		return -ENOTSUPP;
	}

	chip = xzalloc(sizeof(struct mv88e6xxx_chip));
	chip->dev = dev;
	dev->priv = chip;
	chip->info = of_device_get_match_data(dev);

	of_property_read_u32(np, "eeprom-length", &eeprom_len);

	chip->parent_miibus = of_mdio_find_bus(np->parent);
	if (!chip->parent_miibus)
		return -EPROBE_DEFER;

	chip->reset = of_get_named_gpio_flags(np, "reset-gpios", 0, &of_flags);
	if (gpio_is_valid(chip->reset)) {
		unsigned long flags = GPIOF_OUT_INIT_INACTIVE;
		char *name;

		if (of_flags & OF_GPIO_ACTIVE_LOW)
			flags |= GPIOF_ACTIVE_LOW;

		name = basprintf("%s reset", dev_name(dev));
		err = gpio_request_one(chip->reset, flags, name);
		if (err < 0)
			return err;
		/*
		 * We assume that reset line was previously held low
		 * and give the switch time to initialize before
		 * trying to read its registers
		 */
		mv88e6xxx_hardware_reset_delay();
	}

	err = mv88e6xxx_detect(chip);
	if (err)
		return err;

	err = mv88e6xxx_switch_reset(chip);
	if (err)
		return err;

	if (eeprom_len) {
		struct nvmem_config config = {
			.name = basprintf("%s-eeprom", dev_name(dev)),
			.dev = dev,
			.word_size = 1,
			.stride = 1,
			.size = eeprom_len,
			.read_only = false,
			.bus = &mv88e6xxx_eeprom_nvmem_bus,
		};

		if (IS_ERR(nvmem_register(&config)))
			dev_err(dev, "Failed to register EEPROM\n");
	}

	/*
	 * In single-chip address mode addresses 0x10 -
	 * port_base_address are reserved to access various switch
	 * registers and do not correspond to any PHYs, so we mask
	 * them to pervent from being exposed.
	 */
	chip->parent_miibus->phy_mask |= GENMASK(0x1f,
						 chip->info->port_base_addr);
	/*
	 * Mask all of the devices on child MDIO bus. Call to
	 * mv88e6xxx_port_probe() will unmask port that can be probed
	 * using standard methods
	 */
	chip->miibus.phy_mask |= GENMASK(0x1f, 0x00);

	chip->miibus.read = mv88e6xxx_mdio_read;
	chip->miibus.write = mv88e6xxx_mdio_write;

	chip->miibus.priv = chip;
	chip->miibus.parent = dev;

	mdio_node = of_get_child_by_name(np, "mdio");
	if (mdio_node)
		chip->miibus.dev.device_node = mdio_node;

	err = mv88e6xxx_port_probe(chip);
	if (err)
		return err;

	return mdiobus_register(&chip->miibus);
}

static const struct of_device_id mv88e6xxx_of_match[] = {
	{
		.compatible = "marvell,mv88e6085",
		.data = &mv88e6xxx_table[MV88E6085],
	},
	{
		.compatible = "marvell,mv88e6190",
		.data = &mv88e6xxx_table[MV88E6190],
	},
	{},
};

static struct driver_d mv88e6xxx_driver = {
	.name	       = "mv88e6085",
	.probe         = mv88e6xxx_probe,
	.of_compatible = mv88e6xxx_of_match,
};
device_platform_driver(mv88e6xxx_driver);

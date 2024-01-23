// SPDX-License-Identifier: GPL-2.0
/* Realtek SMI subdriver for the Realtek RTL8365MB-VC ethernet switch.
 *
 * Copyright (C) 2021 Alvin Šipraga <alsi@bang-olufsen.dk>
 * Copyright (C) 2021 Michael Rasmussen <mir@bang-olufsen.dk>
 *
 * The RTL8365MB-VC is a 4+1 port 10/100/1000M switch controller. It includes 4
 * integrated PHYs for the user facing ports, and an extension interface which
 * can be connected to the CPU - or another PHY - via either MII, RMII, or
 * RGMII. The switch is configured via the Realtek Simple Management Interface
 * (SMI), which uses the MDIO/MDC lines.
 *
 * Below is a simplified block diagram of the chip and its relevant interfaces.
 *
 *                          .-----------------------------------.
 *                          |                                   |
 *         UTP <---------------> Giga PHY <-> PCS <-> P0 GMAC   |
 *         UTP <---------------> Giga PHY <-> PCS <-> P1 GMAC   |
 *         UTP <---------------> Giga PHY <-> PCS <-> P2 GMAC   |
 *         UTP <---------------> Giga PHY <-> PCS <-> P3 GMAC   |
 *                          |                                   |
 *     CPU/PHY <-MII/RMII/RGMII--->  Extension  <---> Extension |
 *                          |       interface 1        GMAC 1   |
 *                          |                                   |
 *     SMI driver/ <-MDC/SCL---> Management    ~~~~~~~~~~~~~~   |
 *        EEPROM   <-MDIO/SDA--> interface     ~REALTEK ~~~~~   |
 *                          |                  ~RTL8365MB ~~~   |
 *                          |                  ~GXXXC TAIWAN~   |
 *        GPIO <--------------> Reset          ~~~~~~~~~~~~~~   |
 *                          |                                   |
 *      Interrupt  <----------> Link UP/DOWN events             |
 *      controller          |                                   |
 *                          '-----------------------------------'
 *
 * The driver uses DSA to integrate the 4 user and 1 extension ports into the
 * kernel. Netdevices are created for the user ports, as are PHY devices for
 * their integrated PHYs. The device tree firmware should also specify the link
 * partner of the extension port - either via a fixed-link or other phy-handle.
 * See the device tree bindings for more detailed information. Note that the
 * driver has only been tested with a fixed-link, but in principle it should not
 * matter.
 *
 * NOTE: Currently, only the RGMII interface is implemented in this driver.
 *
 * The interrupt line is asserted on link UP/DOWN events. The driver creates a
 * custom irqchip to handle this interrupt and demultiplex the events by reading
 * the status registers via SMI. Interrupts are then propagated to the relevant
 * PHY device.
 *
 * The EEPROM contains initial register values which the chip will read over I2C
 * upon hardware reset. It is also possible to omit the EEPROM. In both cases,
 * the driver will manually reprogram some registers using jam tables to reach
 * an initial state defined by the vendor driver.
 *
 * This Linux driver is written based on an OS-agnostic vendor driver from
 * Realtek. The reference GPL-licensed sources can be found in the OpenWrt
 * source tree under the name rtl8367c. The vendor driver claims to support a
 * number of similar switch controllers from Realtek, but the only hardware we
 * have is the RTL8365MB-VC. Moreover, there does not seem to be any chip under
 * the name RTL8367C. Although one wishes that the 'C' stood for some kind of
 * common hardware revision, there exist examples of chips with the suffix -VC
 * which are explicitly not supported by the rtl8367c driver and which instead
 * require the rtl8367d vendor driver. With all this uncertainty, the driver has
 * been modestly named rtl8365mb. Future implementors may wish to rename things
 * accordingly.
 *
 * In the same family of chips, some carry up to 8 user ports and up to 2
 * extension ports. Where possible this driver tries to make things generic, but
 * more work must be done to support these configurations. According to
 * documentation from Realtek, the family should include the following chips:
 *
 *  - RTL8363NB
 *  - RTL8363NB-VB
 *  - RTL8363SC
 *  - RTL8363SC-VB
 *  - RTL8364NB
 *  - RTL8364NB-VB
 *  - RTL8365MB-VC
 *  - RTL8366SC
 *  - RTL8367RB-VB
 *  - RTL8367SB
 *  - RTL8367S
 *  - RTL8370MB
 *  - RTL8310SR
 *
 * Some of the register logic for these additional chips has been skipped over
 * while implementing this driver. It is therefore not possible to assume that
 * things will work out-of-the-box for other chips, and a careful review of the
 * vendor driver may be needed to expand support. The RTL8365MB-VC seems to be
 * one of the simpler chips.
 */

#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/export.h>
#include <linux/regmap.h>
#include <net.h>
#include <linux/if_bridge.h>

#include "realtek.h"

/* Family-specific data and limits */
#define RTL8365MB_PHYADDRMAX		7
#define RTL8365MB_NUM_PHYREGS		32
#define RTL8365MB_PHYREGMAX		(RTL8365MB_NUM_PHYREGS - 1)
#define RTL8365MB_MAX_NUM_PORTS		11
#define RTL8365MB_MAX_NUM_EXTINTS	3
#define RTL8365MB_LEARN_LIMIT_MAX	2112

/* Chip identification registers */
#define RTL8365MB_CHIP_ID_REG		0x1300

#define RTL8365MB_CHIP_VER_REG		0x1301

#define RTL8365MB_MAGIC_REG		0x13C2
#define   RTL8365MB_MAGIC_VALUE		0x0249

/* Chip reset register */
#define RTL8365MB_CHIP_RESET_REG	0x1322
#define RTL8365MB_CHIP_RESET_SW_MASK	0x0002
#define RTL8365MB_CHIP_RESET_HW_MASK	0x0001

/* Interrupt polarity register */
#define RTL8365MB_INTR_POLARITY_REG	0x1100
#define   RTL8365MB_INTR_POLARITY_MASK	0x0001
#define   RTL8365MB_INTR_POLARITY_HIGH	0
#define   RTL8365MB_INTR_POLARITY_LOW	1

/* Interrupt control/status register - enable/check specific interrupt types */
#define RTL8365MB_INTR_CTRL_REG			0x1101
#define RTL8365MB_INTR_STATUS_REG		0x1102
#define   RTL8365MB_INTR_SLIENT_START_2_MASK	0x1000
#define   RTL8365MB_INTR_SLIENT_START_MASK	0x0800
#define   RTL8365MB_INTR_ACL_ACTION_MASK	0x0200
#define   RTL8365MB_INTR_CABLE_DIAG_FIN_MASK	0x0100
#define   RTL8365MB_INTR_INTERRUPT_8051_MASK	0x0080
#define   RTL8365MB_INTR_LOOP_DETECTION_MASK	0x0040
#define   RTL8365MB_INTR_GREEN_TIMER_MASK	0x0020
#define   RTL8365MB_INTR_SPECIAL_CONGEST_MASK	0x0010
#define   RTL8365MB_INTR_SPEED_CHANGE_MASK	0x0008
#define   RTL8365MB_INTR_LEARN_OVER_MASK	0x0004
#define   RTL8365MB_INTR_METER_EXCEEDED_MASK	0x0002
#define   RTL8365MB_INTR_LINK_CHANGE_MASK	0x0001
#define   RTL8365MB_INTR_ALL_MASK                      \
		(RTL8365MB_INTR_SLIENT_START_2_MASK |  \
		 RTL8365MB_INTR_SLIENT_START_MASK |    \
		 RTL8365MB_INTR_ACL_ACTION_MASK |      \
		 RTL8365MB_INTR_CABLE_DIAG_FIN_MASK |  \
		 RTL8365MB_INTR_INTERRUPT_8051_MASK |  \
		 RTL8365MB_INTR_LOOP_DETECTION_MASK |  \
		 RTL8365MB_INTR_GREEN_TIMER_MASK |     \
		 RTL8365MB_INTR_SPECIAL_CONGEST_MASK | \
		 RTL8365MB_INTR_SPEED_CHANGE_MASK |    \
		 RTL8365MB_INTR_LEARN_OVER_MASK |      \
		 RTL8365MB_INTR_METER_EXCEEDED_MASK |  \
		 RTL8365MB_INTR_LINK_CHANGE_MASK)

/* Per-port interrupt type status registers */
#define RTL8365MB_PORT_LINKDOWN_IND_REG		0x1106
#define   RTL8365MB_PORT_LINKDOWN_IND_MASK	0x07FF

#define RTL8365MB_PORT_LINKUP_IND_REG		0x1107
#define   RTL8365MB_PORT_LINKUP_IND_MASK	0x07FF

/* PHY indirect access registers */
#define RTL8365MB_INDIRECT_ACCESS_CTRL_REG			0x1F00
#define   RTL8365MB_INDIRECT_ACCESS_CTRL_RW_MASK		0x0002
#define   RTL8365MB_INDIRECT_ACCESS_CTRL_RW_READ		0
#define   RTL8365MB_INDIRECT_ACCESS_CTRL_RW_WRITE		1
#define   RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_MASK		0x0001
#define   RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_VALUE		1
#define RTL8365MB_INDIRECT_ACCESS_STATUS_REG			0x1F01
#define RTL8365MB_INDIRECT_ACCESS_ADDRESS_REG			0x1F02
#define   RTL8365MB_INDIRECT_ACCESS_ADDRESS_OCPADR_5_1_MASK	GENMASK(4, 0)
#define   RTL8365MB_INDIRECT_ACCESS_ADDRESS_PHYNUM_MASK		GENMASK(7, 5)
#define   RTL8365MB_INDIRECT_ACCESS_ADDRESS_OCPADR_9_6_MASK	GENMASK(11, 8)
#define   RTL8365MB_PHY_BASE					0x2000
#define RTL8365MB_INDIRECT_ACCESS_WRITE_DATA_REG		0x1F03
#define RTL8365MB_INDIRECT_ACCESS_READ_DATA_REG			0x1F04

/* PHY OCP address prefix register */
#define RTL8365MB_GPHY_OCP_MSB_0_REG			0x1D15
#define   RTL8365MB_GPHY_OCP_MSB_0_CFG_CPU_OCPADR_MASK	0x0FC0
#define RTL8365MB_PHY_OCP_ADDR_PREFIX_MASK		0xFC00

/* The PHY OCP addresses of PHY registers 0~31 start here */
#define RTL8365MB_PHY_OCP_ADDR_PHYREG_BASE		0xA400

/* External interface port mode values - used in DIGITAL_INTERFACE_SELECT */
#define RTL8365MB_EXT_PORT_MODE_DISABLE		0
#define RTL8365MB_EXT_PORT_MODE_RGMII		1
#define RTL8365MB_EXT_PORT_MODE_MII_MAC		2
#define RTL8365MB_EXT_PORT_MODE_MII_PHY		3
#define RTL8365MB_EXT_PORT_MODE_TMII_MAC	4
#define RTL8365MB_EXT_PORT_MODE_TMII_PHY	5
#define RTL8365MB_EXT_PORT_MODE_GMII		6
#define RTL8365MB_EXT_PORT_MODE_RMII_MAC	7
#define RTL8365MB_EXT_PORT_MODE_RMII_PHY	8
#define RTL8365MB_EXT_PORT_MODE_SGMII		9
#define RTL8365MB_EXT_PORT_MODE_HSGMII		10
#define RTL8365MB_EXT_PORT_MODE_1000X_100FX	11
#define RTL8365MB_EXT_PORT_MODE_1000X		12
#define RTL8365MB_EXT_PORT_MODE_100FX		13

/* External interface mode configuration registers 0~1 */
#define RTL8365MB_DIGITAL_INTERFACE_SELECT_REG0		0x1305 /* EXT1 */
#define RTL8365MB_DIGITAL_INTERFACE_SELECT_REG1		0x13C3 /* EXT2 */
#define RTL8365MB_DIGITAL_INTERFACE_SELECT_REG(_extint) \
		((_extint) == 1 ? RTL8365MB_DIGITAL_INTERFACE_SELECT_REG0 : \
		 (_extint) == 2 ? RTL8365MB_DIGITAL_INTERFACE_SELECT_REG1 : \
		 0x0)
#define   RTL8365MB_DIGITAL_INTERFACE_SELECT_MODE_MASK(_extint) \
		(0xF << (((_extint) % 2)))
#define   RTL8365MB_DIGITAL_INTERFACE_SELECT_MODE_OFFSET(_extint) \
		(((_extint) % 2) * 4)

/* External interface RGMII TX/RX delay configuration registers 0~2 */
#define RTL8365MB_EXT_RGMXF_REG0		0x1306 /* EXT0 */
#define RTL8365MB_EXT_RGMXF_REG1		0x1307 /* EXT1 */
#define RTL8365MB_EXT_RGMXF_REG2		0x13C5 /* EXT2 */
#define RTL8365MB_EXT_RGMXF_REG(_extint) \
		((_extint) == 0 ? RTL8365MB_EXT_RGMXF_REG0 : \
		 (_extint) == 1 ? RTL8365MB_EXT_RGMXF_REG1 : \
		 (_extint) == 2 ? RTL8365MB_EXT_RGMXF_REG2 : \
		 0x0)
#define   RTL8365MB_EXT_RGMXF_RXDELAY_MASK	0x0007
#define   RTL8365MB_EXT_RGMXF_TXDELAY_MASK	0x0008

/* External interface port speed values - used in DIGITAL_INTERFACE_FORCE */
#define RTL8365MB_PORT_SPEED_10M	0
#define RTL8365MB_PORT_SPEED_100M	1
#define RTL8365MB_PORT_SPEED_1000M	2

/* External interface force configuration registers 0~2 */
#define RTL8365MB_DIGITAL_INTERFACE_FORCE_REG0		0x1310 /* EXT0 */
#define RTL8365MB_DIGITAL_INTERFACE_FORCE_REG1		0x1311 /* EXT1 */
#define RTL8365MB_DIGITAL_INTERFACE_FORCE_REG2		0x13C4 /* EXT2 */
#define RTL8365MB_DIGITAL_INTERFACE_FORCE_REG(_extint) \
		((_extint) == 0 ? RTL8365MB_DIGITAL_INTERFACE_FORCE_REG0 : \
		 (_extint) == 1 ? RTL8365MB_DIGITAL_INTERFACE_FORCE_REG1 : \
		 (_extint) == 2 ? RTL8365MB_DIGITAL_INTERFACE_FORCE_REG2 : \
		 0x0)
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_EN_MASK		0x1000
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_NWAY_MASK		0x0080
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_TXPAUSE_MASK	0x0040
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_RXPAUSE_MASK	0x0020
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_LINK_MASK		0x0010
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_DUPLEX_MASK		0x0004
#define   RTL8365MB_DIGITAL_INTERFACE_FORCE_SPEED_MASK		0x0003

/* CPU port mask register - controls which ports are treated as CPU ports */
#define RTL8365MB_CPU_PORT_MASK_REG	0x1219
#define   RTL8365MB_CPU_PORT_MASK_MASK	0x07FF

/* CPU control register */
#define RTL8365MB_CPU_CTRL_REG			0x121A
#define   RTL8365MB_CPU_CTRL_TRAP_PORT_EXT_MASK	0x0400
#define   RTL8365MB_CPU_CTRL_TAG_FORMAT_MASK	0x0200
#define   RTL8365MB_CPU_CTRL_RXBYTECOUNT_MASK	0x0080
#define   RTL8365MB_CPU_CTRL_TAG_POSITION_MASK	0x0040
#define   RTL8365MB_CPU_CTRL_TRAP_PORT_MASK	0x0038
#define   RTL8365MB_CPU_CTRL_INSERTMODE_MASK	0x0006
#define   RTL8365MB_CPU_CTRL_EN_MASK		0x0001

/* Maximum packet length register */
#define RTL8365MB_CFG0_MAX_LEN_REG	0x088C
#define   RTL8365MB_CFG0_MAX_LEN_MASK	0x3FFF

/* Port learning limit registers */
#define RTL8365MB_LUT_PORT_LEARN_LIMIT_BASE		0x0A20
#define RTL8365MB_LUT_PORT_LEARN_LIMIT_REG(_physport) \
		(RTL8365MB_LUT_PORT_LEARN_LIMIT_BASE + (_physport))

/* Port isolation (forwarding mask) registers */
#define RTL8365MB_PORT_ISOLATION_REG_BASE		0x08A2
#define RTL8365MB_PORT_ISOLATION_REG(_physport) \
		(RTL8365MB_PORT_ISOLATION_REG_BASE + (_physport))
#define   RTL8365MB_PORT_ISOLATION_MASK			0x07FF

/* MSTP port state registers - indexed by tree instance */
#define RTL8365MB_MSTI_CTRL_BASE			0x0A00
#define RTL8365MB_MSTI_CTRL_REG(_msti, _physport) \
		(RTL8365MB_MSTI_CTRL_BASE + ((_msti) << 1) + ((_physport) >> 3))
#define   RTL8365MB_MSTI_CTRL_PORT_STATE_OFFSET(_physport) ((_physport) << 1)
#define   RTL8365MB_MSTI_CTRL_PORT_STATE_MASK(_physport) \
		(0x3 << RTL8365MB_MSTI_CTRL_PORT_STATE_OFFSET((_physport)))

struct rtl8365mb_jam_tbl_entry {
	u16 reg;
	u16 val;
};

/* Lifted from the vendor driver sources */
static const struct rtl8365mb_jam_tbl_entry rtl8365mb_init_jam_8365mb_vc[] = {
	{ 0x13EB, 0x15BB }, { 0x1303, 0x06D6 }, { 0x1304, 0x0700 },
	{ 0x13E2, 0x003F }, { 0x13F9, 0x0090 }, { 0x121E, 0x03CA },
	{ 0x1233, 0x0352 }, { 0x1237, 0x00A0 }, { 0x123A, 0x0030 },
	{ 0x1239, 0x0084 }, { 0x0301, 0x1000 }, { 0x1349, 0x001F },
	{ 0x18E0, 0x4004 }, { 0x122B, 0x241C }, { 0x1305, 0xC000 },
	{ 0x13F0, 0x0000 },
};

static const struct rtl8365mb_jam_tbl_entry rtl8365mb_init_jam_common[] = {
	{ 0x1200, 0x7FCB }, { 0x0884, 0x0003 }, { 0x06EB, 0x0001 },
	{ 0x03Fa, 0x0007 }, { 0x08C8, 0x00C0 }, { 0x0A30, 0x020E },
	{ 0x0800, 0x0000 }, { 0x0802, 0x0000 }, { 0x09DA, 0x0013 },
	{ 0x1D32, 0x0002 },
};

enum rtl8365mb_phy_interface_mode {
	RTL8365MB_PHY_INTERFACE_MODE_INVAL = 0,
	RTL8365MB_PHY_INTERFACE_MODE_INTERNAL = BIT(0),
	RTL8365MB_PHY_INTERFACE_MODE_MII = BIT(1),
	RTL8365MB_PHY_INTERFACE_MODE_TMII = BIT(2),
	RTL8365MB_PHY_INTERFACE_MODE_RMII = BIT(3),
	RTL8365MB_PHY_INTERFACE_MODE_RGMII = BIT(4),
	RTL8365MB_PHY_INTERFACE_MODE_SGMII = BIT(5),
	RTL8365MB_PHY_INTERFACE_MODE_HSGMII = BIT(6),
};

/**
 * struct rtl8365mb_extint - external interface info
 * @port: the port with an external interface
 * @id: the external interface ID, which is either 0, 1, or 2
 * @supported_interfaces: a bitmask of supported PHY interface modes
 *
 * Represents a mapping: port -> { id, supported_interfaces }. To be embedded
 * in &struct rtl8365mb_chip_info for every port with an external interface.
 */
struct rtl8365mb_extint {
	int port;
	int id;
	unsigned int supported_interfaces;
};

/**
 * struct rtl8365mb_chip_info - static chip-specific info
 * @name: human-readable chip name
 * @chip_id: chip identifier
 * @chip_ver: chip silicon revision
 * @extints: available external interfaces
 * @jam_table: chip-specific initialization jam table
 * @jam_size: size of the chip's jam table
 *
 * These data are specific to a given chip in the family of switches supported
 * by this driver. When adding support for another chip in the family, a new
 * chip info should be added to the rtl8365mb_chip_infos array.
 */
struct rtl8365mb_chip_info {
	const char *name;
	u32 chip_id;
	u32 chip_ver;
	const struct rtl8365mb_extint extints[RTL8365MB_MAX_NUM_EXTINTS];
	const struct rtl8365mb_jam_tbl_entry *jam_table;
	size_t jam_size;
};

/* Chip info for each supported switch in the family */
#define PHY_INTF(_mode) (RTL8365MB_PHY_INTERFACE_MODE_ ## _mode)
static const struct rtl8365mb_chip_info rtl8365mb_chip_infos[] = {
	{
		.name = "RTL8365MB-VC",
		.chip_id = 0x6367,
		.chip_ver = 0x0040,
		.extints = {
			{ 6, 1, PHY_INTF(MII) | PHY_INTF(TMII) |
				PHY_INTF(RMII) | PHY_INTF(RGMII) },
		},
		.jam_table = rtl8365mb_init_jam_8365mb_vc,
		.jam_size = ARRAY_SIZE(rtl8365mb_init_jam_8365mb_vc),
	},
	{
		.name = "RTL8367S",
		.chip_id = 0x6367,
		.chip_ver = 0x00A0,
		.extints = {
			{ 6, 1, PHY_INTF(SGMII) | PHY_INTF(HSGMII) },
			{ 7, 2, PHY_INTF(MII) | PHY_INTF(TMII) |
				PHY_INTF(RMII) | PHY_INTF(RGMII) },
		},
		.jam_table = rtl8365mb_init_jam_8365mb_vc,
		.jam_size = ARRAY_SIZE(rtl8365mb_init_jam_8365mb_vc),
	},
	{
		.name = "RTL8367RB-VB",
		.chip_id = 0x6367,
		.chip_ver = 0x0020,
		.extints = {
			{ 6, 1, PHY_INTF(MII) | PHY_INTF(TMII) |
				PHY_INTF(RMII) | PHY_INTF(RGMII) },
			{ 7, 2, PHY_INTF(MII) | PHY_INTF(TMII) |
				PHY_INTF(RMII) | PHY_INTF(RGMII) },
		},
		.jam_table = rtl8365mb_init_jam_8365mb_vc,
		.jam_size = ARRAY_SIZE(rtl8365mb_init_jam_8365mb_vc),
	},
};

enum rtl8365mb_stp_state {
	RTL8365MB_STP_STATE_DISABLED = 0,
	RTL8365MB_STP_STATE_BLOCKING = 1,
	RTL8365MB_STP_STATE_LEARNING = 2,
	RTL8365MB_STP_STATE_FORWARDING = 3,
};

enum rtl8365mb_cpu_insert {
	RTL8365MB_CPU_INSERT_TO_ALL = 0,
	RTL8365MB_CPU_INSERT_TO_TRAPPING = 1,
	RTL8365MB_CPU_INSERT_TO_NONE = 2,
};

enum rtl8365mb_cpu_position {
	RTL8365MB_CPU_POS_AFTER_SA = 0,
	RTL8365MB_CPU_POS_BEFORE_CRC = 1,
};

enum rtl8365mb_cpu_format {
	RTL8365MB_CPU_FORMAT_8BYTES = 0,
	RTL8365MB_CPU_FORMAT_4BYTES = 1,
};

enum rtl8365mb_cpu_rxlen {
	RTL8365MB_CPU_RXLEN_72BYTES = 0,
	RTL8365MB_CPU_RXLEN_64BYTES = 1,
};

/**
 * struct rtl8365mb_cpu - CPU port configuration
 * @mask: port mask of ports that parse should parse CPU tags
 * @trap_port: forward trapped frames to this port
 * @insert: CPU tag insertion mode in switch->CPU frames
 * @position: position of CPU tag in frame
 * @rx_length: minimum CPU RX length
 * @format: CPU tag format
 *
 * Represents the CPU tagging and CPU port configuration of the switch. These
 * settings are configurable at runtime.
 */
struct rtl8365mb_cpu {
	u32 mask;
	u32 trap_port;
	enum rtl8365mb_cpu_insert insert;
	enum rtl8365mb_cpu_position position;
	enum rtl8365mb_cpu_rxlen rx_length;
	enum rtl8365mb_cpu_format format;
};

/**
 * struct rtl8365mb_port - private per-port data
 * @priv: pointer to parent realtek_priv data
 * @index: DSA port index, same as dsa_port::index
 */
struct rtl8365mb_port {
	struct realtek_priv *priv;
	unsigned int index;
};

/**
 * struct rtl8365mb - driver private data
 * @priv: pointer to parent realtek_priv data
 * @irq: registered IRQ or zero
 * @chip_info: chip-specific info about the attached switch
 * @cpu: CPU tagging and CPU port configuration for this chip
 * @ports: per-port data
 *
 * Private data for this driver.
 */
struct rtl8365mb {
	struct realtek_priv *priv;
	const struct rtl8365mb_chip_info *chip_info;
	struct rtl8365mb_cpu cpu;
	struct rtl8365mb_port ports[RTL8365MB_MAX_NUM_PORTS];
};

static int rtl8365mb_phy_poll_busy(struct realtek_priv *priv)
{
	u32 val;

	return regmap_read_poll_timeout(priv->map_nolock,
					RTL8365MB_INDIRECT_ACCESS_STATUS_REG,
					val, !val, 100);
}

static int rtl8365mb_phy_ocp_prepare(struct realtek_priv *priv, int phy,
				     u32 ocp_addr)
{
	u32 val;
	int ret;

	/* Set OCP prefix */
	val = FIELD_GET(RTL8365MB_PHY_OCP_ADDR_PREFIX_MASK, ocp_addr);
	ret = regmap_update_bits(
		priv->map_nolock, RTL8365MB_GPHY_OCP_MSB_0_REG,
		RTL8365MB_GPHY_OCP_MSB_0_CFG_CPU_OCPADR_MASK,
		FIELD_PREP(RTL8365MB_GPHY_OCP_MSB_0_CFG_CPU_OCPADR_MASK, val));
	if (ret)
		return ret;

	/* Set PHY register address */
	val = RTL8365MB_PHY_BASE;
	val |= FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_ADDRESS_PHYNUM_MASK, phy);
	val |= FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_ADDRESS_OCPADR_5_1_MASK,
			  ocp_addr >> 1);
	val |= FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_ADDRESS_OCPADR_9_6_MASK,
			  ocp_addr >> 6);
	ret = regmap_write(priv->map_nolock,
			   RTL8365MB_INDIRECT_ACCESS_ADDRESS_REG, val);
	if (ret)
		return ret;

	return 0;
}

static int rtl8365mb_phy_ocp_read(struct realtek_priv *priv, int phy,
				  u32 ocp_addr, u16 *data)
{
	u32 val;
	int ret;

	ret = rtl8365mb_phy_poll_busy(priv);
	if (ret)
		goto out;

	ret = rtl8365mb_phy_ocp_prepare(priv, phy, ocp_addr);
	if (ret)
		goto out;

	/* Execute read operation */
	val = FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_MASK,
			 RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_VALUE) |
	      FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_CTRL_RW_MASK,
			 RTL8365MB_INDIRECT_ACCESS_CTRL_RW_READ);
	ret = regmap_write(priv->map_nolock, RTL8365MB_INDIRECT_ACCESS_CTRL_REG,
			   val);
	if (ret)
		goto out;

	ret = rtl8365mb_phy_poll_busy(priv);
	if (ret)
		goto out;

	/* Get PHY register data */
	ret = regmap_read(priv->map_nolock,
			  RTL8365MB_INDIRECT_ACCESS_READ_DATA_REG, &val);
	if (ret)
		goto out;

	*data = val & 0xFFFF;

out:

	return ret;
}

static int rtl8365mb_phy_ocp_write(struct realtek_priv *priv, int phy,
				   u32 ocp_addr, u16 data)
{
	u32 val;
	int ret;

	ret = rtl8365mb_phy_poll_busy(priv);
	if (ret)
		goto out;

	ret = rtl8365mb_phy_ocp_prepare(priv, phy, ocp_addr);
	if (ret)
		goto out;

	/* Set PHY register data */
	ret = regmap_write(priv->map_nolock,
			   RTL8365MB_INDIRECT_ACCESS_WRITE_DATA_REG, data);
	if (ret)
		goto out;

	/* Execute write operation */
	val = FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_MASK,
			 RTL8365MB_INDIRECT_ACCESS_CTRL_CMD_VALUE) |
	      FIELD_PREP(RTL8365MB_INDIRECT_ACCESS_CTRL_RW_MASK,
			 RTL8365MB_INDIRECT_ACCESS_CTRL_RW_WRITE);
	ret = regmap_write(priv->map_nolock, RTL8365MB_INDIRECT_ACCESS_CTRL_REG,
			   val);
	if (ret)
		goto out;

	ret = rtl8365mb_phy_poll_busy(priv);
	if (ret)
		goto out;

out:
	return 0;
}

static int rtl8365mb_phy_read(struct realtek_priv *priv, int phy, int regnum)
{
	u32 ocp_addr;
	u16 val;
	int ret;

	if (phy > RTL8365MB_PHYADDRMAX)
		return -EINVAL;

	if (regnum > RTL8365MB_PHYREGMAX)
		return -EINVAL;

	ocp_addr = RTL8365MB_PHY_OCP_ADDR_PHYREG_BASE + regnum * 2;

	ret = rtl8365mb_phy_ocp_read(priv, phy, ocp_addr, &val);
	if (ret) {
		dev_err(priv->dev,
			"failed to read PHY%d reg %02x @ %04x, ret %d\n", phy,
			regnum, ocp_addr, ret);
		return ret;
	}

	dev_dbg(priv->dev, "read PHY%d register 0x%02x @ %04x, val <- %04x\n",
		phy, regnum, ocp_addr, val);

	return val;
}

static int rtl8365mb_phy_write(struct realtek_priv *priv, int phy, int regnum,
			       u16 val)
{
	u32 ocp_addr;
	int ret;

	if (phy > RTL8365MB_PHYADDRMAX)
		return -EINVAL;

	if (regnum > RTL8365MB_PHYREGMAX)
		return -EINVAL;

	ocp_addr = RTL8365MB_PHY_OCP_ADDR_PHYREG_BASE + regnum * 2;

	ret = rtl8365mb_phy_ocp_write(priv, phy, ocp_addr, val);
	if (ret) {
		dev_err(priv->dev,
			"failed to write PHY%d reg %02x @ %04x, ret %d\n", phy,
			regnum, ocp_addr, ret);
		return ret;
	}

	dev_dbg(priv->dev, "write PHY%d register 0x%02x @ %04x, val -> %04x\n",
		phy, regnum, ocp_addr, val);

	return 0;
}

static const struct rtl8365mb_extint *
rtl8365mb_get_port_extint(struct realtek_priv *priv, int port)
{
	struct rtl8365mb *mb = priv->chip_data;
	int i;

	for (i = 0; i < RTL8365MB_MAX_NUM_EXTINTS; i++) {
		const struct rtl8365mb_extint *extint =
			&mb->chip_info->extints[i];

		if (extint->port == port)
			return extint;
	}

	return NULL;
}

static enum dsa_tag_protocol
rtl8365mb_get_tag_protocol(struct realtek_priv *priv)
{
	struct rtl8365mb_cpu *cpu;
	struct rtl8365mb *mb;

	mb = priv->chip_data;
	cpu = &mb->cpu;

	if (cpu->position == RTL8365MB_CPU_POS_BEFORE_CRC)
		return DSA_TAG_PROTO_RTL8_4T;

	return DSA_TAG_PROTO_RTL8_4;
}

static int rtl8365mb_ext_config_rgmii(struct realtek_priv *priv, int port,
				      phy_interface_t interface)
{
	const struct rtl8365mb_extint *extint =
		rtl8365mb_get_port_extint(priv, port);
	struct device_node *dn;
	struct dsa_port *dp;
	int tx_delay = 0;
	int rx_delay = 0;
	u32 val;
	int ret;

	if (!extint)
		return -ENODEV;

	dp = dsa_to_port(priv->ds, port);
	dn = dp->dev->device_node;

	/* Set the RGMII TX/RX delay
	 *
	 * The Realtek vendor driver indicates the following possible
	 * configuration settings:
	 *
	 *   TX delay:
	 *     0 = no delay, 1 = 2 ns delay
	 *   RX delay:
	 *     0 = no delay, 7 = maximum delay
	 *     Each step is approximately 0.3 ns, so the maximum delay is about
	 *     2.1 ns.
	 *
	 * The vendor driver also states that this must be configured *before*
	 * forcing the external interface into a particular mode, which is done
	 * in the rtl8365mb_phylink_mac_link_{up,down} functions.
	 *
	 * Only configure an RGMII TX (resp. RX) delay if the
	 * tx-internal-delay-ps (resp. rx-internal-delay-ps) OF property is
	 * specified. We ignore the detail of the RGMII interface mode
	 * (RGMII_{RXID, TXID, etc.}), as this is considered to be a PHY-only
	 * property.
	 */
	if (!of_property_read_u32(dn, "tx-internal-delay-ps", &val)) {
		val = val / 1000; /* convert to ns */

		if (val == 0 || val == 2)
			tx_delay = val / 2;
		else
			dev_warn(priv->dev,
				 "RGMII TX delay must be 0 or 2 ns\n");
	}

	if (!of_property_read_u32(dn, "rx-internal-delay-ps", &val)) {
		val = DIV_ROUND_CLOSEST(val, 300); /* convert to 0.3 ns step */

		if (val <= 7)
			rx_delay = val;
		else
			dev_warn(priv->dev,
				 "RGMII RX delay must be 0 to 2.1 ns\n");
	}

	ret = regmap_update_bits(
		priv->map, RTL8365MB_EXT_RGMXF_REG(extint->id),
		RTL8365MB_EXT_RGMXF_TXDELAY_MASK |
			RTL8365MB_EXT_RGMXF_RXDELAY_MASK,
		FIELD_PREP(RTL8365MB_EXT_RGMXF_TXDELAY_MASK, tx_delay) |
			FIELD_PREP(RTL8365MB_EXT_RGMXF_RXDELAY_MASK, rx_delay));
	if (ret)
		return ret;

	ret = regmap_update_bits(
		priv->map, RTL8365MB_DIGITAL_INTERFACE_SELECT_REG(extint->id),
		RTL8365MB_DIGITAL_INTERFACE_SELECT_MODE_MASK(extint->id),
		RTL8365MB_EXT_PORT_MODE_RGMII
			<< RTL8365MB_DIGITAL_INTERFACE_SELECT_MODE_OFFSET(
				   extint->id));
	if (ret)
		return ret;

	return 0;
}

static int rtl8365mb_ext_config_forcemode(struct realtek_priv *priv, int port,
					  bool link, int speed, int duplex,
					  bool tx_pause, bool rx_pause)
{
	const struct rtl8365mb_extint *extint =
		rtl8365mb_get_port_extint(priv, port);
	u32 r_tx_pause;
	u32 r_rx_pause;
	u32 r_duplex;
	u32 r_speed;
	u32 r_link;
	int val;
	int ret;

	if (!extint)
		return -ENODEV;

	if (link) {
		/* Force the link up with the desired configuration */
		r_link = 1;
		r_rx_pause = rx_pause ? 1 : 0;
		r_tx_pause = tx_pause ? 1 : 0;

		if (speed == SPEED_1000) {
			r_speed = RTL8365MB_PORT_SPEED_1000M;
		} else if (speed == SPEED_100) {
			r_speed = RTL8365MB_PORT_SPEED_100M;
		} else if (speed == SPEED_10) {
			r_speed = RTL8365MB_PORT_SPEED_10M;
		} else {
			dev_err(priv->dev, "unsupported port speed %d\n",
				speed);
			dump_stack();
			return -EINVAL;
		}

		if (duplex == DUPLEX_FULL) {
			r_duplex = 1;
		} else if (duplex == DUPLEX_HALF) {
			r_duplex = 0;
		} else {
			dev_err(priv->dev, "unsupported duplex mode %d\n",
				duplex);
			return -EINVAL;
		}
	} else {
		/* Force the link down and reset any programmed configuration */
		r_link = 0;
		r_tx_pause = 0;
		r_rx_pause = 0;
		r_speed = 0;
		r_duplex = 0;
	}

	val = FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_EN_MASK, 1) |
	      FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_TXPAUSE_MASK,
			 r_tx_pause) |
	      FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_RXPAUSE_MASK,
			 r_rx_pause) |
	      FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_LINK_MASK, r_link) |
	      FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_DUPLEX_MASK,
			 r_duplex) |
	      FIELD_PREP(RTL8365MB_DIGITAL_INTERFACE_FORCE_SPEED_MASK, r_speed);
	ret = regmap_write(priv->map,
			   RTL8365MB_DIGITAL_INTERFACE_FORCE_REG(extint->id),
			   val);
	if (ret)
		return ret;

	return 0;
}

static void rtl8365mb_port_stp_state_set(struct dsa_switch *ds, int port,
					 u8 state)
{
	struct realtek_priv *priv = ds->priv;
	enum rtl8365mb_stp_state val;
	int msti = 0;

	switch (state) {
	case BR_STATE_DISABLED:
		val = RTL8365MB_STP_STATE_DISABLED;
		break;
	case BR_STATE_FORWARDING:
		val = RTL8365MB_STP_STATE_FORWARDING;
		break;
	default:
		dev_err(priv->dev, "invalid STP state: %u\n", state);
		return;
	}

	regmap_update_bits(priv->map, RTL8365MB_MSTI_CTRL_REG(msti, port),
			   RTL8365MB_MSTI_CTRL_PORT_STATE_MASK(port),
			   val << RTL8365MB_MSTI_CTRL_PORT_STATE_OFFSET(port));
}

static int rtl8365mb_phylink_mac_config(struct dsa_port *dp, int port,
					phy_interface_t phy_mode)
{
	struct realtek_priv *priv = dp->ds->priv;
	int ret = 0;

	if (phy_interface_mode_is_rgmii(phy_mode)) {
		ret = rtl8365mb_ext_config_rgmii(priv, port, phy_mode);
		if (ret)
			dev_err(priv->dev,
				"failed to configure RGMII mode on port %d: %d\n",
				port, ret);
	}

	return ret;
}

static void rtl8365mb_phylink_mac_link_down(struct dsa_port *dp, int port,
					    struct phy_device *phy)
{
	struct realtek_priv *priv = dp->ds->priv;
	int ret;

	rtl8365mb_port_stp_state_set(dp->ds, port, BR_STATE_DISABLED);

	if (phy_interface_mode_is_rgmii(phy->interface)) {
		ret = rtl8365mb_ext_config_forcemode(priv, port, false,
						     0, 0, 0, 0);
		if (ret)
			dev_err(priv->dev,
				"failed to reset forced mode on port %d: %d\n",
				port, ret);
	}
}

static int rtl8365mb_phylink_mac_link_up(struct dsa_port *dp, int port,
					 struct phy_device *phy)
{
	struct realtek_priv *priv = dp->ds->priv;
	int ret = 0;

	if (phy_interface_mode_is_rgmii(phy->interface)) {
		ret = rtl8365mb_ext_config_forcemode(priv, port, true,
						     phy->speed, phy->duplex,
						     phy->pause, phy->pause);
		if (ret)
			dev_err(priv->dev,
				"failed to force mode on port %d: %d\n", port,
				ret);
	}

	rtl8365mb_port_stp_state_set(dp->ds, port, BR_STATE_FORWARDING);

	return ret;
}

static int rtl8365mb_port_set_learning(struct realtek_priv *priv, int port,
				       bool enable)
{
	/* Enable/disable learning by limiting the number of L2 addresses the
	 * port can learn. Realtek documentation states that a limit of zero
	 * disables learning. When enabling learning, set it to the chip's
	 * maximum.
	 */
	return regmap_write(priv->map, RTL8365MB_LUT_PORT_LEARN_LIMIT_REG(port),
			    enable ? RTL8365MB_LEARN_LIMIT_MAX : 0);
}

static int rtl8365mb_port_set_isolation(struct realtek_priv *priv, int port,
					u32 mask)
{
	return regmap_write(priv->map, RTL8365MB_PORT_ISOLATION_REG(port), mask);
}

static int rtl8365mb_set_irq_enable(struct realtek_priv *priv, bool enable)
{
	return regmap_update_bits(priv->map, RTL8365MB_INTR_CTRL_REG,
				  RTL8365MB_INTR_LINK_CHANGE_MASK,
				  FIELD_PREP(RTL8365MB_INTR_LINK_CHANGE_MASK,
					     enable ? 1 : 0));
}

static int rtl8365mb_irq_disable(struct realtek_priv *priv)
{
	return rtl8365mb_set_irq_enable(priv, false);
}

static int rtl8365mb_irq_setup(struct realtek_priv *priv)
{
	int ret;

	/* Disable the interrupt in case the chip has it enabled on reset */
	ret = rtl8365mb_irq_disable(priv);
	if (ret)
		return ret;

	/* Clear the interrupt status register */
	return regmap_write(priv->map, RTL8365MB_INTR_STATUS_REG,
			    RTL8365MB_INTR_ALL_MASK);
}

static int rtl8365mb_cpu_config(struct realtek_priv *priv)
{
	struct rtl8365mb *mb = priv->chip_data;
	struct rtl8365mb_cpu *cpu = &mb->cpu;
	u32 val;
	int ret;

	ret = regmap_update_bits(priv->map, RTL8365MB_CPU_PORT_MASK_REG,
				 RTL8365MB_CPU_PORT_MASK_MASK,
				 FIELD_PREP(RTL8365MB_CPU_PORT_MASK_MASK,
					    cpu->mask));
	if (ret)
		return ret;

	val = FIELD_PREP(RTL8365MB_CPU_CTRL_EN_MASK, 1) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_INSERTMODE_MASK, cpu->insert) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_TAG_POSITION_MASK, cpu->position) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_RXBYTECOUNT_MASK, cpu->rx_length) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_TAG_FORMAT_MASK, cpu->format) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_TRAP_PORT_MASK, cpu->trap_port & 0x7) |
	      FIELD_PREP(RTL8365MB_CPU_CTRL_TRAP_PORT_EXT_MASK,
			 cpu->trap_port >> 3 & 0x1);
	ret = regmap_write(priv->map, RTL8365MB_CPU_CTRL_REG, val);
	if (ret)
		return ret;

	return 0;
}

static int rtl8365mb_change_tag_protocol(struct realtek_priv *priv,
					 enum dsa_tag_protocol proto)
{
	struct rtl8365mb_cpu *cpu;
	struct rtl8365mb *mb;

	mb = priv->chip_data;
	cpu = &mb->cpu;

	switch (proto) {
	case DSA_TAG_PROTO_RTL8_4:
		cpu->format = RTL8365MB_CPU_FORMAT_8BYTES;
		cpu->position = RTL8365MB_CPU_POS_AFTER_SA;
		break;
	case DSA_TAG_PROTO_RTL8_4T:
		cpu->format = RTL8365MB_CPU_FORMAT_8BYTES;
		cpu->position = RTL8365MB_CPU_POS_BEFORE_CRC;
		break;
	/* The switch also supports a 4-byte format, similar to rtl4a but with
	 * the same 0x04 8-bit version and probably 8-bit port source/dest.
	 * There is no public doc about it. Not supported yet and it will probably
	 * never be.
	 */
	default:
		return -EPROTONOSUPPORT;
	}

	return rtl8365mb_cpu_config(priv);
}

static int rtl8365mb_switch_init(struct realtek_priv *priv)
{
	struct rtl8365mb *mb = priv->chip_data;
	const struct rtl8365mb_chip_info *ci;
	int ret;
	int i;

	ci = mb->chip_info;

	/* Do any chip-specific init jam before getting to the common stuff */
	if (ci->jam_table) {
		for (i = 0; i < ci->jam_size; i++) {
			ret = regmap_write(priv->map, ci->jam_table[i].reg,
					   ci->jam_table[i].val);
			if (ret)
				return ret;
		}
	}

	/* Common init jam */
	for (i = 0; i < ARRAY_SIZE(rtl8365mb_init_jam_common); i++) {
		ret = regmap_write(priv->map, rtl8365mb_init_jam_common[i].reg,
				   rtl8365mb_init_jam_common[i].val);
		if (ret)
			return ret;
	}

	return 0;
}

static int rtl8365mb_reset_chip(struct realtek_priv *priv)
{
	u32 val;

	priv->write_reg_noack(priv, RTL8365MB_CHIP_RESET_REG,
			      FIELD_PREP(RTL8365MB_CHIP_RESET_HW_MASK, 1));

	/* Realtek documentation says the chip needs 1 second to reset. Sleep
	 * for 100 ms before accessing any registers to prevent ACK timeouts.
	 */
	mdelay(100);
	return regmap_read_poll_timeout(priv->map, RTL8365MB_CHIP_RESET_REG, val,
					!(val & RTL8365MB_CHIP_RESET_HW_MASK),
					1e6);
}

static int rtl8365mb_setup(struct realtek_priv *priv)
{
	struct rtl8365mb_cpu *cpu;
	struct dsa_port *cpu_dp;
	struct rtl8365mb *mb;
	int ret;
	int i;

	mb = priv->chip_data;
	cpu = &mb->cpu;

	ret = rtl8365mb_reset_chip(priv);
	if (ret) {
		dev_err(priv->dev, "failed to reset chip: %d\n", ret);
		goto out_error;
	}

	/* Configure switch to vendor-defined initial state */
	ret = rtl8365mb_switch_init(priv);
	if (ret) {
		dev_err(priv->dev, "failed to initialize switch: %d\n", ret);
		goto out_error;
	}

	rtl8365mb_irq_setup(priv);

	/* Configure CPU tagging */
	dsa_switch_for_each_cpu_port(cpu_dp, priv->ds) {
		cpu->mask |= BIT(cpu_dp->index);

		if (cpu->trap_port == RTL8365MB_MAX_NUM_PORTS)
			cpu->trap_port = cpu_dp->index;
	}

	if (cpu->mask == 0) {
		dev_err(priv->dev, "no CPU port found\n");
		goto out_teardown_irq;
	}

	ret = rtl8365mb_cpu_config(priv);
	if (ret)
		goto out_teardown_irq;

	/* Configure ports */
	for (i = 0; i < priv->num_ports; i++) {
		struct rtl8365mb_port *p = &mb->ports[i];

		/* Forward only to the CPU */
		ret = rtl8365mb_port_set_isolation(priv, i, cpu->mask);
		if (ret)
			goto out_teardown_irq;

		/* Disable learning */
		ret = rtl8365mb_port_set_learning(priv, i, false);
		if (ret)
			goto out_teardown_irq;

		/* Set the initial STP state of all ports to DISABLED, otherwise
		 * ports will still forward frames to the CPU despite being
		 * administratively down by default.
		 */
		rtl8365mb_port_stp_state_set(priv->ds, i, BR_STATE_DISABLED);

		/* Set up per-port private data */
		p->priv = priv;
		p->index = i;
	}

	/* Set maximum packet length to 1536 bytes */
	ret = regmap_update_bits(priv->map, RTL8365MB_CFG0_MAX_LEN_REG,
				 RTL8365MB_CFG0_MAX_LEN_MASK,
				 FIELD_PREP(RTL8365MB_CFG0_MAX_LEN_MASK, 1536));
	if (ret)
		goto out_teardown_irq;

	if (priv->setup_interface) {
		ret = priv->setup_interface(priv->ds);
		if (ret) {
			dev_err(priv->dev, "could not set up MDIO bus\n");
			goto out_teardown_irq;
		}
	}

	return 0;

out_teardown_irq:
out_error:
	return ret;
}

static int rtl8365mb_get_chip_id_and_ver(struct regmap *map, u32 *id, u32 *ver)
{
	int ret;

	/* For some reason we have to write a magic value to an arbitrary
	 * register whenever accessing the chip ID/version registers.
	 */
	ret = regmap_write(map, RTL8365MB_MAGIC_REG, RTL8365MB_MAGIC_VALUE);
	if (ret)
		return ret;

	ret = regmap_read(map, RTL8365MB_CHIP_ID_REG, id);
	if (ret)
		return ret;

	ret = regmap_read(map, RTL8365MB_CHIP_VER_REG, ver);
	if (ret)
		return ret;

	/* Reset magic register */
	ret = regmap_write(map, RTL8365MB_MAGIC_REG, 0);
	if (ret)
		return ret;

	return 0;
}

static int rtl8365mb_detect(struct realtek_priv *priv)
{
	struct rtl8365mb *mb = priv->chip_data;
	u32 chip_id;
	u32 chip_ver;
	int ret;
	int i;

	ret = rtl8365mb_get_chip_id_and_ver(priv->map, &chip_id, &chip_ver);
	if (ret) {
		dev_err(priv->dev, "failed to read chip id and version: %d\n",
			ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(rtl8365mb_chip_infos); i++) {
		const struct rtl8365mb_chip_info *ci = &rtl8365mb_chip_infos[i];

		if (ci->chip_id == chip_id && ci->chip_ver == chip_ver) {
			mb->chip_info = ci;
			break;
		}
	}

	if (!mb->chip_info) {
		dev_err(priv->dev,
			"unrecognized switch (id=0x%04x, ver=0x%04x)\n", chip_id,
			chip_ver);
		return -ENODEV;
	}

	dev_info(priv->dev, "found an %s switch\n", mb->chip_info->name);

	priv->num_ports = RTL8365MB_MAX_NUM_PORTS;
	mb->priv = priv;
	mb->cpu.trap_port = RTL8365MB_MAX_NUM_PORTS;
	mb->cpu.insert = RTL8365MB_CPU_INSERT_TO_ALL;
	mb->cpu.position = RTL8365MB_CPU_POS_AFTER_SA;
	mb->cpu.rx_length = RTL8365MB_CPU_RXLEN_64BYTES;
	mb->cpu.format = RTL8365MB_CPU_FORMAT_8BYTES;

	return 0;
}

static const struct dsa_switch_ops rtl8365mb_switch_ops = {
	.port_pre_enable = rtl8365mb_phylink_mac_config,
	.port_disable = rtl8365mb_phylink_mac_link_down,
	.port_enable = rtl8365mb_phylink_mac_link_up,
};

static const struct realtek_ops rtl8365mb_ops = {
	.detect = rtl8365mb_detect,
	.phy_read = rtl8365mb_phy_read,
	.phy_write = rtl8365mb_phy_write,
	.setup = rtl8365mb_setup,
	.get_tag_protocol = rtl8365mb_get_tag_protocol,
	.change_tag_protocol = rtl8365mb_change_tag_protocol,
};

const struct realtek_variant rtl8365mb_variant = {
	.ds_ops = &rtl8365mb_switch_ops,
	.ops = &rtl8365mb_ops,
	.clk_delay = 10,
	.cmd_read = 0xb9,
	.cmd_write = 0xb8,
	.chip_data_sz = sizeof(struct rtl8365mb),
};
EXPORT_SYMBOL_GPL(rtl8365mb_variant);

MODULE_AUTHOR("Alvin Šipraga <alsi@bang-olufsen.dk>");
MODULE_DESCRIPTION("Driver for RTL8365MB-VC ethernet switch");
MODULE_LICENSE("GPL");

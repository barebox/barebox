// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CPSW Ethernet Switch Driver
 */

#include <common.h>
#include <driver.h>
#include <init.h>

#include <command.h>
#include <dma.h>
#include <net.h>
#include <malloc.h>
#include <net.h>
#include <linux/phy.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <pinctrl.h>
#include <of_net.h>
#include <of_address.h>
#include <xfuncs.h>
#include <asm/system.h>
#include <linux/err.h>

#include <mach/cpsw.h>

#define CPSW_VERSION_1		0x19010a
#define CPSW_VERSION_2		0x19010c

#define BITMASK(bits)		((1 << (bits)) - 1)
#define PHY_REG_MASK		0x1f
#define PHY_ID_MASK		0x1f
#define NUM_DESCS		(PKTBUFSRX * 2)
#define PKT_MIN			60
#define PKT_MAX			(1500 + 14 + 4 + 4)

/* DMA Registers */
#define CPDMA_TXCONTROL		0x004
#define CPDMA_RXCONTROL		0x014
#define CPDMA_SOFTRESET		0x01c
#define CPDMA_RXFREE		0x0e0
#define STATERAM_TXHDP		0x000
#define STATERAM_RXHDP		0x020
#define STATERAM_TXCP		0x040
#define STATERAM_RXCP		0x060

#define CPDMA_RAM_ADDR          0x4a102000

/* Descriptor mode bits */
#define CPDMA_DESC_SOP		BIT(31)
#define CPDMA_DESC_EOP		BIT(30)
#define CPDMA_DESC_OWNER	BIT(29)
#define CPDMA_DESC_EOQ		BIT(28)

#define SLIVER_SIZE		0x40

struct cpsw_mdio_regs {
	u32	version;
	u32	control;
#define CONTROL_IDLE		(1 << 31)
#define CONTROL_ENABLE		(1 << 30)

	u32	alive;
	u32	link;
	u32	linkintraw;
	u32	linkintmasked;
	u32	__reserved_0[2];
	u32	userintraw;
	u32	userintmasked;
	u32	userintmaskset;
	u32	userintmaskclr;
	u32	__reserved_1[20];

	struct {
		u32 access;
		u32 physel;
#define USERACCESS_GO		(1 << 31)
#define USERACCESS_WRITE	(1 << 30)
#define USERACCESS_ACK		(1 << 29)
#define USERACCESS_READ		(0)
#define USERACCESS_DATA		(0xffff)
	} user[0];
};

struct cpsw_regs {
	u32	id_ver;
	u32	control;
	u32	soft_reset;
	u32	stat_port_en;
	u32	ptype;
};

struct cpsw_slave_regs {
	u32	max_blks;
	u32	blk_cnt;
	u32	flow_thresh;
	u32	port_vlan;
	u32	tx_pri_map;
	u32	sa_lo;
	u32	sa_hi;
};

struct cpsw_host_regs {
	u32	max_blks;
	u32	blk_cnt;
	u32	flow_thresh;
	u32	port_vlan;
	u32	tx_pri_map;
	u32	cpdma_tx_pri_map;
	u32	cpdma_rx_chan_map;
};

struct cpsw_sliver_regs {
	u32	id_ver;
	u32	mac_control;
	u32	mac_status;
	u32	soft_reset;
	u32	rx_maxlen;
	u32	__reserved_0;
	u32	rx_pause;
	u32	tx_pause;
	u32	__reserved_1;
	u32	rx_pri_map;
};

#define ALE_ENTRY_BITS		68
#define ALE_ENTRY_WORDS		DIV_ROUND_UP(ALE_ENTRY_BITS, 32)

/* ALE Registers */
#define ALE_CONTROL		0x08
#define ALE_UNKNOWNVLAN		0x18
#define ALE_TABLE_CONTROL	0x20
#define ALE_TABLE		0x34
#define ALE_PORTCTL		0x40

#define ALE_TABLE_WRITE		BIT(31)

#define ALE_TYPE_FREE			0
#define ALE_TYPE_ADDR			1
#define ALE_TYPE_VLAN			2
#define ALE_TYPE_VLAN_ADDR		3

#define ALE_UCAST_PERSISTANT		0
#define ALE_UCAST_UNTOUCHED		1
#define ALE_UCAST_OUI			2
#define ALE_UCAST_TOUCHED		3

#define ALE_MCAST_FWD			0
#define ALE_MCAST_BLOCK_LEARN_FWD	1
#define ALE_MCAST_FWD_LEARN		2
#define ALE_MCAST_FWD_2			3

enum cpsw_ale_port_state {
	ALE_PORT_STATE_DISABLE	= 0x00,
	ALE_PORT_STATE_BLOCK	= 0x01,
	ALE_PORT_STATE_LEARN	= 0x02,
	ALE_PORT_STATE_FORWARD	= 0x03,
};

/* ALE unicast entry flags - passed into cpsw_ale_add_ucast() */
#define ALE_SECURE	1
#define ALE_BLOCKED	2

struct cpsw_slave {
	struct cpsw_slave_regs		*regs;
	struct cpsw_sliver_regs		*sliver;
	int				slave_num;
	int				phy_id;
	phy_interface_t			phy_if;
	struct eth_device		edev;
	struct cpsw_priv		*cpsw;
	struct device_d			dev;
};

struct cpdma_desc {
	/* hardware fields */
	u32			hw_next;
	u32			hw_buffer;
	u32			hw_len;
	u32			hw_mode;
	/* software fields */
	u32			sw_buffer;
	u32			sw_len;
};

struct cpdma_chan {
	struct cpdma_desc	*head, *tail;
	void			*hdp, *cp, *rxfree;
};

struct cpsw_priv {
	struct device_d			*dev;
	struct mii_bus			miibus;

	u32				version;
	struct cpsw_platform_data	data;
	int				host_port;
	uint8_t				mac_addr[6];

	struct cpsw_regs		*regs;
	struct cpsw_mdio_regs		*mdio_regs;
	void				*dma_regs;
	struct cpsw_host_regs		*host_port_regs;
	void				*ale_regs;
	void				*state_ram;

	unsigned int			ale_entries;
	unsigned int			num_slaves;
	unsigned int			channels;
	unsigned int			slave_ofs;
	unsigned int			slave_size;
	unsigned int			sliver_ofs;

	struct cpdma_desc		*descs;
	struct cpdma_desc		*desc_free;
	struct cpdma_chan		rx_chan, tx_chan;

	struct cpsw_slave		*slaves;
};

static int cpsw_ale_get_field(u32 *ale_entry, u32 start, u32 bits)
{
	int idx;

	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */

	return (ale_entry[idx] >> start) & BITMASK(bits);
}

static void cpsw_ale_set_field(u32 *ale_entry, u32 start, u32 bits,
				      u32 value)
{
	int idx;

	value &= BITMASK(bits);
	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */
	ale_entry[idx] &= ~(BITMASK(bits) << start);
	ale_entry[idx] |=  (value << start);
}

#define DEFINE_ALE_FIELD(name, start, bits)				\
static inline int cpsw_ale_get_##name(u32 *ale_entry)			\
{									\
	return cpsw_ale_get_field(ale_entry, start, bits);		\
}									\
static inline void cpsw_ale_set_##name(u32 *ale_entry, u32 value)	\
{									\
	cpsw_ale_set_field(ale_entry, start, bits, value);		\
}

DEFINE_ALE_FIELD(entry_type,		60,	2)
DEFINE_ALE_FIELD(mcast_state,		62,	2)
DEFINE_ALE_FIELD(port_mask,		66,	3)
DEFINE_ALE_FIELD(ucast_type,		62,	2)
DEFINE_ALE_FIELD(port_num,		66,	2)
DEFINE_ALE_FIELD(blocked,		65,	1)
DEFINE_ALE_FIELD(secure,		64,	1)
DEFINE_ALE_FIELD(mcast,			40,	1)

static char ethbdaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* The MAC address field in the ALE entry cannot be macroized as above */
static void cpsw_ale_get_addr(u32 *ale_entry, u8 *addr)
{
	int i;

	for (i = 0; i < 6; i++)
		addr[i] = cpsw_ale_get_field(ale_entry, 40 - 8*i, 8);
}

static void cpsw_ale_set_addr(u32 *ale_entry, u8 *addr)
{
	int i;

	for (i = 0; i < 6; i++)
		cpsw_ale_set_field(ale_entry, 40 - 8*i, 8, addr[i]);
}

static int cpsw_ale_read(struct cpsw_priv *priv, int idx, u32 *ale_entry)
{
	int i;

	writel(idx, priv->ale_regs + ALE_TABLE_CONTROL);

	for (i = 0; i < ALE_ENTRY_WORDS; i++)
		ale_entry[i] = readl(priv->ale_regs + ALE_TABLE + 4 * i);

	return idx;
}

static int cpsw_ale_write(struct cpsw_priv *priv, int idx, u32 *ale_entry)
{
	int i;

	for (i = 0; i < ALE_ENTRY_WORDS; i++)
		writel(ale_entry[i], priv->ale_regs + ALE_TABLE + 4 * i);

	writel(idx | ALE_TABLE_WRITE, priv->ale_regs + ALE_TABLE_CONTROL);

	return idx;
}

static int cpsw_ale_match_addr(struct cpsw_priv *priv, u8* addr)
{
	u32 ale_entry[ALE_ENTRY_WORDS];
	int type, idx;

	for (idx = 0; idx < priv->ale_entries; idx++) {
		u8 entry_addr[6];

		cpsw_ale_read(priv, idx, ale_entry);
		type = cpsw_ale_get_entry_type(ale_entry);

		if (type != ALE_TYPE_ADDR && type != ALE_TYPE_VLAN_ADDR)
			continue;

		cpsw_ale_get_addr(ale_entry, entry_addr);

		if (memcmp(entry_addr, addr, 6) == 0)
			return idx;
	}

	return -ENOENT;
}

static int cpsw_ale_match_free(struct cpsw_priv *priv)
{
	u32 ale_entry[ALE_ENTRY_WORDS];
	int type, idx;

	for (idx = 0; idx < priv->ale_entries; idx++) {
		cpsw_ale_read(priv, idx, ale_entry);
		type = cpsw_ale_get_entry_type(ale_entry);

		if (type == ALE_TYPE_FREE)
			return idx;
	}

	return -ENOENT;
}

static int cpsw_ale_find_ageable(struct cpsw_priv *priv)
{
	u32 ale_entry[ALE_ENTRY_WORDS];
	int type, idx;

	for (idx = 0; idx < priv->ale_entries; idx++) {
		cpsw_ale_read(priv, idx, ale_entry);
		type = cpsw_ale_get_entry_type(ale_entry);

		if (type != ALE_TYPE_ADDR && type != ALE_TYPE_VLAN_ADDR)
			continue;

		if (cpsw_ale_get_mcast(ale_entry))
			continue;

		type = cpsw_ale_get_ucast_type(ale_entry);

		if (type != ALE_UCAST_PERSISTANT &&
		    type != ALE_UCAST_OUI)
			return idx;
	}

	return -ENOENT;
}

static int cpsw_ale_add_ucast(struct cpsw_priv *priv, u8 *addr,
			      int port, int flags)
{
	u32 ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
	int idx;

	cpsw_ale_set_entry_type(ale_entry, ALE_TYPE_ADDR);
	cpsw_ale_set_addr(ale_entry, addr);
	cpsw_ale_set_ucast_type(ale_entry, ALE_UCAST_PERSISTANT);
	cpsw_ale_set_secure(ale_entry, (flags & ALE_SECURE) ? 1 : 0);
	cpsw_ale_set_blocked(ale_entry, (flags & ALE_BLOCKED) ? 1 : 0);
	cpsw_ale_set_port_num(ale_entry, port);

	idx = cpsw_ale_match_addr(priv, addr);
	if (idx < 0)
		idx = cpsw_ale_match_free(priv);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(priv);
	if (idx < 0)
		return -ENOMEM;

	cpsw_ale_write(priv, idx, ale_entry);
	return 0;
}

static int cpsw_ale_add_mcast(struct cpsw_priv *priv, u8 *addr, int port_mask)
{
	u32 ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
	int idx, mask;

	idx = cpsw_ale_match_addr(priv, addr);
	if (idx >= 0)
		cpsw_ale_read(priv, idx, ale_entry);

	cpsw_ale_set_entry_type(ale_entry, ALE_TYPE_ADDR);
	cpsw_ale_set_addr(ale_entry, addr);
	cpsw_ale_set_mcast_state(ale_entry, ALE_MCAST_FWD_2);

	mask = cpsw_ale_get_port_mask(ale_entry);
	port_mask |= mask;
	cpsw_ale_set_port_mask(ale_entry, port_mask);

	if (idx < 0)
		idx = cpsw_ale_match_free(priv);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(priv);
	if (idx < 0)
		return -ENOMEM;

	cpsw_ale_write(priv, idx, ale_entry);
	return 0;
}

static inline void cpsw_ale_control(struct cpsw_priv *priv, int bit, int val)
{
	u32 tmp, mask = BIT(bit);

	tmp  = readl(priv->ale_regs + ALE_CONTROL);
	tmp &= ~mask;
	tmp |= val ? mask : 0;
	writel(tmp, priv->ale_regs + ALE_CONTROL);
}

#define cpsw_ale_enable(priv, val)	cpsw_ale_control(priv, 31, val)
#define cpsw_ale_clear(priv, val)	cpsw_ale_control(priv, 30, val)
#define cpsw_ale_bypass(priv, val)	cpsw_ale_control(priv,  4, val)
#define cpsw_ale_vlan_aware(priv, val)	cpsw_ale_control(priv,  2, val)

static inline void cpsw_ale_port_state(struct cpsw_priv *priv, int port,
				       int val)
{
	int offset = ALE_PORTCTL + 4 * port;
	u32 tmp, mask = 0x3;

	tmp  = readl(priv->ale_regs + offset);
	tmp &= ~mask;
	tmp |= val & 0x3;
	writel(tmp, priv->ale_regs + offset);
}

/* wait until hardware is ready for another user access */
static u32 wait_for_user_access(struct cpsw_priv *priv)
{
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;
	u32 tmp;
	uint64_t start = get_time_ns();

	do {
		tmp = readl(&mdio_regs->user[0].access);

		if (!(tmp & USERACCESS_GO))
			break;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(priv->dev, "timeout waiting for user access\n");
			break;
		}
	} while (1);

	return tmp;
}

static int cpsw_mdio_read(struct mii_bus *bus, int phy_id, int phy_reg)
{
	struct cpsw_priv *priv = bus->priv;
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;

	u32 tmp;

	if (phy_reg & ~PHY_REG_MASK || phy_id & ~PHY_ID_MASK)
		return -EINVAL;

	wait_for_user_access(priv);

	tmp = (USERACCESS_GO | USERACCESS_READ | (phy_reg << 21) |
	       (phy_id << 16));
	writel(tmp, &mdio_regs->user[0].access);

	tmp = wait_for_user_access(priv);

	return (tmp & USERACCESS_ACK) ? (tmp & USERACCESS_DATA) : -1;
}

static int cpsw_mdio_write(struct mii_bus *bus, int phy_id, int phy_reg, u16 value)
{
	struct cpsw_priv *priv = bus->priv;
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;
	u32 tmp;

	if (phy_reg & ~PHY_REG_MASK || phy_id & ~PHY_ID_MASK)
		return -EINVAL;

	wait_for_user_access(priv);
	tmp = (USERACCESS_GO | USERACCESS_WRITE | (phy_reg << 21) |
		   (phy_id << 16) | (value & USERACCESS_DATA));
	writel(tmp, &mdio_regs->user[0].access);
	wait_for_user_access(priv);

	return 0;
}

static inline void soft_reset(struct cpsw_priv *priv, void *reg)
{
	int ret;

	writel(1, reg);

	ret = wait_on_timeout(100 * MSECOND, (readl(reg) & 1) == 0);
	if (ret)
		dev_err(priv->dev, "reset timeout\n");
}

#define mac_hi(mac)	(((mac)[0] << 0) | ((mac)[1] << 8) |	\
			 ((mac)[2] << 16) | ((mac)[3] << 24))
#define mac_lo(mac)	(((mac)[4] << 0) | ((mac)[5] << 8))

static int cpsw_get_hwaddr(struct eth_device *edev, unsigned char *mac)
{
	struct cpsw_slave *slave = edev->priv;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	return -1;
}

static int cpsw_set_hwaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	memcpy(&priv->mac_addr, mac, sizeof(priv->mac_addr));

	writel(mac_hi(mac), &slave->regs->sa_hi);
	writel(mac_lo(mac), &slave->regs->sa_lo);

	return 0;
}

static void cpsw_slave_update_link(struct cpsw_slave *slave,
				   struct cpsw_priv *priv, int *link)
{
	struct phy_device *phydev = slave->edev.phydev;
	u32 mac_control = 0;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	if (!phydev)
		return;

	if (phydev->link) {
		*link = 1;
		mac_control = BIT(5); /* MIIEN */
		if (phydev->speed == SPEED_10)
			mac_control |= BIT(18);	/* In Band mode	*/
		else if (phydev->speed == SPEED_100)
			mac_control |= BIT(15);
		else if (phydev->speed == SPEED_1000)
			mac_control |= BIT(7);
		if (phydev->duplex == DUPLEX_FULL)
			mac_control |= BIT(0);	/* FULLDUPLEXEN	*/
	}

	if (mac_control) {
		dev_dbg(&slave->dev, "link up, speed %d, %s duplex\n",
				phydev->speed,
				(phydev->duplex == DUPLEX_FULL) ?  "full" : "half");
	} else {
		dev_dbg(&slave->dev, "link down\n");
	}

	writel(mac_control, &slave->sliver->mac_control);
}

static int cpsw_update_link(struct cpsw_slave *slave, struct cpsw_priv *priv)
{
	int link = 0;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	cpsw_slave_update_link(slave, priv, &link);

	return link;
}

static void cpsw_adjust_link(struct eth_device *edev)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	cpsw_update_link(slave, priv);
}

static inline u32 cpsw_get_slave_port(struct cpsw_priv *priv, u32 slave_num)
{
	if (priv->host_port == 0)
		return slave_num + 1;
	else
		return slave_num;
}

static void cpsw_slave_init(struct cpsw_slave *slave, struct cpsw_priv *priv)
{
	u32	slave_port;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	soft_reset(priv, &slave->sliver->soft_reset);

	/* setup priority mapping */
	writel(0x76543210, &slave->sliver->rx_pri_map);
	writel(0x33221100, &slave->regs->tx_pri_map);

	/* setup max packet size, and mac address */
	writel(PKT_MAX, &slave->sliver->rx_maxlen);

	/* enable forwarding */
	slave_port = cpsw_get_slave_port(priv, slave->slave_num);
	cpsw_ale_port_state(priv, slave_port, ALE_PORT_STATE_FORWARD);

	/* add broadcast address */
	cpsw_ale_add_mcast(priv, ethbdaddr, 1 << slave_port);
}

static struct cpdma_desc *cpdma_desc_alloc(struct cpsw_priv *priv)
{
	struct cpdma_desc *desc = priv->desc_free;

	if (desc)
		priv->desc_free = (void *)readl(&desc->hw_next);

	return desc;
}

static void cpdma_desc_free(struct cpsw_priv *priv, struct cpdma_desc *desc)
{
	dev_dbg(priv->dev, "%s: free desc=0x%p\n", __func__, desc);

	if (desc) {
		writel((u32)priv->desc_free, &desc->hw_next);
		priv->desc_free = desc;
	}
}

static int cpdma_submit(struct cpsw_priv *priv, struct cpdma_chan *chan,
			void *buffer, int len)
{
	struct cpdma_desc *desc, *prev;
	u32 mode;

	desc = cpdma_desc_alloc(priv);
	if (!desc)
		return -ENOMEM;

	if (len < PKT_MIN)
		len = PKT_MIN;

	mode = CPDMA_DESC_OWNER | CPDMA_DESC_SOP | CPDMA_DESC_EOP;

	writel(0, &desc->hw_next);
	writel((u32)buffer, &desc->hw_buffer);
	writel(len, &desc->hw_len);
	writel(mode | len, &desc->hw_mode);
	writel((u32)buffer, &desc->sw_buffer);
	writel((u32)len, &desc->sw_len);

	if (!chan->head) {
		/* simple case - first packet enqueued */
		chan->head = desc;
		chan->tail = desc;
		writel((u32)desc, chan->hdp);
		goto done;
	}

	/* not the first packet - enqueue at the tail */
	prev = chan->tail;
	writel((u32)desc, &prev->hw_next);
	chan->tail = desc;

	/* next check if EOQ has been triggered already */
	if (readl(&prev->hw_mode) & CPDMA_DESC_EOQ)
		writel((u32)desc, chan->hdp);

done:
	if (chan->rxfree)
		writel(1, chan->rxfree);
	return 0;
}

static int cpdma_process(struct cpsw_priv *priv, struct cpdma_chan *chan,
			 void **buffer, int *len)
{
	struct cpdma_desc *desc = chan->head;
	u32 status;

	if (!desc)
		return -ENOENT;

	status = readl(&desc->hw_mode);

	if (len)
		*len = status & 0x7ff;

	if (buffer)
		*buffer = (void *)readl(&desc->sw_buffer);

	if (status & CPDMA_DESC_OWNER) {
		if (readl(chan->hdp) == 0) {
			if (readl(&desc->hw_mode) & CPDMA_DESC_OWNER)
				writel((u32)desc, chan->hdp);
		}
		return -EBUSY;
	}

	chan->head = (void *)readl(&desc->hw_next);

	writel((u32)desc, chan->cp);

	cpdma_desc_free(priv, desc);

	return 0;
}

static int cpsw_init(struct eth_device *edev)
{
	return 0;
}

static int cpsw_open(struct eth_device *edev)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;
	int i, ret;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	/* soft reset the controller and initialize priv */
	soft_reset(priv, &priv->regs->soft_reset);

	/* initialize and reset the address lookup engine */
	cpsw_ale_enable(priv, 1);
	cpsw_ale_clear(priv, 1);
	cpsw_ale_bypass(priv, 0);
	cpsw_ale_vlan_aware(priv, 0); /* vlan unaware mode */

	/* setup host port priority mapping */
	writel(0x76543210, &priv->host_port_regs->cpdma_tx_pri_map);
	writel(0, &priv->host_port_regs->cpdma_rx_chan_map);

	/* disable priority elevation and enable statistics on all ports */
	writel(0, &priv->regs->ptype);

	/* enable statistics collection only on the host port */
	writel(BIT(priv->host_port), &priv->regs->stat_port_en);

	cpsw_ale_port_state(priv, priv->host_port, ALE_PORT_STATE_FORWARD);

	cpsw_ale_add_ucast(priv, priv->mac_addr, priv->host_port,
			   ALE_SECURE);
	cpsw_ale_add_mcast(priv, ethbdaddr, 1 << priv->host_port);

	cpsw_slave_init(slave, priv);
	cpsw_update_link(slave, priv);

	/* init descriptor pool */
	for (i = 0; i < NUM_DESCS; i++) {
		u32 val = (i == (NUM_DESCS - 1)) ? 0 : (u32)&priv->descs[i + 1];

		writel(val, &priv->descs[i].hw_next);
	}

	priv->desc_free = &priv->descs[0];

	/* initialize channels */
	memset(&priv->rx_chan, 0, sizeof(struct cpdma_chan));
	priv->rx_chan.hdp	= priv->state_ram + STATERAM_RXHDP;
	priv->rx_chan.cp	= priv->state_ram + STATERAM_RXCP;
	priv->rx_chan.rxfree	= priv->dma_regs + CPDMA_RXFREE;

	memset(&priv->tx_chan, 0, sizeof(struct cpdma_chan));
	priv->tx_chan.hdp	= priv->state_ram + STATERAM_TXHDP;
	priv->tx_chan.cp	= priv->state_ram + STATERAM_TXCP;

	/* clear dma state */
	soft_reset(priv, priv->dma_regs + CPDMA_SOFTRESET);

	for (i = 0; i < priv->channels; i++) {
		writel(0, priv->state_ram + STATERAM_RXHDP + 4 * i);
		writel(0, priv->dma_regs + CPDMA_RXFREE + 4 * i);
		writel(0, priv->state_ram + STATERAM_RXCP + 4 * i);
		writel(0, priv->state_ram + STATERAM_TXHDP + 4 * i);
		writel(0, priv->state_ram + STATERAM_TXCP + 4 * i);
	}

	writel(1, priv->dma_regs + CPDMA_TXCONTROL);
	writel(1, priv->dma_regs + CPDMA_RXCONTROL);

	/* submit rx descs */
	for (i = 0; i < PKTBUFSRX - 2; i++) {
		ret = cpdma_submit(priv, &priv->rx_chan, NetRxPackets[i],
				   PKTSIZE);
		if (ret < 0) {
			dev_err(&slave->dev, "error %d submitting rx desc\n", ret);
			break;
		}
	}

	return 0;
}

static void cpsw_halt(struct eth_device *edev)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;

	dev_dbg(priv->dev, "* %s slave %d\n", __func__, slave->slave_num);

	writel(0, priv->dma_regs + CPDMA_TXCONTROL);
	writel(0, priv->dma_regs + CPDMA_RXCONTROL);

	/* soft reset the controller */
	soft_reset(priv, &priv->regs->soft_reset);

	/* clear dma state */
	soft_reset(priv, priv->dma_regs + CPDMA_SOFTRESET);
}

static int cpsw_send(struct eth_device *edev, void *packet, int length)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;
	void *buffer;
	int ret, len;

	dev_dbg(&slave->dev, "* %s slave %d\n", __func__, slave->slave_num);

	/* first reap completed packets */
	while (cpdma_process(priv, &priv->tx_chan, &buffer, &len) >= 0);

	dev_dbg(&slave->dev, "%s: %i bytes @ 0x%p\n", __func__, length, packet);

	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);
	ret = cpdma_submit(priv, &priv->tx_chan, packet, length);
	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);

	return ret;
}

static int cpsw_recv(struct eth_device *edev)
{
	struct cpsw_slave *slave = edev->priv;
	struct cpsw_priv *priv = slave->cpsw;
	void *buffer;
	int len;

	while (cpdma_process(priv, &priv->rx_chan, &buffer, &len) >= 0) {
		dma_sync_single_for_cpu((unsigned long)buffer, len,
				DMA_FROM_DEVICE);
		net_receive(edev, buffer, len);
		dma_sync_single_for_device((unsigned long)buffer, len,
				DMA_FROM_DEVICE);
		cpdma_submit(priv, &priv->rx_chan, buffer, PKTSIZE);
	}

	return 0;
}

static void cpsw_slave_init_data(struct cpsw_slave *slave, int slave_num,
			    struct cpsw_priv *priv)
{
	struct cpsw_slave_data	*data = priv->data.slave_data + slave_num;

	slave->phy_id	= data->phy_id;
	slave->phy_if	= data->phy_if;
}

static int cpsw_slave_setup(struct cpsw_slave *slave, int slave_num,
			    struct cpsw_priv *priv)
{
	void			*regs = priv->regs;
	struct eth_device	*edev = &slave->edev;
	struct device_d		*dev = &slave->dev;
	int ret;

	edev->parent = dev;

	ret = phy_device_connect(edev, &priv->miibus, slave->phy_id,
				 cpsw_adjust_link, 0, slave->phy_if);
	if (ret)
		goto err_out;

	dev_set_name(dev, "cpsw-slave");
	dev->id = slave->slave_num;
	dev->parent = priv->dev;
	ret = register_device(dev);
	if (ret)
		goto err_register_dev;

	dev_dbg(&slave->dev, "* %s\n", __func__);

	slave->slave_num = slave_num;
	slave->regs	= regs + priv->slave_ofs + priv->slave_size * slave_num;
	slave->sliver	= regs + priv->sliver_ofs + SLIVER_SIZE * slave_num;
	slave->cpsw	= priv;

	edev->priv	= slave;
	edev->init	= cpsw_init;
	edev->open	= cpsw_open;
	edev->halt	= cpsw_halt;
	edev->send	= cpsw_send;
	edev->recv	= cpsw_recv;
	edev->get_ethaddr = cpsw_get_hwaddr;
	edev->set_ethaddr = cpsw_set_hwaddr;

	ret = eth_register(edev);
	if (ret)
		goto err_register_edev;

	return 0;

err_register_dev:
	phy_unregister_device(edev->phydev);
err_register_edev:
	unregister_device(dev);
err_out:
	slave->slave_num = -1;

	return ret;
}

struct cpsw_data {
	unsigned int host_port_reg_ofs;
	unsigned int cpdma_reg_ofs;
	unsigned int ale_reg_ofs;
	unsigned int state_ram_ofs;
	unsigned int slave_ofs;
	unsigned int slave_size;
	unsigned int sliver_ofs;
	unsigned int mdio_reg_ofs;
	unsigned int cppi_ram_ofs;
};

static struct cpsw_data cpsw1_data = {
	.host_port_reg_ofs	= 0x028,
	.cpdma_reg_ofs		= 0x100,
	.state_ram_ofs		= 0x200,
	.ale_reg_ofs		= 0x600,
	.slave_ofs		= 0x050,
	.slave_size		= 0x040,
	.sliver_ofs		= 0x700,
	/* FIXME: mdio_reg_ofs and cppi_ram_ofs missing */
};

static struct cpsw_data cpsw2_data = {
	.host_port_reg_ofs	= 0x108,
	.cpdma_reg_ofs		= 0x800,
	.state_ram_ofs		= 0xa00,
	.ale_reg_ofs		= 0xd00,
	.slave_ofs		= 0x200,
	.slave_size		= 0x100,
	.sliver_ofs		= 0xd80,
	.mdio_reg_ofs		= 0x1000,
	.cppi_ram_ofs		= 0x2000,
};

static void __iomem *phy_sel_addr;
static bool rmii_clock_external;

static int cpsw_phy_sel_init(struct cpsw_priv *priv, struct device_node *np)
{
	const void *reg;
	unsigned long addr;

	reg = of_get_property(np, "reg", NULL);
	if (!reg)
		return -EINVAL;

	addr = of_translate_address(np, reg);

	phy_sel_addr = (void *)addr;

	if (of_property_read_bool(np, "rmii-clock-ext"))
		rmii_clock_external = true;

	return 0;
}

/* AM33xx SoC specific definitions for the CONTROL port */
#define AM33XX_GMII_SEL_MODE_MII	0
#define AM33XX_GMII_SEL_MODE_RMII	1
#define AM33XX_GMII_SEL_MODE_RGMII	2

#define AM33XX_GMII_SEL_RMII2_IO_CLK_EN	BIT(7)
#define AM33XX_GMII_SEL_RMII1_IO_CLK_EN	BIT(6)

static void cpsw_gmii_sel_am335x(struct cpsw_slave *slave)
{
	u32 reg;
	u32 mask;
	u32 mode = 0;

	reg = readl(phy_sel_addr);

	switch (slave->phy_if) {
	case PHY_INTERFACE_MODE_RMII:
		mode = AM33XX_GMII_SEL_MODE_RMII;
		break;

	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		mode = AM33XX_GMII_SEL_MODE_RGMII;
		break;

	case PHY_INTERFACE_MODE_MII:
	default:
		mode = AM33XX_GMII_SEL_MODE_MII;
		break;
	};

	mask = 0x3 << (slave->slave_num * 2) | BIT(slave->slave_num + 6);
	mode <<= slave->slave_num * 2;

	if (rmii_clock_external) {
		if (slave->slave_num == 0)
			mode |= AM33XX_GMII_SEL_RMII1_IO_CLK_EN;
		else
			mode |= AM33XX_GMII_SEL_RMII2_IO_CLK_EN;
	}

	reg &= ~mask;
	reg |= mode;

	writel(reg, phy_sel_addr);
}

static int cpsw_probe_dt(struct cpsw_priv *priv)
{
	struct device_d *dev = priv->dev;
	struct device_node *np = dev->device_node, *child;
	int ret, i = 0;

	ret = of_property_read_u32(np, "slaves", &priv->num_slaves);
	if (ret)
		return ret;

	priv->slaves = xzalloc(sizeof(struct cpsw_slave) * priv->num_slaves);

	for_each_child_of_node(np, child) {
		if (of_device_is_compatible(child, "ti,am3352-cpsw-phy-sel")) {
			ret = cpsw_phy_sel_init(priv, child);
			if (ret)
				return ret;
		}

		if (of_device_is_compatible(child, "ti,davinci_mdio")) {
			ret = of_pinctrl_select_state_default(child);
			if (ret)
				return ret;
		}

		if (i < priv->num_slaves && !strncmp(child->name, "slave", 5)) {
			struct cpsw_slave *slave = &priv->slaves[i];
			uint32_t phy_id[2] = {-1, -1};

			if (!of_find_node_by_name(child, "fixed-link")) {
				ret = of_property_read_u32_array(child, "phy_id", phy_id, 2);
				if (!ret)
					dev_warn(dev, "phy_id is deprecated, use phy-handle\n");
			}

			slave->dev.device_node = child;
			slave->phy_id = phy_id[1];
			slave->phy_if = of_get_phy_mode(child);
			slave->slave_num = i;

			i++;
		}
	}

	for (i = 0; i < priv->num_slaves; i++) {
		struct cpsw_slave *slave = &priv->slaves[i];

		cpsw_gmii_sel_am335x(slave);
	}

	return 0;
}

static int cpsw_probe(struct device_d *dev)
{
	struct resource *iores;
	struct cpsw_platform_data *data = (struct cpsw_platform_data *)dev->platform_data;
	struct cpsw_priv	*priv;
	void __iomem		*regs;
	uint64_t start;
	uint32_t phy_mask;
	struct cpsw_data *cpsw_data;
	int i, ret;

	dev_dbg(dev, "* %s\n", __func__);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	if (dev->device_node) {
		ret = cpsw_probe_dt(priv);
		if (ret)
			goto out;
	} else {
		priv->data = *data;
		priv->num_slaves = data->num_slaves;
		priv->slaves = xzalloc(sizeof(struct cpsw_slave) * priv->num_slaves);
		cpsw_slave_init_data(&priv->slaves[0], 0, priv);
	}

	priv->channels = 8;
	priv->ale_entries = 1024;

	priv->host_port         = 0;
	priv->regs		= regs;

	priv->version = readl(&priv->regs->id_ver);

	switch (priv->version) {
	case CPSW_VERSION_1:
		cpsw_data = &cpsw1_data;
		break;
	case CPSW_VERSION_2:
		cpsw_data = &cpsw2_data;
		break;
	default:
		ret = -EINVAL;
		goto out;
	}

	priv->descs		= regs + cpsw_data->cppi_ram_ofs;
	priv->host_port_regs	= regs + cpsw_data->host_port_reg_ofs;
	priv->dma_regs		= regs + cpsw_data->cpdma_reg_ofs;
	priv->ale_regs		= regs + cpsw_data->ale_reg_ofs;
	priv->state_ram		= regs + cpsw_data->state_ram_ofs;
	priv->mdio_regs		= regs + cpsw_data->mdio_reg_ofs;

	priv->slave_ofs		= cpsw_data->slave_ofs;
	priv->slave_size	= cpsw_data->slave_size;
	priv->sliver_ofs	= cpsw_data->sliver_ofs;

	priv->miibus.read = cpsw_mdio_read;
	priv->miibus.write = cpsw_mdio_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	/*
	 * set enable and clock divider
	 *
	 * FIXME: Use a clock to calculate the divider
	 */
	writel(0xff | CONTROL_ENABLE, &priv->mdio_regs->control);

	/*
	 * wait for scan logic to settle:
	 * the scan time consists of (a) a large fixed component, and (b) a
	 * small component that varies with the mii bus frequency.  These
	 * were estimated using measurements at 1.1 and 2.2 MHz on tnetv107x
	 * silicon.  Since the effect of (b) was found to be largely
	 * negligible, we keep things simple here.
	 */
	udelay(1000);

	start = get_time_ns();
	while (1) {
		phy_mask = readl(&priv->mdio_regs->alive);
		if (phy_mask) {
			dev_info(dev, "detected phy mask 0x%x\n", phy_mask);
			phy_mask = ~phy_mask;
			break;
		}
		if (is_timeout(start, 256 * MSECOND)) {
			dev_err(dev, "no live phy, scanning all\n");
			phy_mask = 0;
			break;
		}
	}

	priv->miibus.phy_mask = phy_mask;

	mdiobus_register(&priv->miibus);

	for (i = 0; i < priv->num_slaves; i++) {
		ret = cpsw_slave_setup(&priv->slaves[i], i, priv);
		if (ret) {
			dev_err(dev, "Failed to setup slave %d: %s\n", i, strerror(-ret));
			continue;
		}
	}

	dev->priv = priv;

	return 0;
out:
	free(priv->slaves);
	free(priv);

	return ret;
}

static void cpsw_remove(struct device_d *dev)
{
	struct cpsw_priv	*priv = dev->priv;
	int i;

	for (i = 0; i < priv->num_slaves; i++) {
		struct cpsw_slave *slave = &priv->slaves[i];
		if (slave->slave_num < 0)
			continue;

		eth_unregister(&slave->edev);
	}

	mdiobus_unregister(&priv->miibus);
}

static __maybe_unused struct of_device_id cpsw_dt_ids[] = {
	{
		.compatible = "ti,cpsw",
	}, {
		/* sentinel */
	}
};

static struct driver_d cpsw_driver = {
	.name   = "cpsw",
	.probe  = cpsw_probe,
	.remove = cpsw_remove,
	.of_compatible = DRV_OF_COMPAT(cpsw_dt_ids),
};
device_platform_driver(cpsw_driver);

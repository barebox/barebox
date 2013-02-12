#include <common.h>
#include <command.h>
#include <net.h>
#include <io.h>
#include <mach/netx-xc.h>
#include <mach/netx-eth.h>
#include <mach/netx-regs.h>
#include <xfuncs.h>
#include <init.h>
#include <driver.h>
#include <linux/phy.h>

#define ETH_MAC_LOCAL_CONFIG 0x1560
#define ETH_MAC_4321         0x1564
#define ETH_MAC_65           0x1568

#define MAC_TRAFFIC_CLASS_ARRANGEMENT_SHIFT 16
#define MAC_TRAFFIC_CLASS_ARRANGEMENT_MASK (0xf<<MAC_TRAFFIC_CLASS_ARRANGEMENT_SHIFT)
#define MAC_TRAFFIC_CLASS_ARRANGEMENT(x) (((x)<<MAC_TRAFFIC_CLASS_ARRANGEMENT_SHIFT) & MAC_TRAFFIC_CLASS_ARRANGEMENT_MASK)

#define FIFO_PTR_FRAMELEN_SHIFT 0
#define FIFO_PTR_FRAMELEN_MASK  (0x7ff << 0)
#define FIFO_PTR_FRAMELEN(len)  (((len) << 0) & FIFO_PTR_FRAMELEN_MASK)
#define FIFO_PTR_TIMETRIG       (1<<11)
#define FIFO_PTR_MULTI_REQ
#define FIFO_PTR_ORIGIN         (1<<14)
#define FIFO_PTR_VLAN           (1<<15)
#define FIFO_PTR_FRAMENO_SHIFT  16
#define FIFO_PTR_FRAMENO_MASK   (0x3f << 16)
#define FIFO_PTR_FRAMENO(no)    ( ((no) << 16) & FIFO_PTR_FRAMENO_MASK)
#define FIFO_PTR_SEGMENT_SHIFT  22
#define FIFO_PTR_SEGMENT_MASK   (0xf << 22)
#define FIFO_PTR_SEGMENT(seg)   (((seg) & 0xf) << 22)
#define FIFO_PTR_ERROR_SHIFT    28
#define FIFO_PTR_ERROR_MASK     (0xf << 28)

/* use sram 0 for now */
#define SRAM_BASE(xcno) (0x8000 * (xcno))

/* XC Fifo Offsets */
#define EMPTY_PTR_FIFO(xcno)    (0 + ((xcno) << 3))    /* Index of the empty pointer FIFO */
#define IND_FIFO_PORT_HI(xcno)  (1 + ((xcno) << 3))    /* Index of the FIFO where received Data packages are indicated by XC */
#define IND_FIFO_PORT_LO(xcno)  (2 + ((xcno) << 3))    /* Index of the FIFO where received Data packages are indicated by XC */
#define REQ_FIFO_PORT_HI(xcno)  (3 + ((xcno) << 3))    /* Index of the FIFO where Data packages have to be indicated by ARM which shall be sent */
#define REQ_FIFO_PORT_LO(xcno)  (4 + ((xcno) << 3))    /* Index of the FIFO where Data packages have to be indicated by ARM which shall be sent */
#define CON_FIFO_PORT_HI(xcno)  (5 + ((xcno) << 3))    /* Index of the FIFO where sent Data packages are confirmed */
#define CON_FIFO_PORT_LO(xcno)  (6 + ((xcno) << 3))    /* Index of the FIFO where sent Data packages are confirmed */

struct netx_eth_priv {
	struct mii_bus miibus;
	int xcno;
};

static int netx_eth_send (struct eth_device *edev,
		void *packet, int length)
{
	struct netx_eth_priv *priv = (struct netx_eth_priv *)edev->priv;
	int xcno = priv->xcno;
	unsigned int val;
	int timeout = 500;
	unsigned char *dst = (unsigned char *)(SRAM_BASE(xcno) + 1560);

	memcpy(dst, (void *)packet, length);

	if( length < 60 ) {
		memset(dst + length, 0, 60 - length);
		length = 60;
	}

	PFIFO_REG(PFIFO_BASE(REQ_FIFO_PORT_LO(xcno))) =
		FIFO_PTR_SEGMENT(xcno) |
		FIFO_PTR_FRAMENO(1) |
		FIFO_PTR_FRAMELEN(length);

	while (!PFIFO_REG( PFIFO_FILL_LEVEL(CON_FIFO_PORT_LO(xcno))) && timeout) {
		timeout--;
		udelay(100);
	}
#if 0
	if (!timeout) {
		loadxc(0);
		loadxc(1);
		eth_init(gd->bd);
		return -1;
	}
#endif
	val = PFIFO_REG( PFIFO_BASE(CON_FIFO_PORT_LO(xcno)) );
	if((val & FIFO_PTR_ERROR_MASK) & 0x8)
		printf("error sending frame: %d\n",val);

	return 0;
}

static int netx_eth_rx (struct eth_device *edev)
{
	struct netx_eth_priv *priv = (struct netx_eth_priv *)edev->priv;
	int xcno = priv->xcno;
	unsigned int val, frameno, seg, len;

	if(!PFIFO_REG( PFIFO_FILL_LEVEL(IND_FIFO_PORT_LO(xcno)))) {
		return 0;
	}

	val = PFIFO_REG( PFIFO_BASE(IND_FIFO_PORT_LO(xcno)) );

	frameno = (val & FIFO_PTR_FRAMENO_MASK) >> FIFO_PTR_FRAMENO_SHIFT;
	seg = (val & FIFO_PTR_SEGMENT_MASK) >> FIFO_PTR_SEGMENT_SHIFT;
	len = (val & FIFO_PTR_FRAMELEN_MASK) >> FIFO_PTR_FRAMELEN_SHIFT;

	/* get data */
	memcpy((void*)NetRxPackets[0], (void *)(SRAM_BASE(seg) + frameno * 1560), len);
	/* pass to barebox */
	net_receive(NetRxPackets[0], len);

	PFIFO_REG(PFIFO_BASE(EMPTY_PTR_FIFO(xcno))) =
		FIFO_PTR_SEGMENT(seg) |
		FIFO_PTR_FRAMENO(frameno);
	return 0;
}

static int netx_miibus_read(struct mii_bus *bus, int phy_addr, int reg)
{
	int value;

	MIIMU_REG = MIIMU_SNRDY | MIIMU_PREAMBLE | MIIMU_PHYADDR(phy_addr) |
	            MIIMU_REGADDR(reg) | MIIMU_PHY_NRES;

	while(MIIMU_REG & MIIMU_SNRDY);

	value = MIIMU_REG >> 16;

	debug("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n", __func__,
	      phy_addr, reg, value);

	return value;
}

static int netx_miibus_write(struct mii_bus *bus, int phy_addr,
	int reg, u16 val)
{
	debug("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n",__func__,
	      phy_addr, reg, val);

	MIIMU_REG = MIIMU_SNRDY | MIIMU_PREAMBLE | MIIMU_PHYADDR(phy_addr) |
	            MIIMU_REGADDR(reg) | MIIMU_PHY_NRES | MIIMU_OPMODE_WRITE |
		    MIIMU_DATA(val);

	while(MIIMU_REG & MIIMU_SNRDY);

	return 0;
}

static int netx_eth_init_phy(void)
{
	unsigned int phy_control;

	phy_control = PHY_CONTROL_PHY_ADDRESS(0xe) |
	              PHY_CONTROL_PHY1_MODE(PHY_MODE_ALL) |
		      PHY_CONTROL_PHY1_AUTOMDIX |
	              PHY_CONTROL_PHY1_EN |
	              PHY_CONTROL_PHY0_MODE(PHY_MODE_ALL) |
		      PHY_CONTROL_PHY0_AUTOMDIX |
	              PHY_CONTROL_PHY0_EN |
	              PHY_CONTROL_CLK_XLATIN;

	/* enable asic control */
	SYSTEM_REG(SYSTEM_IOC_ACCESS_KEY) = SYSTEM_REG(SYSTEM_IOC_ACCESS_KEY);

	SYSTEM_REG(SYSTEM_PHY_CONTROL) = phy_control | PHY_CONTROL_RESET;
	udelay(100);

	/* enable asic control */
	SYSTEM_REG(SYSTEM_IOC_ACCESS_KEY) = SYSTEM_REG(SYSTEM_IOC_ACCESS_KEY);

	SYSTEM_REG(SYSTEM_PHY_CONTROL) = phy_control;

	return 0;
}

static int netx_eth_init_dev(struct eth_device *edev)
{
	struct netx_eth_priv *priv = (struct netx_eth_priv *)edev->priv;
	int xcno = priv->xcno;
	int i;

	loadxc(xcno);

	/* Fill empty pointer fifo */
	for (i = 2; i <= 18; i++)
		PFIFO_REG( PFIFO_BASE(EMPTY_PTR_FIFO(xcno)) ) = FIFO_PTR_FRAMENO(i) | FIFO_PTR_SEGMENT(xcno);

	return 0;
}

static int netx_eth_open(struct eth_device *edev)
{
	struct netx_eth_priv *priv = (struct netx_eth_priv *)edev->priv;

	return phy_device_connect(edev, &priv->miibus, 0, NULL,
				 0, PHY_INTERFACE_MODE_NA);
}

static void netx_eth_halt (struct eth_device *edev)
{
}

static int netx_eth_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	/* FIXME: get from crypto flash */
        return -1;
}

static int netx_eth_set_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct netx_eth_priv *priv = (struct netx_eth_priv *)edev->priv;
	int xcno = priv->xcno;

	debug("%s\n", __func__);

	/* set MAC address */
	XMAC_REG(xcno, XMAC_RPU_HOLD_PC) = RPU_HOLD_PC;
	XMAC_REG(xcno, XMAC_TPU_HOLD_PC) = TPU_HOLD_PC;
	XPEC_REG(xcno, XPEC_XPU_HOLD_PC) = XPU_HOLD_PC;
	XPEC_REG(xcno, XPEC_RAM_START + ETH_MAC_4321) = adr[0] | adr[1]<<8 | adr[2]<<16 | adr[3]<<24;
	XPEC_REG(xcno, XPEC_RAM_START + ETH_MAC_65) = adr[4] | adr[5]<<8;
	XPEC_REG(xcno, XPEC_RAM_START + ETH_MAC_LOCAL_CONFIG) = MAC_TRAFFIC_CLASS_ARRANGEMENT(8);
	XMAC_REG(xcno, XMAC_RPU_HOLD_PC) = 0;
	XMAC_REG(xcno, XMAC_TPU_HOLD_PC) = 0;
	XPEC_REG(xcno, XPEC_XPU_HOLD_PC) = 0;

#if 0
	for (i = 0; i < 5; i++)
		printf ("%02x:", adr[i]);
	printf ("%02x\n", adr[5]);
#endif
	return -0;
}

static int netx_eth_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct netx_eth_priv *priv;
	struct netx_eth_platform_data *pdata;

	debug("netx_eth_probe()\n");

	pdata = dev->platform_data;

	edev = xzalloc(sizeof(struct eth_device) + sizeof(struct netx_eth_priv));
	edev->priv = (struct netx_priv *)(edev + 1);

	priv = edev->priv;
	priv->xcno = pdata->xcno;

	edev->init = netx_eth_init_dev;
	edev->open = netx_eth_open;
	edev->send = netx_eth_send;
	edev->recv = netx_eth_rx;
	edev->halt = netx_eth_halt;
	edev->get_ethaddr = netx_eth_get_ethaddr;
	edev->set_ethaddr = netx_eth_set_ethaddr;
	edev->parent = dev;

	priv->miibus.read = netx_miibus_read;
	priv->miibus.write = netx_miibus_write;
	priv->miibus.parent = dev;

	netx_eth_init_phy();
	mdiobus_register(&priv->miibus);
	eth_register(edev);

        return 0;
}

static struct driver_d netx_eth_driver = {
        .name  = "netx-eth",
        .probe = netx_eth_probe,
};
device_platform_driver(netx_eth_driver);

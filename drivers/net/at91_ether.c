/*
 * (C) Copyright 2003
 * Author : Hamid Ikdoumi (Atmel)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <at91rm9200_net.h>
#include <init.h>
#include <net.h>
#include <miidev.h>
#include <malloc.h>
#include <driver.h>

/* ----- Ethernet Buffer definitions ----- */

typedef struct {
	unsigned long addr, size;
} rbf_t;

#define RBF_ADDR      0xfffffffc
#define RBF_OWNER     (1<<0)
#define RBF_WRAP      (1<<1)
#define RBF_BROADCAST (1<<31)
#define RBF_MULTICAST (1<<30)
#define RBF_UNICAST   (1<<29)
#define RBF_EXTERNAL  (1<<28)
#define RBF_UNKOWN    (1<<27)
#define RBF_SIZE      0x07ff
#define RBF_LOCAL4    (1<<26)
#define RBF_LOCAL3    (1<<25)
#define RBF_LOCAL2    (1<<24)
#define RBF_LOCAL1    (1<<23)

#define RBF_FRAMEMAX 64
#define RBF_FRAMELEN 0x600

/* alignment as per Errata #11 (64 bytes) is insufficient! */
rbf_t rbfdt[RBF_FRAMEMAX] __attribute((aligned(512)));
rbf_t *rbfp;

unsigned char rbf_framebuf[RBF_FRAMEMAX][RBF_FRAMELEN] __attribute((aligned(4)));

/* structure to interface the PHY */
AT91S_PhyOps PhyOps;

AT91PS_EMAC p_mac;

/*********** EMAC Phy layer Management functions *************************/
/*
 * Name:
 *	at91rm9200_EmacEnableMDIO
 * Description:
 *	Enables the MDIO bit in MAC control register
 * Arguments:
 *	p_mac - pointer to struct AT91S_EMAC
 * Return value:
 *	none
 */
void at91rm9200_EmacEnableMDIO (AT91PS_EMAC p_mac)
{
	/* Mac CTRL reg set for MDIO enable */
	p_mac->EMAC_CTL |= AT91C_EMAC_MPE;	/* Management port enable */
}

/*
 * Name:
 *	at91rm9200_EmacDisableMDIO
 * Description:
 *	Disables the MDIO bit in MAC control register
 * Arguments:
 *	p_mac - pointer to struct AT91S_EMAC
 * Return value:
 *	none
 */
void at91rm9200_EmacDisableMDIO (AT91PS_EMAC p_mac)
{
	/* Mac CTRL reg set for MDIO disable */
	p_mac->EMAC_CTL &= ~AT91C_EMAC_MPE;	/* Management port disable */
}


/*
 * Name:
 *	at91rm9200_EmacReadPhy
 * Description:
 *	Reads data from the PHY register
 * Arguments:
 *	dev - pointer to struct net_device
 *	RegisterAddress - unsigned char
 * 	pInput - pointer to value read from register
 * Return value:
 *	TRUE - if data read successfully
 */
UCHAR at91rm9200_EmacReadPhy (AT91PS_EMAC p_mac,
				     unsigned char RegisterAddress,
				     unsigned short *pInput)
{
	p_mac->EMAC_MAN = (AT91C_EMAC_HIGH & ~AT91C_EMAC_LOW) |
			  (AT91C_EMAC_RW_R) |
			  (RegisterAddress << 18) |
			  (AT91C_EMAC_CODE_802_3);

	udelay (10000);

	*pInput = (unsigned short) p_mac->EMAC_MAN;

	return TRUE;
}


/*
 * Name:
 *	at91rm9200_EmacWritePhy
 * Description:
 *	Writes data to the PHY register
 * Arguments:
 *	dev - pointer to struct net_device
 *	RegisterAddress - unsigned char
 * 	pOutput - pointer to value to be written in the register
 * Return value:
 *	TRUE - if data read successfully
 */
UCHAR at91rm9200_EmacWritePhy (AT91PS_EMAC p_mac,
				      unsigned char RegisterAddress,
				      unsigned short *pOutput)
{
	p_mac->EMAC_MAN = (AT91C_EMAC_HIGH & ~AT91C_EMAC_LOW) |
			AT91C_EMAC_CODE_802_3 | AT91C_EMAC_RW_W |
			(RegisterAddress << 18) | *pOutput;

	udelay (10000);

	return TRUE;
}

static int at91rm9200_eth_open (struct eth_device *edev)
{
	int ret;

	at91rm9200_GetPhyInterface (& PhyOps);

	if (!PhyOps.IsPhyConnected (p_mac))
		printf ("PHY not connected!!\n\r");

	/* MII management start from here */
	if (!(p_mac->EMAC_SR & AT91C_EMAC_LINK)) {
		if (!(ret = PhyOps.Init (p_mac))) {
			printf ("MAC: error during MII initialization\n");
			return 0;
		}
	} else {
		printf ("No link\n\r");
		return 0;
	}
	return 0;
}

static int at91rm9200_eth_send (struct eth_device *edev, volatile void *packet, int length)
{
	while (!(p_mac->EMAC_TSR & AT91C_EMAC_BNQ));
	p_mac->EMAC_TAR = (long) packet;
	p_mac->EMAC_TCR = length;
	while (p_mac->EMAC_TCR & 0x7ff);
	p_mac->EMAC_TSR |= AT91C_EMAC_COMP;
	return 0;
}

static int at91rm9200_eth_rx (struct eth_device *edev)
{
	int size;

	if (!(rbfp->addr & RBF_OWNER))
		return 0;

	size = rbfp->size & RBF_SIZE;
	net_receive((volatile uchar *) (rbfp->addr & RBF_ADDR), size);

	rbfp->addr &= ~RBF_OWNER;
	if (rbfp->addr & RBF_WRAP)
		rbfp = &rbfdt[0];
	else
		rbfp++;

	p_mac->EMAC_RSR |= AT91C_EMAC_REC;

	return size;
}

static void at91rm9200_eth_halt (struct eth_device *edev)
{
};

#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
int  at91rm9200_miidev_read(char *devname, unsigned char addr,
		unsigned char reg, unsigned short * value)
{
	at91rm9200_EmacEnableMDIO (p_mac);
	at91rm9200_EmacReadPhy (p_mac, reg, value);
	at91rm9200_EmacDisableMDIO (p_mac);
	return 0;
}

int  at91rm9200_miidev_write(char *devname, unsigned char addr,
		unsigned char reg, unsigned short value)
{
	at91rm9200_EmacEnableMDIO (p_mac);
	at91rm9200_EmacWritePhy (p_mac, reg, &value);
	at91rm9200_EmacDisableMDIO (p_mac);
	return 0;
}

#endif	/* defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII) */

int at91rm9200_miidev_initialize(void)
{
#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
	mii_register("at91rm9200phy", at91rm9200_miidev_read, at91rm9200_miidev_write);
#endif
	return 0;
}

static int at91rm9200_get_ethaddr(struct eth_device *eth, unsigned char *adr)
{
	/* We have no eeprom */
	return -1;
}

static int at91rm9200_set_ethaddr(struct eth_device *eth, unsigned char *adr)
{
	int i;

	p_mac->EMAC_SA2L = (adr[3] << 24) | (adr[2] << 16)
			 | (adr[1] <<  8) | (adr[0]);
	p_mac->EMAC_SA2H = (adr[5] <<  8) | (adr[4]);

#if 1
	for (i = 0; i < 5; i++)
		printf ("%02x:", adr[i]);
	printf ("%02x\n", adr[5]);
#endif
	return -0;
}

static int at91rm9200_eth_init (struct device_d *dev)
{
	struct eth_device *edev;
	int i;

	edev = xmalloc(sizeof(struct eth_device));
	dev->priv = edev;

	edev->open = at91rm9200_eth_open;
	edev->send = at91rm9200_eth_send;
	edev->recv = at91rm9200_eth_rx;
	edev->halt = at91rm9200_eth_halt;
	edev->get_ethaddr = at91rm9200_get_ethaddr;
	edev->set_ethaddr = at91rm9200_set_ethaddr;

	p_mac = AT91C_BASE_EMAC;

	/* PIO Disable Register */
	*AT91C_PIOA_PDR = AT91C_PA16_EMDIO | AT91C_PA15_EMDC | AT91C_PA14_ERXER |
			  AT91C_PA13_ERX1 | AT91C_PA12_ERX0 | AT91C_PA11_ECRS_ECRSDV |
			  AT91C_PA10_ETX1 | AT91C_PA9_ETX0 | AT91C_PA8_ETXEN |
			  AT91C_PA7_ETXCK_EREFCK;

#ifdef CONFIG_AT91C_USE_RMII
	*AT91C_PIOB_PDR = AT91C_PB19_ERXCK;
	*AT91C_PIOB_BSR = AT91C_PB19_ERXCK;
#else
	*AT91C_PIOB_PDR = AT91C_PB19_ERXCK | AT91C_PB18_ECOL | AT91C_PB17_ERXDV |
			  AT91C_PB16_ERX3 | AT91C_PB15_ERX2 | AT91C_PB14_ETXER |
			  AT91C_PB13_ETX3 | AT91C_PB12_ETX2;

	/* Select B Register */
	*AT91C_PIOB_BSR = AT91C_PB19_ERXCK | AT91C_PB18_ECOL |
			  AT91C_PB17_ERXDV | AT91C_PB16_ERX3 | AT91C_PB15_ERX2 |
			  AT91C_PB14_ETXER | AT91C_PB13_ETX3 | AT91C_PB12_ETX2;
#endif

	*AT91C_PMC_PCER = 1 << AT91C_ID_EMAC;	/* Peripheral Clock Enable Register */

	p_mac->EMAC_CFG |= AT91C_EMAC_CSR;	/* Clear statistics */

	/* Init Ehternet buffers */
	for (i = 0; i < RBF_FRAMEMAX; i++) {
		rbfdt[i].addr = (unsigned long)rbf_framebuf[i];
		rbfdt[i].size = 0;
	}
	rbfdt[RBF_FRAMEMAX - 1].addr |= RBF_WRAP;
	rbfp = &rbfdt[0];

	p_mac->EMAC_RBQP = (long) (&rbfdt[0]);
	p_mac->EMAC_RSR &= ~(AT91C_EMAC_RSR_OVR | AT91C_EMAC_REC | AT91C_EMAC_BNA);

	p_mac->EMAC_CFG = (p_mac->EMAC_CFG | AT91C_EMAC_CAF | AT91C_EMAC_NBC)
			& ~AT91C_EMAC_CLK;

#ifdef CONFIG_AT91C_USE_RMII
	p_mac->EMAC_CFG |= AT91C_EMAC_RMII;
#endif

#if (AT91C_MASTER_CLOCK > 40000000)
	/* MDIO clock must not exceed 2.5 MHz, so enable MCK divider */
	p_mac->EMAC_CFG |= AT91C_EMAC_CLK_HCLK_64;
#endif

	p_mac->EMAC_CTL |= AT91C_EMAC_TE | AT91C_EMAC_RE;

	eth_register(edev);

	return 0;
}

static struct driver_d at91_eth_driver = {
        .name  = "at91_eth",
        .probe = at91rm9200_eth_init,
};

static int at91_eth_init(void)
{
        register_driver(&at91_eth_driver);
        return 0;
}

device_initcall(at91_eth_init);


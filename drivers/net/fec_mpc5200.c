/*
 * (C) Copyright 2003-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This file is based on mpc4200fec.c,
 * (C) Copyright Motorola, Inc., 2000
 */

#include <common.h>
#include <mpc5xxx.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <miiphy.h>
#include <driver.h>
#include <asm/arch/sdma.h>
#include <asm/arch/fec.h>
#include <asm/arch/clocks.h>
#include <miiphy.h>
#include "fec_mpc5200.h"

#define CONFIG_PHY_ADDR 1 /* FIXME */
/* #define DEBUG	0x28 */

#if (DEBUG & 0x60)
static void tfifo_print(char *devname, mpc5xxx_fec_priv *fec);
static void rfifo_print(char *devname, mpc5xxx_fec_priv *fec);
#endif /* DEBUG */

typedef struct {
    uint8 data[1500];           /* actual data */
    int length;                 /* actual length */
    int used;                   /* buffer in use or not */
    uint8 head[16];             /* MAC header(6 + 6 + 2) + 2(aligned) */
} NBUF;

static int fec5xxx_miiphy_read(struct miiphy_device *mdev, uint8 phyAddr, uint8 regAddr, uint16 * retVal);
static int fec5xxx_miiphy_write(struct miiphy_device *mdev, uint8 phyAddr, uint8 regAddr, uint16 data);

/********************************************************************/
static int mpc5xxx_fec_rbd_init(mpc5xxx_fec_priv *fec)
{
	int ix;
	char *data;
	static int once = 0;

	for (ix = 0; ix < FEC_RBD_NUM; ix++) {
		if (!once) {
			data = (char *)malloc(FEC_MAX_PKT_SIZE);
			if (data == NULL) {
				printf ("RBD INIT FAILED\n");
				return -1;
			}
			fec->rbdBase[ix].dataPointer = (uint32)data;
		}
		fec->rbdBase[ix].status = FEC_RBD_EMPTY;
		fec->rbdBase[ix].dataLength = 0;
	}
	once ++;

	/*
	 * have the last RBD to close the ring
	 */
	fec->rbdBase[ix - 1].status |= FEC_RBD_WRAP;
	fec->rbdIndex = 0;

	return 0;
}

/********************************************************************/
static void mpc5xxx_fec_tbd_init(mpc5xxx_fec_priv *fec)
{
	int ix;

	for (ix = 0; ix < FEC_TBD_NUM; ix++) {
		fec->tbdBase[ix].status = 0;
	}

	/*
	 * Have the last TBD to close the ring
	 */
	fec->tbdBase[ix - 1].status |= FEC_TBD_WRAP;

	/*
	 * Initialize some indices
	 */
	fec->tbdIndex = 0;
	fec->usedTbdIndex = 0;
	fec->cleanTbdNum = FEC_TBD_NUM;
}

/********************************************************************/
static void mpc5xxx_fec_rbd_clean(mpc5xxx_fec_priv *fec, volatile FEC_RBD * pRbd)
{
	/*
	 * Reset buffer descriptor as empty
	 */
	if ((fec->rbdIndex) == (FEC_RBD_NUM - 1))
		pRbd->status = (FEC_RBD_WRAP | FEC_RBD_EMPTY);
	else
		pRbd->status = FEC_RBD_EMPTY;

	pRbd->dataLength = 0;

	/*
	 * Now, we have an empty RxBD, restart the SmartDMA receive task
	 */
	SDMA_TASK_ENABLE(FEC_RECV_TASK_NO);

	/*
	 * Increment BD count
	 */
	fec->rbdIndex = (fec->rbdIndex + 1) % FEC_RBD_NUM;
}

/********************************************************************/
static void mpc5xxx_fec_tbd_scrub(mpc5xxx_fec_priv *fec)
{
	volatile FEC_TBD *pUsedTbd;

#if (DEBUG & 0x1)
	printf ("tbd_scrub: fec->cleanTbdNum = %d, fec->usedTbdIndex = %d\n",
		fec->cleanTbdNum, fec->usedTbdIndex);
#endif

	/*
	 * process all the consumed TBDs
	 */
	while (fec->cleanTbdNum < FEC_TBD_NUM) {
		pUsedTbd = &fec->tbdBase[fec->usedTbdIndex];
		if (pUsedTbd->status & FEC_TBD_READY) {
#if (DEBUG & 0x20)
			printf("Cannot clean TBD %d, in use\n", fec->cleanTbdNum);
#endif
			return;
		}

		/*
		 * clean this buffer descriptor
		 */
		if (fec->usedTbdIndex == (FEC_TBD_NUM - 1))
			pUsedTbd->status = FEC_TBD_WRAP;
		else
			pUsedTbd->status = 0;

		/*
		 * update some indeces for a correct handling of the TBD ring
		 */
		fec->cleanTbdNum++;
		fec->usedTbdIndex = (fec->usedTbdIndex + 1) % FEC_TBD_NUM;
	}
}

/********************************************************************/
static int mpc5xxx_fec_get_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	/* no eeprom */
	return -1;
}

static int mpc5xxx_fec_set_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	uint8 currByte;			/* byte for which to compute the CRC */
	int byte;			/* loop - counter */
	int bit;			/* loop - counter */
	uint32 crc = 0xffffffff;	/* initial value */

	/*
	 * The algorithm used is the following:
	 * we loop on each of the six bytes of the provided address,
	 * and we compute the CRC by left-shifting the previous
	 * value by one position, so that each bit in the current
	 * byte of the address may contribute the calculation. If
	 * the latter and the MSB in the CRC are different, then
	 * the CRC value so computed is also ex-ored with the
	 * "polynomium generator". The current byte of the address
	 * is also shifted right by one bit at each iteration.
	 * This is because the CRC generatore in hardware is implemented
	 * as a shift-register with as many ex-ores as the radixes
	 * in the polynomium. This suggests that we represent the
	 * polynomiumm itself as a 32-bit constant.
	 */
	for (byte = 0; byte < 6; byte++) {
		currByte = mac[byte];
		for (bit = 0; bit < 8; bit++) {
			if ((currByte & 0x01) ^ (crc & 0x01)) {
				crc >>= 1;
				crc = crc ^ 0xedb88320;
			} else {
				crc >>= 1;
			}
			currByte >>= 1;
		}
	}

	crc = crc >> 26;

	/*
	 * Set individual hash table register
	 */
	if (crc >= 32) {
		fec->eth->iaddr1 = (1 << (crc - 32));
		fec->eth->iaddr2 = 0;
	} else {
		fec->eth->iaddr1 = 0;
		fec->eth->iaddr2 = (1 << crc);
	}

	/*
	 * Set physical address
	 */
	fec->eth->paddr1 = (mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3];
	fec->eth->paddr2 = (mac[4] << 24) + (mac[5] << 16) + 0x8808;

        return 0;
}

/********************************************************************/
static int mpc5xxx_fec_init(struct eth_device *dev)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5XXX_SDMA;

#if (DEBUG & 0x1)
	printf ("mpc5xxx_fec_init... Begin\n");
#endif

	/*
	 * Initialize RxBD/TxBD rings
	 */
	mpc5xxx_fec_rbd_init(fec);
	mpc5xxx_fec_tbd_init(fec);

	/*
	 * Clear FEC-Lite interrupt event register(IEVENT)
	 */
	fec->eth->ievent = 0xffffffff;

	/*
	 * Set interrupt mask register
	 */
	fec->eth->imask = 0x00000000;

	/*
	 * Set FEC-Lite receive control register(R_CNTRL):
	 */
	if (fec->xcv_type == SEVENWIRE) {
		/*
		 * Frame length=1518; 7-wire mode
		 */
		fec->eth->r_cntrl = 0x05ee0020;	/*0x05ee0000;FIXME */
	} else {
		/*
		 * Frame length=1518; MII mode;
		 */
		fec->eth->r_cntrl = 0x05ee0024;	/*0x05ee0004;FIXME */
	}

	if (fec->xcv_type != SEVENWIRE) {
		/*
		 * Set MII_SPEED = (1/(mii_speed * 2)) * System Clock
		 * and do not drop the Preamble.
		 */
		printf("%s: miispeed\n", __FUNCTION__);
		fec->eth->mii_speed = (((get_ipb_clock() >> 20) / 5) << 1);	/* No MII for 7-wire mode */
		printf("done: %d\n", get_ipb_clock());
	}

	/*
	 * Set Opcode/Pause Duration Register
	 */
	fec->eth->op_pause = 0x00010020;	/*FIXME 0xffff0020; */

	/*
	 * Set Rx FIFO alarm and granularity value
	 */
	fec->eth->rfifo_cntrl = 0x0c000000
				| (fec->eth->rfifo_cntrl & ~0x0f000000);
	fec->eth->rfifo_alarm = 0x0000030c;
#if (DEBUG & 0x22)
	if (fec->eth->rfifo_status & 0x00700000 ) {
		printf("mpc5xxx_fec_init() RFIFO error\n");
	}
#endif

	/*
	 * Set Tx FIFO granularity value
	 */
	fec->eth->tfifo_cntrl = 0x0c000000
				| (fec->eth->tfifo_cntrl & ~0x0f000000);
#if (DEBUG & 0x2)
	printf("tfifo_status: 0x%08x\n", fec->eth->tfifo_status);
	printf("tfifo_alarm: 0x%08x\n", fec->eth->tfifo_alarm);
#endif

	/*
	 * Set transmit fifo watermark register(X_WMRK), default = 64
	 */
	fec->eth->tfifo_alarm = 0x00000080;
	fec->eth->x_wmrk = 0x2;

	/*
	 * Set multicast address filter
	 */
	fec->eth->gaddr1 = 0x00000000;
	fec->eth->gaddr2 = 0x00000000;

	/*
	 * Turn ON cheater FSM: ????
	 */
	fec->eth->xmit_fsm = 0x03000000;

	/*
	 * Set priority of different initiators
	 */
	sdma->IPR0 = 7;		/* always */
	sdma->IPR3 = 6;		/* Eth RX */
	sdma->IPR4 = 5;		/* Eth Tx */

	/*
	 * Clear SmartDMA task interrupt pending bits
	 */
	SDMA_CLEAR_IEVENT(FEC_RECV_TASK_NO);

	/*
	 * Initialize SmartDMA parameters stored in SRAM
	 */
	*(volatile int *)FEC_TBD_BASE = (int)fec->tbdBase;
	*(volatile int *)FEC_RBD_BASE = (int)fec->rbdBase;
	*(volatile int *)FEC_TBD_NEXT = (int)fec->tbdBase;
	*(volatile int *)FEC_RBD_NEXT = (int)fec->rbdBase;

#if (DEBUG & 0x1)
	printf("mpc5xxx_fec_init... Done \n");
#endif
	if (fec->xcv_type != SEVENWIRE)
		miiphy_restart_aneg(&fec->miiphy);

	return 0;
}

static int mpc5xxx_fec_open(struct eth_device *edev)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)edev->priv;

#if defined(CONFIG_MPC5200)
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5XXX_SDMA;
	/*
	 * Turn off COMM bus prefetch in the MGT5200 BestComm. It doesn't
	 * work w/ the current receive task.
	 */
	 sdma->PtdCntrl |= 0x00000001;
#endif

	fec->eth->x_cntrl = 0x00000000;	/* half-duplex, heartbeat disabled */

	/*
	 * Enable FEC-Lite controller
	 */
	fec->eth->ecntrl |= 0x00000006;

	/*
	 * Enable SmartDMA receive task
	 */
	SDMA_TASK_ENABLE(FEC_RECV_TASK_NO);

	if (fec->xcv_type != SEVENWIRE) {
		miiphy_wait_aneg(&fec->miiphy);
		miiphy_print_status(&fec->miiphy);
	}

	return 0;
}

static void mpc5xxx_fec_halt(struct eth_device *dev)
{
#if defined(CONFIG_MPC5200)
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5XXX_SDMA;
#endif
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	int counter = 0xffff;

#if (DEBUG & 0x2)
	if (fec->xcv_type != SEVENWIRE)
		mpc5xxx_fec_phydump ();
#endif

	/*
	 * issue graceful stop command to the FEC transmitter if necessary
	 */
	fec->eth->x_cntrl |= 0x00000001;

	/*
	 * wait for graceful stop to register
	 */
	while ((counter--) && (!(fec->eth->ievent & 0x10000000))) ;

	/*
	 * Disable SmartDMA tasks
	 */
	SDMA_TASK_DISABLE (FEC_XMIT_TASK_NO);
	SDMA_TASK_DISABLE (FEC_RECV_TASK_NO);

#if defined(CONFIG_MPC5200)
	/*
	 * Turn on COMM bus prefetch in the MGT5200 BestComm after we're
	 * done. It doesn't work w/ the current receive task.
	 */
	 sdma->PtdCntrl &= ~0x00000001;
#endif

	/*
	 * Disable the Ethernet Controller
	 */
	fec->eth->ecntrl &= 0xfffffffd;

	/*
	 * Clear FIFO status registers
	 */
	fec->eth->rfifo_status &= 0x00700000;
	fec->eth->tfifo_status &= 0x00700000;

//	fec->eth->reset_cntrl = 0x01000000;

#if (DEBUG & 0x3)
	printf("Ethernet task stopped\n");
#endif
}

#if (DEBUG & 0x60)
/********************************************************************/

static void tfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
	uint16 phyAddr = CONFIG_PHY_ADDR;
	uint16 phyStatus;

	if ((fec->eth->tfifo_lrf_ptr != fec->eth->tfifo_lwf_ptr)
		|| (fec->eth->tfifo_rdptr != fec->eth->tfifo_wrptr)) {

		fec5xxx_miiphy_read(devname, phyAddr, 0x1, &phyStatus);
		printf("\nphyStatus: 0x%04x\n", phyStatus);
		printf("ecntrl:   0x%08x\n", fec->eth->ecntrl);
		printf("ievent:   0x%08x\n", fec->eth->ievent);
		printf("x_status: 0x%08x\n", fec->eth->x_status);
		printf("tfifo: status  0x%08x\n", fec->eth->tfifo_status);

		printf("       control 0x%08x\n", fec->eth->tfifo_cntrl);
		printf("       lrfp    0x%08x\n", fec->eth->tfifo_lrf_ptr);
		printf("       lwfp    0x%08x\n", fec->eth->tfifo_lwf_ptr);
		printf("       alarm   0x%08x\n", fec->eth->tfifo_alarm);
		printf("       readptr 0x%08x\n", fec->eth->tfifo_rdptr);
		printf("       writptr 0x%08x\n", fec->eth->tfifo_wrptr);
	}
}

static void rfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
	uint16 phyAddr = CONFIG_PHY_ADDR;
	uint16 phyStatus;

	if ((fec->eth->rfifo_lrf_ptr != fec->eth->rfifo_lwf_ptr)
		|| (fec->eth->rfifo_rdptr != fec->eth->rfifo_wrptr)) {

		fec5xxx_miiphy_read(devname, phyAddr, 0x1, &phyStatus);
		printf("\nphyStatus: 0x%04x\n", phyStatus);
		printf("ecntrl:   0x%08x\n", fec->eth->ecntrl);
		printf("ievent:   0x%08x\n", fec->eth->ievent);
		printf("x_status: 0x%08x\n", fec->eth->x_status);
		printf("rfifo: status  0x%08x\n", fec->eth->rfifo_status);

		printf("       control 0x%08x\n", fec->eth->rfifo_cntrl);
		printf("       lrfp    0x%08x\n", fec->eth->rfifo_lrf_ptr);
		printf("       lwfp    0x%08x\n", fec->eth->rfifo_lwf_ptr);
		printf("       alarm   0x%08x\n", fec->eth->rfifo_alarm);
		printf("       readptr 0x%08x\n", fec->eth->rfifo_rdptr);
		printf("       writptr 0x%08x\n", fec->eth->rfifo_wrptr);
	}
}
#endif /* DEBUG */

/********************************************************************/

static int mpc5xxx_fec_send(struct eth_device *dev, void *eth_data,
		int data_length)
{
	/*
	 * This routine transmits one frame.  This routine only accepts
	 * 6-byte Ethernet addresses.
	 */
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	volatile FEC_TBD *pTbd;

#if (DEBUG & 0x20)
	printf("tbd status: 0x%04x\n", fec->tbdBase[0].status);
	tfifo_print(dev->name, fec);
#endif

	/*
	 * Clear Tx BD ring at first
	 */
	mpc5xxx_fec_tbd_scrub(fec);

	/*
	 * Check for valid length of data.
	 */
	if ((data_length > 1500) || (data_length <= 0)) {
		return -1;
	}

	/*
	 * Check the number of vacant TxBDs.
	 */
	if (fec->cleanTbdNum < 1) {
#if (DEBUG & 0x20)
		printf("No available TxBDs ...\n");
#endif
		return -1;
	}

	/*
	 * Get the first TxBD to send the mac header
	 */
	pTbd = &fec->tbdBase[fec->tbdIndex];
	pTbd->dataLength = data_length;
	pTbd->dataPointer = (uint32)eth_data;
	pTbd->status |= FEC_TBD_LAST | FEC_TBD_TC | FEC_TBD_READY;
	fec->tbdIndex = (fec->tbdIndex + 1) % FEC_TBD_NUM;

#if (DEBUG & 0x100)
	printf("SDMA_TASK_ENABLE, fec->tbdIndex = %d \n", fec->tbdIndex);
#endif

	/*
	 * Kick the MII i/f
	 */
	if (fec->xcv_type != SEVENWIRE) {
		uint16 phyStatus;
		fec5xxx_miiphy_read(&fec->miiphy, 0, 0x1, &phyStatus);
	}

	/*
	 * Enable SmartDMA transmit task
	 */

#if (DEBUG & 0x20)
	tfifo_print(dev->name, fec);
#endif
	SDMA_TASK_ENABLE (FEC_XMIT_TASK_NO);
#if (DEBUG & 0x20)
	tfifo_print(dev->name, fec);
#endif
#if (DEBUG & 0x8)
	printf( "+" );
#endif

	fec->cleanTbdNum -= 1;

#if (DEBUG & 0x129) && (DEBUG & 0x80000000)
	printf ("smartDMA ethernet Tx task enabled\n");
#endif
	/*
	 * wait until frame is sent .
	 */
	while (pTbd->status & FEC_TBD_READY) {
		udelay(10);
#if (DEBUG & 0x8)
		printf ("TDB status = %04x\n", pTbd->status);
#endif
	}

	return 0;
}


/********************************************************************/
static int mpc5xxx_fec_recv(struct eth_device *dev)
{
	/*
	 * This command pulls one frame from the card
	 */
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	volatile FEC_RBD *pRbd = &fec->rbdBase[fec->rbdIndex];
	unsigned long ievent;
	int frame_length, len = 0;
	NBUF *frame;
	uchar buff[FEC_MAX_PKT_SIZE];

#if (DEBUG & 0x1)
	printf ("mpc5xxx_fec_recv %d Start...\n", fec->rbdIndex);
#endif
#if (DEBUG & 0x8)
	printf( "-" );
#endif

	/*
	 * Check if any critical events have happened
	 */
	ievent = fec->eth->ievent;
	fec->eth->ievent = ievent;
	if (ievent & 0x20060000) {
		/* BABT, Rx/Tx FIFO errors */
		mpc5xxx_fec_halt(dev);
		mpc5xxx_fec_init(dev);
		return 0;
	}
	if (ievent & 0x80000000) {
		/* Heartbeat error */
		fec->eth->x_cntrl |= 0x00000001;
	}
	if (ievent & 0x10000000) {
		/* Graceful stop complete */
		if (fec->eth->x_cntrl & 0x00000001) {
			mpc5xxx_fec_halt(dev);
			fec->eth->x_cntrl &= ~0x00000001;
			mpc5xxx_fec_init(dev);
		}
	}

	if (!(pRbd->status & FEC_RBD_EMPTY)) {
		if ((pRbd->status & FEC_RBD_LAST) && !(pRbd->status & FEC_RBD_ERR) &&
			((pRbd->dataLength - 4) > 14)) {

			/*
			 * Get buffer address and size
			 */
			frame = (NBUF *)pRbd->dataPointer;
			frame_length = pRbd->dataLength - 4;

#if (DEBUG & 0x20)
			{
				int i;
				printf("recv data hdr:");
				for (i = 0; i < 14; i++)
					printf("%02x ", *(frame->head + i));
				printf("\n");
			}
#endif
			/*
			 *  Fill the buffer and pass it to upper layers
			 */
			memcpy(buff, frame->head, 14);
			memcpy(buff + 14, frame->data, frame_length);
			NetReceive(buff, frame_length);
			len = frame_length;
		}
		/*
		 * Reset buffer descriptor as empty
		 */
		mpc5xxx_fec_rbd_clean(fec, pRbd);
	}

	SDMA_CLEAR_IEVENT (FEC_RECV_TASK_NO);
	return len;
}

/********************************************************************/
int mpc5xxx_fec_probe(struct device_d *dev)
{
        struct mpc5xxx_fec_platform_data *pdata = (struct mpc5xxx_fec_platform_data *)dev->platform_data;
        struct eth_device *edev;
	mpc5xxx_fec_priv *fec;

        edev = (struct eth_device *)malloc(sizeof(struct eth_device));
        dev->type_data = edev;
	fec = (mpc5xxx_fec_priv *)malloc(sizeof(*fec));
        edev->priv = fec;
        edev->dev  = dev;
	edev->open = mpc5xxx_fec_open,
	edev->init = mpc5xxx_fec_init,
	edev->send = mpc5xxx_fec_send,
	edev->recv = mpc5xxx_fec_recv,
	edev->halt = mpc5xxx_fec_halt,
	edev->get_mac_address = mpc5xxx_fec_get_hwaddr,
	edev->set_mac_address = mpc5xxx_fec_set_hwaddr,

	fec->eth = (ethernet_regs *)MPC5XXX_FEC;
	fec->tbdBase = (FEC_TBD *)FEC_BD_BASE;
	fec->rbdBase = (FEC_RBD *)(FEC_BD_BASE + FEC_TBD_NUM * sizeof(FEC_TBD));

	fec->xcv_type = pdata->xcv_type;

	sprintf(dev->name, "FEC ETHERNET");

	if (fec->xcv_type != SEVENWIRE) {
		fec->miiphy.read = fec5xxx_miiphy_read;
		fec->miiphy.write = fec5xxx_miiphy_write;
		fec->miiphy.address = CONFIG_PHY_ADDR;
		fec->miiphy.flags = pdata->xcv_type == MII10 ? MIIPHY_FORCE_10 : 0;

		miiphy_register(&fec->miiphy);
	}

	eth_register(edev);
	return 0;
}

/* MII-interface related functions */
/********************************************************************/
static int fec5xxx_miiphy_read(struct miiphy_device *mdev, uint8_t phyAddr, uint8_t regAddr, uint16_t * retVal)
{
	ethernet_regs *eth = (ethernet_regs *)MPC5XXX_FEC;
	uint32 reg;		/* convenient holder for the PHY register */
	uint32 phy;		/* convenient holder for the PHY */
	int timeout = 0xffff;

	/*
	 * reading from any PHY's register is done by properly
	 * programming the FEC's MII data register.
	 */
	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	eth->mii_data = (FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA | phy | reg);

	/*
	 * wait for the related interrupt
	 */
	while ((timeout--) && (!(eth->ievent & 0x00800000))) ;

	if (timeout == 0) {
#if (DEBUG & 0x2)
		printf ("Read MDIO failed...\n");
#endif
		return -1;
	}

	/*
	 * clear mii interrupt bit
	 */
	eth->ievent = 0x00800000;

	/*
	 * it's now safe to read the PHY's register
	 */
	*retVal = (uint16) eth->mii_data;

	return 0;
}

/********************************************************************/
static int fec5xxx_miiphy_write(struct miiphy_device *mdev, uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	ethernet_regs *eth = (ethernet_regs *)MPC5XXX_FEC;
	uint32 reg;		/* convenient holder for the PHY register */
	uint32 phy;		/* convenient holder for the PHY */
	int timeout = 0xffff;

	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	eth->mii_data = (FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR |
			FEC_MII_DATA_TA | phy | reg | data);

	/*
	 * wait for the MII interrupt
	 */
	while ((timeout--) && (!(eth->ievent & 0x00800000))) ;

	if (timeout == 0) {
#if (DEBUG & 0x2)
		printf ("Write MDIO failed...\n");
#endif
		return -1;
	}

	/*
	 * clear MII interrupt bit
	 */
	eth->ievent = 0x00800000;

	return 0;
}

static struct driver_d mpc5xxx_driver = {
        .name  = "fec_mpc5xxx",
        .probe = mpc5xxx_fec_probe,
        .type  = DEVICE_TYPE_ETHER,
};

static int mpc5xxx_fec_register(void)
{
        register_driver(&mpc5xxx_driver);
        return 0;
}

device_initcall(mpc5xxx_fec_register);



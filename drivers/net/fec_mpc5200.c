/*
 * (C) Copyright 2003-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This file is based on mpc4200fec.c,
 * (C) Copyright Motorola, Inc., 2000
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

#define DEBUG

#include <common.h>
//#include <asm/arch/mpc5xxx.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <miiphy.h>
#include <driver.h>
//#include <asm/arch/sdma.h>
//#include <asm/arch/fec.h>
#include <asm-ppc/arch-mpc5200/fec.h>
//#include <asm/arch/clocks.h>
#include <miiphy.h>
#include "fec_mpc5200.h"

#include <asm/io.h>

#ifdef CONFIG_ARCH_IMX27
#include <asm/arch/imx-regs.h>
#include <clock.h>
#include <asm/arch/clock.h>
#include <xfuncs.h>
#endif

extern int memory_display(char *addr, ulong offs, ulong nbytes, int size);

#define CONFIG_PHY_ADDR 1 /* FIXME */

typedef struct {
	uint8 data[1500];           /* actual data */
	int length;                 /* actual length */
	int used;                   /* buffer in use or not */
	uint8 head[16];             /* MAC header(6 + 6 + 2) + 2(aligned) */
} NBUF;

/*
 * MII-interface related functions
 */
static int fec5xxx_miiphy_read(struct miiphy_device *mdev, uint8_t phyAddr,
	uint8_t regAddr, uint16_t * retVal)
{
	struct eth_device *edev = mdev->edev;
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)edev->priv;

	uint32 reg;		/* convenient holder for the PHY register */
	uint32 phy;		/* convenient holder for the PHY */
	uint64_t start;

	/*
	 * reading from any PHY's register is done by properly
	 * programming the FEC's MII data register.
	 */
	writel(FEC_IEVENT_MII, &fec->eth->ievent);
	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA | phy | reg, &fec->eth->mii_data);

	/*
	 * wait for the related interrupt
	 */
	start = get_time_ns();
	while (!(readl(&fec->eth->ievent) & FEC_IEVENT_MII)) {
		if (is_timeout(start, MSECOND)) {
			printf("Read MDIO failed...\n");
			return -1;
		}
	}

	/*
	 * clear mii interrupt bit
	 */
	writel(FEC_IEVENT_MII, &fec->eth->ievent);

	/*
	 * it's now safe to read the PHY's register
	 */
	*retVal = readl(&fec->eth->mii_data);

	return 0;
}

static int fec5xxx_miiphy_write(struct miiphy_device *mdev, uint8_t phyAddr,
	uint8_t regAddr, uint16_t data)
{
	struct eth_device *edev = mdev->edev;
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)edev->priv;

	uint32 reg;		/* convenient holder for the PHY register */
	uint32 phy;		/* convenient holder for the PHY */
	uint64_t start;

	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR |
		FEC_MII_DATA_TA | phy | reg | data, &fec->eth->mii_data);

	/*
	 * wait for the MII interrupt
	 */
	start = get_time_ns();
	while (!(readl(&fec->eth->ievent) & FEC_IEVENT_MII)) {
		if (is_timeout(start, MSECOND)) {
			printf("Write MDIO failed...\n");
			return -1;
		}
	}

	/*
	 * clear MII interrupt bit
	 */
	writel(FEC_IEVENT_MII, &fec->eth->ievent);

	return 0;
}

#ifdef CONFIG_MPC5200
static int mpc5xxx_fec_rx_task_enable(mpc5xxx_fec_priv *fec)
{
	SDMA_TASK_ENABLE(FEC_RECV_TASK_NO);
	return 0;
}

static int mpc5xxx_fec_rx_task_disable(mpc5xxx_fec_priv *fec)
{
	SDMA_TASK_DISABLE(FEC_RECV_TASK_NO);
	return 0;
}

static int mpc5xxx_fec_tx_task_enable(mpc5xxx_fec_priv *fec)
{
	SDMA_TASK_ENABLE(FEC_XMIT_TASK_NO);
	return 0;
}

static int mpc5xxx_fec_tx_task_disable(mpc5xxx_fec_priv *fec)
{
	SDMA_TASK_DISABLE(FEC_XMIT_TASK_NO);
	return 0;
}
#endif

#ifdef CONFIG_ARCH_IMX27
static int mpc5xxx_fec_rx_task_enable(mpc5xxx_fec_priv *fec)
{
	writel(1 << 24, &fec->eth->r_des_active);
	return 0;
}

static int mpc5xxx_fec_rx_task_disable(mpc5xxx_fec_priv *fec)
{
	return 0;
}

static int mpc5xxx_fec_tx_task_enable(mpc5xxx_fec_priv *fec)
{
	writel(1 << 24, &fec->eth->x_des_active);
	return 0;
}

static int mpc5xxx_fec_tx_task_disable(mpc5xxx_fec_priv *fec)
{
	return 0;
}
#endif

/**
 * allocate and link buffers for the receive task
 * @param[in] fec all we know about the device yet
 * @param[in] count receive buffer count to be allocated
 * @param[in] size size of each receive buffer
 * @return 0 on success
 *
 * We need some alignment for the buffers. Thy must be
 * aligned to a specific boundary each. See RDB_ALIGNMENT
 */
static int mpc5xxx_fec_rbd_init(mpc5xxx_fec_priv *fec, int count, int size)
{
	int ix;
	static int once = 0;
	uint32 p;

	printf("%s\n", __FUNCTION__);

	size += RDB_ALIGNMENT;	/* enlarge the size for alignment */

	for (ix = 0; ix < count; ix++) {
		if (!once) {
			p = (uint32)xzalloc(size);
			p += RDB_ALIGNMENT - 1;
			p &= ~(RDB_ALIGNMENT - 1);
			writel(p, &fec->rbdBase[ix].dataPointer);
		}
		writew(FEC_RBD_EMPTY, &fec->rbdBase[ix].status);
		writew(0, &fec->rbdBase[ix].dataLength);
	}
	once ++;

	/*
	 * have the last RBD to close the ring
	 */
	writew(FEC_RBD_WRAP | readl(&fec->rbdBase[ix - 1].status), &fec->rbdBase[ix - 1].status);
	fec->rbdIndex = 0;

	return 0;
}

/**
 * initialize buffers for the transmit task
 * @param[in] fec all we know about the device yet
 *
 * Nothing special here to do. We ony using one bufffer
 * for all transmit operations.
 */
static void mpc5xxx_fec_tbd_init(mpc5xxx_fec_priv *fec)
{
	writew(FEC_TBD_WRAP, &fec->tbdBase[0].status);
}

/**
 * Mark the given read buffer descriptor as free
 * @param[in] last 1 if this is the last buffer descriptor in the chain, else 0
 * @param[in] pRbd buffer descriptor to mark free again
 */
static void mpc5xxx_fec_rbd_clean(int last, FEC_RBD *pRbd)
{
	/*
	 * Reset buffer descriptor as empty
	 */
	if (last)
		writew(FEC_RBD_WRAP | FEC_RBD_EMPTY, &pRbd->status);
	else
		writew(FEC_RBD_EMPTY, &pRbd->status);
	/*
	 * no data in it
	 */
	writew(0, &pRbd->dataLength);
}

static int mpc5xxx_fec_get_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	/* no eeprom */
	return -1;
}

static int mpc5xxx_fec_set_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
//#define WTF_IS_THIS
#ifdef WTF_IS_THIS
	uint32 crc = 0xffffffff;	/* initial value */
	uint8 currByte;			/* byte for which to compute the CRC */
	int byte;			/* loop - counter */
	int bit;			/* loop - counter */

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
#else
	writel(0, &fec->eth->iaddr1);
	writel(0, &fec->eth->iaddr2);
	writel(0, &fec->eth->gaddr1);
	writel(0, &fec->eth->gaddr2);
#endif
	/*
	 * Set physical address
	 */
	writel((mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3], &fec->eth->paddr1);
	writel((mac[4] << 24) + (mac[5] << 16) + 0x8808, &fec->eth->paddr2);

        return 0;
}

static int mpc5xxx_fec_init(struct eth_device *dev)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
#ifdef CONFIG_MPC5200
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5XXX_SDMA;
#endif
	debug("mpc5xxx_fec_init... Begin\n");

	/*
	 * Initialize RxBD/TxBD rings
	 */
	mpc5xxx_fec_rbd_init(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);
	mpc5xxx_fec_tbd_init(fec);

	/*
	 * Clear FEC-Lite interrupt event register(IEVENT)
	 */
	writel(0xffffffff, &fec->eth->ievent);

	/*
	 * Set interrupt mask register
	 */
	writel(0x00000000, &fec->eth->imask);

	/*
	 * Set FEC-Lite receive control register(R_CNTRL):
	 */
	if (fec->xcv_type == SEVENWIRE) {
		/*
		 * Frame length=1518; 7-wire mode
		 */
		writel(0x05ee0020, &fec->eth->r_cntrl);	/* FIXME 0x05ee0000 */
	} else {
		/*
		 * Frame length=1518; MII mode;
		 */
		writel(0x05ee0024, &fec->eth->r_cntrl);	/* FIXME 0x05ee0004 */

		/*
		 * Set MII_SPEED = (1/(mii_speed * 2)) * System Clock
		 * and do not drop the Preamble.
		 */
#ifdef CONFIG_MPC5200
		writel(((get_ipb_clock() >> 20) / 5) << 1, &fec->eth->mii_speed);	/* No MII for 7-wire mode */
#endif
#ifdef CONFIG_ARCH_IMX27
		writel(((imx_get_ahbclk() >> 20) / 5) << 1, &fec->eth->mii_speed);	/* No MII for 7-wire mode */
#endif
	}

	/*
	 * Set Opcode/Pause Duration Register
	 */
	writel(0x00010020, &fec->eth->op_pause);	/* FIXME 0xffff0020; */

#ifdef CONFIG_MPC5200
	/*
	 * Set Rx FIFO alarm and granularity value
	 */
	writel(0x0c000000 | (readl(&fec->eth->rfifo_cntrl) & ~0x0f000000)), &fec->eth->rfifo_cntrl);
	writel(0x0000030c, &fec->eth->rfifo_alarm);

	if (readl(&fec->eth->rfifo_status) & 0x00700000 ) {
		debug("mpc5xxx_fec_init() RFIFO error\n");
	}

	/*
	 * Set Tx FIFO granularity value
	 */
	writel(0x0c000000 | (readl(&fec->eth->tfifo_cntrl)& ~0x0f000000), &fec->eth->tfifo_cntrl);

	debug("tfifo_status: 0x%08x\n", readl(&fec->eth->tfifo_status));
	debug("tfifo_alarm: 0x%08x\n", readl(&fec->eth->tfifo_alarm));

	/*
	 * Set transmit fifo watermark register(X_WMRK), default = 64
	 */
	writel(0x00000080, &fec->eth->tfifo_alarm);

	/*
	 * Turn ON cheater FSM: ????
	 */
	writel(0x03000000, &fec->eth->xmit_fsm);
#endif

	writel(0x2, &fec->eth->x_wmrk);

	/*
	 * Set multicast address filter
	 */
	writel(0x00000000, &fec->eth->gaddr1);
	writel(0x00000000, &fec->eth->gaddr2);

#ifdef CONFIG_MPC5200
	/*
	 * Set priority of different initiators
	 */
	sdma->IPR0 = 7;		/* always */
	sdma->IPR3 = 6;		/* Eth RX */
	sdma->IPR4 = 5;		/* Eth Tx */
#endif

#ifdef CONFIG_ARCH_IMX27
	writel(2048-16, &fec->eth->emrbr);
#endif
	/*
	 * Clear SmartDMA task interrupt pending bits
	 */
//	SDMA_CLEAR_IEVENT(FEC_RECV_TASK_NO);

	/*
	 * Initialize SmartDMA parameters stored in SRAM
	 */
#ifdef CONFIG_MPC5200
	*(volatile int *)FEC_TBD_BASE = (int)fec->tbdBase;
	*(volatile int *)FEC_RBD_BASE = (int)fec->rbdBase;
	*(volatile int *)FEC_TBD_NEXT = (int)fec->tbdBase;
	*(volatile int *)FEC_RBD_NEXT = (int)fec->rbdBase;
#endif
	debug("mpc5xxx_fec_init... Done \n");

	if (fec->xcv_type != SEVENWIRE)
		miiphy_restart_aneg(&fec->miiphy);

	return 0;
}

static int mpc5xxx_fec_open(struct eth_device *edev)
{
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)edev->priv;

	printf("%s\n", __FUNCTION__);

#if defined(CONFIG_MPC5200)
	struct mpc5xxx_sdma *sdma = (struct mpc5xxx_sdma *)MPC5XXX_SDMA;
	/*
	 * Turn off COMM bus prefetch in the MGT5200 BestComm. It doesn't
	 * work w/ the current receive task.
	 */
	 sdma->PtdCntrl |= 0x00000001;
#endif

#if 0
	writel(0x00000000, &fec->eth->x_cntrl);	/* half-duplex, heartbeat disabled */
#else
	writel(1 << 2, &fec->eth->x_cntrl);	/* full-duplex, heartbeat disabled */
#endif

	fec->rbdIndex = 0;

	/*
	 * Enable FEC-Lite controller
	 */
#if defined(CONFIG_MPC5200)
	writel(0x00000006 | readl(&fec->eth->ecntrl), &fec->eth->ecntrl);
#endif
#if defined(CONFIG_ARCH_IMX27)
	writel(0x00000002 | readl(&fec->eth->ecntrl), &fec->eth->ecntrl);
#endif
	/*
	 * Enable SmartDMA receive task
	 */
	mpc5xxx_fec_rx_task_enable(fec);

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

	/*
	 * issue graceful stop command to the FEC transmitter if necessary
	 */
	writel(0x00000001 | readl(&fec->eth->x_cntrl), &fec->eth->x_cntrl);

	/*
	 * wait for graceful stop to register
	 */
	while ((counter--) && (!(readl(&fec->eth->ievent) & FEC_IEVENT_GRA)))
		;

	/*
	 * Disable SmartDMA tasks
	 */
	mpc5xxx_fec_tx_task_disable(fec);
	mpc5xxx_fec_rx_task_disable(fec);

#ifdef CONFIG_MPC5200
	/*
	 * Turn on COMM bus prefetch in the MGT5200 BestComm after we're
	 * done. It doesn't work w/ the current receive task.
	 */
	 sdma->PtdCntrl &= ~0x00000001;
#endif

	/*
	 * Disable the Ethernet Controller
	 */
	writel(0, &fec->eth->ecntrl);

#ifdef CONFIG_MPC5200
	/*
	 * Clear FIFO status registers
	 */
	writel(0x00700000 & readl(&fec->eth->rfifo_status), &fec->eth->rfifo_status);
	writel(0x00700000 & readl(&fec->eth->tfifo_status), &fec->eth->tfifo_status);
#endif

//	writel(0x01000000, &fec->eth->reset_cntrl);

	debug("Ethernet task stopped\n");
}

#ifdef DEBUG_FIFO
static void tfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
	if ((readl(&fec->eth->tfifo_lrf_ptr) != readl(&fec->eth->tfifo_lwf_ptr))
		|| (readl(&fec->eth->tfifo_rdptr) != readl(&fec->eth->tfifo_wrptr))) {

		printf("ecntrl:   0x%08x\n", readl(&fec->eth->ecntrl));
		printf("ievent:   0x%08x\n", readl(&fec->eth->ievent));
		printf("x_status: 0x%08x\n", readl(&fec->eth->x_status));
		printf("tfifo: status  0x%08x\n", readl(&fec->eth->tfifo_status));

		printf("       control 0x%08x\n", readl(&fec->eth->tfifo_cntrl));
		printf("       lrfp    0x%08x\n", readl(&fec->eth->tfifo_lrf_ptr));
		printf("       lwfp    0x%08x\n", readl(&fec->eth->tfifo_lwf_ptr));
		printf("       alarm   0x%08x\n", readl(&fec->eth->tfifo_alarm));
		printf("       readptr 0x%08x\n", readl(&fec->eth->tfifo_rdptr));
		printf("       writptr 0x%08x\n", readl(&fec->eth->tfifo_wrptr));
	}
}

static void rfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
	if ((readl(&fec->eth->rfifo_lrf_ptr) != readl(&fec->eth->rfifo_lwf_ptr))
		|| (readl(&fec->eth->rfifo_rdptr) != readl(&fec->eth->rfifo_wrptr))) {

		printf("ecntrl:   0x%08x\n", readl(&fec->eth->ecntrl));
		printf("ievent:   0x%08x\n", readl(&fec->eth->ievent));
		printf("x_status: 0x%08x\n", readl(&fec->eth->x_status));
		printf("rfifo: status  0x%08x\n", readl(&fec->eth->rfifo_status));

		printf("       control 0x%08x\n", readl(&fec->eth->rfifo_cntrl));
		printf("       lrfp    0x%08x\n", readl(&fec->eth->rfifo_lrf_ptr));
		printf("       lwfp    0x%08x\n", readl(&fec->eth->rfifo_lwf_ptr));
		printf("       alarm   0x%08x\n", readl(&fec->eth->rfifo_alarm));
		printf("       readptr 0x%08x\n", readl(&fec->eth->rfifo_rdptr));
		printf("       writptr 0x%08x\n", readl(&fec->eth->rfifo_wrptr));
	}
}
#else
static void tfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
}

static void __maybe_unused rfifo_print(char *devname, mpc5xxx_fec_priv *fec)
{
}
#endif /* DEBUG_FIFO */

static int mpc5xxx_fec_send(struct eth_device *dev, void *eth_data,
		int data_length)
{
	/*
	 * This routine transmits one frame.  This routine only accepts
	 * 6-byte Ethernet addresses.
	 */
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	volatile FEC_TBD *pTbd;

//	printf("%s length=%d data=0x%08x\n", __FUNCTION__, data_length, eth_data);

#ifdef DEBUG_FIFO
	debug_fifo("tbd status: 0x%04x\n", fec->tbdBase[0].status);
	tfifo_print(dev->name, fec);
#endif
	/*
	 * Check for valid length of data.
	 */
	if ((data_length > 1500) || (data_length <= 0)) {
		return -1;
	}

	/*
	 * Get the first TxBD to send the mac header
	 */
	pTbd = &fec->tbdBase[0];
	pTbd->dataLength = data_length;
	pTbd->dataPointer = (uint32)eth_data;
	pTbd->status = FEC_TBD_LAST | FEC_TBD_TC | FEC_TBD_READY | FEC_TBD_WRAP;

	/*
	 * Enable SmartDMA transmit task
	 */
	mpc5xxx_fec_tx_task_enable(fec);

	/*
	 * wait until frame is sent .
	 */
	while (pTbd->status & FEC_TBD_READY) {
		/* FIXME: Timeout */
	}

	return 0;
}

static int mpc5xxx_fec_recv(struct eth_device *dev)
{
	/*
	 * This command pulls one frame from the card
	 */
	mpc5xxx_fec_priv *fec = (mpc5xxx_fec_priv *)dev->priv;
	FEC_RBD *pRbd = &fec->rbdBase[fec->rbdIndex];
	unsigned long ievent;
	int frame_length, len = 0;
	NBUF *frame;
	uchar buff[FEC_MAX_PKT_SIZE];

//	printf("%s\n", __FUNCTION__);
	/*
	 * Check if any critical events have happened
	 */
	ievent = readl(&fec->eth->ievent);
	writel(ievent, &fec->eth->ievent);
	if (ievent & (FEC_IEVENT_BABT | FEC_IEVENT_XFIFO_ERROR |
				FEC_IEVENT_RFIFO_ERROR)) {
		/* BABT, Rx/Tx FIFO errors */
		mpc5xxx_fec_halt(dev);
		mpc5xxx_fec_init(dev);
		printf("some error: 0x%08x\n", ievent);
		return 0;
	}
	if (ievent & FEC_IEVENT_HBERR) {
		/* Heartbeat error */
		writel(0x00000001 | readl(&fec->eth->x_cntrl), &fec->eth->x_cntrl);
	}
	if (ievent & FEC_IEVENT_GRA) {
		/* Graceful stop complete */
		if (readl(&fec->eth->x_cntrl) & 0x00000001) {
			mpc5xxx_fec_halt(dev);
			writel(~0x00000001 & readl(&fec->eth->x_cntrl), &fec->eth->x_cntrl);
			mpc5xxx_fec_init(dev);
		}
	}

	if (!(pRbd->status & FEC_RBD_EMPTY)) {
		if ((pRbd->status & FEC_RBD_LAST) && !(pRbd->status & FEC_RBD_ERR) &&
			((pRbd->dataLength - 4) > 14)) {
			printf("read from %d (0x%08x) rbd=0x%08x", fec->rbdIndex, pRbd->dataPointer, pRbd);

			/*
			 * Get buffer address and size
			 */
			frame = (NBUF *)pRbd->dataPointer;
			frame_length = pRbd->dataLength - 4;
			printf(" len=%d\n", frame_length);
#define DEBUG_RX_HEADER
#ifdef DEBUG_RX_HEADER
			{
				printf("recv data hdr:\n");
				memory_display(frame->data, 0, frame_length, 1);
			}
#endif
			/*
			 *  Fill the buffer and pass it to upper layers
			 */
#ifdef CONFIG_MPC5200
			memcpy(buff, frame->head, 14);
			memcpy(buff + 14, frame->data, frame_length);
#endif
#ifdef CONFIG_ARCH_IMX27
			memcpy(buff, frame->data, frame_length);
#endif
			NetReceive(buff, frame_length);
			len = frame_length;
		} else {
			if (pRbd->status & FEC_RBD_ERR) {
				printf("error frame: 0x%08x 0x%08x\n", pRbd, pRbd->status);
			}
		}
		/*
		 * free the current buffer, restart the engine and move
		 * forward to the next buffer
		 */
		mpc5xxx_fec_rbd_clean(fec->rbdIndex == (FEC_RBD_NUM - 1) ? 1 : 0, pRbd);
		mpc5xxx_fec_rx_task_enable(fec);
		fec->rbdIndex = (fec->rbdIndex + 1) % FEC_RBD_NUM;

	}

//	SDMA_CLEAR_IEVENT (FEC_RECV_TASK_NO);
	return len;
}

int mpc5xxx_fec_probe(struct device_d *dev)
{
        struct mpc5xxx_fec_platform_data *pdata = (struct mpc5xxx_fec_platform_data *)dev->platform_data;
        struct eth_device *edev;
	mpc5xxx_fec_priv *fec;

	printf("%s\n", __FUNCTION__);

#ifdef CONFIG_ARCH_IMX27
	PCCR0 |= PCCR0_FEC_EN;
#endif
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
	edev->get_ethaddr = mpc5xxx_fec_get_hwaddr,
	edev->set_ethaddr = mpc5xxx_fec_set_hwaddr,

	fec->eth = (ethernet_regs *)dev->map_base;

#ifdef CONFIG_MPC5200
	fec->tbdBase = (FEC_TBD *)FEC_BD_BASE;
	fec->rbdBase = (FEC_RBD *)(FEC_BD_BASE + FEC_TBD_NUM * sizeof(FEC_TBD));
#endif
#ifdef CONFIG_ARCH_IMX27
	/* Reset chip. FIXME: shouldn't it be done for mpc5200 aswell? */
	writel(1, &fec->eth->ecntrl);
	while(readl(&fec->eth->ecntrl) & 1) {
		udelay(10);
	}

	{
		unsigned long base;
	
		base = ((unsigned long)xzalloc(sizeof(FEC_TBD) + 32) + 31) & ~0x1f;
		fec->tbdBase = (FEC_TBD *)base;
		writel(fec->tbdBase, &fec->eth->etdsr);
		base = ((unsigned long)xzalloc(FEC_RBD_NUM * sizeof(FEC_RBD) + 32) + 31) & ~0x1f;
		fec->rbdBase = (FEC_RBD *)base;
		writel(fec->rbdBase, &fec->eth->erdsr);
	}
#endif

	fec->xcv_type = pdata->xcv_type;

	sprintf(dev->name, "FEC ETHERNET");
#ifdef CONFIG_MPC5200
	loadtask(0, 2);
#endif
	if (fec->xcv_type != SEVENWIRE) {
		fec->miiphy.read = fec5xxx_miiphy_read;
		fec->miiphy.write = fec5xxx_miiphy_write;
		fec->miiphy.address = CONFIG_PHY_ADDR;
		fec->miiphy.flags = pdata->xcv_type == MII10 ? MIIPHY_FORCE_10 : 0;
		fec->miiphy.edev = edev;

		miiphy_register(&fec->miiphy);
	}

	eth_register(edev);
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

/**
 * @file
 * @brief Network driver for FreeScale's FEC implementation.
 * This type of hardware can be found on MPC52xx and i.MX27 CPUs
 */

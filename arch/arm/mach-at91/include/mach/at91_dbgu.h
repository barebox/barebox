/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2005 Ivan Kokshaysky */
/* SPDX-FileCopyrightText: SAN People */

/*
 * arch/arm/mach-at91/include/mach/at91_dbgu.h
 *
 * Debug Unit (DBGU) - System peripherals registers.
 * Based on AT91RM9200 datasheet revision E and SAMA5D3 datasheet revision B.
 */

#ifndef AT91_DBGU_H
#define AT91_DBGU_H

#define AT91_DBGU_CR		(0x00)	/* Control Register */
#define 	AT91_DBGU_RSTRX		(1 << 2)		/* Reset Receiver */
#define 	AT91_DBGU_RSTTX		(1 << 3)		/* Reset Transmitter */
#define 	AT91_DBGU_RXEN		(1 << 4)		/* Receiver Enable */
#define 	AT91_DBGU_RXDIS		(1 << 5)		/* Receiver Disable */
#define 	AT91_DBGU_TXEN		(1 << 6)		/* Transmitter Enable */
#define 	AT91_DBGU_TXDIS		(1 << 7)		/* Transmitter Disable */
#define 	AT91_DBGU_RSTSTA	(1 << 8)		/* Reset Status Bits */
#define AT91_DBGU_MR		(0x04)	/* Mode Register */
#define		AT91_DBGU_NBSTOP_1BIT		(0 << 12) /* 1 stop bit */
#define		AT91_DBGU_NBSTOP_1_5BIT	(1 << 12) /* 1.5 stop bits */
#define		AT91_DBGU_NBSTOP_2BIT		(2 << 12) /* 2 stop bits */

#define		AT91_DBGU_CHRL_5BIT	(0 << 6) /* 5 bit character length */
#define		AT91_DBGU_CHRL_6BIT	(1 << 6) /* 6 bit character length */
#define		AT91_DBGU_CHRL_7BIT	(2 << 6) /* 7 bit character length */
#define		AT91_DBGU_CHRL_8BIT	(3 << 6) /* 8 bit character length */

#define 	AT91_DBGU_PAR_EVEN	(0 << 9)		/* Even Parity */
#define 	AT91_DBGU_PAR_ODD	(1 << 9)		/* Odd Parity */
#define 	AT91_DBGU_PAR_SPACE	(2 << 9)		/* Space: Force Parity to 0 */
#define 	AT91_DBGU_PAR_MARK	(3 << 9)		/* Mark: Force Parity to 1 */
#define 	AT91_DBGU_PAR_NONE	(4 << 9)		/* No Parity */

#define 	AT91_DBGU_CHMODE_NORMAL	(0 << 14) /* Normal mode */
#define 	AT91_DBGU_CHMODE_AUTO		(1 << 14) /* Automatic Echo */
#define 	AT91_DBGU_CHMODE_LOCAL		(2 << 14) /* Local Loopback */
#define 	AT91_DBGU_CHMODE_REMOTE	(3 << 14) /* Remote Loopback */
#define AT91_DBGU_IER		(0x08)	/* Interrupt Enable Register */
#define		AT91_DBGU_TXRDY		(1 << 1)		/* Transmitter Ready */
#define		AT91_DBGU_TXEMPTY	(1 << 9)		/* Transmitter Empty */
#define AT91_DBGU_IDR		(0x0c)	/* Interrupt Disable Register */
#define AT91_DBGU_IMR		(0x10)	/* Interrupt Mask Register */
#define AT91_DBGU_SR		(0x14)	/* Status Register */
#define AT91_DBGU_RHR		(0x18)	/* Receiver Holding Register */
#define AT91_DBGU_THR		(0x1c)	/* Transmitter Holding Register */
#define AT91_DBGU_BRGR		(0x20)	/* Baud Rate Generator Register */

#define AT91_DBGU_CIDR		(0x40)	/* Chip ID Register */
#define AT91_DBGU_EXID		(0x44)	/* Chip ID Extension Register */
#define AT91_DBGU_FNR		(0x48)	/* Force NTRST Register [SAM9 only] */
#define		AT91_DBGU_FNTRST	(1 << 0)		/* Force NTRST */


/*
 * Some AT91 parts that don't have full DEBUG units still support the ID
 * and extensions register.
 */
#define		AT91_CIDR_VERSION	(0x1f << 0)		/* Version of the Device */
#define		AT91_CIDR_EPROC		(7    << 5)		/* Embedded Processor */
#define		AT91_CIDR_NVPSIZ	(0xf  << 8)		/* Nonvolatile Program Memory Size */
#define		AT91_CIDR_NVPSIZ2	(0xf  << 12)		/* Second Nonvolatile Program Memory Size */
#define		AT91_CIDR_SRAMSIZ	(0xf  << 16)		/* Internal SRAM Size */
#define			AT91_CIDR_SRAMSIZ_1K	(1 << 16)
#define			AT91_CIDR_SRAMSIZ_2K	(2 << 16)
#define			AT91_CIDR_SRAMSIZ_112K	(4 << 16)
#define			AT91_CIDR_SRAMSIZ_4K	(5 << 16)
#define			AT91_CIDR_SRAMSIZ_80K	(6 << 16)
#define			AT91_CIDR_SRAMSIZ_160K	(7 << 16)
#define			AT91_CIDR_SRAMSIZ_8K	(8 << 16)
#define			AT91_CIDR_SRAMSIZ_16K	(9 << 16)
#define			AT91_CIDR_SRAMSIZ_32K	(10 << 16)
#define			AT91_CIDR_SRAMSIZ_64K	(11 << 16)
#define			AT91_CIDR_SRAMSIZ_128K	(12 << 16)
#define			AT91_CIDR_SRAMSIZ_256K	(13 << 16)
#define			AT91_CIDR_SRAMSIZ_96K	(14 << 16)
#define			AT91_CIDR_SRAMSIZ_512K	(15 << 16)
#define		AT91_CIDR_ARCH		(0xff << 20)		/* Architecture Identifier */
#define		AT91_CIDR_NVPTYP	(7    << 28)		/* Nonvolatile Program Memory Type */
#define		AT91_CIDR_EXT		(1    << 31)		/* Extension Flag */

#ifndef __ASSEMBLY__

#include <asm/io.h>
static inline void at91_dbgu_setup_ll(void __iomem *dbgu_base,
				      unsigned mck,
				      unsigned baudrate)
{
	u32 brgr = mck / (baudrate * 16);

	if ((mck / (baudrate * 16)) % 10 >= 5)
		brgr++;

	writel(~0, dbgu_base + AT91_DBGU_IDR);

	writel(AT91_DBGU_RSTRX
	       | AT91_DBGU_RSTTX
	       | AT91_DBGU_RXDIS
	       | AT91_DBGU_TXDIS,
	       dbgu_base + AT91_DBGU_CR);

	writel(brgr, dbgu_base + AT91_DBGU_BRGR);

	writel(AT91_DBGU_PAR_NONE
	       | AT91_DBGU_CHMODE_NORMAL
	       | AT91_DBGU_CHRL_8BIT
	       | AT91_DBGU_NBSTOP_1BIT,
	       dbgu_base + AT91_DBGU_MR);

	writel(AT91_DBGU_RXEN | AT91_DBGU_TXEN, dbgu_base + AT91_DBGU_CR);
}

#endif

#endif

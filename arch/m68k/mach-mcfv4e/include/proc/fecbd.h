/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Provide a simple buffer management driver
 */

#ifndef _FECBD_H_
#define _FECBD_H_

/********************************************************************/

#define Rx  1
#define Tx  0

/*
 * Buffer sizes in bytes
 */
#ifndef RX_BUF_SZ
#define RX_BUF_SZ  NBUF_SZ
#endif
#ifndef TX_BUF_SZ
#define TX_BUF_SZ  NBUF_SZ
#endif

/*
 * Number of Rx and Tx Buffers and Buffer Descriptors
 */
#ifndef NRXBD
#define NRXBD   10
#endif
#ifndef NTXBD
#define NTXBD   4
#endif

/*
 * Buffer Descriptor Format
 */
typedef struct
{
    uint16_t status;  /* control and status */
    uint16_t length;  /* transfer length */
    uint8_t  *data;   /* buffer address */
} FECBD;

/*
 * Bit level definitions for status field of buffer descriptors
 */
#define TX_BD_R         0x8000
#define TX_BD_TO1       0x4000
#define TX_BD_W         0x2000
#define TX_BD_TO2       0x1000
#define TX_BD_INTERRUPT 0x1000  /* MCF547x/8x Only */
#define TX_BD_L         0x0800
#define TX_BD_TC        0x0400
#define TX_BD_DEF       0x0200  /* MCF5272 Only */
#define TX_BD_ABC       0x0200
#define TX_BD_HB        0x0100  /* MCF5272 Only */
#define TX_BD_LC        0x0080  /* MCF5272 Only */
#define TX_BD_RL        0x0040  /* MCF5272 Only */
#define TX_BD_UN        0x0002  /* MCF5272 Only */
#define TX_BD_CSL       0x0001  /* MCF5272 Only */

#define RX_BD_E         0x8000
#define RX_BD_R01       0x4000
#define RX_BD_W         0x2000
#define RX_BD_R02       0x1000
#define RX_BD_INTERRUPT 0x1000  /* MCF547x/8x Only */
#define RX_BD_L         0x0800
#define RX_BD_M         0x0100
#define RX_BD_BC        0x0080
#define RX_BD_MC        0x0040
#define RX_BD_LG        0x0020
#define RX_BD_NO        0x0010
#define RX_BD_CR        0x0004
#define RX_BD_OV        0x0002
#define RX_BD_TR        0x0001
#define RX_BD_ERROR     (RX_BD_NO | RX_BD_CR | RX_BD_OV | RX_BD_TR)

/*
 * Functions provided in fec_bd.c
 */
void
fecbd_init(uint8_t);

uint32_t
fecbd_get_start(uint8_t, uint8_t);

FECBD *
fecbd_rx_alloc(uint8_t);

FECBD *
fecbd_tx_alloc(uint8_t);

FECBD *
fecbd_tx_free(uint8_t);

#endif /* _FECBD_H_ */

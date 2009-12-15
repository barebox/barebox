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
 *  Declaration for the Fast Ethernet Controller (FEC)
 */
#ifndef _FEC_H_
#define _FEC_H_

// FIXME
#define NIF void

/********************************************************************/
/* MII Speed Settings */
#define FEC_MII_10BASE_T        0
#define FEC_MII_100BASE_TX      1

/* MII Duplex Settings */
#define FEC_MII_HALF_DUPLEX     0
#define FEC_MII_FULL_DUPLEX     1

/* Timeout for MII communications */
#define FEC_MII_TIMEOUT         0x10000

/* External Interface Modes */
#define FEC_MODE_7WIRE          0
#define FEC_MODE_MII            1
#define FEC_MODE_LOOPBACK       2   /* Internal Loopback */

/*
 * FEC Event Log
 */
typedef struct {
    int total;      /* total count of errors   */
    int hberr;      /* heartbeat error         */
    int babr;       /* babbling receiver       */
    int babt;       /* babbling transmitter    */
    int gra;        /* graceful stop complete  */
    int txf;        /* transmit frame          */
    int mii;        /* MII                     */
    int lc;         /* late collision          */
    int rl;         /* collision retry limit   */
    int xfun;       /* transmit FIFO underrrun */
    int xferr;      /* transmit FIFO error     */
    int rferr;      /* receive FIFO error      */
    int dtxf;       /* DMA transmit frame      */
    int drxf;       /* DMA receive frame       */
    int rfsw_inv;   /* Invalid bit in RFSW     */
    int rfsw_l;     /* RFSW Last in Frame      */
    int rfsw_m;     /* RFSW Miss               */
    int rfsw_bc;    /* RFSW Broadcast          */
    int rfsw_mc;    /* RFSW Multicast          */
    int rfsw_lg;    /* RFSW Length Violation   */
    int rfsw_no;    /* RFSW Non-octet          */
    int rfsw_cr;    /* RFSW Bad CRC            */
    int rfsw_ov;    /* RFSW Overflow           */
    int rfsw_tr;    /* RFSW Truncated          */
} FEC_EVENT_LOG;


int fec_mii_write(uint8_t , uint8_t , uint8_t , uint16_t );
int fec_mii_read(uint8_t , uint8_t , uint8_t , uint16_t *x);
void fec_mii_init(uint8_t, uint32_t);

void fec_mib_init(uint8_t);
void fec_mib_dump(uint8_t);

void fec_log_init(uint8_t);
void fec_log_dump(uint8_t);

void fec_debug_dump(uint8_t);
void fec_duplex (uint8_t, uint8_t);

uint8_t fec_hash_address(const uint8_t *);
void fec_set_address (uint8_t ch, const uint8_t *);

void fec_reset (uint8_t);
void fec_init (uint8_t, uint8_t, const uint8_t *);

void fec_rx_start(uint8_t, int8_t *);
void fec_rx_restart(uint8_t);
void fec_rx_stop (uint8_t);

NBUF * fec_rx_frame(uint8_t ch, NIF *nif);

void fec0_rx_frame(void);
void fec1_rx_frame(void);

void fec_tx_start(uint8_t, int8_t *);
void fec_tx_restart(uint8_t);
void fec_tx_stop (uint8_t);

void fec0_tx_frame(void);
void fec1_tx_frame(void);

int  fec_send(uint8_t, NIF *, uint8_t *, uint8_t *, uint16_t , NBUF *);
int  fec0_send(NIF *, uint8_t *, uint8_t *, uint16_t , NBUF *);
int  fec1_send(NIF *, uint8_t *, uint8_t *, uint16_t , NBUF *);

void fec_irq_enable(uint8_t, uint8_t, uint8_t);
void fec_irq_disable(uint8_t);

void fec_interrupt_handler(uint8_t);
int  fec0_interrupt_handler(void *, void *);
int  fec1_interrupt_handler(void *, void *);

void fec_eth_setup(uint8_t, uint8_t, uint8_t, uint8_t, const uint8_t *);

void fec_eth_reset(uint8_t);

void fec_eth_stop(uint8_t);

#endif /* _FEC_H_ */

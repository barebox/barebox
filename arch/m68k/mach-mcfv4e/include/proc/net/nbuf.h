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
 *  Definitions for network buffer management
 */

#ifndef _MCFV4E_NBUF_H_
#define _MCFV4E_NBUF_H_

/*
 * Include the Queue structure definitions
 */
#include "queue.h"

/*
 * Number of network buffers to use
 */
#define NBUF_MAX    30

/*
 * Size of each buffer in bytes
 */
#ifndef NBUF_SZ
#define NBUF_SZ     1520
#endif

/*
 * Defines to identify all the buffer queues
 *  - FREE must always be defined as 0
 */
#define NBUF_FREE       0   /* available buffers */
#define NBUF_TX_RING    1   /* buffers in the Tx BD ring */
#define NBUF_RX_RING    2   /* buffers in the Rx BD ring */
#define NBUF_SCRATCH    3   /* misc */
#define NBUF_MAXQ       4   /* total number of queueus */

/*
 * Buffer Descriptor Format
 *
 * Fields:
 * next     Pointer to next node in the queue
 * data     Pointer to the data buffer
 * offset   Index into buffer
 * length   Remaining bytes in buffer from (data + offset)
 */
typedef struct
{
    QNODE node;
    uint8_t *data;
    uint16_t offset;
    uint16_t length;
} NBUF;

/*
 * Functions to manipulate the network buffers.
 */
int nbuf_init(void);
void nbuf_flush(void);

NBUF * nbuf_alloc (void);
void nbuf_free(NBUF *);

NBUF *nbuf_remove(int);
void nbuf_add(int, NBUF *);

void nbuf_reset(void);
void nbuf_debug_dump(void);


#endif  /* _MCFV4E_NBUF_H_ */

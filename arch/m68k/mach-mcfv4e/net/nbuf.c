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
 *  Implementation of network buffer scheme.
 * @todo Obsolete this file
 */
#include <common.h>
#include <malloc.h>
#include <linux/types.h>

#include <proc/net/queue.h>
#include <proc/net/net.h>

#include <mach/mcf54xx-regs.h>


#define ASSERT(x)

/**
 * Queues used for network buffer storage
 */
QUEUE nbuf_queue[NBUF_MAXQ];

/*
 * Some devices require line-aligned buffers.  In order to accomplish
 * this, the nbuf data is over-allocated and adjusted.  The following
 * array keeps track of the original data pointer returned by malloc
 */
ADDRESS unaligned_buffers[NBUF_MAX];

/**
 * Initialize all the network buffer queues
 *
 * Return Value:
 *  0 success
 *  1 failure
 */
int
nbuf_init(void)
{
    int i;
    NBUF *nbuf;

    for (i=0; i<NBUF_MAXQ; ++i)
    {
        /* Initialize all the queues */
        queue_init(&nbuf_queue[i]);
    }

    #ifdef CONFIG_DRIVER_NET_MCF54XX_DEBUG
        printf("Creating %d net buffers of %d bytes\n",NBUF_MAX,NBUF_SZ);
    #endif

    for (i=0; i<NBUF_MAX; ++i)
    {
        /* Allocate memory for the network buffer structure */
        nbuf = (NBUF *)malloc(sizeof(NBUF));
        if (!nbuf)
        {
            ASSERT(nbuf);
            return 1;
        }

        /* Allocate memory for the actual data */
        unaligned_buffers[i] = (ADDRESS)malloc(NBUF_SZ + 16);
        nbuf->data = (uint8_t *)((uint32_t)(unaligned_buffers[i] + 15) & 0xFFFFFFF0);
        if (!nbuf->data)
        {
            ASSERT(nbuf->data);
            return 1;
        }

        /* Initialize the network buffer */
        nbuf->offset = 0;
        nbuf->length = 0;

        /* Add the network buffer to the free list */
        queue_add(&nbuf_queue[NBUF_FREE], (QNODE *)nbuf);
    }

    #ifdef CONFIG_DRIVER_NET_MCF54XX_DEBUG
        printf("NBUF allocation complete\n");
        nbuf_debug_dump();
    #endif

    return 0;
}
/**
 * Return all the allocated memory to the heap
 */
void
nbuf_flush(void)
{
    NBUF *nbuf;
    int i, level = asm_set_ipl(7);
    int n = 0;

    for (i=0; i<NBUF_MAX; ++i)
        free((uint8_t*)unaligned_buffers[i]);

    for (i=0; i<NBUF_MAXQ; ++i)
    {
        while ((nbuf = (NBUF *)queue_remove(&nbuf_queue[i])) != NULL)
        {
            free(nbuf);
            ++n;
        }
    }
    ASSERT(n == NBUF_MAX);
    asm_set_ipl(level);
}
/**
 * Allocate a network buffer from the free list
 *
 * Return Value:
 *  Pointer to a free network buffer
 *  NULL if none are available
 */
NBUF *
nbuf_alloc(void)
{
    NBUF *nbuf;
    int level = asm_set_ipl(7);

    nbuf = (NBUF *)queue_remove(&nbuf_queue[NBUF_FREE]);
    asm_set_ipl(level);
    return nbuf;
}
/**
 * Add the specified network buffer back to the free list
 *
 * Parameters:
 *  nbuf    Buffer to add back to the free list
 */
void
nbuf_free(NBUF *nbuf)
{
    int level = asm_set_ipl(7);

    nbuf->offset = 0;
    nbuf->length = NBUF_SZ;
    queue_add(&nbuf_queue[NBUF_FREE],(QNODE *)nbuf);

    asm_set_ipl(level);
}
/**
 * Remove a network buffer from the specified queue
 *
 * Parameters:
 *  q   The index that identifies the queue to pull the buffer from
 */
NBUF *
nbuf_remove(int q)
{
    NBUF *nbuf;
    int level = asm_set_ipl(7);

    nbuf = (NBUF *)queue_remove(&nbuf_queue[q]);
    asm_set_ipl(level);
    return nbuf;
}
/**
 * Add a network buffer to the specified queue
 *
 * Parameters:
 *  q   The index that identifies the queue to add the buffer to
 */
void
nbuf_add(int q, NBUF *nbuf)
{
    int level = asm_set_ipl(7);
    queue_add(&nbuf_queue[q],(QNODE *)nbuf);
    asm_set_ipl(level);
}
/**
 * Put all the network buffers back into the free list
 */
void
nbuf_reset(void)
{
    NBUF *nbuf;
    int i, level = asm_set_ipl(7);

    for (i=1; i<NBUF_MAXQ; ++i)
    {
        while ((nbuf = nbuf_remove(i)) != NULL)
            nbuf_free(nbuf);
    }
    asm_set_ipl(level);
}
/**
 * Display all the nbuf queues
 */
void
nbuf_debug_dump(void)
{
#ifdef CONFIG_DRIVER_NET_MCF54XX_DEBUG
    NBUF *nbuf;
    int i, j, level;

    level = asm_set_ipl(7);

    for (i=0; i<NBUF_MAXQ; ++i)
    {
        printf("\n\nQueue #%d\n\n",i);
        printf("\tBuffer Location\tOffset\tLength\n");
        printf("--------------------------------------\n");
        j = 0;
        nbuf = (NBUF *)queue_peek(&nbuf_queue[i]);
        while (nbuf != NULL)
        {
           printf("%d\t  0x%08x\t0x%04x\t0x%04x\n",j++,nbuf->data,
                                                       nbuf->offset,
                                                       nbuf->length);
           nbuf = (NBUF *)nbuf->node.next;
        }
    }

    asm_set_ipl(level);
#endif
}

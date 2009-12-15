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
 *  Implements a first in, first out linked list
 *
 * @note Simple selfcontaining basic code
 * @todo Replace by barebox standard list functions
 */
#include <linux/types.h>
#include <proc/net/queue.h>

/** Initialize the specified queue to an empty state
 *
 * @param[in]
 *  q   Pointer to queue structure
 */
void queue_init(QUEUE *q)
{
    q->head = NULL;
}

/**
 * Check for an empty queue
 *
 * @param[in] q    Pointer to queue structure
 * @return
 *  1 if Queue is empty
 *  0 otherwise
 */
int
queue_isempty(QUEUE *q)
{
    return (q->head == NULL);
}

/**
 * Add an item to the end of the queue
 *
 * @param[in] q     Pointer to queue structure
 * @param[in] node  New node to add to the queue
 */
void queue_add(QUEUE *q, QNODE *node)
{
    if (queue_isempty(q))
    {
        q->head = q->tail = node;
    }
    else
    {
        q->tail->next = node;
        q->tail = node;
    }

    node->next = NULL;
}

/** Remove and return first (oldest) entry from the specified queue
 *
 * @param[in] q     Pointer to queue structure
 * @return
 *  Node at head of queue - NULL if queue is empty
 */
QNODE*
queue_remove(QUEUE *q)
{
    QNODE *oldest;

    if (queue_isempty(q))
        return NULL;

    oldest = q->head;
    q->head = oldest->next;
    return oldest;
}

/** Peek into the queue and return pointer to first (oldest) entry.
 *
 * The queue is not modified
 *
 * @param[in] q       Pointer to queue structure
 * @return
 *  Node at head of queue - NULL if queue is empty
 */
QNODE*
queue_peek(QUEUE *q)
{
    return q->head;
}

/** Move entire contents of one queue to the other
 *
 * @param[in] src     Pointer to source queue
 * @param[in] dst     Pointer to destination queue
 */
void
queue_move(QUEUE *dst, QUEUE *src)
{
    if (queue_isempty(src))
        return;

    if (queue_isempty(dst))
        dst->head = src->head;
    else
        dst->tail->next = src->head;

    dst->tail = src->tail;
    src->head = NULL;
    return;
}


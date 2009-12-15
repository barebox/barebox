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
 *  Implement a first in, first out linked list
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_

/*
 * Individual queue node
 */
typedef struct NODE
{
    struct NODE *next;
} QNODE;

/*
 * Queue Struture - linked list of qentry items
 */
typedef struct
{
    QNODE *head;
    QNODE *tail;
} QUEUE;

/*
 * Functions provided by queue.c
 */
void queue_init(QUEUE *);
int queue_isempty(QUEUE *);
void queue_add(QUEUE *, QNODE *);
QNODE* queue_remove(QUEUE *);
QNODE* queue_peek(QUEUE *);
void queue_move(QUEUE *, QUEUE *);

#endif /* _QUEUE_H_ */

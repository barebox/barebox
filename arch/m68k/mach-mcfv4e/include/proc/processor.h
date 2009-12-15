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
 *  Coldfire V4e processor specific defines
 */

/* Empty dummy FIXME */

/* interrupt management */

void mcf_interrupts_initialize (void);
int mcf_interrupts_register_handler (int vector, int (*handler)(void *, void *), void *hdev, void *harg);
void mcf_interrupts_remove_handler (int (*handler)(void *, void *));
int mcf_execute_irq_handler (struct pt_regs *pt_regs,int);


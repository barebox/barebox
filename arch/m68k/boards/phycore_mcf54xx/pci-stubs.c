/*
 * (C) 2007,2008 Carsten Schlote <schlote@vahanus.net>
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
 *  @brief This file contains callbacks for the PCI subsystem
 *
 */
#include <common.h>
#include <config.h>


/** Returns mapping from PCI slot to CPU irq for the target board
 * @return Coldfire IRQ vector number, or -1 for no irq
 */
int mcfv4e_pci_gethostirq(uint8_t slot, uint8_t irqpin)
{
	int rc = -1;
	switch (slot)
	{
	case 16        :  break;
	case 17 ... 21 :  rc = 64 + 7; break;    // Eport IRQ7
	}
	return rc;
}

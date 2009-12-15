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
 *  Interrupt routines and supporting code for Coldfire V4E
 */
#include <common.h>
#include <asm/ptrace.h>
#include <mach/mcf54xx-regs.h>

#ifdef CONFIG_USE_IRQ
void enable_interrupts(void)
{
	asm_set_ipl(0);
}

int disable_interrupts(void)
{
	return asm_set_ipl(7) ? 1 : 0;
}
#endif

/**
 */
static void mcf_bad_mode (void)
{
	panic ("Resetting CPU ...\n");
	mdelay(3000);
	reset_cpu (0);
}

/**
 */
static void mcf_show_regs (struct pt_regs *regs)
{
	unsigned long flags;
	flags = condition_codes (regs);

	printf ("pc : [<%08lx>]\n"
	        "sp : %08lx  fp : %08lx\n",
	        instruction_pointer (regs),
	        regs->M68K_sp, regs->M68K_a6);

	printf ("d0-d3 : %08lx %08lx %08lx %08lx\n",
	        regs->M68K_d0, regs->M68K_d1, regs->M68K_d2, regs->M68K_d3);
	printf ("d3-d7 : %08lx %08lx %08lx %08lx\n",
	        regs->M68K_d3, regs->M68K_d4, regs->M68K_d5, regs->M68K_d6);

	printf ("a0-d3 : %08lx %08lx %08lx %08lx\n",
	        regs->M68K_a0, regs->M68K_a1, regs->M68K_a2, regs->M68K_a3);
	printf ("a3-d7 : %08lx %08lx %08lx %08lx\n",
	        regs->M68K_a3, regs->M68K_a4, regs->M68K_a5, regs->M68K_a6);

	printf ("fp0 : %08lx%08lx   fp1 : %08lx%08lx\n",
	        regs->M68K_fp0+1, regs->M68K_fp0, regs->M68K_fp1+1, regs->M68K_fp1);
	printf ("fp2 : %08lx%08lx   fp3 : %08lx%08lx\n",
	        regs->M68K_fp2+1, regs->M68K_fp2, regs->M68K_fp3+1, regs->M68K_fp3);
	printf ("fp4 : %08lx%08lx   fp5 : %08lx%08lx\n",
	        regs->M68K_fp4+1, regs->M68K_fp4, regs->M68K_fp5+1, regs->M68K_fp5);
	printf ("fp6 : %08lx%08lx   fp7 : %08lx%08lx\n",
	        regs->M68K_fp6+1, regs->M68K_fp6, regs->M68K_fp7+1, regs->M68K_fp7);

	printf ("Flags: %c%c%c%c",
	        flags & CC_X_BIT ? 'X' : 'x',
	        flags & CC_N_BIT ? 'N' : 'n',
	        flags & CC_Z_BIT ? 'Z' : 'z',
	        flags & CC_V_BIT ? 'V' : 'v',
	        flags & CC_C_BIT ? 'C' : 'c' );

	printf ("  IRQs %s (%0x) Mode %s\n",
	        interrupts_enabled (regs) ? "on" : "off", interrupts_enabled (regs),
	        user_mode (regs) ? "user" : "supervisor");
}

void mcf_execute_exception_handler (struct pt_regs *pt_regs)
{
	printf ("unhandled exception\n");
	mcf_show_regs (pt_regs);
	mcf_bad_mode ();
}

#ifndef CONFIG_USE_IRQ

void mcf_execute_irq_handler (struct pt_regs *pt_regs, int vector)
{
	printf ("interrupt request\n");
	mcf_show_regs (pt_regs);
	mcf_bad_mode ();
}

#else

#ifndef CONFIG_MAX_ISR_HANDLERS
#define CONFIG_MAX_ISR_HANDLERS   (20)
#endif

typedef struct
{
	int     vector;
	int     (*handler)(void *, void *);
	void    *hdev;
	void    *harg;
}
mcfv4e_irq_handler_s;

mcfv4e_irq_handler_s irq_handler_table[CONFIG_MAX_ISR_HANDLERS];

/** Initialize an empty interrupt handler list
 */
void mcf_interrupts_initialize (void)
{
	int index;
	for (index = 0; index < CONFIG_MAX_ISR_HANDLERS; index++)
	{
		irq_handler_table[index].vector = 0;
		irq_handler_table[index].handler = 0;
		irq_handler_table[index].hdev = 0;
		irq_handler_table[index].harg = 0;
	}
}

/** Add an interrupt handler to the handler list
 *
 * @param vector : M68k exception/interrupt vector number
 * @param handler : Pointer to handler function
 * @param hdev : Handler specific data
 * @param harg : Handler specific arg
 */
int mcf_interrupts_register_handler(
        int vector,
        int (*handler)(void *, void *), void *hdev, void *harg)
{
	/*
	* This function places an interrupt handler in the ISR table,
	* thereby registering it so that the low-level handler may call it.
	*
	* The two parameters are intended for the first arg to be a
	* pointer to the device itself, and the second a pointer to a data
	* structure used by the device driver for that particular device.
	*/
	int index;

	if ((vector == 0) || (handler == NULL))
	{
		return 0;
	}

	for (index = 0; index < CONFIG_MAX_ISR_HANDLERS; index++)
	{
		if (irq_handler_table[index].vector == vector)
		{
			/* only one entry of each type per vector */
			return 0;
		}

		if (irq_handler_table[index].vector == 0)
		{
			irq_handler_table[index].vector = vector;
			irq_handler_table[index].handler = handler;
			irq_handler_table[index].hdev = hdev;
			irq_handler_table[index].harg = harg;
			return 1;
		}
	}
	return 0;   /* no available slots */
}

/** Remove an interrupt handler from the handler list
 *
 * @param type : FIXME
 * @param handler : Pointer of handler function to remove.
 */
void mcf_interrupts_remove_handler (int type ,int (*handler)(void *, void *))
{
	/*
	* This routine removes from the ISR table all
	* entries that matches 'handler'.
	*/
	int index;

	for (index = 0; index < CONFIG_MAX_ISR_HANDLERS; index++)
	{
		if (irq_handler_table[index].handler == handler)
		{
			irq_handler_table[index].vector = 0;
			irq_handler_table[index].handler = 0;
			irq_handler_table[index].hdev = 0;
			irq_handler_table[index].harg = 0;
		}
	}
}

/** Traverse list of registered interrupts and call matching handlers.
 *
 * @param pt_regs : Pointer to saved register context
 * @param vector : M68k exception/interrupt vector number
 */
int mcf_execute_irq_handler (struct pt_regs *pt_regs, int vector)
{
	/*
	* This routine searches the ISR table for an entry that matches
	* 'vector'.  If one is found, then 'handler' is executed.
	*/
	int index, retval = 0;

	/*
	* Try to locate a user-registered Interrupt Service Routine handler.
	*/
	for (index = 0; index < CONFIG_MAX_ISR_HANDLERS; index++)
	{
		if (irq_handler_table[index].handler == NULL)
		{
			printf("\nFault: No handler for IRQ vector %ld found.\n", vector);
			break;
		}
		if (irq_handler_table[index].vector == vector)
		{
			if (irq_handler_table[index].handler(irq_handler_table[index].hdev,irq_handler_table[index].harg))
			{
				retval = 1;
				break;
			}
		}
	}

	return retval;
}

#endif

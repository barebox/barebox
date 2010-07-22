/*
 * (C) 2007,2008 konzeptpark, Carsten Schlote <c.schlote@konzeptpark.de>
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
 *  @brief This file contains high-level init functions.
 *
 */
#include <common.h>
#include <reloc.h>
#include <config.h>
#include <mach/mcf54xx-regs.h>

static void board_gpio_init(void)
{
	/*
	 * Enable Ethernet signals so that, if a cable is plugged into
	 * the ports, the lines won't be floating and potentially cause
	 * erroneous transmissions
	 */
	MCF_GPIO_PAR_FECI2CIRQ = 0
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDC_EMDC
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDIO_EMDIO
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MII
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E17
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDC
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDIO
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MII
	                         | MCF_GPIO_PAR_FECI2CIRQ_PAR_E07;
}


static void board_psc_init(void)
{
#if (CFG_EARLY_UART_PORT == 0)
	MCF_GPIO_PAR_PSC0 = (0
#ifdef HARDWARE_FLOW_CONTROL
	                     | MCF_GPIO_PAR_PSC0_PAR_CTS0_CTS
	                     | MCF_GPIO_PAR_PSC0_PAR_RTS0_RTS
#endif
	                     | MCF_GPIO_PAR_PSC0_PAR_TXD0
	                     | MCF_GPIO_PAR_PSC0_PAR_RXD0);
#elif (CFG_EARLY_UART_PORT == 1)
	MCF_GPIO_PAR_PSC1 = (0
#ifdef HARDWARE_FLOW_CONTROL
	                     | MCF_GPIO_PAR_PSC1_PAR_CTS1_CTS
	                     | MCF_GPIO_PAR_PSC1_PAR_RTS1_RTS
#endif
	                     | MCF_GPIO_PAR_PSC1_PAR_TXD1
	                     | MCF_GPIO_PAR_PSC1_PAR_RXD1);
#elif (CFG_EARLY_UART_PORT == 2)
	MCF_GPIO_PAR_PSC2 = (0
#ifdef HARDWARE_FLOW_CONTROL
	                     | MCF_GPIO_PAR_PSC2_PAR_CTS2_CTS
	                     | MCF_GPIO_PAR_PSC2_PAR_RTS2_RTS
#endif
	                     | MCF_GPIO_PAR_PSC2_PAR_TXD2
	                     | MCF_GPIO_PAR_PSC2_PAR_RXD2);
#elif (CFG_EARLY_UART_PORT == 3)
	MCF_GPIO_PAR_PSC3 = (0
#ifdef HARDWARE_FLOW_CONTROL
	                     | MCF_GPIO_PAR_PSC3_PAR_CTS3_CTS
	                     | MCF_GPIO_PAR_PSC3_PAR_RTS3_RTS
#endif
	                     | MCF_GPIO_PAR_PSC3_PAR_TXD3
	                     | MCF_GPIO_PAR_PSC3_PAR_RXD3);
#else
#error "Invalid CFG_EARLY_UART_PORT setting"
#endif

	/* Put PSC in UART mode */
	MCF_PSC_SICR(CFG_EARLY_UART_PORT) = MCF_PSC_SICR_SIM_UART;

	/* Call generic UART initialization */
//    mcf5xxx_uart_init(DBUG_UART_PORT, board_get_baud());
}


/** Do board specific early init
 *
 * @note We run at link address now, you can now call other code
 */
void board_init_highlevel(void)
{
	/* Initialize platform specific GPIOs */
	board_gpio_init();

	/* Init UART GPIOs and Modes */
	board_psc_init();

	/* Setup the early init data */
#ifdef CONFIG_HAS_EARLY_INIT
	early_init();
#endif
	/* Configure the early debug output facility */
#ifdef CONFIG_DEBUG_LL
	early_debug_init();
#endif
}

/** Provide address of early debug low-level output
 *
 * @todo Should return real address for UART register map.
 */
void *get_early_console_base(const char *name)
{
	return (void*)1 + CFG_EARLY_UART_PORT;
}

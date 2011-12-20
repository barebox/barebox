#ifndef _GENERIC_NAMES_H_
#define _GENERIC_NAMES_H_

#define CONFIG_SYS_CLK_FREQ		20000000

#define OPENRISC_TIMER_FREQ		20000000

#define OPENRISC_SOPC_MEMORY_BASE	0x00000000
#define OPENRISC_SOPC_MEMORY_SIZE	0x02000000

#define OPENRISC_SOPC_UART_BASE		0x90000000
#define CONFIG_SYS_UART_FREQ		CONFIG_SYS_CLK_FREQ
#define CONFIG_SYS_UART_BAUD		115200

/* We reserve 256K for barebox */
#define BAREBOX_RESERVED_SIZE		0x40000

/* Barebox will be at top of main memory */
#define OPENRISC_SOPC_TEXT_BASE		(OPENRISC_SOPC_MEMORY_BASE + OPENRISC_SOPC_MEMORY_SIZE - BAREBOX_RESERVED_SIZE)

/*
* TEXT_BASE is defined here because STACK_BASE definition
*  in include/asm-generic/memory_layout.h uses this name
*/

#define TEXT_BASE                       OPENRISC_SOPC_TEXT_BASE

#endif

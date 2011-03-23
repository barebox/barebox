#ifndef _GENERIC_NAMES_H_
#define _GENERIC_NAMES_H_

#include "nios_sopc.h"

#ifndef MMU_PRESENT
#define IO_REGION_BASE                  0x80000000
#define KERNEL_REGION_BASE              0x00000000
#endif

/*Name of the RAM memory in your SOPC project */
#define NIOS_SOPC_MEMORY_BASE           (KERNEL_REGION_BASE | DDR_SDRAM_BASE)
#define NIOS_SOPC_MEMORY_SIZE           DDR_SDRAM_SPAN

/*Name of the timer in your SOPC project */
#define NIOS_SOPC_TIMER_BASE            (IO_REGION_BASE | SYS_CLK_TIMER_BASE)
#define NIOS_SOPC_TIMER_FREQ            SYS_CLK_TIMER_FREQ

/*Name of TSE and SGDMA in your SOPC project */
#define NIOS_SOPC_SGDMA_RX_BASE         (IO_REGION_BASE | SGDMA_RX_BASE)
#define NIOS_SOPC_SGDMA_TX_BASE         (IO_REGION_BASE | SGDMA_TX_BASE)
#define NIOS_SOPC_TSE_BASE              (IO_REGION_BASE | TSE_BASE)
#define NIOS_SOPC_TSE_DESC_MEM_BASE     (IO_REGION_BASE | DESCRIPTOR_MEMORY_BASE)

/*Name of the UART in your SOPC project */
#define NIOS_SOPC_UART_BASE             (IO_REGION_BASE | UART_BASE)

/*Name of the JTAG UART in your SOPC project */
#define NIOS_SOPC_JTAG_UART_BASE        (IO_REGION_BASE | JTAG_UART_BASE)

/*Name of the CFI flash in your SOPC project */
#define NIOS_SOPC_FLASH_BASE            (IO_REGION_BASE | CFI_FLASH_BASE)
#define NIOS_SOPC_FLASH_SIZE            CFI_FLASH_SPAN

/*Name of the EPCS flash controller in your SOPC project */

#define NIOS_SOPC_EPCS_BASE             (IO_REGION_BASE | (EPCS_FLASH_CONTROLLER_BASE + EPCS_FLASH_CONTROLLER_REGISTER_OFFSET))

/* PHY MDIO Address */
#define NIOS_SOPC_PHY_ADDR              1

/* We reserve 256K for barebox */
#define BAREBOX_RESERVED_SIZE           0x80000

/* Barebox will be at top of main memory */
#define NIOS_SOPC_TEXT_BASE             (NIOS_SOPC_MEMORY_BASE + NIOS_SOPC_MEMORY_SIZE - BAREBOX_RESERVED_SIZE)

/*
* TEXT_BASE is defined here because STACK_BASE definition
*  in include/asm-generic/memory_layout.h uses this name
*/

#define TEXT_BASE                       NIOS_SOPC_TEXT_BASE

/* Board banner */

#define BOARD_BANNER "\
\033[44;1m***********************************************\e[0m\n\
\033[44;1m*             Altera generic board            *\e[0m\n\
\033[44;1m***********************************************\e[0m\
\e[0m\n\n"

#endif

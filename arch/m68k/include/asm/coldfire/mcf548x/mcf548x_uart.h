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
 *  Register and bit definitions for the MCF548X and MCF547x
 *  Programmable Serial Controller (UART Compatible Definitions) (UART)
 */
#ifndef __MCF548X_UART_H__
#define __MCF548X_UART_H__

/*
 *  Programmable Serial Controller (UART Compatible Definitions) (UART)
 */

/* Register read/write macros */
#define MCF_UART_UMR0        (*(vuint8_t *)(&__MBAR[0x008600]))
#define MCF_UART_USR0        (*(vuint8_t *)(&__MBAR[0x008604]))
#define MCF_UART_UCSR0       (*(vuint8_t *)(&__MBAR[0x008604]))
#define MCF_UART_UCR0        (*(vuint8_t *)(&__MBAR[0x008608]))
#define MCF_UART_URB0        (*(vuint8_t *)(&__MBAR[0x00860C]))
#define MCF_UART_UTB0        (*(vuint8_t *)(&__MBAR[0x00860C]))
#define MCF_UART_UIPCR0      (*(vuint8_t *)(&__MBAR[0x008610]))
#define MCF_UART_UACR0       (*(vuint8_t *)(&__MBAR[0x008610]))
#define MCF_UART_UISR0       (*(vuint8_t *)(&__MBAR[0x008614]))
#define MCF_UART_UIMR0       (*(vuint8_t *)(&__MBAR[0x008614]))
#define MCF_UART_UBG10       (*(vuint8_t *)(&__MBAR[0x008618]))
#define MCF_UART_UBG20       (*(vuint8_t *)(&__MBAR[0x00861C]))
#define MCF_UART_UIP0        (*(vuint8_t *)(&__MBAR[0x008634]))
#define MCF_UART_UOP10       (*(vuint8_t *)(&__MBAR[0x008638]))
#define MCF_UART_UOP00       (*(vuint8_t *)(&__MBAR[0x00863C]))

#define MCF_UART_UMR1        (*(vuint8_t *)(&__MBAR[0x008700]))
#define MCF_UART_USR1        (*(vuint8_t *)(&__MBAR[0x008704]))
#define MCF_UART_UCSR1       (*(vuint8_t *)(&__MBAR[0x008704]))
#define MCF_UART_UCR1        (*(vuint8_t *)(&__MBAR[0x008708]))
#define MCF_UART_URB1        (*(vuint8_t *)(&__MBAR[0x00870C]))
#define MCF_UART_UTB1        (*(vuint8_t *)(&__MBAR[0x00870C]))
#define MCF_UART_UIPCR1      (*(vuint8_t *)(&__MBAR[0x008710]))
#define MCF_UART_UACR1       (*(vuint8_t *)(&__MBAR[0x008710]))
#define MCF_UART_UISR1       (*(vuint8_t *)(&__MBAR[0x008714]))
#define MCF_UART_UIMR1       (*(vuint8_t *)(&__MBAR[0x008714]))
#define MCF_UART_UBG11       (*(vuint8_t *)(&__MBAR[0x008718]))
#define MCF_UART_UBG21       (*(vuint8_t *)(&__MBAR[0x00871C]))
#define MCF_UART_UIP1        (*(vuint8_t *)(&__MBAR[0x008734]))
#define MCF_UART_UOP11       (*(vuint8_t *)(&__MBAR[0x008738]))
#define MCF_UART_UOP01       (*(vuint8_t *)(&__MBAR[0x00873C]))

#define MCF_UART_UMR2        (*(vuint8_t *)(&__MBAR[0x008800]))
#define MCF_UART_USR2        (*(vuint8_t *)(&__MBAR[0x008804]))
#define MCF_UART_UCSR2       (*(vuint8_t *)(&__MBAR[0x008804]))
#define MCF_UART_UCR2        (*(vuint8_t *)(&__MBAR[0x008808]))
#define MCF_UART_URB2        (*(vuint8_t *)(&__MBAR[0x00880C]))
#define MCF_UART_UTB2        (*(vuint8_t *)(&__MBAR[0x00880C]))
#define MCF_UART_UIPCR2      (*(vuint8_t *)(&__MBAR[0x008810]))
#define MCF_UART_UACR2       (*(vuint8_t *)(&__MBAR[0x008810]))
#define MCF_UART_UISR2       (*(vuint8_t *)(&__MBAR[0x008814]))
#define MCF_UART_UIMR2       (*(vuint8_t *)(&__MBAR[0x008814]))
#define MCF_UART_UBG12       (*(vuint8_t *)(&__MBAR[0x008818]))
#define MCF_UART_UBG22       (*(vuint8_t *)(&__MBAR[0x00881C]))
#define MCF_UART_UIP2        (*(vuint8_t *)(&__MBAR[0x008834]))
#define MCF_UART_UOP12       (*(vuint8_t *)(&__MBAR[0x008838]))
#define MCF_UART_UOP02       (*(vuint8_t *)(&__MBAR[0x00883C]))

#define MCF_UART_UMR3        (*(vuint8_t *)(&__MBAR[0x008900]))
#define MCF_UART_USR3        (*(vuint8_t *)(&__MBAR[0x008904]))
#define MCF_UART_UCSR3       (*(vuint8_t *)(&__MBAR[0x008904]))
#define MCF_UART_UCR3        (*(vuint8_t *)(&__MBAR[0x008908]))
#define MCF_UART_URB3        (*(vuint8_t *)(&__MBAR[0x00890C]))
#define MCF_UART_UTB3        (*(vuint8_t *)(&__MBAR[0x00890C]))
#define MCF_UART_UIPCR3      (*(vuint8_t *)(&__MBAR[0x008910]))
#define MCF_UART_UACR3       (*(vuint8_t *)(&__MBAR[0x008910]))
#define MCF_UART_UISR3       (*(vuint8_t *)(&__MBAR[0x008914]))
#define MCF_UART_UIMR3       (*(vuint8_t *)(&__MBAR[0x008914]))
#define MCF_UART_UBG13       (*(vuint8_t *)(&__MBAR[0x008918]))
#define MCF_UART_UBG23       (*(vuint8_t *)(&__MBAR[0x00891C]))
#define MCF_UART_UIP3        (*(vuint8_t *)(&__MBAR[0x008934]))
#define MCF_UART_UOP13       (*(vuint8_t *)(&__MBAR[0x008938]))
#define MCF_UART_UOP03       (*(vuint8_t *)(&__MBAR[0x00893C]))


#define MCF_UART_UMR(x)      (*(vuint8_t *)(&__MBAR[0x008600+((x)*0x100)]))
#define MCF_UART_USR(x)      (*(vuint8_t *)(&__MBAR[0x008604+((x)*0x100)]))
#define MCF_UART_UCSR(x)     (*(vuint8_t *)(&__MBAR[0x008604+((x)*0x100)]))
#define MCF_UART_UCR(x)      (*(vuint8_t *)(&__MBAR[0x008608+((x)*0x100)]))
#define MCF_UART_URB(x)      (*(vuint8_t *)(&__MBAR[0x00860C+((x)*0x100)]))
#define MCF_UART_UTB(x)      (*(vuint8_t *)(&__MBAR[0x00860C+((x)*0x100)]))
#define MCF_UART_UIPCR(x)    (*(vuint8_t *)(&__MBAR[0x008610+((x)*0x100)]))
#define MCF_UART_UACR(x)     (*(vuint8_t *)(&__MBAR[0x008610+((x)*0x100)]))
#define MCF_UART_UISR(x)     (*(vuint8_t *)(&__MBAR[0x008614+((x)*0x100)]))
#define MCF_UART_UIMR(x)     (*(vuint8_t *)(&__MBAR[0x008614+((x)*0x100)]))
#define MCF_UART_UBG1(x)     (*(vuint8_t *)(&__MBAR[0x008618+((x)*0x100)]))
#define MCF_UART_UBG2(x)     (*(vuint8_t *)(&__MBAR[0x00861C+((x)*0x100)]))
#define MCF_UART_UIP(x)      (*(vuint8_t *)(&__MBAR[0x008634+((x)*0x100)]))
#define MCF_UART_UOP1(x)     (*(vuint8_t *)(&__MBAR[0x008638+((x)*0x100)]))
#define MCF_UART_UOP0(x)     (*(vuint8_t *)(&__MBAR[0x00863C+((x)*0x100)]))

/* Bit definitions and macros for MCF_UART_UMR */
#define MCF_UART_UMR_BC(x)              (((x)&0x03)<<0)
#define MCF_UART_UMR_PT                 (0x04)
#define MCF_UART_UMR_PM(x)              (((x)&0x03)<<3)
#define MCF_UART_UMR_ERR                (0x20)
#define MCF_UART_UMR_RXIRQ              (0x40)
#define MCF_UART_UMR_RXRTS              (0x80)
#define MCF_UART_UMR_SB(x)              (((x)&0x0F)<<0)
#define MCF_UART_UMR_TXCTS              (0x10)
#define MCF_UART_UMR_TXRTS              (0x20)
#define MCF_UART_UMR_CM(x)              (((x)&0x03)<<6)
#define MCF_UART_UMR_PM_MULTI_ADDR      (0x1C)
#define MCF_UART_UMR_PM_MULTI_DATA      (0x18)
#define MCF_UART_UMR_PM_NONE            (0x10)
#define MCF_UART_UMR_PM_FORCE_HI        (0x0C)
#define MCF_UART_UMR_PM_FORCE_LO        (0x08)
#define MCF_UART_UMR_PM_ODD             (0x04)
#define MCF_UART_UMR_PM_EVEN            (0x00)
#define MCF_UART_UMR_BC_5               (0x00)
#define MCF_UART_UMR_BC_6               (0x01)
#define MCF_UART_UMR_BC_7               (0x02)
#define MCF_UART_UMR_BC_8               (0x03)
#define MCF_UART_UMR_CM_NORMAL          (0x00)
#define MCF_UART_UMR_CM_ECHO            (0x40)
#define MCF_UART_UMR_CM_LOCAL_LOOP      (0x80)
#define MCF_UART_UMR_CM_REMOTE_LOOP     (0xC0)
#define MCF_UART_UMR_SB_STOP_BITS_1     (0x07)
#define MCF_UART_UMR_SB_STOP_BITS_15    (0x08)
#define MCF_UART_UMR_SB_STOP_BITS_2     (0x0F)

/* Bit definitions and macros for MCF_UART_USR */
#define MCF_UART_USR_RXRDY              (0x01)
#define MCF_UART_USR_FFULL              (0x02)
#define MCF_UART_USR_TXRDY              (0x04)
#define MCF_UART_USR_TXEMP              (0x08)
#define MCF_UART_USR_OE                 (0x10)
#define MCF_UART_USR_PE                 (0x20)
#define MCF_UART_USR_FE                 (0x40)
#define MCF_UART_USR_RB                 (0x80)

/* Bit definitions and macros for MCF_UART_UCSR */
#define MCF_UART_UCSR_TCS(x)            (((x)&0x0F)<<0)
#define MCF_UART_UCSR_RCS(x)            (((x)&0x0F)<<4)
#define MCF_UART_UCSR_RCS_SYS_CLK       (0xD0)
#define MCF_UART_UCSR_RCS_CTM16         (0xE0)
#define MCF_UART_UCSR_RCS_CTM           (0xF0)
#define MCF_UART_UCSR_TCS_SYS_CLK       (0x0D)
#define MCF_UART_UCSR_TCS_CTM16         (0x0E)
#define MCF_UART_UCSR_TCS_CTM           (0x0F)

/* Bit definitions and macros for MCF_UART_UCR */
#define MCF_UART_UCR_RXC(x)             (((x)&0x03)<<0)
#define MCF_UART_UCR_TXC(x)             (((x)&0x03)<<2)
#define MCF_UART_UCR_MISC(x)            (((x)&0x07)<<4)
#define MCF_UART_UCR_NONE               (0x00)
#define MCF_UART_UCR_STOP_BREAK         (0x70)
#define MCF_UART_UCR_START_BREAK        (0x60)
#define MCF_UART_UCR_BKCHGINT           (0x50)
#define MCF_UART_UCR_RESET_ERROR        (0x40)
#define MCF_UART_UCR_RESET_TX           (0x30)
#define MCF_UART_UCR_RESET_RX           (0x20)
#define MCF_UART_UCR_RESET_MR           (0x10)
#define MCF_UART_UCR_TX_DISABLED        (0x08)
#define MCF_UART_UCR_TX_ENABLED         (0x04)
#define MCF_UART_UCR_RX_DISABLED        (0x02)
#define MCF_UART_UCR_RX_ENABLED         (0x01)

/* Bit definitions and macros for MCF_UART_UIPCR */
#define MCF_UART_UIPCR_CTS              (0x01)
#define MCF_UART_UIPCR_COS              (0x10)

/* Bit definitions and macros for MCF_UART_UACR */
#define MCF_UART_UACR_IEC               (0x01)

/* Bit definitions and macros for MCF_UART_UISR */
#define MCF_UART_UISR_TXRDY             (0x01)
#define MCF_UART_UISR_RXRDY_FU          (0x02)
#define MCF_UART_UISR_DB                (0x04)
#define MCF_UART_UISR_RXFTO             (0x08)
#define MCF_UART_UISR_TXFIFO            (0x10)
#define MCF_UART_UISR_RXFIFO            (0x20)
#define MCF_UART_UISR_COS               (0x80)

/* Bit definitions and macros for MCF_UART_UIMR */
#define MCF_UART_UIMR_TXRDY             (0x01)
#define MCF_UART_UIMR_RXRDY_FU          (0x02)
#define MCF_UART_UIMR_DB                (0x04)
#define MCF_UART_UIMR_COS               (0x80)

/* Bit definitions and macros for MCF_UART_UIP */
#define MCF_UART_UIP_CTS                (0x01)

/* Bit definitions and macros for MCF_UART_UOP1 */
#define MCF_UART_UOP1_RTS               (0x01)

/* Bit definitions and macros for MCF_UART_UOP0 */
#define MCF_UART_UOP0_RTS               (0x01)

/* The UART registers for mem mapped access */
struct m5407uart
{
	vuint8_t	umr;	vuint24_t	reserved0;
	vuint8_t 	usr;    vuint24_t	reserved1;   /* ucsr */
	vuint8_t	ucr;    vuint24_t	reserved2;
	vuint8_t	urb;    vuint24_t	reserved3;   /* utb */
	vuint8_t	uipcr;  vuint24_t	reserved4;   /* uacr */
	vuint8_t	uisr;   vuint24_t	reserved5;   /* uimr */
	vuint8_t	udu;    vuint24_t	reserved6;
	vuint8_t	ubg1;   vuint24_t	reserved7;
	vuint8_t	ubg2;   vuint24_t	reserved8;
	const uint8_t	uip;    vuint24_t	reserved9;
	vuint8_t	uop1;   vuint24_t	reserved10;
	vuint8_t	uop0;   vuint24_t	reserved11;
} __attribute((packed));

#define MCF_UART(x)      (*(struct m5407uart *)(&__MBAR[0x008600+((x)*0x100)]))


#endif /* __MCF548X_UART_H__ */


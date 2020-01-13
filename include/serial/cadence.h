#ifndef __CADENCE_UART_H__
#define __CADENCE_UART_H__

#define CADENCE_UART_CONTROL		0x00
#define CADENCE_UART_MODE		0x04
#define CADENCE_UART_BAUD_GEN		0x18
#define CADENCE_UART_CHANNEL_STS	0x2C
#define CADENCE_UART_RXTXFIFO		0x30
#define CADENCE_UART_BAUD_DIV		0x34

#define CADENCE_CTRL_RXRES		(1 << 0)
#define CADENCE_CTRL_TXRES		(1 << 1)
#define CADENCE_CTRL_RXEN		(1 << 2)
#define CADENCE_CTRL_RXDIS		(1 << 3)
#define CADENCE_CTRL_TXEN		(1 << 4)
#define CADENCE_CTRL_TXDIS		(1 << 5)
#define CADENCE_CTRL_RSTTO		(1 << 6)
#define CADENCE_CTRL_STTBRK		(1 << 7)
#define CADENCE_CTRL_STPBRK		(1 << 8)

#define CADENCE_MODE_CLK_REF		(0 << 0)
#define CADENCE_MODE_CLK_REF_DIV	(1 << 0)
#define CADENCE_MODE_CHRL_6		(3 << 1)
#define CADENCE_MODE_CHRL_7		(2 << 1)
#define CADENCE_MODE_CHRL_8		(0 << 1)
#define CADENCE_MODE_PAR_EVEN		(0 << 3)
#define CADENCE_MODE_PAR_ODD		(1 << 3)
#define CADENCE_MODE_PAR_SPACE		(2 << 3)
#define CADENCE_MODE_PAR_MARK		(3 << 3)
#define CADENCE_MODE_PAR_NONE		(4 << 3)

#define CADENCE_STS_REMPTY		(1 << 1)
#define CADENCE_STS_RFUL		(1 << 2)
#define CADENCE_STS_TEMPTY		(1 << 3)
#define CADENCE_STS_TFUL		(1 << 4)

#endif /* __CADENCE_UART_H__ */

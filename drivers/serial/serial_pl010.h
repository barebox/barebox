/*
 * Copyright (C) 2010 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * (C) Copyright 2003, 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

struct hldc_struct {
	uint32_t ctrl;
	uint32_t addmtchval;
	uint32_t addmask;
	uint32_t rxinfobuf;
	uint32_t sts;
};

struct pl010_struct {
	uint32_t data;
	union {
		uint32_t rxsts;
		uint32_t errclr;
	};
	uint32_t linctrlhigh;
	uint32_t linctrlmid;
	uint32_t linctrllow;
	uint32_t ctrl;
	uint32_t flag;
	union {
		uint32_t intid;
		uint32_t intclr;
	};
	uint32_t not_used0[2];
	uint32_t dmactrl;
	uint32_t not_used1[53];
	uint32_t modemctrl;
	uint32_t modemsts;
	uint32_t not_used2[65];
	struct hldc_struct hldc;
};

#define UART_PL010_RSR_OE	(1 << 3)
#define UART_PL010_RSR_BE	(1 << 2)
#define UART_PL010_RSR_PE	(1 << 1)
#define UART_PL010_RSR_FE	(1 << 0)

#define UART_PL010_FR_TXFE	(1 << 7)
#define UART_PL010_FR_RXFF	(1 << 6)
#define UART_PL010_FR_TXFF	(1 << 5)
#define UART_PL010_FR_RXFE	(1 << 4)
#define UART_PL010_FR_BUSY	(1 << 3)
#define UART_PL010_FR_TMSK	(UART_PL010_FR_TXFF + UART_PL010_FR_BUSY)

#define UART_PL010_CR_LPE	(1 << 7)
#define UART_PL010_CR_RTIE	(1 << 6)
#define UART_PL010_CR_TIE	(1 << 5)
#define UART_PL010_CR_RIE	(1 << 4)
#define UART_PL010_CR_MSIE	(1 << 3)
#define UART_PL010_CR_IIRLP	(1 << 2)
#define UART_PL010_CR_SIREN	(1 << 1)
#define UART_PL010_CR_UARTEN	(1 << 0)

#define UART_PL010_LCRH_WLEN_8	(3 << 5)
#define UART_PL010_LCRH_WLEN_7	(2 << 5)
#define UART_PL010_LCRH_WLEN_6	(1 << 5)
#define UART_PL010_LCRH_WLEN_5	(0 << 5)
#define UART_PL010_LCRH_FEN	(1 << 4)
#define UART_PL010_LCRH_STP2	(1 << 3)
#define UART_PL010_LCRH_EPS	(1 << 2)
#define UART_PL010_LCRH_PEN	(1 << 1)
#define UART_PL010_LCRH_BRK	(1 << 0)

#define UART_PL010_BAUD_460800	1
#define UART_PL010_BAUD_230400	3
#define UART_PL010_BAUD_115200	7
#define UART_PL010_BAUD_57600	15
#define UART_PL010_BAUD_38400	23
#define UART_PL010_BAUD_19200	47
#define UART_PL010_BAUD_14400	63
#define UART_PL010_BAUD_9600	95
#define UART_PL010_BAUD_4800	191
#define UART_PL010_BAUD_2400	383
#define UART_PL010_BAUD_1200	767

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
 *  I2C Module (I2C)
 */
#ifndef __MCF548X_I2C_H__
#define __MCF548X_I2C_H__

/*
 *  I2C Module (I2C)
 */

/* Register read/write macros */
#define MCF_I2C_I2AR     (*(vuint8_t *)(&__MBAR[0x008F00]))
#define MCF_I2C_I2FDR    (*(vuint8_t *)(&__MBAR[0x008F04]))
#define MCF_I2C_I2CR     (*(vuint8_t *)(&__MBAR[0x008F08]))
#define MCF_I2C_I2SR     (*(vuint8_t *)(&__MBAR[0x008F0C]))
#define MCF_I2C_I2DR     (*(vuint8_t *)(&__MBAR[0x008F10]))
#define MCF_I2C_I2ICR    (*(vuint8_t *)(&__MBAR[0x008F20]))

/* Bit definitions and macros for MCF_I2C_I2AR */
#define MCF_I2C_I2AR_ADR(x)    (((x)&0x7F)<<1)

/* Bit definitions and macros for MCF_I2C_I2FDR */
#define MCF_I2C_I2FDR_IC(x)    (((x)&0x3F)<<0)

/* Bit definitions and macros for MCF_I2C_I2CR */
#define MCF_I2C_I2CR_RSTA      (0x04)
#define MCF_I2C_I2CR_TXAK      (0x08)
#define MCF_I2C_I2CR_MTX       (0x10)
#define MCF_I2C_I2CR_MSTA      (0x20)
#define MCF_I2C_I2CR_IIEN      (0x40)
#define MCF_I2C_I2CR_IEN       (0x80)

/* Bit definitions and macros for MCF_I2C_I2SR */
#define MCF_I2C_I2SR_RXAK      (0x01)
#define MCF_I2C_I2SR_IIF       (0x02)
#define MCF_I2C_I2SR_SRW       (0x04)
#define MCF_I2C_I2SR_IAL       (0x10)
#define MCF_I2C_I2SR_IBB       (0x20)
#define MCF_I2C_I2SR_IAAS      (0x40)
#define MCF_I2C_I2SR_ICF       (0x80)

/* Bit definitions and macros for MCF_I2C_I2ICR */
#define MCF_I2C_I2ICR_IE       (0x01)
#define MCF_I2C_I2ICR_RE       (0x02)
#define MCF_I2C_I2ICR_TE       (0x04)
#define MCF_I2C_I2ICR_BNBE     (0x08)

#endif /* __MCF548X_I2C_H__ */

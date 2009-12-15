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
 *  Edge Port Module (EPORT)
 */
#ifndef __MCF548X_EPORT_H__
#define __MCF548X_EPORT_H__

/*
 *  Edge Port Module (EPORT)
 */

/* Register read/write macros */
#define MCF_EPORT_EPPAR    (*(vuint16_t*)(&__MBAR[0x000F00]))
#define MCF_EPORT_EPDDR    (*(vuint8_t *)(&__MBAR[0x000F04]))
#define MCF_EPORT_EPIER    (*(vuint8_t *)(&__MBAR[0x000F05]))
#define MCF_EPORT_EPDR     (*(vuint8_t *)(&__MBAR[0x000F08]))
#define MCF_EPORT_EPPDR    (*(vuint8_t *)(&__MBAR[0x000F09]))
#define MCF_EPORT_EPFR     (*(vuint8_t *)(&__MBAR[0x000F0C]))

/* Bit definitions and macros for MCF_EPORT_EPPAR */
#define MCF_EPORT_EPPAR_EPPA1(x)         (((x)&0x0003)<<2)
#define MCF_EPORT_EPPAR_EPPA2(x)         (((x)&0x0003)<<4)
#define MCF_EPORT_EPPAR_EPPA3(x)         (((x)&0x0003)<<6)
#define MCF_EPORT_EPPAR_EPPA4(x)         (((x)&0x0003)<<8)
#define MCF_EPORT_EPPAR_EPPA5(x)         (((x)&0x0003)<<10)
#define MCF_EPORT_EPPAR_EPPA6(x)         (((x)&0x0003)<<12)
#define MCF_EPORT_EPPAR_EPPA7(x)         (((x)&0x0003)<<14)
#define MCF_EPORT_EPPAR_EPPAx_LEVEL      (0)
#define MCF_EPORT_EPPAR_EPPAx_RISING     (1)
#define MCF_EPORT_EPPAR_EPPAx_FALLING    (2)
#define MCF_EPORT_EPPAR_EPPAx_BOTH       (3)

/* Bit definitions and macros for MCF_EPORT_EPDDR */
#define MCF_EPORT_EPDDR_EPDD1            (0x02)
#define MCF_EPORT_EPDDR_EPDD2            (0x04)
#define MCF_EPORT_EPDDR_EPDD3            (0x08)
#define MCF_EPORT_EPDDR_EPDD4            (0x10)
#define MCF_EPORT_EPDDR_EPDD5            (0x20)
#define MCF_EPORT_EPDDR_EPDD6            (0x40)
#define MCF_EPORT_EPDDR_EPDD7            (0x80)

/* Bit definitions and macros for MCF_EPORT_EPIER */
#define MCF_EPORT_EPIER_EPIE1            (0x02)
#define MCF_EPORT_EPIER_EPIE2            (0x04)
#define MCF_EPORT_EPIER_EPIE3            (0x08)
#define MCF_EPORT_EPIER_EPIE4            (0x10)
#define MCF_EPORT_EPIER_EPIE5            (0x20)
#define MCF_EPORT_EPIER_EPIE6            (0x40)
#define MCF_EPORT_EPIER_EPIE7            (0x80)

/* Bit definitions and macros for MCF_EPORT_EPDR */
#define MCF_EPORT_EPDR_EPD1              (0x02)
#define MCF_EPORT_EPDR_EPD2              (0x04)
#define MCF_EPORT_EPDR_EPD3              (0x08)
#define MCF_EPORT_EPDR_EPD4              (0x10)
#define MCF_EPORT_EPDR_EPD5              (0x20)
#define MCF_EPORT_EPDR_EPD6              (0x40)
#define MCF_EPORT_EPDR_EPD7              (0x80)

/* Bit definitions and macros for MCF_EPORT_EPPDR */
#define MCF_EPORT_EPPDR_EPPD1            (0x02)
#define MCF_EPORT_EPPDR_EPPD2            (0x04)
#define MCF_EPORT_EPPDR_EPPD3            (0x08)
#define MCF_EPORT_EPPDR_EPPD4            (0x10)
#define MCF_EPORT_EPPDR_EPPD5            (0x20)
#define MCF_EPORT_EPPDR_EPPD6            (0x40)
#define MCF_EPORT_EPPDR_EPPD7            (0x80)

/* Bit definitions and macros for MCF_EPORT_EPFR */
#define MCF_EPORT_EPFR_EPF1              (0x02)
#define MCF_EPORT_EPFR_EPF2              (0x04)
#define MCF_EPORT_EPFR_EPF3              (0x08)
#define MCF_EPORT_EPFR_EPF4              (0x10)
#define MCF_EPORT_EPFR_EPF5              (0x20)
#define MCF_EPORT_EPFR_EPF6              (0x40)
#define MCF_EPORT_EPFR_EPF7              (0x80)

#endif /* __MCF548X_EPORT_H__ */

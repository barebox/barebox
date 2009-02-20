/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _IMX_REGS_H
#define _IMX_REGS_H

/* ------------------------------------------------------------------------
 *  Motorola IMX system registers
 * ------------------------------------------------------------------------
 */

# ifndef __ASSEMBLY__
# define __REG(x)	(*((volatile u32 *)(x)))
# define __REG16(x)     (*(volatile u16 *)(x))
# define __REG2(x,y)    (*(volatile u32 *)((u32)&__REG(x) + (y)))
# else
#  define __REG(x) (x)
#  define __REG16(x) (x)
#  define __REG2(x,y) ((x)+(y))
#endif

#ifdef CONFIG_ARCH_IMX1
# include <asm/arch/imx1-regs.h>
#elif defined CONFIG_ARCH_IMX21
# include <asm/arch/imx21-regs.h>
#elif defined CONFIG_ARCH_IMX27
# include <asm/arch/imx27-regs.h>
#elif defined CONFIG_ARCH_IMX31
# include <asm/arch/imx31-regs.h>
#elif defined CONFIG_ARCH_IMX35
# include <asm/arch/imx35-regs.h>
#elif defined CONFIG_ARCH_IMX25
# include <asm/arch/imx25-regs.h>
#else
# error "unknown i.MX soc type"
#endif

/*
 *  GPIO Module and I/O Multiplexer
 *  x = 0..3 for reg_A, reg_B, reg_C, reg_D
 *
 *  i.MX1 and i.MXL: 0 <= x <= 3
 *  i.MX27         : 0 <= x <= 5
 */
#define DDIR(x)    __REG2(IMX_GPIO_BASE + 0x00, ((x) & 7) << 8)
#define OCR1(x)    __REG2(IMX_GPIO_BASE + 0x04, ((x) & 7) << 8)
#define OCR2(x)    __REG2(IMX_GPIO_BASE + 0x08, ((x) & 7) << 8)
#define ICONFA1(x) __REG2(IMX_GPIO_BASE + 0x0c, ((x) & 7) << 8)
#define ICONFA2(x) __REG2(IMX_GPIO_BASE + 0x10, ((x) & 7) << 8)
#define ICONFB1(x) __REG2(IMX_GPIO_BASE + 0x14, ((x) & 7) << 8)
#define ICONFB2(x) __REG2(IMX_GPIO_BASE + 0x18, ((x) & 7) << 8)
#define DR(x)      __REG2(IMX_GPIO_BASE + 0x1c, ((x) & 7) << 8)
#define GIUS(x)    __REG2(IMX_GPIO_BASE + 0x20, ((x) & 7) << 8)
#define SSR(x)     __REG2(IMX_GPIO_BASE + 0x24, ((x) & 7) << 8)
#define ICR1(x)    __REG2(IMX_GPIO_BASE + 0x28, ((x) & 7) << 8)
#define ICR2(x)    __REG2(IMX_GPIO_BASE + 0x2c, ((x) & 7) << 8)
#define IMR(x)     __REG2(IMX_GPIO_BASE + 0x30, ((x) & 7) << 8)
#define ISR(x)     __REG2(IMX_GPIO_BASE + 0x34, ((x) & 7) << 8)
#define GPR(x)     __REG2(IMX_GPIO_BASE + 0x38, ((x) & 7) << 8)
#define SWR(x)     __REG2(IMX_GPIO_BASE + 0x3c, ((x) & 7) << 8)
#define PUEN(x)    __REG2(IMX_GPIO_BASE + 0x40, ((x) & 7) << 8)

#define GPIO_PIN_MASK 0x1f

#define GPIO_PORT_SHIFT 5
#define GPIO_PORT_MASK (0x7 << GPIO_PORT_SHIFT)

#define GPIO_PORTA (0 << GPIO_PORT_SHIFT)
#define GPIO_PORTB (1 << GPIO_PORT_SHIFT)
#define GPIO_PORTC (2 << GPIO_PORT_SHIFT)
#define GPIO_PORTD (3 << GPIO_PORT_SHIFT)
#define GPIO_PORTE (4 << GPIO_PORT_SHIFT)
#define GPIO_PORTF (5 << GPIO_PORT_SHIFT)

#define GPIO_OUT   (1 << 8)
#define GPIO_IN    (0 << 8)
#define GPIO_PUEN  (1 << 9)

#define GPIO_PF    (1 << 10)
#define GPIO_AF    (1 << 11)

#define GPIO_OCR_SHIFT 12
#define GPIO_OCR_MASK (3 << GPIO_OCR_SHIFT)
#define GPIO_AIN   (0 << GPIO_OCR_SHIFT)
#define GPIO_BIN   (1 << GPIO_OCR_SHIFT)
#define GPIO_CIN   (2 << GPIO_OCR_SHIFT)
#define GPIO_GPIO  (3 << GPIO_OCR_SHIFT)

#define GPIO_AOUT_SHIFT 14
#define GPIO_AOUT_MASK (3 << GPIO_AOUT_SHIFT)
#define GPIO_AOUT     (0 << GPIO_AOUT_SHIFT)
#define GPIO_AOUT_ISR (1 << GPIO_AOUT_SHIFT)
#define GPIO_AOUT_0   (2 << GPIO_AOUT_SHIFT)
#define GPIO_AOUT_1   (3 << GPIO_AOUT_SHIFT)

#define GPIO_BOUT_SHIFT 16
#define GPIO_BOUT_MASK (3 << GPIO_BOUT_SHIFT)
#define GPIO_BOUT      (0 << GPIO_BOUT_SHIFT)
#define GPIO_BOUT_ISR  (1 << GPIO_BOUT_SHIFT)
#define GPIO_BOUT_0    (2 << GPIO_BOUT_SHIFT)
#define GPIO_BOUT_1    (3 << GPIO_BOUT_SHIFT)

#define GPIO_GIUS      (1<<16)

#endif				/* _IMX_REGS_H */

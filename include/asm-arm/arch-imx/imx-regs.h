#ifndef _IMX_REGS_H
#define _IMX_REGS_H
/* ------------------------------------------------------------------------
 *  Motorola IMX system registers
 * ------------------------------------------------------------------------
 *
 */

# ifndef __ASSEMBLY__
# define __REG(x)	(*((volatile u32 *)(x)))
# define __REG2(x,y)        (*(volatile u32 *)((u32)&__REG(x) + (y)))
# else
#  define __REG(x) (x)
#  define __REG2(x,y) ((x)+(y))
#endif

#ifdef CONFIG_ARCH_IMX1
#include <asm/arch/imx1-regs.h>
#elif defined CONFIG_ARCH_IMX27
#include <asm/arch/imx27-regs.h>
#else
#error "unknown i.MX soc type"
#endif

/*
 *  GPIO Module and I/O Multiplexer
 *  x = 0..3 for reg_A, reg_B, reg_C, reg_D
 *
 *  i.MX1 and i.MXL: 0 <= x <= 3
 *  i.MX27         : 0 <= x <= 5
 */
#define DDIR(x)    __REG2(IMX_GPIO_BASE + 0x00, ((x) & 3) << 8)
#define OCR1(x)    __REG2(IMX_GPIO_BASE + 0x04, ((x) & 3) << 8)
#define OCR2(x)    __REG2(IMX_GPIO_BASE + 0x08, ((x) & 3) << 8)
#define ICONFA1(x) __REG2(IMX_GPIO_BASE + 0x0c, ((x) & 3) << 8)
#define ICONFA2(x) __REG2(IMX_GPIO_BASE + 0x10, ((x) & 3) << 8)
#define ICONFB1(x) __REG2(IMX_GPIO_BASE + 0x14, ((x) & 3) << 8)
#define ICONFB2(x) __REG2(IMX_GPIO_BASE + 0x18, ((x) & 3) << 8)
#define DR(x)      __REG2(IMX_GPIO_BASE + 0x1c, ((x) & 3) << 8)
#define GIUS(x)    __REG2(IMX_GPIO_BASE + 0x20, ((x) & 3) << 8)
#define SSR(x)     __REG2(IMX_GPIO_BASE + 0x24, ((x) & 3) << 8)
#define ICR1(x)    __REG2(IMX_GPIO_BASE + 0x28, ((x) & 3) << 8)
#define ICR2(x)    __REG2(IMX_GPIO_BASE + 0x2c, ((x) & 3) << 8)
#define IMR(x)     __REG2(IMX_GPIO_BASE + 0x30, ((x) & 3) << 8)
#define ISR(x)     __REG2(IMX_GPIO_BASE + 0x34, ((x) & 3) << 8)
#define GPR(x)     __REG2(IMX_GPIO_BASE + 0x38, ((x) & 3) << 8)
#define SWR(x)     __REG2(IMX_GPIO_BASE + 0x3c, ((x) & 3) << 8)
#define PUEN(x)    __REG2(IMX_GPIO_BASE + 0x40, ((x) & 3) << 8)

#define GPIO_PIN_MASK 0x1f
#define GPIO_PORT_MASK (0x7 << 5)

#define GPIO_PORTA (0 << 5)
#define GPIO_PORTB (1 << 5)
#define GPIO_PORTC (2 << 5)
#define GPIO_PORTD (3 << 5)
#define GPIO_PORTE (4 << 5)
#define GPIO_PORTF (5 << 5)

#define GPIO_OUT   (1 << 8)
#define GPIO_IN    (0 << 8)
#define GPIO_PUEN  (1 << 9)

#define GPIO_PF    (0 << 10)
#define GPIO_AF    (1 << 10)

#define GPIO_OCR_MASK (3 << 11)
#define GPIO_AIN   (0 << 11)
#define GPIO_BIN   (1 << 11)
#define GPIO_CIN   (2 << 11)
#define GPIO_GPIO  (3 << 11)

#define GPIO_AOUT  (1 << 13)
#define GPIO_BOUT  (1 << 14)

/* General purpose timers registers */
#define GPT_TCTL   0x00
#define GPT_TPRER  0x04
#define GPT_TCMP   0x08
#define GPT_TCR    0x0c
#define GPT_TCN    0x10
#define GPT_TSTAT  0x14

/* General purpose timers bitfields */
#define TCTL_SWR       (1<<15) /* Software reset */
#define TCTL_FRR       (1<<8)  /* Freerun / restart */
#define TCTL_CAP       (3<<6)  /* Capture Edge */
#define TCTL_OM        (1<<5)  /* output mode */
#define TCTL_IRQEN     (1<<4)  /* interrupt enable */
#define TCTL_CLKSOURCE (7<<1)  /* Clock source */
#define TCTL_TEN       (1)     /* Timer enable */
#define TPRER_PRES     (0xff)  /* Prescale */
#define TSTAT_CAPT     (1<<1)  /* Capture event */
#define TSTAT_COMP     (1)     /* Compare event */

#endif				/* _IMX_REGS_H */

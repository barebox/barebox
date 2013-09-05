#ifndef __MACH_IOMUX_V1_H__
#define __MACH_IOMUX_V1_H__

#include <linux/compiler.h>

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

#define GPIO_GIUS      (1 << 16)

void imx_iomuxv1_init(void __iomem *base);
void imx_gpio_mode(int gpio_mode);

#endif /* __MACH_IOMUX_V1_H__ */

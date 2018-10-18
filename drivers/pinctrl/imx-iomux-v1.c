#include <common.h>
#include <io.h>
#include <init.h>
#include <malloc.h>
#include <pinctrl.h>
#include <mach/iomux-v1.h>
#include <linux/err.h>

/*
 *  GPIO Module and I/O Multiplexer
 *  x = 0..3 for reg_A, reg_B, reg_C, reg_D
 *
 *  i.MX1 and i.MXL: 0 <= x <= 3
 *  i.MX27         : 0 <= x <= 5
 */
#define DDIR    0x00
#define OCR1    0x04
#define OCR2    0x08
#define ICONFA1 0x0c
#define ICONFA2 0x10
#define ICONFB1 0x14
#define ICONFB2 0x18
#define DR      0x1c
#define GIUS    0x20
#define SSR     0x24
#define ICR1    0x28
#define ICR2    0x2c
#define IMR     0x30
#define ISR     0x34
#define GPR     0x38
#define SWR     0x3c
#define PUEN    0x40

static void __iomem *iomuxv1_base;

struct imx_iomux_v1 {
	void __iomem *base;
	struct pinctrl_device pinctrl;
};

void imx_gpio_mode(int gpio_mode)
{
	unsigned int pin = gpio_mode & GPIO_PIN_MASK;
	unsigned int port = (gpio_mode & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	unsigned int ocr = (gpio_mode & GPIO_OCR_MASK) >> GPIO_OCR_SHIFT;
	unsigned int aout = (gpio_mode & GPIO_AOUT_MASK) >> GPIO_AOUT_SHIFT;
	unsigned int bout = (gpio_mode & GPIO_BOUT_MASK) >> GPIO_BOUT_SHIFT;
	void __iomem *portbase = iomuxv1_base + port * 0x100;
	uint32_t val;

	if (!iomuxv1_base)
		return;

	/* Pullup enable */
	val = readl(portbase + PUEN);
	if (gpio_mode & GPIO_PUEN)
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + PUEN);

	/* Data direction */
	val = readl(portbase + DDIR);
	if (gpio_mode & GPIO_OUT)
		val |= 1 << pin;
	else
		val &= ~(1 << pin);
	writel(val, portbase + DDIR);

	/* Primary / alternate function */
	val = readl(portbase + GPR);
	if (gpio_mode & GPIO_AF)
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + GPR);

	/* use as gpio? */
	val = readl(portbase + GIUS);
	if (!(gpio_mode & (GPIO_PF | GPIO_AF)))
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + GIUS);

	/* Output / input configuration */
	if (pin < 16) {
		val = readl(portbase + OCR1);
		val &= ~(3 << (pin * 2));
		val |= (ocr << (pin * 2));
		writel(val, portbase + OCR1);

		val = readl(portbase + ICONFA1);
		val &= ~(3 << (pin * 2));
		val |= aout << (pin * 2);
		writel(val, portbase + ICONFA1);

		val = readl(portbase + ICONFB1);
		val &= ~(3 << (pin * 2));
		val |= bout << (pin * 2);
		writel(val, portbase + ICONFB1);
	} else {
		pin -= 16;

		val = readl(portbase + OCR2);
		val &= ~(3 << (pin * 2));
		val |= (ocr << (pin * 2));
		writel(val, portbase + OCR2);

		val = readl(portbase + ICONFA2);
		val &= ~(3 << (pin * 2));
		val |= aout << (pin * 2);
		writel(val, portbase + ICONFA2);

		val = readl(portbase + ICONFB2);
		val &= ~(3 << (pin * 2));
		val |= bout << (pin * 2);
		writel(val, portbase + ICONFB2);
	}
}

void imx_iomuxv1_init(void __iomem *base)
{
	iomuxv1_base = base;
}

/*
 * MUX_ID format defines
 */
#define MX1_MUX_FUNCTION(val) (BIT(0) & val)
#define MX1_MUX_GPIO(val) ((BIT(1) & val) >> 1)
#define MX1_MUX_DIR(val) ((BIT(2) & val) >> 2)
#define MX1_MUX_OCONF(val) (((BIT(4) | BIT(5)) & val) >> 4)
#define MX1_MUX_ICONFA(val) (((BIT(8) | BIT(9)) & val) >> 8)
#define MX1_MUX_ICONFB(val) (((BIT(10) | BIT(11)) & val) >> 10)

#define MX1_PORT_STRIDE 0x100

/*
 * IMX1 IOMUXC manages the pins based on ports. Each port has 32 pins. IOMUX
 * control register are seperated into function, output configuration, input
 * configuration A, input configuration B, GPIO in use and data direction.
 *
 * Those controls that are represented by 1 bit have a direct mapping between
 * bit position and pin id. If they are represented by 2 bit, the lower 16 pins
 * are in the first register and the upper 16 pins in the second (next)
 * register. pin_id is stored in bit (pin_id%16)*2 and the bit above.
 */

/*
 * Calculates the register offset from a pin_id
 */
static void __iomem *imx1_mem(struct imx_iomux_v1 *iomux, unsigned int pin_id)
{
	unsigned int port = pin_id / 32;
	return iomux->base + port * MX1_PORT_STRIDE;
}

/*
 * Write to a register with 2 bits per pin. The function will automatically
 * use the next register if the pin is managed in the second register.
 */
static void imx1_write_2bit(struct imx_iomux_v1 *iomux, unsigned int pin_id,
		u32 value, u32 reg_offset)
{
	void __iomem *reg = imx1_mem(iomux, pin_id) + reg_offset;
	int offset = (pin_id % 16) * 2; /* offset, regardless of register used */
	int mask = ~(0x3 << offset); /* Mask for 2 bits at offset */
	u32 old_val;
	u32 new_val;

	dev_dbg(iomux->pinctrl.dev, "write: register 0x%p offset %d value 0x%x\n",
			reg, offset, value);

	/* Use the next register if the pin's port pin number is >=16 */
	if (pin_id % 32 >= 16)
		reg += 0x04;

	/* Get current state of pins */
	old_val = readl(reg);
	old_val &= mask;

	new_val = value & 0x3; /* Make sure value is really 2 bit */
	new_val <<= offset;
	new_val |= old_val;/* Set new state for pin_id */

	writel(new_val, reg);
}

static void imx1_write_bit(struct imx_iomux_v1 *iomux, unsigned int pin_id,
		u32 value, u32 reg_offset)
{
	void __iomem *reg = imx1_mem(iomux, pin_id) + reg_offset;
	int offset = pin_id % 32;
	int mask = ~BIT_MASK(offset);
	u32 old_val;
	u32 new_val;

	/* Get current state of pins */
	old_val = readl(reg);
	old_val &= mask;

	new_val = value & 0x1; /* Make sure value is really 1 bit */
	new_val <<= offset;
	new_val |= old_val;/* Set new state for pin_id */

	writel(new_val, reg);
}

static int imx_iomux_v1_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct imx_iomux_v1 *iomux = container_of(pdev, struct imx_iomux_v1, pinctrl);
	const __be32 *list;
	int npins, size, i;

	dev_dbg(iomux->pinctrl.dev, "set state: %s\n", np->full_name);

	list = of_get_property(np, "fsl,pins", &size);
	if (!list)
		return -EINVAL;

	npins = size / 12;

	for (i = 0; i < npins; i++) {
		unsigned int pin_id = be32_to_cpu(*list++);
		unsigned int mux = be32_to_cpu(*list++);
		unsigned int config = be32_to_cpu(*list++);
		unsigned int afunction = MX1_MUX_FUNCTION(mux);
		unsigned int gpio_in_use = MX1_MUX_GPIO(mux);
		unsigned int direction = MX1_MUX_DIR(mux);
		unsigned int gpio_oconf = MX1_MUX_OCONF(mux);
		unsigned int gpio_iconfa = MX1_MUX_ICONFA(mux);
		unsigned int gpio_iconfb = MX1_MUX_ICONFB(mux);

		dev_dbg(pdev->dev, "%s, pin 0x%x, function %d, gpio %d, direction %d, oconf %d, iconfa %d, iconfb %d\n",
				np->full_name, pin_id, afunction, gpio_in_use,
				direction, gpio_oconf, gpio_iconfa,
				gpio_iconfb);

		imx1_write_bit(iomux, pin_id, gpio_in_use, GIUS);
		imx1_write_bit(iomux, pin_id, direction, DDIR);

		if (gpio_in_use) {
			imx1_write_2bit(iomux, pin_id, gpio_oconf, OCR1);
			imx1_write_2bit(iomux, pin_id, gpio_iconfa, ICONFA1);
			imx1_write_2bit(iomux, pin_id, gpio_iconfb, ICONFB1);
		} else {
			imx1_write_bit(iomux, pin_id, afunction, GPR);
		}

		imx1_write_bit(iomux, pin_id, config & 0x01, PUEN);
	}

	return 0;
}

static struct pinctrl_ops imx_iomux_v1_ops = {
	.set_state = imx_iomux_v1_set_state,
};

static int imx_pinctrl_dt(struct device_d *dev, void __iomem *base)
{
	struct imx_iomux_v1 *iomux;
	int ret;

	iomux = xzalloc(sizeof(*iomux));

	iomux->base = base;

	iomux->pinctrl.dev = dev;
	iomux->pinctrl.ops = &imx_iomux_v1_ops;

	ret = pinctrl_register(&iomux->pinctrl);
	if (ret)
		free(iomux);

	return ret;
}

static int imx_iomux_v1_probe(struct device_d *dev)
{
	int ret = 0;

	if (iomuxv1_base)
		return -EBUSY;

	iomuxv1_base = dev_get_mem_region(dev, 0);
	if (IS_ERR(iomuxv1_base))
		return PTR_ERR(iomuxv1_base);

	ret = of_platform_populate(dev->device_node, NULL, NULL);

	if (IS_ENABLED(CONFIG_PINCTRL) && dev->device_node)
		ret = imx_pinctrl_dt(dev, iomuxv1_base);

	return ret;
}

static __maybe_unused struct of_device_id imx_iomux_v1_dt_ids[] = {
	{
		.compatible = "fsl,imx27-iomuxc",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_iomux_v1_driver = {
	.name		= "imx-iomuxv1",
	.probe		= imx_iomux_v1_probe,
	.of_compatible	= DRV_OF_COMPAT(imx_iomux_v1_dt_ids),
};

static int imx_iomux_v1_init(void)
{
	return platform_driver_register(&imx_iomux_v1_driver);
}
core_initcall(imx_iomux_v1_init);

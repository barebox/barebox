#include <common.h>
#include <console.h>
#include <init.h>
#include <fs.h>
#include <driver.h>
#include <io.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <linux/stat.h>
#include <generated/mach-types.h>
#include <mach/silicon.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/control.h>
#include <usb/ehci.h>
#include <linux/err.h>
#include <sizes.h>
#include <asm/mmu.h>
#include <mach/gpio.h>
#include <environment.h>
#include <mach/xload.h>
#include <i2c/i2c.h>
#include <gpio.h>
#include <led.h>

static int board_revision;

#define GPIO_HUB_POWER 1
#define GPIO_HUB_NRESET_39 39
#define GPIO_HUB_NRESET_62 62
#define GPIO_BOARD_ID0 182
#define GPIO_BOARD_ID1 101
#define GPIO_BOARD_ID2 171

static struct NS16550_plat serial_plat = {
	.clock = 48000000,      /* 48MHz (APLL96/2) */
	.shift = 2,
};

static int panda_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, OMAP44XX_UART3_BASE, 1024,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(panda_console_init);

static int panda_mem_init(void)
{
	arm_add_mem_device("ram0", 0x80000000, SZ_1G);

	return 0;
}
mem_initcall(panda_mem_init);

#ifdef CONFIG_USB_EHCI
static struct ehci_platform_data ehci_pdata = {
	.flags = 0,
};

static void panda_ehci_init(void)
{
	u32 val;
	int hub_nreset;

	if (board_revision)
		hub_nreset = GPIO_HUB_NRESET_62;
	else
		hub_nreset = GPIO_HUB_NRESET_39;

	/* disable the power to the usb hub prior to init */
	gpio_direction_output(GPIO_HUB_POWER, 0);
	gpio_set_value(GPIO_HUB_POWER, 0);

	/* reset phy+hub */
	gpio_direction_output(hub_nreset, 0);
	gpio_set_value(hub_nreset, 0);
	gpio_set_value(hub_nreset, 1);
	val = readl(0x4a009358);
	val |= (1 << 24);
	val |= 0x2;
	writel(val, 0x4a009358);
	writel(0x7, 0x4a008180);
	mdelay(10);

	writel(0x00000014, 0x4a064010);
	writel(0x8000001c, 0x4a064040);

	/* enable power to hub */
	gpio_set_value(GPIO_HUB_POWER, 1);

	add_usb_ehci_device(-1, 0x4a064c00,
			    0x4a064c00 + 0x10, &ehci_pdata);
}
#else
static void panda_ehci_init(void)
{}
#endif

static void __init panda_boardrev_init(void)
{
	board_revision = gpio_get_value(GPIO_BOARD_ID0);
	board_revision |= (gpio_get_value(GPIO_BOARD_ID1)<<1);
	board_revision |= (gpio_get_value(GPIO_BOARD_ID2)<<2);

	pr_info("PandaBoard Revision: %03d\n", board_revision);
}

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("twl6030", 0x48),
	},
};

struct gpio_led panda_leds[] = {
	{
		.gpio = 7,
		.led = {
			.name = "heartbeat",
		},
	},
};

static void panda_led_init(void)
{
	gpio_direction_output(7, 0);
	led_gpio_register(&panda_leds[0]);
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &panda_leds[0].led);
}

static int panda_devices_init(void)
{
	panda_boardrev_init();

	if (gpio_get_value(182)) {
		/* enable software ioreq */
		sr32(OMAP44XX_SCRM_AUXCLK3, 8, 1, 0x1);
		/* set for sys_clk (38.4MHz) */
		sr32(OMAP44XX_SCRM_AUXCLK3, 1, 2, 0x0);
		/* set divisor to 2 */
		sr32(OMAP44XX_SCRM_AUXCLK3, 16, 4, 0x1);
		/* set the clock source to active */
		sr32(OMAP44XX_SCRM_ALTCLKSRC, 0, 1, 0x1);
		/* enable clocks */
		sr32(OMAP44XX_SCRM_ALTCLKSRC, 2, 2, 0x3);
	} else {
		/* enable software ioreq */
		sr32(OMAP44XX_SCRM_AUXCLK1, 8, 1, 0x1);
		/* set for PER_DPLL */
		sr32(OMAP44XX_SCRM_AUXCLK1, 1, 2, 0x2);
		/* set divisor to 16 */
		sr32(OMAP44XX_SCRM_AUXCLK1, 16, 4, 0xf);
		/* set the clock source to active */
		sr32(OMAP44XX_SCRM_ALTCLKSRC, 0, 1, 0x1);
		/* enable clocks */
		sr32(OMAP44XX_SCRM_ALTCLKSRC, 2, 2, 0x3);
	}

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	add_generic_device("i2c-omap", DEVICE_ID_DYNAMIC,
				NULL, 0x48070000, 0x1000,
				IORESOURCE_MEM, NULL);


	add_generic_device("omap-hsmmc", DEVICE_ID_DYNAMIC, NULL, 0x4809C100, SZ_4K,
			   IORESOURCE_MEM, NULL);
	panda_ehci_init();

	panda_led_init();
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_OMAP4_PANDA);

	return 0;
}
device_initcall(panda_devices_init);

#ifdef CONFIG_DEFAULT_ENVIRONMENT
static int panda_env_init(void)
{
	struct stat s;
	char *diskdev = "/dev/disk0.0";
	int ret;

	ret = stat(diskdev, &s);
	if (ret) {
		printf("no %s. using default env\n", diskdev);
		return 0;
	}

	mkdir ("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot");
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	default_environment_path = "/boot/bareboxenv";

	return 0;
}
late_initcall(panda_env_init);
#endif

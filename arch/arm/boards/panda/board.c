#include <common.h>
#include <console.h>
#include <init.h>
#include <fs.h>
#include <driver.h>
#include <asm/io.h>
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

static int board_revision;

#define GPIO_HUB_POWER 1
#define GPIO_HUB_NRESET_39 39
#define GPIO_HUB_NRESET_62 62
#define GPIO_BOARD_ID0 182
#define GPIO_BOARD_ID1 101
#define GPIO_BOARD_ID2 171

static struct NS16550_plat serial_plat = {
	.clock = 48000000,      /* 48MHz (APLL96/2) */
	.f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR,
	.reg_read = omap_uart_read,
	.reg_write = omap_uart_write,
};

static struct device_d panda_serial_device = {
	.id = -1,
	.name = "serial_ns16550",
	.map_base = OMAP44XX_UART3_BASE,
	.size = 1024,
	.platform_data = (void *)&serial_plat,
};

static int panda_console_init(void)
{
	/* Register the serial port */
	return register_device(&panda_serial_device);
}
console_initcall(panda_console_init);

static struct memory_platform_data sram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id = -1,
	.name = "mem",
	.map_base = 0x80000000,
	.size = SZ_1G,
	.platform_data = &sram_pdata,
};

#ifdef CONFIG_MMU
static int panda_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 256, PMD_SECT_DEF_CACHED);
	arm_create_section(0x90000000, 0x80000000, 256, PMD_SECT_DEF_UNCACHED);

	mmu_enable();

	return 0;
}
device_initcall(panda_mmu_init);
#endif

static struct ehci_platform_data ehci_pdata = {
	.flags = 0,
	.hccr_offset = 0x0,
	.hcor_offset = 0x10,
};

static struct device_d usbh_dev = {
	.id	  = -1,
	.name     = "ehci",
	.map_base = 0x4a064c00,
	.size     = 4 * 1024,
	.platform_data = &ehci_pdata,
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
	writel(val, 0x4a009358);
	writel(0x7, 0x4a008180);
	mdelay(10);

	writel(0x00000014, 0x4a064010);
	writel(0x8000001c, 0x4a064040);

	/* enable power to hub */
	gpio_set_value(GPIO_HUB_POWER, 1);

	register_device(&usbh_dev);
}

static void __init panda_boardrev_init(void)
{
	board_revision = gpio_get_value(GPIO_BOARD_ID0);
	board_revision |= (gpio_get_value(GPIO_BOARD_ID1)<<1);
	board_revision |= (gpio_get_value(GPIO_BOARD_ID2)<<2);

	pr_info("PandaBoard Revision: %03d\n", board_revision);
}

static struct device_d hsmmc_dev = {
	.id = -1,
	.name = "omap-hsmmc",
	.map_base = 0x4809C100,
	.size = SZ_4K,
};

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

	register_device(&sdram_dev);
	register_device(&hsmmc_dev);
	panda_ehci_init();

	armlinux_add_dram(&sdram_dev);
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


#ifdef CONFIG_SHELL_NONE
int run_shell(void)
{
	int (*func)(void);

	func = omap_xload_boot_mmc();
	if (!func) {
		printf("booting failed\n");
		while (1);
	}

	shutdown_barebox();
	func();

	while (1);
}
#endif

// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <mach/core.h>
#include <mach/mbox.h>
#include <mach/platform.h>
#include <dt-bindings/clock/bcm2835.h>

#define BCM2711_CLOCK_END			(BCM2711_CLOCK_EMMC2 + 1)

static struct clk *clks[BCM2711_CLOCK_END];
static struct clk_onecell_data clk_data;

struct msg_get_clock_rate {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_clock_rate get_clock_rate;
	u32 end_tag;
};

static struct clk *rpi_register_firmware_clock(u32 clock_id, const char *name)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_clock_rate, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_clock_rate, GET_CLOCK_RATE);
	msg->get_clock_rate.body.req.clock_id = clock_id;

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret)
		return ERR_PTR(ret);

	return clk_fixed(name, msg->get_clock_rate.body.resp.rate_hz);
}

static int bcm2835_cprman_probe(struct device_d *dev)
{
	struct clk *clk_cs;

	clks[BCM2835_CLOCK_EMMC] =
		rpi_register_firmware_clock(BCM2835_MBOX_CLOCK_ID_EMMC,
					 "bcm2835_mci0");
	if (IS_ERR(clks[BCM2835_CLOCK_EMMC]))
		return PTR_ERR(clks[BCM2835_CLOCK_EMMC]);

	clks[BCM2835_CLOCK_VPU] =
		rpi_register_firmware_clock(BCM2835_MBOX_CLOCK_ID_CORE,
					    "vpu");
	if (IS_ERR(clks[BCM2835_CLOCK_VPU]))
		return PTR_ERR(clks[BCM2835_CLOCK_VPU]);

	clks[BCM2835_CLOCK_UART] = clk_fixed("uart0-pl0110", 48 * 1000 * 1000);
	clk_register_clkdev(clks[BCM2835_CLOCK_UART], NULL, "uart0-pl0110");

	clk_cs = clk_fixed("bcm2835-cs", 1 * 1000 * 1000);
	clk_register_clkdev(clk_cs, NULL, "bcm2835-cs");

	clk_data.clks = clks;
	clk_data.clk_num = BCM2711_CLOCK_END;
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get, &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id bcm2835_cprman_dt_ids[] = {
	{
		.compatible = "brcm,bcm2835-cprman",
	}, {
		/* sentinel */
	}
};

static struct driver_d bcm2835_cprman_driver = {
	.probe	= bcm2835_cprman_probe,
	.name	= "bcm2835-cprman",
	.of_compatible = DRV_OF_COMPAT(bcm2835_cprman_dt_ids),
};
core_platform_driver(bcm2835_cprman_driver);

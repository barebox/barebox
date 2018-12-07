/*
 * OMAP GPMC driver. Based upon the corresponding Linux Code
 *
 * Copyright (C) 2013 Sascha Hauer, Pengutronix, <s.hauer@pengutronix.de>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/sizes.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <of_mtd.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/gpmc_nand.h>
#include <mach/gpmc.h>

#define GPMC_CS_NUM	8
#define GPMC_NR_WAITPINS		4

#define GPMC_BURST_4			4	/* 4 word burst */
#define GPMC_BURST_8			8	/* 8 word burst */
#define GPMC_BURST_16			16	/* 16 word burst */
#define GPMC_DEVWIDTH_8BIT		1	/* 8-bit device width */
#define GPMC_DEVWIDTH_16BIT		2	/* 16-bit device width */
#define GPMC_MUX_AAD			1	/* Addr-Addr-Data multiplex */
#define GPMC_MUX_AD			2	/* Addr-Data multiplex */

#define GPMC_CONFIG1_WRAPBURST_SUPP		(1 << 31)
#define GPMC_CONFIG1_READMULTIPLE_SUPP		(1 << 30)
#define GPMC_CONFIG1_READTYPE_ASYNC		(0 << 29)
#define GPMC_CONFIG1_READTYPE_SYNC		(1 << 29)
#define GPMC_CONFIG1_WRITEMULTIPLE_SUPP		(1 << 28)
#define GPMC_CONFIG1_WRITETYPE_ASYNC		(0 << 27)
#define GPMC_CONFIG1_WRITETYPE_SYNC		(1 << 27)
#define GPMC_CONFIG1_CLKACTIVATIONTIME(val)	((val & 3) << 25)
#define GPMC_CONFIG1_PAGE_LEN(val)		((val & 3) << 23)
#define GPMC_CONFIG1_WAIT_READ_MON		(1 << 22)
#define GPMC_CONFIG1_WAIT_WRITE_MON		(1 << 21)
#define GPMC_CONFIG1_WAIT_MON_IIME(val)		((val & 3) << 18)
#define GPMC_CONFIG1_WAIT_PIN_SEL(val)		((val & 3) << 16)
#define GPMC_CONFIG1_DEVICESIZE(val)		((val & 3) << 12)
#define GPMC_CONFIG1_DEVICESIZE_16		GPMC_CONFIG1_DEVICESIZE(1)
#define GPMC_CONFIG1_DEVICETYPE(val)		((val & 3) << 10)
#define GPMC_CONFIG1_DEVICETYPE_NOR		GPMC_CONFIG1_DEVICETYPE(0)
#define GPMC_CONFIG1_MUXTYPE(val)		((val & 3) << 8)
#define GPMC_CONFIG1_TIME_PARA_GRAN		(1 << 4)
#define GPMC_CONFIG1_FCLK_DIV(val)		(val & 3)
#define GPMC_CONFIG1_FCLK_DIV2			(GPMC_CONFIG1_FCLK_DIV(1))
#define GPMC_CONFIG1_FCLK_DIV3			(GPMC_CONFIG1_FCLK_DIV(2))
#define GPMC_CONFIG1_FCLK_DIV4			(GPMC_CONFIG1_FCLK_DIV(3))
#define	GPMC_CONFIG2_CSEXTRADELAY		(1 << 7)
#define	GPMC_CONFIG3_ADVEXTRADELAY		(1 << 7)
#define	GPMC_CONFIG4_OEEXTRADELAY		(1 << 7)
#define	GPMC_CONFIG4_WEEXTRADELAY		(1 << 23)
#define	GPMC_CONFIG6_CYCLE2CYCLEDIFFCSEN	(1 << 6)
#define	GPMC_CONFIG6_CYCLE2CYCLESAMECSEN	(1 << 7)
#define GPMC_CONFIG7_CSVALID			(1 << 6)

#define GPMC_DEVICETYPE_NOR		0
#define GPMC_DEVICETYPE_NAND		2

static unsigned int gpmc_cs_num = GPMC_CS_NUM;
static unsigned int gpmc_nr_waitpins;
static unsigned long gpmc_l3_clk = 100000000; /* This should be a proper clock */
static void __iomem *gpmc_base;

/* bool type time settings */
struct gpmc_bool_timings {
	bool cycle2cyclediffcsen;
	bool cycle2cyclesamecsen;
	bool we_extra_delay;
	bool oe_extra_delay;
	bool adv_extra_delay;
	bool cs_extra_delay;
	bool time_para_granularity;
};

/*
 * Note that all values in this struct are in nanoseconds except sync_clk
 * (which is in picoseconds), while the register values are in gpmc_fck cycles.
 */
struct gpmc_timings {
	/* Minimum clock period for synchronous mode (in picoseconds) */
	u32 sync_clk;

	/* Chip-select signal timings corresponding to 1 */
	u32 cs_on;		/* Assertion time */
	u32 cs_rd_off;		/* Read deassertion time */
	u32 cs_wr_off;		/* Write deassertion time */

	/* ADV signal timings corresponding to GPMC_CONFIG3 */
	u32 adv_on;		/* Assertion time */
	u32 adv_rd_off;		/* Read deassertion time */
	u32 adv_wr_off;		/* Write deassertion time */

	/* WE signals timings corresponding to GPMC_CONFIG4 */
	u32 we_on;		/* WE assertion time */
	u32 we_off;		/* WE deassertion time */

	/* OE signals timings corresponding to GPMC_CONFIG4 */
	u32 oe_on;		/* OE assertion time */
	u32 oe_off;		/* OE deassertion time */

	/* Access time and cycle time timings corresponding to GPMC_CONFIG5 */
	u32 page_burst_access;	/* Multiple access word delay */
	u32 access;		/* Start-cycle to first data valid delay */
	u32 rd_cycle;		/* Total read cycle time */
	u32 wr_cycle;		/* Total write cycle time */

	u32 bus_turnaround;
	u32 cycle2cycle_delay;

	u32 wait_monitoring;
	u32 clk_activation;

	/* The following are only on OMAP3430 */
	u32 wr_access;		/* WRACCESSTIME */
	u32 wr_data_mux_bus;	/* WRDATAONADMUXBUS */

	struct gpmc_bool_timings bool_timings;
};

struct gpmc_settings {
	bool burst_wrap;	/* enables wrap bursting */
	bool burst_read;	/* enables read page/burst mode */
	bool burst_write;	/* enables write page/burst mode */
	bool device_nand;	/* device is NAND */
	bool sync_read;		/* enables synchronous reads */
	bool sync_write;	/* enables synchronous writes */
	bool wait_on_read;	/* monitor wait on reads */
	bool wait_on_write;	/* monitor wait on writes */
	u32 burst_len;		/* page/burst length */
	u32 device_width;	/* device bus width (8 or 16 bit) */
	u32 mux_add_data;	/* multiplex address & data */
	u32 wait_pin;		/* wait-pin to be used */
};

struct imx_gpmc {
	struct device_d *dev;
	void __iomem *base;
	struct imx_gpmc_devtype *devtype;
};

static void gpmc_cs_bool_timings(struct gpmc_config *gpmc_config, const struct gpmc_bool_timings *p)
{
	if (p->time_para_granularity)
		gpmc_config->cfg[0] |= GPMC_CONFIG1_TIME_PARA_GRAN;

	if (p->cs_extra_delay)
		gpmc_config->cfg[1] |= GPMC_CONFIG2_CSEXTRADELAY;
	if (p->adv_extra_delay)
		gpmc_config->cfg[2] |= GPMC_CONFIG3_ADVEXTRADELAY;
	if (p->oe_extra_delay)
		gpmc_config->cfg[3] |= GPMC_CONFIG4_OEEXTRADELAY;
	if (p->we_extra_delay)
		gpmc_config->cfg[3] |= GPMC_CONFIG4_WEEXTRADELAY;
	if (p->cycle2cyclesamecsen)
		gpmc_config->cfg[5] |= GPMC_CONFIG6_CYCLE2CYCLESAMECSEN;
	if (p->cycle2cyclediffcsen)
		gpmc_config->cfg[5] |= GPMC_CONFIG6_CYCLE2CYCLEDIFFCSEN;
}

static unsigned long gpmc_get_fclk_period(void)
{
	unsigned long rate = gpmc_l3_clk;

	rate /= 1000;
	rate = 1000000000 / rate;	/* In picoseconds */

	return rate;
}

static unsigned int gpmc_ns_to_ticks(unsigned int time_ns)
{
	unsigned long tick_ps;

	/* Calculate in picosecs to yield more exact results */
	tick_ps = gpmc_get_fclk_period();

	return (time_ns * 1000 + tick_ps - 1) / tick_ps;
}

static int gpmc_calc_divider(unsigned int sync_clk)
{
	int div;
	u32 l;

	l = sync_clk + (gpmc_get_fclk_period() - 1);
	div = l / gpmc_get_fclk_period();
	if (div > 4)
		return -1;
	if (div <= 0)
		div = 1;

	return div;
}

static int set_cfg(struct gpmc_config *gpmc_config, int reg,
		int st_bit, int end_bit, int time)
{
	int ticks, mask, nr_bits;

	if (time == 0)
		ticks = 0;
	else
		ticks = gpmc_ns_to_ticks(time);

	nr_bits = end_bit - st_bit + 1;
	if (ticks >= 1 << nr_bits)
		return -EINVAL;

	mask = (1 << nr_bits) - 1;
	gpmc_config->cfg[reg] &= ~(mask << st_bit);
	gpmc_config->cfg[reg] |= ticks << st_bit;

	return 0;
}

static int gpmc_timings_to_config(struct gpmc_config *gpmc_config, const struct gpmc_timings *t)
{
	int div, ret = 0;

	div = gpmc_calc_divider(t->sync_clk);
	if (div < 0)
		return div;

	gpmc_config->cfg[0] |= div - 1;

	ret |= set_cfg(gpmc_config, 0, 18, 19, t->wait_monitoring);
	ret |= set_cfg(gpmc_config, 0, 25, 26, t->clk_activation);

	ret |= set_cfg(gpmc_config, 1,  0,  3, t->cs_on);
	ret |= set_cfg(gpmc_config, 1,  8, 12, t->cs_rd_off);
	ret |= set_cfg(gpmc_config, 1, 16, 20, t->cs_wr_off);

	ret |= set_cfg(gpmc_config, 2,  0,  3, t->adv_on);
	ret |= set_cfg(gpmc_config, 2,  8, 12, t->adv_rd_off);
	ret |= set_cfg(gpmc_config, 2, 16, 20, t->adv_wr_off);

	ret |= set_cfg(gpmc_config, 3,  0,  3, t->oe_on);
	ret |= set_cfg(gpmc_config, 3,  8, 12, t->oe_off);
	ret |= set_cfg(gpmc_config, 3, 16, 19, t->we_on);
	ret |= set_cfg(gpmc_config, 3, 24, 28, t->we_off);

	ret |= set_cfg(gpmc_config, 4,  0,  4, t->rd_cycle);
	ret |= set_cfg(gpmc_config, 4,  8, 12, t->wr_cycle);
	ret |= set_cfg(gpmc_config, 4, 16, 20, t->access);

	ret |= set_cfg(gpmc_config, 4, 24, 27, t->page_burst_access);

	ret |= set_cfg(gpmc_config, 5, 0, 3, t->bus_turnaround);
	ret |= set_cfg(gpmc_config, 5, 8, 11, t->cycle2cycle_delay);
	ret |= set_cfg(gpmc_config, 5, 16, 19, t->wr_data_mux_bus);
	ret |= set_cfg(gpmc_config, 5, 24, 28, t->wr_access);

	if (ret)
		return ret;

	gpmc_cs_bool_timings(gpmc_config, &t->bool_timings);

	return 0;
}

static int gpmc_settings_to_config(struct gpmc_config *gpmc_config,
		struct gpmc_settings *p)
{
	u32 config1 = gpmc_config->cfg[0];

	/* Page/burst mode supports lengths of 4, 8 and 16 bytes */
	if (p->burst_read || p->burst_write) {
		switch (p->burst_len) {
		case GPMC_BURST_4:
		case GPMC_BURST_8:
		case GPMC_BURST_16:
			break;
		default:
			pr_err("%s: invalid page/burst-length (%d)\n",
			       __func__, p->burst_len);
			return -EINVAL;
		}
	}

	if (p->wait_pin > gpmc_nr_waitpins) {
		pr_err("%s: invalid wait-pin (%d)\n", __func__, p->wait_pin);
		return -EINVAL;
	}

	config1 |= GPMC_CONFIG1_DEVICESIZE((p->device_width - 1));

	if (p->sync_read)
		config1 |= GPMC_CONFIG1_READTYPE_SYNC;
	if (p->sync_write)
		config1 |= GPMC_CONFIG1_WRITETYPE_SYNC;
	if (p->wait_on_read)
		config1 |= GPMC_CONFIG1_WAIT_READ_MON;
	if (p->wait_on_write)
		config1 |= GPMC_CONFIG1_WAIT_WRITE_MON;
	if (p->wait_on_read || p->wait_on_write)
		config1 |= GPMC_CONFIG1_WAIT_PIN_SEL(p->wait_pin);
	if (p->device_nand)
		config1	|= GPMC_CONFIG1_DEVICETYPE(GPMC_DEVICETYPE_NAND);
	if (p->mux_add_data)
		config1	|= GPMC_CONFIG1_MUXTYPE(p->mux_add_data);
	if (p->burst_read)
		config1 |= GPMC_CONFIG1_READMULTIPLE_SUPP;
	if (p->burst_write)
		config1 |= GPMC_CONFIG1_WRITEMULTIPLE_SUPP;
	if (p->burst_read || p->burst_write) {
		config1 |= GPMC_CONFIG1_PAGE_LEN(p->burst_len >> 3);
		config1 |= p->burst_wrap ? GPMC_CONFIG1_WRAPBURST_SUPP : 0;
	}

	gpmc_config->cfg[0] = config1;

	return 0;
}

/**
 * gpmc_read_settings_dt - read gpmc settings from device-tree
 * @np:		pointer to device-tree node for a gpmc child device
 * @p:		pointer to gpmc settings structure
 *
 * Reads the GPMC settings for a GPMC child device from device-tree and
 * stores them in the GPMC settings structure passed. The GPMC settings
 * structure is initialised to zero by this function and so any
 * previously stored settings will be cleared.
 */
static void gpmc_read_settings_dt(struct device_node *np, struct gpmc_settings *p)
{
	memset(p, 0, sizeof(struct gpmc_settings));

	p->sync_read = of_property_read_bool(np, "gpmc,sync-read");
	p->sync_write = of_property_read_bool(np, "gpmc,sync-write");
	of_property_read_u32(np, "gpmc,device-width", &p->device_width);
	of_property_read_u32(np, "gpmc,mux-add-data", &p->mux_add_data);

	if (!of_property_read_u32(np, "gpmc,burst-length", &p->burst_len)) {
		p->burst_wrap = of_property_read_bool(np, "gpmc,burst-wrap");
		p->burst_read = of_property_read_bool(np, "gpmc,burst-read");
		p->burst_write = of_property_read_bool(np, "gpmc,burst-write");
		if (!p->burst_read && !p->burst_write)
			pr_warn("%s: page/burst-length set but not used!\n",
				__func__);
	}

	if (!of_property_read_u32(np, "gpmc,wait-pin", &p->wait_pin)) {
		p->wait_on_read = of_property_read_bool(np,
							"gpmc,wait-on-read");
		p->wait_on_write = of_property_read_bool(np,
							 "gpmc,wait-on-write");
		if (!p->wait_on_read && !p->wait_on_write)
			pr_warn("%s: read/write wait monitoring not enabled!\n",
				__func__);
	}
}

static void gpmc_read_timings_dt(struct device_node *np,
						struct gpmc_timings *gpmc_t)
{
	struct gpmc_bool_timings *p;

	if (!np || !gpmc_t)
		return;

	memset(gpmc_t, 0, sizeof(*gpmc_t));

	/* minimum clock period for syncronous mode */
	of_property_read_u32(np, "gpmc,sync-clk-ps", &gpmc_t->sync_clk);

	/* chip select timtings */
	of_property_read_u32(np, "gpmc,cs-on-ns", &gpmc_t->cs_on);
	of_property_read_u32(np, "gpmc,cs-rd-off-ns", &gpmc_t->cs_rd_off);
	of_property_read_u32(np, "gpmc,cs-wr-off-ns", &gpmc_t->cs_wr_off);

	/* ADV signal timings */
	of_property_read_u32(np, "gpmc,adv-on-ns", &gpmc_t->adv_on);
	of_property_read_u32(np, "gpmc,adv-rd-off-ns", &gpmc_t->adv_rd_off);
	of_property_read_u32(np, "gpmc,adv-wr-off-ns", &gpmc_t->adv_wr_off);

	/* WE signal timings */
	of_property_read_u32(np, "gpmc,we-on-ns", &gpmc_t->we_on);
	of_property_read_u32(np, "gpmc,we-off-ns", &gpmc_t->we_off);

	/* OE signal timings */
	of_property_read_u32(np, "gpmc,oe-on-ns", &gpmc_t->oe_on);
	of_property_read_u32(np, "gpmc,oe-off-ns", &gpmc_t->oe_off);

	/* access and cycle timings */
	of_property_read_u32(np, "gpmc,page-burst-access-ns",
			     &gpmc_t->page_burst_access);
	of_property_read_u32(np, "gpmc,access-ns", &gpmc_t->access);
	of_property_read_u32(np, "gpmc,rd-cycle-ns", &gpmc_t->rd_cycle);
	of_property_read_u32(np, "gpmc,wr-cycle-ns", &gpmc_t->wr_cycle);
	of_property_read_u32(np, "gpmc,bus-turnaround-ns",
			     &gpmc_t->bus_turnaround);
	of_property_read_u32(np, "gpmc,cycle2cycle-delay-ns",
			     &gpmc_t->cycle2cycle_delay);
	of_property_read_u32(np, "gpmc,wait-monitoring-ns",
			     &gpmc_t->wait_monitoring);
	of_property_read_u32(np, "gpmc,clk-activation-ns",
			     &gpmc_t->clk_activation);

	/* only applicable to OMAP3+ */
	of_property_read_u32(np, "gpmc,wr-access-ns", &gpmc_t->wr_access);
	of_property_read_u32(np, "gpmc,wr-data-mux-bus-ns",
			     &gpmc_t->wr_data_mux_bus);

	/* bool timing parameters */
	p = &gpmc_t->bool_timings;

	p->cycle2cyclediffcsen =
		of_property_read_bool(np, "gpmc,cycle2cycle-diffcsen");
	p->cycle2cyclesamecsen =
		of_property_read_bool(np, "gpmc,cycle2cycle-samecsen");
	p->we_extra_delay = of_property_read_bool(np, "gpmc,we-extra-delay");
	p->oe_extra_delay = of_property_read_bool(np, "gpmc,oe-extra-delay");
	p->adv_extra_delay = of_property_read_bool(np, "gpmc,adv-extra-delay");
	p->cs_extra_delay = of_property_read_bool(np, "gpmc,cs-extra-delay");
	p->time_para_granularity =
		of_property_read_bool(np, "gpmc,time-para-granularity");
}

struct dt_eccmode {
	const char *name;
	enum gpmc_ecc_mode mode;
};

static struct dt_eccmode modes[] = {
	{
		.name = "ham1",
		.mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	}, {
		.name = "sw",
		.mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	}, {
		.name = "hw",
		.mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	}, {
		.name = "hw-romcode",
		.mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	}, {
		.name = "bch8",
		.mode = OMAP_ECC_BCH8_CODE_HW_ROMCODE,
	},
};

static int gpmc_probe_nand_child(struct device_d *dev,
				 struct device_node *child)
{
	u32 val;
	const char *s;
	struct gpmc_timings gpmc_t;
	struct gpmc_nand_platform_data gpmc_nand_data = {};
	struct resource res;
	struct gpmc_settings gpmc_settings = {};
	int ret, i;

	if (of_property_read_u32(child, "reg", &val) < 0) {
		dev_err(dev, "%s has no 'reg' property\n",
			child->full_name);
		return -ENODEV;
	}

	gpmc_base = dev_get_mem_region(dev, 0);
	if (IS_ERR(gpmc_base))
		return PTR_ERR(gpmc_base);

	gpmc_nand_data.cs = val;
	gpmc_nand_data.of_node = child;

	/* Detect availability of ELM module */
	gpmc_nand_data.elm_of_node = of_parse_phandle(child, "ti,elm-id", 0);
	if (gpmc_nand_data.elm_of_node == NULL)
		gpmc_nand_data.elm_of_node =
					of_parse_phandle(child, "elm_id", 0);
	if (gpmc_nand_data.elm_of_node == NULL)
		pr_warn("%s: ti,elm-id property not found\n", __func__);

	/* select ecc-scheme for NAND */
	if (of_property_read_string(child, "ti,nand-ecc-opt", &s)) {
		pr_err("%s: ti,nand-ecc-opt not found\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		struct dt_eccmode *mode = &modes[i];
		if (!strcmp(s, mode->name))
			gpmc_nand_data.ecc_mode = mode->mode;
	}

	val = of_get_nand_bus_width(child);
	if (val == 16)
		gpmc_nand_data.device_width = NAND_BUSWIDTH_16;

	gpmc_read_timings_dt(child, &gpmc_t);
	gpmc_read_settings_dt(child, &gpmc_settings);

	gpmc_nand_data.wait_mon_pin = gpmc_settings.wait_pin;

	gpmc_nand_data.nand_cfg = xzalloc(sizeof(*gpmc_nand_data.nand_cfg));

	gpmc_timings_to_config(gpmc_nand_data.nand_cfg, &gpmc_t);

	gpmc_nand_data.nand_cfg->cfg[0] |= 2 << 10;

	ret = of_address_to_resource(child, 0, &res);
	if (ret)
		pr_err("of_address_to_resource failed\n");

	gpmc_nand_data.nand_cfg->base = res.start;
	gpmc_nand_data.nand_cfg->size = GPMC_SIZE_16M;

	gpmc_cs_config(gpmc_nand_data.cs, gpmc_nand_data.nand_cfg);

	dev = device_alloc("gpmc_nand", DEVICE_ID_DYNAMIC);
	device_add_resource(dev, NULL, (resource_size_t)gpmc_base, SZ_4K, IORESOURCE_MEM);
	device_add_data(dev, &gpmc_nand_data, sizeof(gpmc_nand_data));
	dev->device_node = child;
	platform_device_register(dev);

	return 0;
}

/**
 * gpmc_probe_generic_child - configures the gpmc for a child device
 * @pdev:	pointer to gpmc platform device
 * @child:	pointer to device-tree node for child device
 *
 * Allocates and configures a GPMC chip-select for a child device.
 * Returns 0 on success and appropriate negative error code on failure.
 */
static int gpmc_probe_generic_child(struct device_d *dev,
				struct device_node *child)
{
	struct gpmc_settings gpmc_s = {};
	struct gpmc_timings gpmc_t = {};
	struct resource res;
	int ret, cs;
	struct gpmc_config cfg = {};
	resource_size_t size;

	if (of_property_read_u32(child, "reg", &cs) < 0) {
		dev_err(dev, "%s has no 'reg' property\n",
			child->full_name);
		return -ENODEV;
	}

	if (of_address_to_resource(child, 0, &res) < 0) {
		dev_err(dev, "%s has malformed 'reg' property\n",
			child->full_name);
		return -ENODEV;
	}

	gpmc_read_settings_dt(child, &gpmc_s);
	gpmc_read_timings_dt(child, &gpmc_t);

	ret = of_property_read_u32(child, "bank-width", &gpmc_s.device_width);
	if (ret < 0)
		goto err;

	gpmc_timings_to_config(&cfg, &gpmc_t);

	cfg.base = res.start;

	size = resource_size(&res);
	if (size > SZ_64M)
		cfg.size = GPMC_SIZE_128M;
	else if (size > SZ_32M)
		cfg.size = GPMC_SIZE_64M;
	else if (size > SZ_16M)
		cfg.size = GPMC_SIZE_32M;
	else
		cfg.size = GPMC_SIZE_16M;

	gpmc_settings_to_config(&cfg, &gpmc_s);

	gpmc_cs_config(cs, &cfg);

	/* create platform device, NULL on error or when disabled */
	if (of_get_property(child, "compatible", NULL) && !of_platform_device_create(child, dev))
		goto err_child_fail;

	return 0;

err_child_fail:

	dev_err(dev, "failed to create gpmc child %s\n", child->name);
	ret = -ENODEV;

err:

	return ret;
}

static int gpmc_probe(struct device_d *dev)
{
	struct device_node *child, *node = dev->device_node;
	int ret;

	gpmc_generic_init(0x12);

	ret = of_property_read_u32(node, "gpmc,num-cs",
				   &gpmc_cs_num);
	if (ret < 0)
		return ret;

	ret = of_property_read_u32(node, "gpmc,num-waitpins",
				   &gpmc_nr_waitpins);
	if (ret < 0)
		return ret;

	for_each_child_of_node(node, child) {

		if (!child->name)
			continue;

		if (!strncmp(child->name, "nand", 4))
			ret = gpmc_probe_nand_child(dev, child);
		else if (strncmp(child->name, "ethernet", 8) == 0 ||
				strncmp(child->name, "nor", 3) == 0 ||
				strncmp(child->name, "uart", 4) == 0)
			ret = gpmc_probe_generic_child(dev, child);
		else
			dev_warn(dev, "unhandled child %s\n", child->name);

		if (ret)
			break;
	}

	if (ret)
		goto gpmc_err;

	return 0;

gpmc_err:
	return ret;
}

static struct of_device_id gpmc_id_table[] = {
	{ .compatible = "ti,omap2420-gpmc" },
	{ .compatible = "ti,omap2430-gpmc" },
	{ .compatible = "ti,omap3430-gpmc" },	/* omap3430 & omap3630 */
	{ .compatible = "ti,omap4430-gpmc" },	/* omap4430 & omap4460 & omap543x */
	{ .compatible = "ti,am3352-gpmc" },	/* am335x devices */
	{ }
};

static struct driver_d gpmc_driver = {
	.name = "omap-gpmc",
	.of_compatible = DRV_OF_COMPAT(gpmc_id_table),
	.probe   = gpmc_probe,
};
device_platform_driver(gpmc_driver);

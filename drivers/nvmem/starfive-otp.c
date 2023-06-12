// SPDX-License-Identifier: GPL-2.0-only
/*
 *   Copyright 2021 StarFive, Inc
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <gpiod.h>
#include <init.h>
#include <net.h>
#include <io.h>
#include <of.h>
#include <regmap.h>
#include <machine_id.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/nvmem-provider.h>

// otp reg offset
#define OTP_CFGR        0x00
#define OTPC_IER        0x04
#define OTPC_SRR        0x08
#define OTP_OPRR        0x0c
#define OTPC_CTLR       0x10
#define OTPC_ADDRR      0x14
#define OTPC_DINR       0x18
#define OTPC_DOUTR      0x1c

#define OTP_EMPTY_CELL_VALUE  0xffffffffUL

// cfgr (offset 0x00)
#define OTP_CFGR_PRG_CNT_MASK   0xff
#define OTP_CFGR_PRG_CNT_SHIFT  0
#define OTP_CFGR_DIV_1US_MASK   0xff
#define OTP_CFGR_DIV_1US_SHIFT  8
#define OTP_CFGR_RD_CYC_MASK    0x0f
#define OTP_CFGR_RD_CYC_SHIFT   16

// ier (offset 0x04)
#define OTPC_IER_DONE_IE        BIT(0)
#define OTPC_IER_BUSY_OPR_IE    BIT(1)

// srr (offset 0x08)
#define OTPC_SRR_DONE           BIT(0)
#define OTPC_SRR_BUSY_OPR       BIT(1)
#define OTPC_SRR_INFO_RD_LOCK   BIT(29)
#define OTPC_SRR_INFO_WR_LOCK   BIT(30)
#define OTPC_SRR_BUSY           BIT(31)

// oprr (offset 0x0c)
#define OTP_OPRR_OPR_MASK           0x00000007
#define OTP_OPRR_OPR_SHIFT          0

#define OTP_OPR_STANDBY             0x0 // user mode
#define OTP_OPR_READ                0x1 // user mode
#define OTP_OPR_MARGIN_READ_PROG    0x2 // testing mode
#define OTP_OPR_MARGIN_READ_INIT    0x3 // testing mode
#define OTP_OPR_PROGRAM             0x4 // user mode
#define OTP_OPR_DEEP_STANDBY        0x5 // user mode
#define OTP_OPR_DEBUG               0x6 // user mode

// ctlr (offset 0x10, see EG512X32TH028CW01_v1.0.pdf "Pin Description")
#define OTPC_CTLR_PCE               BIT(0)
#define OTPC_CTLR_PTM_MASK          0x0000000e
#define OTPC_CTLR_PTM_SHIFT         1
#define OTPC_CTLR_PDSTB             BIT(4)
#define OTPC_CTLR_PTR               BIT(5)
#define OTPC_CTLR_PPROG             BIT(6)
#define OTPC_CTLR_PWE               BIT(7)
#define OTPC_CTLR_PCLK              BIT(8)

// addrr (offset 0x14)
#define OTPC_ADDRR_PA_MASK          0x000001ff
#define OTPC_ADDRR_PA_SHIFT         0

/*
 * data format:
 * struct starfive_otp_data{
 * 	char vendor[32];
 * 	uint64_t sn;
 * 	uint8_t mac_addr[6];
 * 	uint8_t padding_0[2];
 * }
 */

struct starfive_otp {
	int power_gpio;
	struct starfive_otp_regs __iomem *regs;
};

struct starfive_otp_regs {
	/* TODO: add otp ememory_eg512x32 registers define */
	u32 otp_cfg;		/* timing Register */
	u32 otpc_ie;		/* interrupt Enable */
	u32 otpc_sr;		/* status Register */
	u32 otp_opr;		/* operation mode select Register */
	u32 otpc_ctl;		/* otp control port */
	u32 otpc_addr;		/* otp pa port */
	u32 otpc_din;		/* otp pdin port */
	u32 otpc_dout;		/* otp pdout */
	u32 reserved[504];
	u32 mem[512];
};

/*
 * offset and size are assumed aligned to the size of the fuses (32-bit).
 */
static int starfive_otp_read(void *ctx, unsigned offset, unsigned *val)
{
	struct starfive_otp *priv = ctx;

	gpio_set_active(priv->power_gpio, true);
	mdelay(10);

	//otp set to read mode
	writel(OTP_OPR_READ, &priv->regs->otp_opr);
	mdelay(5);

	/* read all requested fuses */
	*val = readl(&priv->regs->mem[offset / 4]);

	gpio_set_active(priv->power_gpio, false);
	mdelay(5);

	return 0;
}

static int starfive_otp_write(void *ctx, unsigned offset, unsigned val)
{
	return -EOPNOTSUPP;
}

static struct regmap_bus starfive_otp_regmap_bus = {
	.reg_read = starfive_otp_read,
	.reg_write = starfive_otp_write,
};

static int starfive_otp_probe(struct device *dev)
{
	struct starfive_otp *priv;
	struct regmap_config config = {};
	struct resource *iores;
	struct regmap *map;
	struct clk *clk;
	u32 total_fuses;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk_enable(clk);

	ret = device_reset(dev);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	ret = of_property_read_u32(dev->of_node, "fuse-count", &total_fuses);
	if (ret < 0) {
		dev_err(dev, "missing required fuse-count property\n");
		return ret;
	}

	config.name = "starfive-otp";
	config.reg_bits = 32;
	config.val_bits = 32;
	config.reg_stride = 4;
	config.max_register = total_fuses;

	priv = xzalloc(sizeof(*priv));

	priv->regs = IOMEM(iores->start);
	priv->power_gpio = gpiod_get(dev, "power", GPIOD_OUT_LOW);
	if (priv->power_gpio < 0)
		return priv->power_gpio;

	map = regmap_init(dev, &starfive_otp_regmap_bus, priv, &config);
	if (IS_ERR(map))
		return PTR_ERR(map);

	return PTR_ERR_OR_ZERO(nvmem_regmap_register(map, "starfive-otp"));
}

static struct of_device_id starfive_otp_dt_ids[] = {
	{ .compatible = "starfive,fu740-otp" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, starfive_otp_dt_ids);

static struct driver starfive_otp_driver = {
	.name	= "starfive_otp",
	.probe	= starfive_otp_probe,
	.of_compatible = starfive_otp_dt_ids,
};
device_platform_driver(starfive_otp_driver);

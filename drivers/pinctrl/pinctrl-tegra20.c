/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Partly based on code
 * Copyright (C) 2011-2012 NVIDIA Corporation <www.nvidia.com>
 * Copyright (C) 2010 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Device driver for the Tegra 20 pincontrol hardware module.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <pinctrl.h>
#include <linux/err.h>

struct pinctrl_tegra20 {
	struct {
		u32 __iomem *tri;
		u32 __iomem *mux;
		u32 __iomem *pull;
	} regs;
	struct pinctrl_device pinctrl;
};

struct tegra20_pingroup {
	const char *name;
	const char *funcs[4];
	s16 trictrl_id;
	s16 muxctrl_id;
	s16 pullctrl_id;
};

#define PG(pg_name, f0, f1, f2, f3, tri, mux, pull)	\
	{						\
		.name = #pg_name,			\
		.funcs = { #f0, #f1, #f2, #f3, },	\
		.trictrl_id = tri,			\
		.muxctrl_id = mux,			\
		.pullctrl_id = pull			\
	}

static const struct tegra20_pingroup tegra20_groups[] = {
	/* name,   f0,        f1,        f2,        f3,            tri,     mux,     pull */
	PG(ata,    ide,       nand,      gmi,       rsvd4,         0,       12,      0  ),
	PG(atb,    ide,       nand,      gmi,       sdio4,         1,       8,       1  ),
	PG(atc,    ide,       nand,      gmi,       sdio4,         2,       11,      2  ),
	PG(atd,    ide,       nand,      gmi,       sdio4,         3,       10,      3  ),
	PG(ate,    ide,       nand,      gmi,       rsvd4,         57,      6,       4  ),
	PG(cdev1,  osc,       plla_out,  pllm_out1, audio_sync,    4,       33,      32 ),
	PG(cdev2,  osc,       ahb_clk,   apb_clk,   pllp_out4,     5,       34,      33 ),
	PG(crtp,   crt,       rsvd2,     rsvd3,     rsvd4,         110,     105,     28 ),
	PG(csus,   pllc_out1, pllp_out2, pllp_out3, vi_sensor_clk, 6,       35,      60 ),
	PG(dap1,   dap1,      rsvd2,     gmi,       sdio2,         7,       42,      5  ),
	PG(dap2,   dap2,      twc,       rsvd3,     gmi,           8,       43,      6  ),
	PG(dap3,   dap3,      rsvd2,     rsvd3,     rsvd4,         9,       44,      7  ),
	PG(dap4,   dap4,      rsvd2,     gmi,       rsvd4,         10,      45,      8  ),
	PG(ddc,    i2c2,      rsvd2,     rsvd3,     rsvd4,         63,      32,      78 ),
	PG(dta,    rsvd1,     sdio2,     vi,        rsvd4,         11,      26,      9  ),
	PG(dtb,    rsvd1,     rsvd2,     vi,        spi1,          12,      27,      10 ),
	PG(dtc,    rsvd1,     rsvd2,     vi,        rsvd4,         13,      29,      11 ),
	PG(dtd,    rsvd1,     sdio2,     vi,        rsvd4,         14,      30,      12 ),
	PG(dte,    rsvd1,     rsvd2,     vi,        spi1,          15,      31,      13 ),
	PG(dtf,    i2c3,      rsvd2,     vi,        rsvd4,         108,     110,     14 ),
	PG(gma,    uarte,     spi3,      gmi,       sdio4,         28,      16,      74 ),
	PG(gmb,    ide,       nand,      gmi,       gmi_int,       61,      46,      75 ),
	PG(gmc,    uartd,     spi4,      gmi,       sflash,        29,      17,      76 ),
	PG(gmd,    rsvd1,     nand,      gmi,       sflash,        62,      47,      77 ),
	PG(gme,    rsvd1,     dap5,      gmi,       sdio4,         32,      48,      44 ),
	PG(gpu,    pwm,       uarta,     gmi,       rsvd4,         16,      50,      26 ),
	PG(gpu7,   rtck,      rsvd2,     rsvd3,     rsvd4,         107,     109,     19 ),
	PG(gpv,    pcie,      rsvd2,     rsvd3,     rsvd4,         17,      49,      15 ),
	PG(hdint,  hdmi,      rsvd2,     rsvd3,     rsvd4,         87,      18,      -1 ),
	PG(i2cp,   i2cp,      rsvd2,     rsvd3,     rsvd4,         18,      36,      17 ),
	PG(irrx,   uarta,     uartb,     gmi,       spi4,          20,      41,      43 ),
	PG(irtx,   uarta,     uartb,     gmi,       spi4,          19,      40,      42 ),
	PG(kbca,   kbc,       nand,      sdio2,     emc_test0_dll, 22,      37,      20 ),
	PG(kbcb,   kbc,       nand,      sdio2,     mio,           21,      38,      21 ),
	PG(kbcc,   kbc,       nand,      trace,     emc_test1_dll, 58,      39,      22 ),
	PG(kbcd,   kbc,       nand,      sdio2,     mio,           106,     108,     23 ),
	PG(kbce,   kbc,       nand,      owr,       rsvd4,         26,      14,      65 ),
	PG(kbcf,   kbc,       nand,      trace,     mio,           27,      13,      64 ),
	PG(lcsn,   displaya,  displayb,  spi3,      rsvd4,         95,      70,      -1 ),
	PG(ld0,    displaya,  displayb,  xio,       rsvd4,         64,      80,      -1 ),
	PG(ld1,    displaya,  displayb,  xio,       rsvd4,         65,      81,      -1 ),
	PG(ld2,    displaya,  displayb,  xio,       rsvd4,         66,      82,      -1 ),
	PG(ld3,    displaya,  displayb,  xio,       rsvd4,         67,      83,      -1 ),
	PG(ld4,    displaya,  displayb,  xio,       rsvd4,         68,      84,      -1 ),
	PG(ld5,    displaya,  displayb,  xio,       rsvd4,         69,      85,      -1 ),
	PG(ld6,    displaya,  displayb,  xio,       rsvd4,         70,      86,      -1 ),
	PG(ld7,    displaya,  displayb,  xio,       rsvd4,         71,      87,      -1 ),
	PG(ld8,    displaya,  displayb,  xio,       rsvd4,         72,      88,      -1 ),
	PG(ld9,    displaya,  displayb,  xio,       rsvd4,         73,      89,      -1 ),
	PG(ld10,   displaya,  displayb,  xio,       rsvd4,         74,      90,      -1 ),
	PG(ld11,   displaya,  displayb,  xio,       rsvd4,         75,      91,      -1 ),
	PG(ld12,   displaya,  displayb,  xio,       rsvd4,         76,      92,      -1 ),
	PG(ld13,   displaya,  displayb,  xio,       rsvd4,         77,      93,      -1 ),
	PG(ld14,   displaya,  displayb,  xio,       rsvd4,         78,      94,      -1 ),
	PG(ld15,   displaya,  displayb,  xio,       rsvd4,         79,      95,      -1 ),
	PG(ld16,   displaya,  displayb,  xio,       rsvd4,         80,      96,      -1 ),
	PG(ld17,   displaya,  displayb,  rsvd3,     rsvd4,         81,      97,      -1 ),
	PG(ldc,    displaya,  displayb,  rsvd3,     rsvd4,         94,      71,      -1 ),
	PG(ldi,    displaya,  displayb,  rsvd3,     rsvd4,         102,     104,     -1 ),
	PG(lhp0,   displaya,  displayb,  rsvd3,     rsvd4,         82,      101,     -1 ),
	PG(lhp1,   displaya,  displayb,  rsvd3,     rsvd4,         83,      98,      -1 ),
	PG(lhp2,   displaya,  displayb,  rsvd3,     rsvd4,         84,      99,      -1 ),
	PG(lhs,    displaya,  displayb,  xio,       rsvd4,         103,     75,      -1 ),
	PG(lm0,    displaya,  displayb,  spi3,      rsvd4,         88,      77,      -1 ),
	PG(lm1,    displaya,  displayb,  rsvd3,     CRT,           89,      78,      -1 ),
	PG(lpp,    displaya,  displayb,  rsvd3,     rsvd4,         104,     103,     -1 ),
	PG(lpw0,   displaya,  displayb,  spi3,      hdmi,          99,      64,      -1 ),
	PG(lpw1,   displaya,  displayb,  rsvd3,     rsvd4,         100,     65,      -1 ),
	PG(lpw2,   displaya,  displayb,  spi3,      hdmi,          101,     66,      -1 ),
	PG(lsc0,   displaya,  displayb,  xio,       rsvd4,         91,      73,      -1 ),
	PG(lsc1,   displaya,  displayb,  spi3,      hdmi,          92,      74,      -1 ),
	PG(lsck,   displaya,  displayb,  spi3,      hdmi,          93,      72,      -1 ),
	PG(lsda,   displaya,  displayb,  spi3,      hdmi,          97,      68,      -1 ),
	PG(lsdi,   displaya,  displayb,  spi3,      rsvd4,         98,      67,      -1 ),
	PG(lspi,   displaya,  displayb,  xio,       hdmi,          96,      69,      -1 ),
	PG(lvp0,   displaya,  displayb,  rsvd3,     rsvd4,         85,      79,      -1 ),
	PG(lvp1,   displaya,  displayb,  rsvd3,     rsvd4,         86,      100,     -1 ),
	PG(lvs,    displaya,  displayb,  xio,       rsvd4,         90,      76,      -1 ),
	PG(owc,    owr,       rsvd2,     rsvd3,     rsvd4,         31,      20,      79 ),
	PG(pmc,    pwr_on,    pwr_intr,  rsvd3,     rsvd4,         23,      105,     -1 ),
	PG(pta,    i2c2,      hdmi,      gmi,       rsvd4,         24,      107,     18 ),
	PG(rm,     i2c1,      rsvd2,     rsvd3,     rsvd4,         25,      7,       16 ),
	PG(sdb,    uarta,     pwm,       sdio3,     spi2,          111,     53,      -1 ),
	PG(sdc,    pwm,       twc,       sdio3,     spi3,          33,      54,      62 ),
	PG(sdd,    uarta,     pwm,       sdio3,     spi3,          34,      55,      63 ),
	PG(sdio1,  sdio1,     rsvd2,     uarte,     uarta,         30,      15,      73 ),
	PG(slxa,   pcie,      spi4,      sdio3,     spi2,          36,      19,      27 ),
	PG(slxc,   spdif,     spi4,      sdio3,     spi2,          37,      21,      29 ),
	PG(slxd,   spdif,     spi4,      sdio3,     spi2,          38,      22,      30 ),
	PG(slxk,   pcie,      spi4,      sdio3,     spi2,          39,      23,      31 ),
	PG(spdi,   spdif,     rsvd2,     i2c1,      sdio2,         40,      52,      24 ),
	PG(spdo,   spdif,     rsvd2,     i2c1,      sdio2,         41,      51,      25 ),
	PG(spia,   spi1,      spi2,      spi3,      gmi,           42,      63,      34 ),
	PG(spib,   spi1,      spi2,      spi3,      gmi,           43,      62,      35 ),
	PG(spic,   spi1,      spi2,      spi3,      gmi,           44,      61,      36 ),
	PG(spid,   spi2,      spi1,      spi2_alt,  gmi,           45,      60,      37 ),
	PG(spie,   spi2,      spi1,      spi2_alt,  gmi,           46,      59,      38 ),
	PG(spif,   spi3,      spi1,      spi2,      rsvd4,         47,      58,      39 ),
	PG(spig,   spi3,      spi2,      spi2_alt,  i2c1,          48,      57,      40 ),
	PG(spih,   spi3,      spi2,      spi2_alt,  i2c1,          49,      56,      41 ),
	PG(uaa,    spi3,      mipi_hs,   uarta,     ulpi,          50,      0,       48 ),
	PG(uab,    spi2,      mipi_hs,   uarta,     ulpi,          51,      1,       49 ),
	PG(uac,    owr,       rsvd2,     rsvd3,     rsvd4,         52,      2,       50 ),
	PG(uad,    irda,      spdif,     uarta,     spi4,          53,      3,       51 ),
	PG(uca,    uartc,     rsvd2,     gmi,       rsvd4,         54,      24,      52 ),
	PG(ucb,    uartc,     pwm,       gmi,       rsvd4,         55,      25,      53 ),
	PG(uda,    spi1,      rsvd2,     uartd,     ulpi,          109,     4,       72 ),
};

static void pinctrl_tegra20_set_func(struct pinctrl_tegra20 *ctrl,
				     int muxctrl_id, int func)
{
	u32 __iomem *regaddr = ctrl->regs.mux;
	u32 reg;
	int maskbit;

	regaddr += muxctrl_id >> 4;
	maskbit = (muxctrl_id << 1) & 0x1f;

	reg = readl(regaddr);
	reg &= ~(0x3 << maskbit);
	reg |= func << maskbit;
	writel(reg, regaddr);
}

static void pinctrl_tegra20_set_pull(struct pinctrl_tegra20 *ctrl,
				     int pullctrl_id, int pull)
{
	u32 __iomem *regaddr = ctrl->regs.pull;
	u32 reg;
	int maskbit;

	regaddr += pullctrl_id >> 4;
	maskbit = (pullctrl_id << 1) & 0x1f;

	reg = readl(regaddr);
	reg &= ~(0x3 << maskbit);
	reg |= pull << maskbit;
	writel(reg, regaddr);
}

static void pinctrl_tegra20_set_tristate(struct pinctrl_tegra20 *ctrl,
					 int trictrl_id, int tristate)
{
	u32 __iomem *regaddr = ctrl->regs.tri;
	u32 reg;
	int maskbit;

	regaddr += trictrl_id >> 5;
	maskbit = trictrl_id & 0x1f;

	reg = readl(regaddr);
	reg &= ~(1 << maskbit);
	reg |= tristate << maskbit;
	writel(reg, regaddr);
}

static int pinctrl_tegra20_set_state(struct pinctrl_device *pdev,
				     struct device_node *np)
{
	struct pinctrl_tegra20 *ctrl =
			container_of(pdev, struct pinctrl_tegra20, pinctrl);
	struct device_node *childnode;
	int pull = -1, tri = -1, i, j, k;
	const char *pins, *func = NULL;
	const struct tegra20_pingroup *group;

	/*
	 * At first look if the node we are pointed at has children,
	 * which we may want to visit.
	 */
	list_for_each_entry(childnode, &np->children, parent_list)
		pinctrl_tegra20_set_state(pdev, childnode);

	/* read relevant state from devicetree */
	of_property_read_string(np, "nvidia,function", &func);
	of_property_read_u32_array(np, "nvidia,pull", &pull, 1);
	of_property_read_u32_array(np, "nvidia,tristate", &tri, 1);

	/* iterate over all pingroups referenced in the dt node */
	for (i = 0; ; i++) {
		if (of_property_read_string_index(np, "nvidia,pins", i, &pins))
			break;

		for (j = 0; j < ARRAY_SIZE(tegra20_groups); j++) {
			if (!strcmp(pins, tegra20_groups[j].name)) {
				group = &tegra20_groups[j];
				break;
			}
		}
		/* if no matching pingroup is found bail out */
		if (j == ARRAY_SIZE(tegra20_groups)) {
			dev_warn(ctrl->pinctrl.dev,
				 "invalid pingroup %s referenced in node %s\n",
				 pins, np->name);
			continue;
		}

		if (func) {
			for (k = 0; k < 4; k++) {
				if (!strcmp(func, group->funcs[k]))
					break;
			}
			if (k < 4)
				pinctrl_tegra20_set_func(ctrl,
							 group->muxctrl_id, k);
			else
				dev_warn(ctrl->pinctrl.dev,
					 "invalid function %s for pingroup %s in node %s\n",
					 func, group->name, np->name);
		}

		if (pull >= 0) {
			if (group->pullctrl_id >= 0)
				pinctrl_tegra20_set_pull(ctrl,
							 group->pullctrl_id,
							 pull);
			else
				dev_warn(ctrl->pinctrl.dev,
					 "pingroup %s in node %s doesn't support pull configuration\n",
					 group->name, np->name);
		}

		if (tri >= 0)
			pinctrl_tegra20_set_tristate(ctrl,
						     group->trictrl_id, tri);
	}

	return 0;
}

static struct pinctrl_ops pinctrl_tegra20_ops = {
	.set_state = pinctrl_tegra20_set_state,
};

static int pinctrl_tegra20_probe(struct device_d *dev)
{
	struct resource *iores;
	struct pinctrl_tegra20 *ctrl;
	int i, ret;
	u32 **regs;

	ctrl = xzalloc(sizeof(*ctrl));

	/*
	 * Tegra pincontrol is split out into four independent memory ranges:
	 * tristate control, function mux, pullup/down control, pad control
	 * (from lowest to highest hardware address).
	 * We are only interested in the first three for now.
	 */
	regs = (u32 **)&ctrl->regs;
	for (i = 0; i <= 2; i++) {
		iores = dev_request_mem_resource(dev, i);
		if (IS_ERR(iores)) {
			dev_err(dev, "Could not get iomem region %d\n", i);
			return PTR_ERR(iores);
		}
		regs[i] = IOMEM(iores->start);
	}

	ctrl->pinctrl.dev = dev;
	ctrl->pinctrl.ops = &pinctrl_tegra20_ops;

	ret = pinctrl_register(&ctrl->pinctrl);
	if (ret) {
		free(ctrl);
		return ret;
	}

	of_pinctrl_select_state(dev->device_node, "boot");

	return 0;
}

static __maybe_unused struct of_device_id pinctrl_tegra20_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-pinmux",
	}, {
		/* sentinel */
	}
};

static struct driver_d pinctrl_tegra20_driver = {
	.name		= "pinctrl-tegra20",
	.probe		= pinctrl_tegra20_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_tegra20_dt_ids),
};

static int pinctrl_tegra20_init(void)
{
	return platform_driver_register(&pinctrl_tegra20_driver);
}
core_initcall(pinctrl_tegra20_init);

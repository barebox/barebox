// SPDX-License-Identifier: GPL-2.0-or-later
/* am335x_adc.c
 *
 * Copyright © 2019 Synapse Product Development
 *
 * Author: Trent Piepho <trent.piepho@synapse.com>
 *
 * This is a simple driver for the ADC in TI's AM335x SoCs.  It's designed to
 * produce one-shot readings and doesn't use the more advanced features, like
 * the FIFO, triggering, DMA, multi-channel scan programs, etc.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <io.h>
#include <linux/log2.h>
#include <aiodev.h>
#include <mach/am33xx-clock.h>
#include "ti_am335x_tscadc.h"

struct am335x_adc_data {
	struct aiodevice aiodev;
	void __iomem *base;
	struct aiochannel *channels;
};

static inline void tiadc_write(const struct am335x_adc_data *data, u32 value,
			       u32 reg)
{
	writel(value, data->base + reg);
}

static inline u32 tiadc_read(const struct am335x_adc_data *data, u32 reg)
{
	return readl(data->base + reg);
}

static int am335x_adc_read(struct aiochannel *chan, int *val)
{
	struct am335x_adc_data *data =
		container_of(chan->aiodev, struct am335x_adc_data, aiodev);
	int timeout = IDLE_TIMEOUT;
	/* This assumes VREFN = 0V and VREFP = 1.8V */
	const u32 vrefp = 1800;	 /* ceil(log2(vrefp)) = 11 */
	/* Left shift vrefp/4095 by as much as possible without overflowing 32 bits */
	const u32 shift = 32 - (const_ilog2(vrefp) + 1);
	const u32 factor = (vrefp << shift) / 4095u;
	u32 counts;

	/* Make sure FIFO is empty before we start, so we don't get old data */
	while ((tiadc_read(data, REG_FIFO1CNT) & 0x7f) > 0)
		tiadc_read(data, REG_FIFO1);

	tiadc_write(data, ENB(chan->index + 1), REG_SE);  /* ENB(1) is 1st channel */
	tiadc_write(data, CNTRLREG_TSCSSENB, REG_CTRL);

	while ((tiadc_read(data, REG_FIFO1CNT) & 0x7f) == 0) {
		if (--timeout == 0)
			return -ETIMEDOUT;
		mdelay(1);
	}

	counts = tiadc_read(data, REG_FIFO1) & FIFOREAD_DATA_MASK;
	*val = (counts * factor) >> shift;

	tiadc_write(data, 0, REG_CTRL);

	return 0;
}

static int am335x_adc_probe(struct device_d *dev)
{
	struct device_node *node;
	struct am335x_adc_data *data;
	int i, ret;

	data = xzalloc(sizeof(*data));
	data->aiodev.hwdev = dev;
	data->aiodev.read = am335x_adc_read;
	data->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(data->base)) {
		ret = PTR_ERR(data->base);
		goto fail_data;
	}

	node = of_find_compatible_node(dev->device_node, NULL, "ti,am3359-adc");
	if (!node) {
		ret = -EINVAL;
		goto fail_data;
	}

	if (!of_find_property(node, "ti,adc-channels",
			      &data->aiodev.num_channels))
		return -EINVAL;
	data->aiodev.num_channels /= sizeof(u32);

	data->channels = xzalloc(sizeof(*data->channels) *
				 data->aiodev.num_channels);
	data->aiodev.channels = xmalloc(sizeof(*data->aiodev.channels) *
					data->aiodev.num_channels);

	/* Max ADC clock is 24 MHz or 3 MHz, depending on if one looks at the
	 * reference manual or data sheet.
	 */
	tiadc_write(data, DIV_ROUND_UP(am33xx_get_osc_clock(), ADC_CLK) - 1,
		    REG_CLKDIV);
	tiadc_write(data, ~0, REG_IRQCLR);
	tiadc_write(data, ~0, REG_IRQSTATUS);
	tiadc_write(data, 0x3, REG_DMAENABLE_CLEAR);
	tiadc_write(data, CNTRLREG_STEPCONFIGWRT, REG_CTRL);
	tiadc_write(data,
		    STEPCONFIG_RFP_VREFP | STEPCONFIG_RFM_VREFN |
		    STEPCONFIG_INM_ADCREFM | STEPCONFIG_INP_ADCREFM,
		    REG_IDLECONFIG);


	for (i = 0; i < data->aiodev.num_channels; i++) {
		u32 config, delay, ain, odelay, sdelay, avg;

		data->aiodev.channels[i] = &data->channels[i];
		data->channels[i].unit = "mV";
		ret = of_property_read_u32_index(node, "ti,adc-channels",
						 i, &ain);
		if (ret)
			goto fail_channels;

		ret = of_property_read_u32_index(node, "ti,chan-step-opendelay",
						 i, &odelay);
		odelay = ret ? STEPCONFIG_OPENDLY : STEPDELAY_OPEN(odelay);

		ret = of_property_read_u32_index(node, "ti,chan-step-sampledelay",
						 i, &sdelay);
		sdelay = ret ? STEPCONFIG_SAMPLEDLY : STEPDELAY_SAMPLE(sdelay);

		ret = of_property_read_u32_index(node, "ti,chan-step-avg",
						 i, &avg);
		avg = ret ? STEPCONFIG_AVG_16 : STEPCONFIG_AVG(ilog2(avg ? : 1));

		/* We program each step with one of the channels in the DT */
		config = STEPCONFIG_RFP_VREFP | STEPCONFIG_RFM_VREFN | /* External refs */
			 /* Internal reference, use STEPCONFIG_RFP(0) | STEPCONFIG_RFM(0) */
			 STEPCONFIG_INM_ADCREFM |  /* Not important, SE rather than diff */
			 STEPCONFIG_MODE(0) | STEPCONFIG_FIFO1 |  /* One-shot and data to FIFO1 */
			 avg | STEPCONFIG_INP(ain);
		delay = odelay | sdelay;

		tiadc_write(data, config, REG_STEPCONFIG(i));
		tiadc_write(data, delay, REG_STEPDELAY(i));
	}
	tiadc_write(data, 0, REG_CTRL);

	ret = aiodevice_register(&data->aiodev);
	if (ret)
		goto fail_channels;

	dev_info(dev, "TI AM335x ADC (%d ch) registered as %s\n",
		 data->aiodev.num_channels, dev_name(&data->aiodev.dev));
	return 0;

 fail_channels:
	kfree(data->channels);
	kfree(data->aiodev.channels);

 fail_data:
	kfree(data);
	return ret;
}

static const struct of_device_id of_am335x_adc_match[] = {
	{ .compatible = "ti,am3359-tscadc", },
	{ /* end */ }
};

static struct driver_d am335x_adc_driver = {
	.name		= "am335x_adc",
	.probe		= am335x_adc_probe,
	.of_compatible	= DRV_OF_COMPAT(of_am335x_adc_match),
};
device_platform_driver(am335x_adc_driver);

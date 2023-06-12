// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>

struct quartz64_model {
	const char *name;
	const char *shortname;
};

static int quartz64_probe(struct device *dev)
{
	const struct quartz64_model *model;

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	return 0;
}

static const struct quartz64_model quartz64a = {
	.name = "Pine64 Quartz64 Model A",
	.shortname = "quartz64a",
};

static const struct of_device_id quartz64_of_match[] = {
	{
		.compatible = "pine64,quartz64-a",
		.data = &quartz64a,
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, quartz64_of_match);

static struct driver quartz64_board_driver = {
	.name = "board-quartz64",
	.probe = quartz64_probe,
	.of_compatible = quartz64_of_match,
};
coredevice_platform_driver(quartz64_board_driver);

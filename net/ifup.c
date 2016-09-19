/*
 * ifup.c - bring up network interfaces
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more detaiifup.
 *
 */
#define pr_fmt(fmt)  "ifup: " fmt

#include <environment.h>
#include <command.h>
#include <common.h>
#include <getopt.h>
#include <net.h>
#include <fs.h>
#include <linux/stat.h>

static char *vars[] = {
	"ipaddr",
	"netmask",
	"gateway",
	"serverip",
};

static int eth_set_param(struct device_d *dev, const char *param)
{
	const char *value = getenv(param);

	if (!value)
		return 0;
	if (!*value)
		return 0;

	return dev_set_param(dev, param, value);
}

int ifup(const char *name, unsigned flags)
{
	int ret;
	char *cmd, *cmd_discover;
	const char *ip;
	struct stat s;
	int i;
	struct device_d *dev;
	struct eth_device *edev = eth_get_byname(name);

	if (edev && edev->ipaddr && !(flags & IFUP_FLAG_FORCE))
		return 0;

	eth_set_current(edev);

	env_push_context();

	setenv("ip", "");

	for (i = 0; i < ARRAY_SIZE(vars); i++)
		setenv(vars[i], "");

	cmd = basprintf("source /env/network/%s", name);
	cmd_discover = basprintf("/env/network/%s-discover", name);

	ret = run_command(cmd);
	if (ret) {
		pr_err("Running '%s' failed with %d\n", cmd, ret);
		goto out;
	}

	ret = stat(cmd_discover, &s);
	if (!ret) {
		ret = run_command(cmd_discover);
		if (ret) {
			pr_err("Running '%s' failed with %d\n", cmd, ret);
			goto out;
		}
	}

	dev = get_device_by_name(name);
	if (!dev) {
		pr_err("Cannot find device %s\n", name);
		goto out;
	}

	ret = eth_set_param(dev, "ethaddr");
	if (ret)
		goto out;

	ip = getenv("ip");
	if (!ip)
		ip = "";

	if (!strcmp(ip, "dhcp")) {
		ret = run_command("dhcp");
		if (ret)
			goto out;
		ret = eth_set_param(dev, "serverip");
		if (ret)
			goto out;
		dev_set_param(dev, "linux.bootargs", "ip=dhcp");
	} else if (!strcmp(ip, "static")) {
		char *bootarg;
		for (i = 0; i < ARRAY_SIZE(vars); i++) {
			ret = eth_set_param(dev, vars[i]);
			if (ret)
				goto out;
		}
		bootarg = basprintf("ip=%pI4:%pI4:%pI4:%pI4:::",
				&edev->ipaddr,
				&edev->serverip,
				&edev->gateway,
				&edev->netmask);
		dev_set_param(dev, "linux.bootargs", bootarg);
		free(bootarg);
	} else {
		pr_err("unknown ip type: %s\n", ip);
		ret = -EINVAL;
		goto out;
	}

	ret = 0;
out:
	env_pop_context();
	free(cmd);
	free(cmd_discover);

	return ret;
}

int ifup_all(unsigned flags)
{
	DIR *dir;
	struct dirent *d;

	dir = opendir("/env/network");
	if (!dir)
		return -ENOENT;

	while ((d = readdir(dir))) {
		if (*d->d_name == '.')
			continue;
		/*
		 * Skip xxx-discover files since these are no
		 * network configuration files, but scripts to bring
		 * up network interface xxx.
		 */
		if (strstr(d->d_name, "-discover"))
			continue;
		ifup(d->d_name, flags);
	}

	closedir(dir);

	return 0;
}

#if IS_ENABLED(CONFIG_NET_CMD_IFUP)

static int do_ifup(int argc, char *argv[])
{
	int opt;
	unsigned flags = 0;
	int all = 0;

	while ((opt = getopt(argc, argv, "af")) > 0) {
		switch (opt) {
		case 'f':
			flags |= IFUP_FLAG_FORCE;
			break;
		case 'a':
			all = 1;
			break;
		}
	}

	if (all)
		return ifup_all(flags);

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	return ifup(argv[optind], flags);
}

BAREBOX_CMD_HELP_START(ifup)
BAREBOX_CMD_HELP_TEXT("Each INTF must have a script /env/network/INTF that set the variables")
BAREBOX_CMD_HELP_TEXT("ip (to 'static' or 'dynamic'), ipaddr, netmask, gateway, serverip and/or")
BAREBOX_CMD_HELP_TEXT("ethaddr. A script /env/network/INTF-discover can contains for discovering")
BAREBOX_CMD_HELP_TEXT("the ethernet device, e.g. 'usb'.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",  "bring up all interfaces")
BAREBOX_CMD_HELP_OPT ("-f",  "Force. Configure even if ip already set")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ifup)
	.cmd		= do_ifup,
	BAREBOX_CMD_DESC("bring a network interface up")
	BAREBOX_CMD_OPTS("[-af] [INTF]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_HELP(cmd_ifup_help)
BAREBOX_CMD_END

#endif

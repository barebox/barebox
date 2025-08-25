// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: © 2021 Ahmad Fatoum, Pengutronix

#include <command.h>
#include <common.h>
#include <environment.h>
#include <getopt.h>
#include <linux/nvmem-consumer.h>
#include <linux/sizes.h>

/* Maximum size for dynamically created NVMEM devices.
 * This is a reasonable limit for RAM-backed devices, but can be adjusted
 * based on system capabilities and requirements.
 */
#define NVMEM_MAX_SIZE		SZ_1M

/* Static counter to ensure unique device IDs for dynamically created rmem
 * devices
 */
static int dynamic_rmem_idx;

/**
 * nvmem_create_dynamic_rmem - Creates a dynamic RAM-backed NVMEM device.
 * @create_size: Size of the NVMEM device to create.
 * @var_name: Optional environment variable name to store the created device's
 *            name.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int nvmem_create_dynamic_rmem(unsigned long create_size,
				     const char *var_name)
{
	struct device *dev;
	void *buffer;
	int ret = 0;

	buffer = xzalloc(create_size);
	dev = add_generic_device("nvmem-rmem", dynamic_rmem_idx, NULL,
				 (resource_size_t)(uintptr_t)buffer,
				 (resource_size_t)create_size,
				 IORESOURCE_MEM, NULL);

	if (var_name)
		ret = pr_setenv(var_name, "rmem%u", dynamic_rmem_idx);

	dynamic_rmem_idx++;

	return ret;
}

static int nvmem_parse_and_validate_create_size(const char *size_arg,
						unsigned long *out_size)
{

	if (!IS_ENABLED(CONFIG_NVMEM_RMEM)) {
		printf("Error: rmem NVMEM driver (CONFIG_NVMEM_RMEM) is not enabled in this build.\n");
		return -EINVAL;
	}

	*out_size = strtoul_suffix(optarg, NULL, 0);
	if (!*out_size) {
		printf("Error: Invalid size '%s' for -c option. Must be non-zero.\n",
		       optarg);
		return COMMAND_ERROR_USAGE;
	}

	if (*out_size > NVMEM_MAX_SIZE) {
		printf("Error: Size '%lu' exceeds maximum allowed size of %d bytes.\n",
		       *out_size, NVMEM_MAX_SIZE);
		return COMMAND_ERROR_USAGE;
	}

	return 0;
}

static int do_nvmem(int argc, char *argv[])
{
	unsigned long create_size = 0;
	char *optarg_v = NULL;
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "c:v:h")) != -1) {
		switch (opt) {
		case 'c':
			ret = nvmem_parse_and_validate_create_size(optarg,
								&create_size);
			if (ret)
				return ret;
			break;
		case 'v':
			optarg_v = optarg;
			break;
		case 'h': /* fallthrough */
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optarg_v && !create_size) {
		printf("Error: -v <VARNAME> option requires -c <SIZE> to create a device.\n");
		return COMMAND_ERROR_USAGE;
	}

	if (create_size > 0)
		ret = nvmem_create_dynamic_rmem(create_size, optarg_v);
	else
		nvmem_devices_print();

	return ret;
}

BAREBOX_CMD_HELP_START(nvmem)
BAREBOX_CMD_HELP_TEXT("List NVMEM devices or create dynamic RAM-backed NVMEM")
BAREBOX_CMD_HELP_TEXT("devices. If no arguments are provided, it lists all")
BAREBOX_CMD_HELP_TEXT("available NVMEM devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-c <size>", "Create a new RAM-backed NVMEM device of")
BAREBOX_CMD_HELP_OPT("         ", "<size> bytes. (Requires CONFIG_NVMEM_RMEM")
BAREBOX_CMD_HELP_OPT("         ", "to be enabled). <size> must be a non-zero.")
BAREBOX_CMD_HELP_OPT("-v <variable>", "When using -c, set environment variable")
BAREBOX_CMD_HELP_OPT("             ", "<variable> to the name of the created")
BAREBOX_CMD_HELP_OPT("             ", "NVMEM device (e.g., rmem0).")
BAREBOX_CMD_HELP_END;

BAREBOX_CMD_START(nvmem)
	.cmd		= do_nvmem,
	BAREBOX_CMD_DESC("list or create NVMEM devices")
	BAREBOX_CMD_OPTS("[-c <size> [-v <varname>]]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nvmem_help)
BAREBOX_CMD_END

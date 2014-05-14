/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/*
 * cbootimage.c - Implementation of the cbootimage tool.
 */

#include <strings.h>
#include "cbootimage.h"
#include "crypto.h"
#include "data_layout.h"
#include "parse.h"
#include "set.h"
#include "context.h"
#include <getopt.h>

/*
 * Global data
 */
int enable_debug;
static int help_only; // Only print help & exit
cbootimage_soc_config * g_soc_config;

/*
 * Function prototypes
 */
int main(int argc, char *argv[]);

struct option cbootcmd[] = {
	{"help", 0, NULL, 'h'},
	{"debug", 0, NULL, 'd'},
	{"generate", 1, NULL, 'g'},
	{"tegra", 1, NULL, 't'},
	{"odmdata", 1, NULL, 'o'},
	{"soc", 1, NULL, 's'},
	{0, 0, 0, 0},
};

int
write_image_file(build_image_context *context)
{
	assert(context != NULL);

	return write_block_raw(context);
}

static void
usage(void)
{
	printf("Usage: cbootimage [options] configfile imagename\n");
	printf("    options:\n");
	printf("    -h, --help, -?        Display this message.\n");
	printf("    -d, --debug           Output debugging information.\n");
	printf("    -gbct                 Generate the new bct file.\n");
	printf("    -o<ODM_DATA>          Specify the odm_data(in hex).\n");
	printf("    -t|--tegra NN         Select target device. Must be one of:\n");
	printf("                          20, 30, 114, 124.\n");
	printf("                          Default: 20. This option is deprecated\n");
	printf("    -s|--soc tegraNN      Select target device. Must be one of:\n");
	printf("                          tegra20, tegra30, tegra114, tegra124.\n");
	printf("                          Default: tegra20.\n");
	printf("    configfile            File with configuration information\n");
	printf("    imagename             Output image name\n");
}

static int
process_command_line(int argc, char *argv[], build_image_context *context)
{
	int c;

	context->generate_bct = 0;

	while ((c = getopt_long(argc, argv, "hdg:t:o:s:", cbootcmd, NULL)) != -1) {
		switch (c) {
		case 'h':
			help_only = 1;
			usage();
			return 0;
		case 'd':
			enable_debug = 1;
			break;
		case 'g':
			if (!strcasecmp("bct", optarg)) {
				context->generate_bct = 1;
			} else {
				printf("Invalid argument!\n");
				usage();
				return -EINVAL;
			}
			break;
		case 's':
			if (strncmp("tegra", optarg, 5)) {
				printf("Unsupported chipname!\n");
				usage();
				return -EINVAL;
			}
			optarg += 5;
			/* Deliberate fall-through */
		case 't':
			/* Assign the soc_config based on the chip. */
			if (!strcasecmp("20", optarg)) {
				t20_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("30", optarg)) {
				t30_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("114", optarg)) {
				t114_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("124", optarg)) {
				t124_get_soc_config(context, &g_soc_config);
			} else {
				printf("Unsupported chipname!\n");
				usage();
				return -EINVAL;
			}
			break;
		case 'o':
			context->odm_data = strtoul(optarg, NULL, 16);
			break;
		}
	}

	if (argc - optind != 2) {
		printf("Missing configuration and/or image file name.\n");
		usage();
		return -EINVAL;
	}

	/* If SoC is not specified, make the default soc_config to t20. */
	if (!context->boot_data_version)
		t20_get_soc_config(context, &g_soc_config);

	/* Open the configuration file. */
	context->config_file = fopen(argv[optind], "r");
	if (context->config_file == NULL) {
		printf("Error opening config file.\n");
		return -ENODATA;
	}

	/* Record the output filename */
	context->image_filename = argv[optind + 1];

	return 0;
}

int
main(int argc, char *argv[])
{
	int e = 0;
	build_image_context context;

	memset(&context, 0, sizeof(build_image_context));

	/* Process command line arguments. */
	if (process_command_line(argc, argv, &context) != 0)
		return -EINVAL;

	if (help_only)
		return 1;

	assert(g_soc_config != NULL);

	g_soc_config->get_value(token_bct_size,
					&context.bct_size,
					context.bct);

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	if (enable_debug) {
		/* Debugging information... */
		printf("bct size: %d\n", e == 0 ? context.bct_size : -1);
	}

	/* Open the raw output file. */
	context.raw_file = fopen(context.image_filename,
		                 "w+");
	if (context.raw_file == NULL) {
		printf("Error opening raw file %s.\n",
			context.image_filename);
		goto fail;
	}

	/* first, if we aren't generating the bct, read in config file */
	if (context.generate_bct == 0) {
		process_config_file(&context, 1);
	}
	/* Generate the new bct file */
	else {
		/* Initialize the bct memory */
		init_bct(&context);
		/* Parse & process the contents of the config file. */
		process_config_file(&context, 0);
		/* Update the BCT */
		begin_update(&context);
		/* Signing the bct. */
		e = sign_bct(&context, context.bct);
		if (e != 0)
			printf("Signing BCT failed, error: %d.\n", e);

		fwrite(context.bct, 1, context.bct_size,
			context.raw_file);
		goto fail;
	}

	/* Peform final signing & encryption of bct. */
	e = sign_bct(&context, context.bct);
	if (e != 0) {
		printf("Signing BCT failed, error: %d.\n", e);
		goto fail;
	}
	/* Write the image file. */
	/* The image hasn't been written yet. */
	if (write_image_file(&context) != 0)
		printf("Error writing image file.\n");

 fail:
	/* Close the file(s). */
	if (context.raw_file)
		fclose(context.raw_file);

	/* Clean up memory. */
	cleanup_context(&context);
	return e;
}

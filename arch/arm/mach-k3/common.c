// SPDX-License-Identifier: GPL-2.0-only
#include <of.h>
#include <io.h>
#include <dma.h>
#include <deep-probe.h>
#include <init.h>
#include <envfs.h>
#include <fs.h>
#include <stdio.h>
#include <xfuncs.h>
#include <malloc.h>
#include <command.h>
#include <getopt.h>
#include <libfile.h>
#include <linux/bits.h>
#include <linux/bitfield.h>
#include <mach/k3/common.h>
#include <soc/ti/ti_sci_protocol.h>

static const struct of_device_id k3_of_match[] = {
	{
		.compatible = "ti,am625",
	}, {
		.compatible = "ti,am62l3",
	}, {
		/* sentinel */
	},
};
BAREBOX_DEEP_PROBE_ENABLE(k3_of_match);

int k3_env_init(void)
{
	char *partname, *cdevname, *envpath;
	struct cdev *cdev;
	const char *rootpath;
	int instance;

	instance = bootsource_get_instance();

	cdevname = xasprintf("mmc%d", instance);
	partname = xasprintf("mmc%d.0", instance);

	device_detect_by_name(cdevname);

	cdev = cdev_open_by_name(partname, O_RDONLY);
	if (!cdev) {
		pr_err("Failed to get device %s\n", partname);
		goto out;
	}

	rootpath = cdev_mount_default(cdev, NULL);

	cdev_close(cdev);

	if (IS_ERR(rootpath)) {
		pr_err("Failed to load environment: mount %s failed (%ld)\n",
						cdev->name, PTR_ERR(rootpath));
		goto out;
	}

	symlink(rootpath, "/boot");

	if (IS_ENABLED(CONFIG_ENV_HANDLING)) {
		envpath = xasprintf("%s/barebox.env", rootpath);

		pr_debug("Loading default env from %s on device %s\n",
			 envpath, partname);

		default_environment_path_set(envpath);

		free(envpath);
	}

out:
	free(partname);
	free(cdevname);

	return 0;
}

int k3_authenticate_image(void **buf, size_t *size)
{
	const struct ti_sci_handle *ti_sci;
	u64 image_addr;
	int ret;
	unsigned int s = *size;

	ti_sci = ti_sci_get_handle(NULL);
	if (IS_ERR(ti_sci))
		return -EINVAL;

	image_addr = dma_map_single(NULL, *buf, *size, DMA_BIDIRECTIONAL);

	ret = ti_sci->ops.proc_ops.proc_auth_boot_image(ti_sci, &image_addr, &s);

	dma_unmap_single(NULL, image_addr, *size, DMA_BIDIRECTIONAL);

	if (ret)
		*size = 0;
	else
		*size = s;

	return ret;
}

#ifdef CONFIG_ARCH_K3_COMMAND_AUTHENTICATE
static int do_k3_authenticate_image(int argc, char *argv[])
{
	void *buf;
	size_t size;
	int ret;
	int opt;
	const char *outfile = NULL;
	const char *filename;

	while ((opt = getopt(argc, argv, "o:")) > 0) {
		switch (opt) {
		case 'o':
			outfile = optarg;
			break;
		}
	}

	argc -= optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	filename = argv[0];

	ret = read_file_2(filename, &size, &buf, FILESIZE_MAX);
	if (ret)
		return ret;

	ret = k3_authenticate_image(&buf, &size);
	if (ret) {
		printf("authenticating %s failed: %pe\n", filename, ERR_PTR(ret));
		ret = COMMAND_ERROR;
		goto err;
	}

	printf("%s successfully authenticated\n", filename);

	if (outfile) {
		ret = write_file(outfile, buf, size);
		if (ret) {
			printf("Failed to write output file: %pe\n", ERR_PTR(ret));
			goto err;
		}
	}

	ret = 0;
err:
	free(buf);

	return ret;
}

BAREBOX_CMD_HELP_START(k3_authenticate_image)
BAREBOX_CMD_HELP_TEXT("Authenticate image with K3 ROM API")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-o <out>",  "store unpacked authenticated image in <out>")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(k3_authenticate_image)
	.cmd = do_k3_authenticate_image,
	BAREBOX_CMD_DESC("authenticate file with K3 ROM API")
	BAREBOX_CMD_OPTS("[-o <out>] file")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(k3_authenticate_image)
BAREBOX_CMD_END
#endif

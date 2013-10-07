#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <ioctl.h>
#include <errno.h>
#include <getopt.h>
#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/ubi-user.h>
#include <mtd/ubi-media.h>

static int do_ubimkvol(int argc, char *argv[])
{
	struct ubi_mkvol_req req;
	int fd, ret;
	size_t size;

	if (argc != 4)
		return COMMAND_ERROR_USAGE;

	size = strtoul_suffix(argv[3], NULL, 0);
	req.name_len = min_t(int, strlen(argv[2]), UBI_VOL_NAME_MAX);
	strncpy(req.name, argv[2], req.name_len);
	req.name[req.name_len] = 0;

	req.vol_type = UBI_DYNAMIC_VOLUME;
	req.bytes = size;
	req.vol_id = UBI_VOL_NUM_AUTO;
	req.alignment = 1;

	fd = open(argv[1], O_WRONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = ioctl(fd, UBI_IOCMKVOL, &req);
	if (ret)
		printf("failed to create: %s\n", strerror(-ret));
	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_ubimkvol_help[] =
"Usage: ubimkvol <ubidev> <name> <size>\n"
"Create an ubi volume on <ubidev> with name <name> and size <size>\n"
"If size is zero all available space is used for the volume\n";

BAREBOX_CMD_START(ubimkvol)
	.cmd		= do_ubimkvol,
	.usage		= "create an ubi volume",
	BAREBOX_CMD_HELP(cmd_ubimkvol_help)
BAREBOX_CMD_END


static int do_ubiattach(int argc, char *argv[])
{
	int opt;
	struct mtd_info_user user;
	int fd, ret;
	int vid_hdr_offset = 0;

	while((opt = getopt(argc, argv, "O:")) > 0) {
		switch(opt) {
		case 'O':
			vid_hdr_offset = simple_strtoul(optarg, NULL, 0);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = ioctl(fd, MEMGETINFO, &user);
	if (ret) {
		printf("MEMGETINFO failed: %s\n", strerror(-ret));
		goto err;
	}

	ret = ubi_attach_mtd_dev(user.mtd, UBI_DEV_NUM_AUTO, vid_hdr_offset, 20);
	if (ret < 0)
		printf("failed to attach: %s\n", strerror(-ret));
	else
		ret = 0;
err:
	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_ubiattach_help[] =
"Usage: ubiattach [-O vid-hdr-offset] <mtddev>\n"
"Attach <mtddev> to ubi\n";

BAREBOX_CMD_START(ubiattach)
	.cmd		= do_ubiattach,
	.usage		= "attach a mtd dev to ubi",
	BAREBOX_CMD_HELP(cmd_ubiattach_help)
BAREBOX_CMD_END

static int do_ubidetach(int argc, char *argv[])
{
	int ubi_num, ret;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	ubi_num = simple_strtoul(argv[1], NULL, 0);
	ret = ubi_detach_mtd_dev(ubi_num, 1);

	if (ret)
		printf("failed to detach: %s\n", strerror(-ret));

	return ret;
}

static const __maybe_unused char cmd_ubidetach_help[] =
"Usage: ubidetach <ubinum>\n"
"Detach <ubinum> from ubi\n";

BAREBOX_CMD_START(ubidetach)
	.cmd		= do_ubidetach,
	.usage		= "detach an ubi dev",
	BAREBOX_CMD_HELP(cmd_ubidetach_help)
BAREBOX_CMD_END

static int do_ubirmvol(int argc, char *argv[])
{
	struct ubi_mkvol_req req;
	int fd, ret;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	strncpy(req.name, argv[2], UBI_VOL_NAME_MAX);
	req.name[UBI_VOL_NAME_MAX] = 0;

	fd = open(argv[1], O_WRONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = ioctl(fd, UBI_IOCRMVOL, &req);
	if (ret)
		printf("failed to delete: %s\n", strerror(-ret));
	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_ubirmvol_help[] =
"Usage: ubirmvol <ubidev> <name>\n"
"Delete ubi volume <name> from <ubidev>\n";

BAREBOX_CMD_START(ubirmvol)
	.cmd		= do_ubirmvol,
	.usage		= "delete an ubi volume",
	BAREBOX_CMD_HELP(cmd_ubirmvol_help)
BAREBOX_CMD_END


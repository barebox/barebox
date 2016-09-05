#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <ioctl.h>
#include <errno.h>
#include <getopt.h>
#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/ubi-user.h>
#include <mtd/ubi-media.h>

static int do_ubiupdatevol(int argc, char *argv[])
{
	int count, fd_img, fd_vol, ret = 0;
	uint64_t size = 0;
	struct stat st;
	void *buf;

	if (argc - optind < 2)
		return COMMAND_ERROR_USAGE;

	if (stat(argv[optind + 1], &st)) {
		perror("stat image");
		return 1;
	}

	size = st.st_size;

	if (size == FILESIZE_MAX) {
		printf("%s has unknown filesize, this is not supported\n",
		       argv[optind + 1]);
		return 1;
	}

	fd_img  = open(argv[optind + 1], O_RDONLY);
	if (fd_img < 0) {
		perror("open image");
		return 1;
	}

	fd_vol = open(argv[optind], O_WRONLY);
	if (fd_vol < 0) {
		perror("open volume");
		ret = 1;
		goto error_img;
	}

	ret = ioctl(fd_vol, UBI_IOCVOLUP, &size);
	if (ret) {
		printf("failed to start update: %s\n", strerror(-ret));
		goto error;
	}

	buf = xmalloc(RW_BUF_SIZE);

	while (size) {

		count = read(fd_img, buf, RW_BUF_SIZE);
		if (count < 0) {
			perror("read");
			ret = 1;
			break;
		}

		count = write(fd_vol, buf, count);
		if (count < 0) {
			perror("write");
			ret = 1;
			break;
		}

		size -= count;
	}

	free(buf);

error:
	close(fd_vol);
error_img:
	close(fd_img);
	return ret;
}


BAREBOX_CMD_HELP_START(ubiupdatevol)
BAREBOX_CMD_HELP_TEXT("Update UBI volume with an image.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubiupdatevol)
	.cmd		= do_ubiupdatevol,
	BAREBOX_CMD_DESC("Update an UBI volume")
	BAREBOX_CMD_OPTS("UBIVOL IMAGE")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_ubiupdatevol_help)
BAREBOX_CMD_END


static int do_ubimkvol(int argc, char *argv[])
{
	int opt;
	struct ubi_mkvol_req req;
	int fd, ret;
	uint64_t size;

	req.vol_type = UBI_DYNAMIC_VOLUME;

	while ((opt = getopt(argc, argv, "t:")) > 0) {
		switch (opt) {
		case 't':
			if (!strcmp(optarg, "dynamic"))
				req.vol_type = UBI_DYNAMIC_VOLUME;
			else if (!strcmp(optarg, "static"))
				req.vol_type = UBI_STATIC_VOLUME;
			else
				return COMMAND_ERROR_USAGE;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind != 3)
		return COMMAND_ERROR_USAGE;

	size = strtoull_suffix(argv[optind+2], NULL, 0);
	req.name_len = min_t(int, strlen(argv[optind+1]), UBI_VOL_NAME_MAX);
	strncpy(req.name, argv[optind+1], req.name_len);
	req.name[req.name_len] = 0;

	req.bytes = size;
	req.vol_id = UBI_VOL_NUM_AUTO;
	req.alignment = 1;

	fd = open(argv[optind], O_WRONLY);
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


BAREBOX_CMD_HELP_START(ubimkvol)
BAREBOX_CMD_HELP_TEXT("Create an UBI volume on UBIDEV with NAME and SIZE.")
BAREBOX_CMD_HELP_TEXT("If SIZE is 0 all available space is used for the volume.")
BAREBOX_CMD_HELP_OPT("-t <static|dynamic>",  "volume type, default is dynamic")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubimkvol)
	.cmd		= do_ubimkvol,
	BAREBOX_CMD_DESC("create an UBI volume")
	BAREBOX_CMD_OPTS("[-t] UBIDEV NAME SIZE")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_ubimkvol_help)
BAREBOX_CMD_END


static int do_ubiattach(int argc, char *argv[])
{
	int opt;
	struct mtd_info_user user;
	int fd, ret;
	int vid_hdr_offset = 0;
	int devnum = UBI_DEV_NUM_AUTO;

	while((opt = getopt(argc, argv, "d:O:")) > 0) {
		switch(opt) {
		case 'd':
			devnum = simple_strtoul(optarg, NULL, 0);
			break;
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

	ret = ubi_attach_mtd_dev(user.mtd, devnum, vid_hdr_offset, 20);
	if (ret < 0)
		printf("failed to attach: %s\n", strerror(-ret));
	else
		ret = 0;
err:
	close(fd);

	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(ubiattach)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-d DEVNUM",  "device number")
BAREBOX_CMD_HELP_OPT ("-O OFFS",  "VID header offset")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubiattach)
	.cmd		= do_ubiattach,
	BAREBOX_CMD_DESC("attach mtd device to UBI")
	BAREBOX_CMD_OPTS("[-dO] MTDDEV")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_ubiattach_help)
BAREBOX_CMD_END

static int do_ubidetach(int argc, char *argv[])
{
	int fd, ret;
	struct mtd_info_user user;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		int ubi_num = simple_strtoul(argv[1], NULL, 0);
		ret = ubi_detach(ubi_num);
		goto out;
	}

	ret = ioctl(fd, MEMGETINFO, &user);
	if (!ret) {
		int ubi_num = ubi_num_get_by_mtd(user.mtd);
		if (ubi_num < 0) {
			ret = ubi_num;
			goto out;
		}

		ret = ubi_detach(ubi_num);
		if (!ret)
			goto out_close;
	}

out_close:
	close(fd);

out:
	if (ret)
		printf("failed to detach: %s\n", strerror(-ret));

	return ret;
}

BAREBOX_CMD_START(ubidetach)
	.cmd		= do_ubidetach,
	BAREBOX_CMD_DESC("detach an UBI device")
	BAREBOX_CMD_OPTS("mtd device/UBINUM")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
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

BAREBOX_CMD_HELP_START(ubirmvol)
BAREBOX_CMD_HELP_TEXT("Delete UBI volume NAME from UBIDEV")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubirmvol)
	.cmd		= do_ubirmvol,
	BAREBOX_CMD_DESC("delete an UBI volume")
	BAREBOX_CMD_OPTS("UBIDEV NAME")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_ubirmvol_help)
BAREBOX_CMD_END

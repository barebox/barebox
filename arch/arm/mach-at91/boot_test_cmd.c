/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <command.h>
#include <libbb.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>

static int do_at91_boot_test(int argc, char *argv[])
{
	int opt;
	u32 *buf32;
	void *buf;
	void (*jump)(void) = NULL;
	int fd;
	int ret = 1;
	char *sram = "/dev/sram0";
	u32 read_size, write_size;
	u32 tmp = 0;

	while ((opt = getopt(argc, argv, "j:s:")) > 0) {
		switch (opt) {
		case 'j':
			jump = (void*)simple_strtoul(optarg, NULL, 0);
			break;
		case 's':
			sram = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc < optind + 1)
		return COMMAND_ERROR_USAGE;

	buf32 = buf = read_file(argv[optind], &read_size);
	if (!buf)
		return -EINVAL;

	write_size = buf32[5];

	printf("size of the size %d\n", read_size);
	printf("size to load in sram %d\n", write_size);

	if (write_size > read_size) {
		printf("file smaller than requested sram loading size (%d < %d)\n", write_size, read_size);
		goto err;
	}

	fd = open(sram, O_WRONLY);
	if (fd < 0) {
		printf("could not open %s: %s\n", sram, errno_str());
		ret = fd;
		goto err;
	}

	while (write_size) {
		tmp = write(fd, buf, write_size);
		if (tmp < 0) {
			perror("write");
			goto err_open;
		}
		buf += tmp;
		write_size -= tmp;
	}

	shutdown_barebox();

	jump();

err_open:
	close(fd);
err:
	free(buf);
	return ret;
}

BAREBOX_CMD_HELP_START(at91_boot_test)
BAREBOX_CMD_HELP_USAGE("at91_boot_test [-j <jump addr>] [-s <sram>] file\n")
BAREBOX_CMD_HELP_SHORT("upload the binary to sram and jump as will do the romcode\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(at91_boot_test)
	.cmd		= do_at91_boot_test,
	.usage		= "upload the binary to sram and jump as will do the romcode",
	BAREBOX_CMD_HELP(cmd_at91_boot_test_help)
BAREBOX_CMD_END

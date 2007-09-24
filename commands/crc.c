#include <common.h>
#include <command.h>
#include <fs.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>
#include <linux/ctype.h>

int do_crc (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong start = 0, size = ~0, total = 0, now;
	ulong crc = 0, vcrc = 0;
	char *filename = "/dev/mem";
	char *buf;
	int fd, opt, err = 0, filegiven = 0, verify = 0;

	getopt_reset();

	while((opt = getopt(argc, argv, "f:v:")) > 0) {
		switch(opt) {
		case 'f':
			filename = optarg;
			filegiven = 1;
			break;
		case 'v':
			verify = 1;
			vcrc = simple_strtoul(optarg, NULL, 0);
			break;
		}
	}

	if (!filegiven && optind == argc) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	if (optind < argc) {
		if (parse_area_spec(argv[optind], &start, &size)) {
			printf("could not parse area description: %s\n", argv[optind]);
			return 1;
		}
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("open %s: %s\n", filename, errno_str());
		return 1;
	}

	if (lseek(fd, start, SEEK_SET) < 0) {
		printf("file is smaller than start address\n");
		err = 1;
		goto out;
	}

	buf = xmalloc(4096);

	while (size) {
		now = min((ulong)4096, size);
		now = read(fd, buf, now);
		if (!now)
			break;
		crc = crc32(crc, buf, now);
		size -= now;
		total += now;
	}

	printf ("CRC32 for %s 0x%08lx ... 0x%08lx ==> 0x%08lx",
			filename, start, start + total - 1, crc);

	if (verify && crc != vcrc) {
		printf(" != 0x%08x ** ERROR **", vcrc);
		err = 1;
	}

	printf("\n");

	free(buf);
out:
	close(fd);

	return err;
}

static __maybe_unused char cmd_crc_help[] =
"Usage: crc32 [OPTION] [AREA]\n"
"Calculate a crc32 checksum of a memory area\n"
"Options:\n"
"  -f <file>   Use file instead of memory\n"
"  -v <crc>    Verfify\n";

U_BOOT_CMD_START(crc32)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_crc,
	.usage		= "crc32 checksum calculation",
	U_BOOT_CMD_HELP(cmd_crc_help)
U_BOOT_CMD_END


#include <common.h>
#include <command.h>
#include <image.h>
#include <fs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>

static int image_info(image_header_t *hdr)
{
	u32 len, checksum;

	if (image_get_magic(hdr) != IH_MAGIC) {
		puts ("   Bad Magic Number\n");
		return 1;
	}

	len = image_get_header_size();

	checksum = image_get_hcrc(hdr);
	hdr->ih_hcrc = 0;

	if (crc32 (0, hdr, len) != checksum) {
		puts ("   Bad Header Checksum\n");
		return 1;
	}

	image_print_contents(hdr, NULL);

	return 0;
}

static int do_iminfo(struct command *cmdtp, int argc, char *argv[])
{
	int rcode = 1;
	int fd;
	int ret;
	image_header_t hdr;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = read(fd, &hdr, sizeof(image_header_t));
	if (ret != sizeof(image_header_t))
		goto err_out;

	printf("Image at %s:\n", argv[1]);
	image_info(&hdr);

err_out:
	close(fd);

	return rcode;
}

BAREBOX_CMD_HELP_START(iminfo)
BAREBOX_CMD_HELP_USAGE("iminfo\n")
BAREBOX_CMD_HELP_SHORT("Print header information for an application image.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(iminfo)
	.cmd		= do_iminfo,
	.usage		= "print header information for an application image",
	BAREBOX_CMD_HELP(cmd_iminfo_help)
BAREBOX_CMD_END

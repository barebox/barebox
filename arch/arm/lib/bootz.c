#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <asm/armlinux.h>
#include <asm/system.h>

struct zimage_header {
	u32	unused[9];
	u32	magic;
	u32	start;
	u32	end;
};

#define ZIMAGE_MAGIC 0x016F2818

static int do_bootz(struct command *cmdtp, int argc, char *argv[])
{
	int fd, ret, swap = 0;
	struct zimage_header header;
	void *zimage;
	u32 end;

	if (argc != 2) {
		barebox_cmd_usage(cmdtp);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = read(fd, &header, sizeof(header));
	if (ret < sizeof(header)) {
		printf("could not read %s\n", argv[1]);
		goto err_out;
	}

	switch (header.magic) {
#ifdef CONFIG_BOOT_ENDIANNESS_SWITCH
	case swab32(ZIMAGE_MAGIC):
		swap = 1;
		/* fall through */
#endif
	case ZIMAGE_MAGIC:
		break;
	default:
		printf("invalid magic 0x%08x\n", header.magic);
		goto err_out;
	}

	end = header.end;

	if (swap)
		end = swab32(end);

	zimage = xmalloc(end);
	memcpy(zimage, &header, sizeof(header));

	ret = read(fd, zimage + sizeof(header), end - sizeof(header));
	if (ret < end - sizeof(header)) {
		printf("could not read %s\n", argv[1]);
		goto err_out1;
	}

	if (swap) {
		void *ptr;
		for (ptr = zimage; ptr < zimage + end; ptr += 4)
			*(u32 *)ptr = swab32(*(u32 *)ptr);
	}

	printf("loaded zImage from %s with size %d\n", argv[1], end);

	start_linux(zimage, swap, NULL);

	return 0;

err_out1:
	free(zimage);
err_out:
	close(fd);

	return 1;
}

static const __maybe_unused char cmd_bootz_help[] =
"Usage: bootz [FILE]\n"
"Boot a Linux zImage\n";

BAREBOX_CMD_START(bootz)
	.cmd            = do_bootz,
	.usage          = "bootz - start a zImage",
	BAREBOX_CMD_HELP(cmd_bootz_help)
BAREBOX_CMD_END


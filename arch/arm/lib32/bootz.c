#include <common.h>
#include <command.h>
#include <fs.h>
#include <of.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <linux/sizes.h>
#include <asm/byteorder.h>
#include <asm/armlinux.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <memory.h>

struct zimage_header {
	u32	unused[9];
	u32	magic;
	u32	start;
	u32	end;
};

#define ZIMAGE_MAGIC 0x016F2818

static int do_bootz(int argc, char *argv[])
{
	int fd, ret, swap = 0;
	struct zimage_header __header, *header;
	void *zimage;
	void *oftree = NULL;
	u32 end;
	int usemap = 0;
	struct memory_bank *bank = list_first_entry(&memory_banks, struct memory_bank, list);
	struct resource *res = NULL;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	/*
	 * We can save the memcpy of the zImage if it already is in
	 * the first 128MB of SDRAM.
	 */
	zimage = memmap(fd, PROT_READ);
	if (zimage && (unsigned long)zimage  >= bank->start &&
			(unsigned long)zimage < bank->start + SZ_128M) {
		usemap = 1;
		header = zimage;
	}

	if (!usemap) {
		header = &__header;
		ret = read(fd, header, sizeof(*header));
		if (ret < sizeof(*header)) {
			printf("could not read %s\n", argv[1]);
			goto err_out;
		}
	}

	switch (header->magic) {
#ifdef CONFIG_BOOT_ENDIANNESS_SWITCH
	case swab32(ZIMAGE_MAGIC):
		swap = 1;
		/* fall through */
#endif
	case ZIMAGE_MAGIC:
		break;
	default:
		printf("invalid magic 0x%08x\n", header->magic);
		goto err_out;
	}

	end = header->end;

	if (swap)
		end = swab32(end);

	if (!usemap) {
		if (bank->size <= SZ_128M) {
			zimage = xmalloc(end);
		} else {
			zimage = (void *)bank->start + SZ_8M;
			res = request_sdram_region("zimage",
					bank->start + SZ_8M, end);
			if (!res) {
				printf("can't request region for kernel\n");
				goto err_out1;
			}
		}

		memcpy(zimage, header, sizeof(*header));

		ret = read(fd, zimage + sizeof(*header), end - sizeof(*header));
		if (ret < end - sizeof(*header)) {
			printf("could not read %s\n", argv[1]);
			goto err_out2;
		}
	}

	if (swap) {
		void *ptr;
		for (ptr = zimage; ptr < zimage + end; ptr += 4)
			*(u32 *)ptr = swab32(*(u32 *)ptr);
	}

	printf("loaded zImage from %s with size %d\n", argv[1], end);
#ifdef CONFIG_OFTREE
	oftree = of_get_fixed_tree(NULL);
#endif

	start_linux(zimage, swap, 0, 0, oftree, ARM_STATE_SECURE);

	return 0;

err_out2:
	if (res)
		release_sdram_region(res);
err_out1:
	free(zimage);
err_out:
	close(fd);

	return 1;
}

BAREBOX_CMD_START(bootz)
	.cmd            = do_bootz,
	BAREBOX_CMD_DESC("boot Linux zImage")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
BAREBOX_CMD_END


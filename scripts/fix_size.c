#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#ifndef _BSD_SOURCE
#define _BSD_SOURCE             /* See feature_test_macros(7) */
#endif
#include <endian.h>

int main(int argc, char**argv)
{
	struct stat s;
	int c;
	int fd;
	uint64_t offset = 0;
	uint32_t size = 0;
	char *file = NULL;
	int ret = 1;
	int is_bigendian = 0;

	while ((c = getopt (argc, argv, "hf:o:b")) != -1) {
		switch (c) {
		case 'f':
			file = optarg;
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 16);
			break;
		case 'b':
			is_bigendian = 1;
			break;
		}
	}

	if (!file) {
		fprintf(stderr, "missing file\n");
		return 1;
	}

	if (stat(file, &s)) {
		perror("stat");
		return 1;
	}

	fd = open(file, O_WRONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0) {
		perror("lseek");
		ret = 1;
		goto err;
	}

	size = s.st_size;

	if (is_bigendian)
		size = htobe32(size);
	else
		size = htole32(size);

	ret = write(fd, &size, 4);
	if (ret != 4) {
		perror("write");
		ret = 1;
		goto err;
	}

	ret = 0;
err:

	close(fd);

	return ret;
}

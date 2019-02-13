#define _BSD_SOURCE             /* See feature_test_macros(7) */
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <endian.h>

int main(int argc, char**argv)
{
	struct stat s;
	int c;
	int fd;
	uint32_t size = 0;
	char *file = NULL;
	int ret = 1;
	int is_bigendian = 0;
	char magic[8];
	int ignore_unknown = 0;

	while ((c = getopt (argc, argv, "if:b")) != -1) {
		switch (c) {
		case 'f':
			file = optarg;
			break;
		case 'b':
			is_bigendian = 1;
			break;
		case 'i':
			ignore_unknown = 1;
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

	fd = open(file, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = lseek(fd, 0x20, SEEK_SET);
	if (ret < 0) {
		perror("lseek");
		ret = 1;
		goto err;
	}

	ret = read(fd, magic, sizeof(magic));
	if (ret < 0) {
		perror("read");
		ret = 1;
		goto err;
	}

	if (strcmp(magic, "barebox")) {
		if (ignore_unknown) {
			ret = 0;
		} else {
			fprintf(stderr, "invalid magic\n");
			ret = 1;
		}
		goto err;
	}

	ret = lseek(fd, 0x2c, SEEK_SET);
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

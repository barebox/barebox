#include <common.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <libbb.h>

#define RW_BUF_SIZE	(ulong)4096

/**
 * @param[in] src FIXME
 * @param[out] dst FIXME
 */
int copy_file(const char *src, const char *dst)
{
	char *rw_buf = NULL;
	int srcfd = 0, dstfd = 0;
	int r, w;
	int ret = 1;
	void *buf;

	rw_buf = xmalloc(RW_BUF_SIZE);

	srcfd = open(src, O_RDONLY);
	if (srcfd < 0) {
		printf("could not open %s: %s\n", src, errno_str());
		goto out;
	}

	dstfd = open(dst, O_WRONLY | O_CREAT);
	if (dstfd < 0) {
		printf("could not open %s: %s\n", dst, errno_str());
		goto out;
	}

	while(1) {
		r = read(srcfd, rw_buf, RW_BUF_SIZE);
		if (r < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			break;

		buf = rw_buf;
		while (r) {
			w = write(dstfd, buf, r);
			if (w < 0) {
				perror("write");
				goto out;
			}
			buf += w;
			r -= w;
		}
	}

	ret = 0;
out:
	free(rw_buf);
	if (srcfd > 0)
		close(srcfd);
	if (dstfd > 0)
		close(dstfd);

	return ret;
}


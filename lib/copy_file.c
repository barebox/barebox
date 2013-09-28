#include <common.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <libbb.h>
#include <progress.h>

/**
 * @param[in] src FIXME
 * @param[out] dst FIXME
 * @param[in] verbose FIXME
 */
int copy_file(const char *src, const char *dst, int verbose)
{
	char *rw_buf = NULL;
	int srcfd = 0, dstfd = 0;
	int r, w;
	int ret = 1;
	void *buf;
	int total = 0;
	struct stat statbuf;

	rw_buf = xmalloc(RW_BUF_SIZE);

	srcfd = open(src, O_RDONLY);
	if (srcfd < 0) {
		printf("could not open %s: %s\n", src, errno_str());
		goto out;
	}

	dstfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
	if (dstfd < 0) {
		printf("could not open %s: %s\n", dst, errno_str());
		goto out;
	}

	if (verbose) {
		if (stat(src, &statbuf) < 0)
			statbuf.st_size = 0;

		init_progression_bar(statbuf.st_size);
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
			total += w;
		}

		if (verbose) {
			if (statbuf.st_size && statbuf.st_size != FILESIZE_MAX)
				show_progress(total);
			else
				show_progress(total / 16384);
		}
	}

	ret = 0;
out:
	if (verbose)
		putchar('\n');

	free(rw_buf);
	if (srcfd > 0)
		close(srcfd);
	if (dstfd > 0)
		close(dstfd);

	return ret;
}


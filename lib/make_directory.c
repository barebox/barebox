// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <errno.h>
#ifdef __BAREBOX__
#include <fs.h>
#include <libfile.h>
#include <malloc.h>
#include <common.h>
#define STATIC
#else
#define STATIC static inline
#endif

STATIC int make_directory(const char *dir)
{
	char *s = strdup(dir);
	char *path = s;
	char c;
	int ret = 0;

	if (!s)
		return -ENOMEM;

	do {
		c = 0;

		/* Bypass leading non-'/'s and then subsequent '/'s. */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;		/* Save the current char */
				*s = 0;		/* and replace it with nul. */
				break;
			}
			++s;
		}

		if (mkdir(path, 0777) < 0) {

			/* If we failed for any other reason than the directory
			 * already exists, output a diagnostic and return -1.*/
			if (errno != EEXIST) {
				ret = -errno;
				break;
			}
		}
		if (!c)
			goto out;

		/* Remove any inserted nul from the path (recursive mode). */
		*s = c;

	} while (1);

out:
	free(path);
	if (ret)
		errno = -ret;
	return ret;
}
#ifdef __BAREBOX__
EXPORT_SYMBOL(make_directory);
#endif


#include <string.h>
#include <errno.h>
#ifdef __BAREBOX__
#include <fs.h>
#include <malloc.h>
#include <common.h>
#endif

int make_directory(const char *dir)
{
	char *s = strdup(dir);
	char *path = s;
	char c;

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
#ifdef __BAREBOX__
			if (errno != -EEXIST)
#else
			if (errno != EEXIST)
#endif
				break;
		}
		if (!c)
			goto out;

		/* Remove any inserted nul from the path (recursive mode). */
		*s = c;

	} while (1);

out:
	free(path);
	return errno;
}
#ifdef __BAREBOX__
EXPORT_SYMBOL(make_directory);
#endif

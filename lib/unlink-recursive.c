#include <common.h>
#include <libfile.h>
#include <errno.h>
#include <libbb.h>
#include <fs.h>

static char unlink_recursive_failedpath[PATH_MAX];

static int file_action(const char *filename, struct stat *statbuf,
			    void *userdata, int depth)
{
	if (unlink(filename)) {
		strcpy(unlink_recursive_failedpath, filename);
		return 0;
	}

	return 1;
}

static int dir_action(const char *dirname, struct stat *statbuf,
			    void *userdata, int depth)
{
	if (rmdir(dirname)) {
		strcpy(unlink_recursive_failedpath, dirname);
		return 0;
	}

	return 1;
}

int unlink_recursive(const char *path, char **failedpath)
{
	int ret;

	if (failedpath)
		*failedpath = NULL;

	ret = recursive_action(path, ACTION_RECURSE | ACTION_DEPTHFIRST,
			file_action, dir_action, NULL, 0);

	if (!ret && failedpath)
		*failedpath = unlink_recursive_failedpath;

	return ret ? 0 : -errno;
}

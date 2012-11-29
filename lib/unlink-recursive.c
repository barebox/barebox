#include <common.h>
#include <libbb.h>
#include <fs.h>

static char unlink_recursive_failedpath[PATH_MAX];

struct data {
	int error;
};

static int file_action(const char *filename, struct stat *statbuf,
			    void *userdata, int depth)
{
	struct data *data = userdata;
	int ret;

	ret = unlink(filename);
	if (ret) {
		strcpy(unlink_recursive_failedpath, filename);
		data->error = ret;
	}

	return ret ? 0 : 1;
}

static int dir_action(const char *dirname, struct stat *statbuf,
			    void *userdata, int depth)
{
	struct data *data = userdata;
	int ret;

	ret = rmdir(dirname);
	if (ret) {
		strcpy(unlink_recursive_failedpath, dirname);
		data->error = ret;
	}

	return ret ? 0 : 1;
}

int unlink_recursive(const char *path, char **failedpath)
{
	struct data data = {};
	int ret;

	if (failedpath)
		*failedpath = NULL;

	ret = recursive_action(path, ACTION_RECURSE | ACTION_DEPTHFIRST,
			file_action, dir_action, &data, 0);

	if (!ret && failedpath)
		*failedpath = unlink_recursive_failedpath;

	return ret ? 0 : errno;
}

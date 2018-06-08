#include <bbu.h>
#include <libfile.h>
#include <printk.h>

#include <mach/bbu.h>

struct mvebu_bbu_handler {
	struct bbu_handler bbuh;
	int version;
};

static int mvebu_bbu_flash_update_handler(struct bbu_handler *bbuh,
					  struct bbu_data *data)
{
	struct mvebu_bbu_handler *mbbuh =
		container_of(bbuh, struct mvebu_bbu_handler, bbuh);
	const void *image = data->image;
	size_t size = data->len;
	enum filetype ft = file_detect_type(image, size);

	if ((mbbuh->version == 0 && ft == filetype_kwbimage_v0) ||
	    (mbbuh->version == 1 && ft == filetype_kwbimage_v1) ||
	    data->flags & BBU_FLAG_FORCE) {
		int ret = bbu_confirm(data);
		if (ret)
			return ret;

		return write_file_flash(bbuh->devicefile, image, size);
	} else {
		pr_err("%s is not a valid kwbimage\n", data->imagefile);
		return -EINVAL;
	}
}

int mvebu_bbu_flash_register_handler(const char *name,
				     char *devicefile, int version,
				     bool isdefault)
{
	struct mvebu_bbu_handler *mbbuh;
	int ret;

	mbbuh = xzalloc(sizeof(*mbbuh));
	mbbuh->bbuh.devicefile = devicefile;
	mbbuh->bbuh.handler = mvebu_bbu_flash_update_handler;
	mbbuh->bbuh.flags = isdefault ? BBU_HANDLER_FLAG_DEFAULT : 0;
	mbbuh->bbuh.name = name;
	mbbuh->version = version;

	ret = bbu_register_handler(&mbbuh->bbuh);
	if (ret)
		free(mbbuh);

	return ret;
}

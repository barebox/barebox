#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <asm/byteorder.h>
#include <init.h>
#include <gui/image_renderer.h>
#include <gui/graphic_utils.h>
#include <linux/zlib.h>

#include "picopng.h"
#include "png.h"

unsigned picopng_zlib_decompress(unsigned char* out, size_t outsize,
				 const unsigned char* in, size_t insize)
{
	int err;

	png_stream.next_in = in;
	png_stream.avail_in = insize;

	png_stream.next_out = out;
	png_stream.avail_out = outsize;

	err = zlib_inflateReset(&png_stream);
	if (err != Z_OK) {
		printk("zlib_inflateReset error %d\n", err);
		zlib_inflateEnd(&png_stream);
		zlib_inflateInit(&png_stream);
	}

	err = zlib_inflate(&png_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		goto err;
	return 0;

err:
	printk("Error %d while decompressing!\n", err);
	printk("%p(%zd)->%p(%zd)\n", in, insize, out, outsize);
	return -EIO;
}

struct image *png_open(char *inbuf, int insize)
{
	PNG_info_t *png_info;
	int ret;
	struct image *img = calloc(1, sizeof(struct image));

	if (!img)
		return ERR_PTR(-ENOMEM);

	ret = png_uncompress_init();
	if (ret)
		goto err;

	/* rgba */
	png_info = PNG_decode(inbuf, insize);

	if(PNG_error) {
		printf("error %u:\n", PNG_error);
		ret = -EINVAL;
		goto err;
	}

	img->width = png_info->width;
	img->height = png_info->height;
	img->bits_per_pixel = 4 << 3;
	img->data = png_info->image->data;

	pr_debug("png: %d x %d data@0x%p\n", img->width, img->height, img->data);

	png_alloc_free_all();

	return img;
err:
	png_alloc_free_all();
	free(img);
	return ERR_PTR(ret);
}

void png_close(struct image *img)
{
	free(img->data);
	png_alloc_free_all();
}

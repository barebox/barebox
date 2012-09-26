#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <asm/byteorder.h>
#include <init.h>
#include <gui/image_renderer.h>
#include <gui/graphic_utils.h>
#include <linux/zlib.h>

#include "lodepng.h"
#include "png.h"

unsigned lodepng_custom_zlib_decompress(unsigned char** out, size_t* outsize,
					const unsigned char* in, size_t insize,
					const LodePNGDecompressSettings* settings)
{
	int err;

	png_stream.next_in = in;
	png_stream.avail_in = insize;

	png_stream.next_out = *out;
	png_stream.avail_out = *outsize;

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
	printk("%p(%zd)->%p(%zd)\n", in, insize, *out, *outsize);
	return -EIO;
}

struct image *png_open(char *inbuf, int insize)
{
	LodePNGState state;
	int ret;
	unsigned error;
	struct image *img = calloc(1, sizeof(struct image));
	unsigned char *png;

	if (!img)
		return ERR_PTR(-ENOMEM);

	ret = png_uncompress_init();
	if (ret)
		goto err;

	lodepng_state_init(&state);

	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;

	error = lodepng_decode(&png, &img->width, &img->height, &state, inbuf, insize);

	if(error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
		ret = -EINVAL;
		goto err;
	}

	img->bits_per_pixel = 4 << 3;
	img->data = png;

	pr_debug("png: %d x %d data@0x%p\n", img->width, img->height, img->data);

	lodepng_state_cleanup(&state);

	return img;
err:
	free(png);
	free(img);
	return ERR_PTR(ret);
}

void png_close(struct image *img)
{
	free(img->data);
	png_uncompress_exit();
}

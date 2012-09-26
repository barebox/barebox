#ifndef _PICOPNG_H
#define _PICOPNG_H

typedef struct {
	uint32_t *data;
	size_t size;
	size_t allocsize;
} vector32_t;

typedef struct {
	uint8_t *data;
	size_t size;
	size_t allocsize;
} vector8_t;

typedef struct {
	uint32_t width, height;
	uint32_t colorType, bitDepth;
	uint32_t compressionMethod, filterMethod, interlaceMethod;
	uint32_t key_r, key_g, key_b;
	bool key_defined; // is a transparent color key given?
	vector8_t *palette;
	vector8_t *image;
} PNG_info_t;

PNG_info_t *PNG_decode(const uint8_t *in, uint32_t size);
void png_alloc_free_all(void);

unsigned picopng_zlib_decompress(unsigned char* out, size_t outsize,
				 const unsigned char* in, size_t insize);

extern int PNG_error;

#endif

// picoPNG version 20080503 (cleaned up and ported to c by kaitek)
// Copyright (c) 2005-2008 Lode Vandevenne
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//   2. Altered source versions must be plainly marked as such, and must not be
//      misrepresented as being the original software.
//   3. This notice may not be removed or altered from any source distribution.

#include <common.h>
#include <malloc.h>
#include "picopng.h"

/*************************************************************************************************/

typedef struct png_alloc_node {
	struct png_alloc_node *prev, *next;
	void *addr;
	size_t size;
} png_alloc_node_t;

png_alloc_node_t *png_alloc_head = NULL;
png_alloc_node_t *png_alloc_tail = NULL;

png_alloc_node_t *png_alloc_find_node(void *addr)
{
	png_alloc_node_t *node;
	for (node = png_alloc_head; node; node = node->next)
		if (node->addr == addr)
			break;
	return node;
}

void png_alloc_add_node(void *addr, size_t size)
{
	png_alloc_node_t *node;
	if (png_alloc_find_node(addr))
		return;
	node = malloc(sizeof (png_alloc_node_t));
	node->addr = addr;
	node->size = size;
	node->prev = png_alloc_tail;
	node->next = NULL;
	png_alloc_tail = node;
	if (node->prev)
		node->prev->next = node;
	if (!png_alloc_head)
		png_alloc_head = node;
}

void png_alloc_remove_node(png_alloc_node_t *node)
{
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	if (node == png_alloc_head)
		png_alloc_head = node->next;
	if (node == png_alloc_tail)
		png_alloc_tail = node->prev;
	node->prev = node->next = node->addr = NULL;
	free(node);
}

void *png_alloc_malloc(size_t size)
{
	void *addr = malloc(size);
	png_alloc_add_node(addr, size);
	return addr;
}

void *png_alloc_realloc(void *addr, size_t size)
{
	void *new_addr;
	if (!addr)
		return png_alloc_malloc(size);
	new_addr = realloc(addr, size);
	if (new_addr != addr) {
		png_alloc_node_t *old_node;
		old_node = png_alloc_find_node(addr);
		png_alloc_remove_node(old_node);
		png_alloc_add_node(new_addr, size);
	}
	return new_addr;
}

void png_alloc_free(void *addr)
{
	png_alloc_node_t *node = png_alloc_find_node(addr);
	if (!node)
		return;
	png_alloc_remove_node(node);
	free(addr);
}

void png_alloc_free_all()
{
	while (png_alloc_tail) {
		void *addr = png_alloc_tail->addr;
		png_alloc_remove_node(png_alloc_tail);
		free(addr);
	}
}

/*************************************************************************************************/

__maybe_unused void vector32_cleanup(vector32_t *p)
{
	p->size = p->allocsize = 0;
	if (p->data)
		png_alloc_free(p->data);
	p->data = NULL;
}

uint32_t vector32_resize(vector32_t *p, size_t size)
{	// returns 1 if success, 0 if failure ==> nothing done
	if (size * sizeof (uint32_t) > p->allocsize) {
		size_t newsize = size * sizeof (uint32_t) * 2;
		void *data = png_alloc_realloc(p->data, newsize);
		if (data) {
			p->allocsize = newsize;
			p->data = (uint32_t *) data;
			p->size = size;
		} else
			return 0;
	} else
		p->size = size;
	return 1;
}

uint32_t vector32_resizev(vector32_t *p, size_t size, uint32_t value)
{	// resize and give all new elements the value
	size_t oldsize = p->size, i;
	if (!vector32_resize(p, size))
		return 0;
	for (i = oldsize; i < size; i++)
		p->data[i] = value;
	return 1;
}

void vector32_init(vector32_t *p)
{
	p->data = NULL;
	p->size = p->allocsize = 0;
}

vector32_t *vector32_new(size_t size, uint32_t value)
{
	vector32_t *p = png_alloc_malloc(sizeof (vector32_t));
	vector32_init(p);
	if (size && !vector32_resizev(p, size, value))
		return NULL;
	return p;
}

/*************************************************************************************************/

__maybe_unused void vector8_cleanup(vector8_t *p)
{
	p->size = p->allocsize = 0;
	if (p->data)
		png_alloc_free(p->data);
	p->data = NULL;
}

uint32_t vector8_resize(vector8_t *p, size_t size)
{	// returns 1 if success, 0 if failure ==> nothing done
	// xxx: the use of sizeof uint32_t here seems like a bug (this descends from the lodepng vector
	// compatibility functions which do the same). without this there is corruption in certain cases,
	// so this was probably done to cover up allocation bug(s) in the original picopng code!
	if (size * sizeof (uint32_t) > p->allocsize) {
		size_t newsize = size * sizeof (uint32_t) * 2;
		void *data = png_alloc_realloc(p->data, newsize);
		if (data) {
			p->allocsize = newsize;
			p->data = (uint8_t *) data;
			p->size = size;
		} else
			return 0; // error: not enough memory
	} else
		p->size = size;
	return 1;
}

uint32_t vector8_resizev(vector8_t *p, size_t size, uint8_t value)
{	// resize and give all new elements the value
	size_t oldsize = p->size, i;
	if (!vector8_resize(p, size))
		return 0;
	for (i = oldsize; i < size; i++)
		p->data[i] = value;
	return 1;
}

void vector8_init(vector8_t *p)
{
	p->data = NULL;
	p->size = p->allocsize = 0;
}

vector8_t *vector8_new(size_t size, uint8_t value)
{
	vector8_t *p = png_alloc_malloc(sizeof (vector8_t));
	vector8_init(p);
	if (size && !vector8_resizev(p, size, value))
		return NULL;
	return p;
}

vector8_t *vector8_copy(vector8_t *p)
{
	vector8_t *q = vector8_new(p->size, 0);
	uint32_t n;
	for (n = 0; n < q->size; n++)
		q->data[n] = p->data[n];
	return q;
}

/*************************************************************************************************/
int Zlib_decompress(vector8_t *out, const vector8_t *in) // returns error value
{
	return picopng_zlib_decompress(out->data, out->size, in->data, in->size);
}

/*************************************************************************************************/

#define PNG_SIGNATURE	0x0a1a0a0d474e5089ull

#define CHUNK_IHDR		0x52444849
#define CHUNK_IDAT		0x54414449
#define CHUNK_IEND		0x444e4549
#define CHUNK_PLTE		0x45544c50
#define CHUNK_tRNS		0x534e5274

int PNG_error;

uint32_t PNG_readBitFromReversedStream(size_t *bitp, const uint8_t *bits)
{
	uint32_t result = (bits[*bitp >> 3] >> (7 - (*bitp & 0x7))) & 1;
	(*bitp)++;
	return result;
}

uint32_t PNG_readBitsFromReversedStream(size_t *bitp, const uint8_t *bits, uint32_t nbits)
{
	uint32_t i, result = 0;
	for (i = nbits - 1; i < nbits; i--)
		result += ((PNG_readBitFromReversedStream(bitp, bits)) << i);
	return result;
}

void PNG_setBitOfReversedStream(size_t *bitp, uint8_t *bits, uint32_t bit)
{
	bits[*bitp >> 3] |= (bit << (7 - (*bitp & 0x7)));
	(*bitp)++;
}

uint32_t PNG_read32bitInt(const uint8_t *buffer)
{
	return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

int PNG_checkColorValidity(uint32_t colorType, uint32_t bd) // return type is a LodePNG error code
{
	if ((colorType == 2 || colorType == 4 || colorType == 6)) {
		if (!(bd == 8 || bd == 16))
			return 37;
		else
			return 0;
	} else if (colorType == 0) {
		if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16))
			return 37;
		else
			return 0;
	} else if (colorType == 3) {
		if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8))
			return 37;
		else
			return 0;
	} else
		return 31; // nonexistent color type
}

uint32_t PNG_getBpp(const PNG_info_t *info)
{
	uint32_t bitDepth, colorType;
	bitDepth = info->bitDepth;
	colorType = info->colorType;
	if (colorType == 2)
		return (3 * bitDepth);
	else if (colorType >= 4)
		return (colorType - 2) * bitDepth;
	else
		return bitDepth;
}

void PNG_readPngHeader(PNG_info_t *info, const uint8_t *in, size_t inlength)
{	// read the information from the header and store it in the Info
	if (inlength < 29) {
		PNG_error = 27; // error: the data length is smaller than the length of the header
		return;
	}
	if (*(uint64_t *) in != PNG_SIGNATURE) {
		PNG_error = 28; // no PNG signature
		return;
	}
	if (*(uint32_t *) &in[12] != CHUNK_IHDR) {
		PNG_error = 29; // error: it doesn't start with a IHDR chunk!
		return;
	}
	info->width = PNG_read32bitInt(&in[16]);
	info->height = PNG_read32bitInt(&in[20]);
	info->bitDepth = in[24];
	info->colorType = in[25];
	info->compressionMethod = in[26];
	if (in[26] != 0) {
		PNG_error = 32; // error: only compression method 0 is allowed in the specification
		return;
	}
	info->filterMethod = in[27];
	if (in[27] != 0) {
		PNG_error = 33; // error: only filter method 0 is allowed in the specification
		return;
	}
	info->interlaceMethod = in[28];
	if (in[28] > 1) {
		PNG_error = 34; // error: only interlace methods 0 and 1 exist in the specification
		return;
	}
	PNG_error = PNG_checkColorValidity(info->colorType, info->bitDepth);
}

int PNG_paethPredictor(int a, int b, int c) // Paeth predicter, used by PNG filter type 4
{
	int p, pa, pb, pc;
	p = a + b - c;
	pa = p > a ? (p - a) : (a - p);
	pb = p > b ? (p - b) : (b - p);
	pc = p > c ? (p - c) : (c - p);
	return (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
}

void PNG_unFilterScanline(uint8_t *recon, const uint8_t *scanline, const uint8_t *precon,
		size_t bytewidth, uint32_t filterType, size_t length)
{
	size_t i;
	switch (filterType) {
	case 0:
		for (i = 0; i < length; i++)
			recon[i] = scanline[i];
		break;
	case 1:
		for (i = 0; i < bytewidth; i++)
			recon[i] = scanline[i];
		for (i = bytewidth; i < length; i++)
			recon[i] = scanline[i] + recon[i - bytewidth];
		break;
	case 2:
		if (precon)
			for (i = 0; i < length; i++)
				recon[i] = scanline[i] + precon[i];
		else
			for (i = 0; i < length; i++)
				recon[i] = scanline[i];
		break;
	case 3:
		if (precon) {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i] + precon[i] / 2;
			for (i = bytewidth; i < length; i++)
				recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
		} else {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i];
			for (i = bytewidth; i < length; i++)
				recon[i] = scanline[i] + recon[i - bytewidth] / 2;
		}
		break;
	case 4:
		if (precon) {
			for (i = 0; i < bytewidth; i++)
				recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(0, precon[i], 0));
			for (i = bytewidth; i < length; i++)
				recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(recon[i - bytewidth],
						precon[i], precon[i - bytewidth]));
		} else {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i];
			for (i = bytewidth; i < length; i++)
				recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(recon[i - bytewidth], 0, 0));
		}
		break;
	default:
		PNG_error = 36; // error: nonexistent filter type given
		return;
	}
}

void PNG_adam7Pass(uint8_t *out, uint8_t *linen, uint8_t *lineo, const uint8_t *in, uint32_t w,
		size_t passleft, size_t passtop, size_t spacex, size_t spacey, size_t passw, size_t passh,
		uint32_t bpp)
{
	size_t bytewidth, linelength;
	uint32_t y;
	uint8_t *temp;
	// filter and reposition the pixels into the output when the image is Adam7 interlaced. This
	// function can only do it after the full image is already decoded. The out buffer must have
	// the correct allocated memory size already.
	if (passw == 0)
		return;
	bytewidth = (bpp + 7) / 8;
	linelength = 1 + ((bpp * passw + 7) / 8);
	for (y = 0; y < passh; y++) {
		size_t i, b;
		uint8_t filterType = in[y * linelength], *prevline = (y == 0) ? 0 : lineo;
		PNG_unFilterScanline(linen, &in[y * linelength + 1], prevline, bytewidth, filterType,
				(w * bpp + 7) / 8);
		if (PNG_error)
			return;
		if (bpp >= 8)
			for (i = 0; i < passw; i++)
				for (b = 0; b < bytewidth; b++) // b = current byte of this pixel
					out[bytewidth * w * (passtop + spacey * y) + bytewidth *
							(passleft + spacex * i) + b] = linen[bytewidth * i + b];
		else
			for (i = 0; i < passw; i++) {
				size_t obp, bp;
				obp = bpp * w * (passtop + spacey * y) + bpp * (passleft + spacex * i);
				bp = i * bpp;
				for (b = 0; b < bpp; b++)
					PNG_setBitOfReversedStream(&obp, out, PNG_readBitFromReversedStream(&bp, linen));
			}
		temp = linen;
		linen = lineo;
		lineo = temp; // swap the two buffer pointers "line old" and "line new"
	}
}

int PNG_convert(const PNG_info_t *info, vector8_t *out, const uint8_t *in)
{	// converts from any color type to 32-bit. return value = LodePNG error code
	size_t i, c;
	uint32_t bitDepth, colorType;
	size_t numpixels, bp;
	uint8_t *out_data;

	bitDepth = info->bitDepth;
	colorType = info->colorType;
	numpixels = info->width * info->height;
	bp = 0;
	vector8_resize(out, numpixels * 4);
	out_data = out->size ? out->data : 0;
	if (bitDepth == 8 && colorType == 0) // greyscale
		for (i = 0; i < numpixels; i++) {
			out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[i];
			out_data[4 * i + 3] = (info->key_defined && (in[i] == info->key_r)) ? 0 : 255;
		}
	else if (bitDepth == 8 && colorType == 2) // RGB color
		for (i = 0; i < numpixels; i++) {
			for (c = 0; c < 3; c++)
				out_data[4 * i + c] = in[3 * i + c];
			out_data[4 * i + 3] = (info->key_defined && (in[3 * i + 0] == info->key_r) &&
					(in[3 * i + 1] == info->key_g) && (in[3 * i + 2] == info->key_b)) ? 0 : 255;
		}
	else if (bitDepth == 8 && colorType == 3) // indexed color (palette)
		for (i = 0; i < numpixels; i++) {
			if (4U * in[i] >= info->palette->size)
				return 46;
			for (c = 0; c < 4; c++) // get rgb colors from the palette
				out_data[4 * i + c] = info->palette->data[4 * in[i] + c];
		}
	else if (bitDepth == 8 && colorType == 4) // greyscale with alpha
		for (i = 0; i < numpixels; i++) {
			out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[2 * i + 0];
			out_data[4 * i + 3] = in[2 * i + 1];
		}
	else if (bitDepth == 8 && colorType == 6)
		for (i = 0; i < numpixels; i++)
			for (c = 0; c < 4; c++)
				out_data[4 * i + c] = in[4 * i + c]; // RGB with alpha
	else if (bitDepth == 16 && colorType == 0) // greyscale
		for (i = 0; i < numpixels; i++) {
			out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[2 * i];
			out_data[4 * i + 3] = (info->key_defined && (256U * in[i] + in[i + 1] == info->key_r))
					? 0 : 255;
		}
	else if (bitDepth == 16 && colorType == 2) // RGB color
		for (i = 0; i < numpixels; i++) {
			for (c = 0; c < 3; c++)
				out_data[4 * i + c] = in[6 * i + 2 * c];
			out_data[4 * i + 3] = (info->key_defined &&
					(256U * in[6 * i + 0] + in[6 * i + 1] == info->key_r) &&
					(256U * in[6 * i + 2] + in[6 * i + 3] == info->key_g) &&
					(256U * in[6 * i + 4] + in[6 * i + 5] == info->key_b)) ? 0 : 255;
		}
	else if (bitDepth == 16 && colorType == 4) // greyscale with alpha
		for (i = 0; i < numpixels; i++) {
			out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[4 * i]; // msb
			out_data[4 * i + 3] = in[4 * i + 2];
		}
	else if (bitDepth == 16 && colorType == 6)
		for (i = 0; i < numpixels; i++)
			for (c = 0; c < 4; c++)
				out_data[4 * i + c] = in[8 * i + 2 * c]; // RGB with alpha
	else if (bitDepth < 8 && colorType == 0) // greyscale
		for (i = 0; i < numpixels; i++) {
			uint32_t value = (PNG_readBitsFromReversedStream(&bp, in, bitDepth) * 255) /
					((1 << bitDepth) - 1); // scale value from 0 to 255
			out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = (uint8_t) value;
			out_data[4 * i + 3] = (info->key_defined && value &&
					(((1U << bitDepth) - 1U) == info->key_r) && ((1U << bitDepth) - 1U)) ? 0 : 255;
		}
	else if (bitDepth < 8 && colorType == 3) // palette
		for (i = 0; i < numpixels; i++) {
			uint32_t value = PNG_readBitsFromReversedStream(&bp, in, bitDepth);
			if (4 * value >= info->palette->size)
				return 47;
			for (c = 0; c < 4; c++) // get rgb colors from the palette
				out_data[4 * i + c] = info->palette->data[4 * value + c];
		}
	return 0;
}

PNG_info_t *PNG_info_new(void)
{
	PNG_info_t *info = png_alloc_malloc(sizeof (PNG_info_t));
	uint32_t i;
	for (i = 0; i < sizeof (PNG_info_t); i++)
		((uint8_t *) info)[i] = 0;
	info->palette = vector8_new(0, 0);
	info->image = vector8_new(0, 0);
	return info;
}

PNG_info_t *PNG_decode(const uint8_t *in, uint32_t size)
{
	PNG_info_t *info;
	size_t pos;
	vector8_t *idat;
	bool IEND, known_type;
	uint32_t bpp;
	vector8_t *scanlines; // now the out buffer will be filled
	size_t bytewidth, outlength;
	uint8_t *out_data;

	PNG_error = 0;

	if (size == 0 || in == 0) {
		PNG_error = 48; // the given data is empty
		return NULL;
	}
	info = PNG_info_new();
	PNG_readPngHeader(info, in, size);
	if (PNG_error)
		return NULL;
	pos = 33; // first byte of the first chunk after the header
	idat = NULL; // the data from idat chunks
	IEND = false;
	known_type = true;
	info->key_defined = false;
	// loop through the chunks, ignoring unknown chunks and stopping at IEND chunk. IDAT data is
	// put at the start of the in buffer
	while (!IEND) {
		size_t i, j;
		size_t chunkLength;
		uint32_t chunkType;

		if (pos + 8 >= size) {
			PNG_error = 30; // error: size of the in buffer too small to contain next chunk
			return NULL;
		}
		chunkLength = PNG_read32bitInt(&in[pos]);
		pos += 4;
		if (chunkLength > 0x7fffffff) {
			PNG_error = 63;
			return NULL;
		}
		if (pos + chunkLength >= size) {
			PNG_error = 35; // error: size of the in buffer too small to contain next chunk
			return NULL;
		}
		chunkType = *(uint32_t *) &in[pos];
		if (chunkType == CHUNK_IDAT) { // IDAT: compressed image data chunk
			size_t offset = 0;
			if (idat) {
				offset = idat->size;
				vector8_resize(idat, offset + chunkLength);
			} else
				idat = vector8_new(chunkLength, 0);
			for (i = 0; i < chunkLength; i++)
				idat->data[offset + i] = in[pos + 4 + i];
			pos += (4 + chunkLength);
		} else if (chunkType == CHUNK_IEND) { // IEND
			pos += 4;
			IEND = true;
		} else if (chunkType == CHUNK_PLTE) { // PLTE: palette chunk
			pos += 4; // go after the 4 letters
			vector8_resize(info->palette, 4 * (chunkLength / 3));
			if (info->palette->size > (4 * 256)) {
				PNG_error = 38; // error: palette too big
				return NULL;
			}
			for (i = 0; i < info->palette->size; i += 4) {
				for (j = 0; j < 3; j++)
					info->palette->data[i + j] = in[pos++]; // RGB
				info->palette->data[i + 3] = 255; // alpha
			}
		} else if (chunkType == CHUNK_tRNS) { // tRNS: palette transparency chunk
			pos += 4; // go after the 4 letters
			if (info->colorType == 3) {
				if (4 * chunkLength > info->palette->size) {
					PNG_error = 39; // error: more alpha values given than there are palette entries
					return NULL;
				}
				for (i = 0; i < chunkLength; i++)
					info->palette->data[4 * i + 3] = in[pos++];
			} else if (info->colorType == 0) {
				if (chunkLength != 2) {
					PNG_error = 40; // error: this chunk must be 2 bytes for greyscale image
					return NULL;
				}
				info->key_defined = true;
				info->key_r = info->key_g = info->key_b = 256 * in[pos] + in[pos + 1];
				pos += 2;
			} else if (info->colorType == 2) {
				if (chunkLength != 6) {
					PNG_error = 41; // error: this chunk must be 6 bytes for RGB image
					return NULL;
				}
				info->key_defined = true;
				info->key_r = 256 * in[pos] + in[pos + 1];
				pos += 2;
				info->key_g = 256 * in[pos] + in[pos + 1];
				pos += 2;
				info->key_b = 256 * in[pos] + in[pos + 1];
				pos += 2;
			} else {
				PNG_error = 42; // error: tRNS chunk not allowed for other color models
				return NULL;
			}
		} else { // it's not an implemented chunk type, so ignore it: skip over the data
			if (!(in[pos + 0] & 32)) {
				// error: unknown critical chunk (5th bit of first byte of chunk type is 0)
				PNG_error = 69;
				return NULL;
			}
			pos += (chunkLength + 4); // skip 4 letters and uninterpreted data of unimplemented chunk
			known_type = false;
		}
		pos += 4; // step over CRC (which is ignored)
	}
	bpp = PNG_getBpp(info);
	scanlines = vector8_new(((info->width * (info->height * bpp + 7)) / 8) + info->height, 0);
	PNG_error = Zlib_decompress(scanlines, idat);
	if (PNG_error)
		return NULL; // stop if the zlib decompressor returned an error
	bytewidth = (bpp + 7) / 8;
	outlength = (info->height * info->width * bpp + 7) / 8;
	vector8_resize(info->image, outlength); // time to fill the out buffer
	out_data = outlength ? info->image->data : 0;
	if (info->interlaceMethod == 0) { // no interlace, just filter
		size_t y, obp, bp;
		size_t linestart, linelength;
		linestart = 0;
		// length in bytes of a scanline, excluding the filtertype byte
		linelength = (info->width * bpp + 7) / 8;
		if (bpp >= 8) // byte per byte
			for (y = 0; y < info->height; y++) {
				uint32_t filterType = scanlines->data[linestart];
				const uint8_t *prevline;
				prevline = (y == 0) ? 0 : &out_data[(y - 1) * info->width * bytewidth];
				PNG_unFilterScanline(&out_data[linestart - y], &scanlines->data[linestart + 1],
						prevline, bytewidth, filterType, linelength);
				if (PNG_error)
					return NULL;
				linestart += (1 + linelength); // go to start of next scanline
		} else { // less than 8 bits per pixel, so fill it up bit per bit
			vector8_t *templine; // only used if bpp < 8
			templine = vector8_new((info->width * bpp + 7) >> 3, 0);
			for (y = 0, obp = 0; y < info->height; y++) {
				uint32_t filterType = scanlines->data[linestart];
				const uint8_t *prevline;
				prevline = (y == 0) ? 0 : &out_data[(y - 1) * info->width * bytewidth];
				PNG_unFilterScanline(templine->data, &scanlines->data[linestart + 1], prevline,
						bytewidth, filterType, linelength);
				if (PNG_error)
					return NULL;
				for (bp = 0; bp < info->width * bpp;)
					PNG_setBitOfReversedStream(&obp, out_data, PNG_readBitFromReversedStream(&bp,
							templine->data));
				linestart += (1 + linelength); // go to start of next scanline
			}
		}
	} else { // interlaceMethod is 1 (Adam7)
		int i;
		vector8_t *scanlineo, *scanlinen; // "old" and "new" scanline
		size_t passw[7] = {
			(info->width + 7) / 8, (info->width + 3) / 8, (info->width + 3) / 4,
			(info->width + 1) / 4, (info->width + 1) / 2, (info->width + 0) / 2,
			(info->width + 0) / 1
		};
		size_t passh[7] = {
			(info->height + 7) / 8, (info->height + 7) / 8, (info->height + 3) / 8,
			(info->height + 3) / 4, (info->height + 1) / 4, (info->height + 1) / 2,
			(info->height + 0) / 2
		};
		size_t passstart[7] = { 0 };
		size_t pattern[28] = { 0, 4, 0, 2, 0, 1, 0, 0, 0, 4, 0, 2, 0, 1, 8, 8, 4, 4, 2, 2, 1, 8, 8,
				8, 4, 4, 2, 2 }; // values for the adam7 passes
		for (i = 0; i < 6; i++)
			passstart[i + 1] = passstart[i] + passh[i] * ((passw[i] ? 1 : 0) + (passw[i] * bpp + 7) / 8);
		scanlineo = vector8_new((info->width * bpp + 7) / 8, 0);
		scanlinen = vector8_new((info->width * bpp + 7) / 8, 0);
		for (i = 0; i < 7; i++)
			PNG_adam7Pass(out_data, scanlinen->data, scanlineo->data, &scanlines->data[passstart[i]],
					info->width, pattern[i], pattern[i + 7], pattern[i + 14], pattern[i + 21],
					passw[i], passh[i], bpp);
	}
	if (info->colorType != 6 || info->bitDepth != 8) { // conversion needed
		vector8_t *copy = vector8_copy(info->image); // xxx: is this copy necessary?
		PNG_error = PNG_convert(info, info->image, copy->data);
	}
	return info;
}

/*************************************************************************************************/

#ifdef TEST

#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	char *fname = (argc > 1) ? argv[1] : "test.png";
	PNG_info_t *info;
	struct stat statbuf;
	uint32_t insize, outsize;
	FILE *infp, *outfp;
	uint8_t *inbuf;
	uint32_t n;

	if (stat(fname, &statbuf) != 0) {
		perror("stat");
		return 1;
	} else if (!statbuf.st_size) {
		printf("file empty\n");
		return 1;
	}
	insize = (uint32_t) statbuf.st_size;
	inbuf = malloc(insize);
	infp = fopen(fname, "rb");
	if (!infp) {
		perror("fopen");
		return 1;
	} else if (fread(inbuf, 1, insize, infp) != insize) {
		perror("fread");
		return 1;
	}
	fclose(infp);

	printf("input file: %s (size: %d)\n", fname, insize);

	info = PNG_decode(inbuf, insize);
	free(inbuf);
	printf("PNG_error: %d\n", PNG_error);
	if (PNG_error != 0)
		return 1;

	printf("width: %d, height: %d\nfirst 16 bytes: ", info->width, info->height);
	for (n = 0; n < 16; n++)
		printf("%02x ", info->image->data[n]);
	printf("\n");

	outsize = info->width * info->height * 4;
	printf("image size: %d\n", outsize);
	if (outsize != info->image->size) {
		printf("error: image size doesn't match dimensions\n");
		return 1;
	}
	outfp = fopen("out.bin", "wb");
	if (!outfp) {
		perror("fopen");
		return 1;
	} else if (fwrite(info->image->data, 1, outsize, outfp) != outsize) {
		perror("fwrite");
		return 1;
	}
	fclose(outfp);

#ifdef ALLOC_DEBUG
	png_alloc_node_t *node;
	for (node = png_alloc_head, n = 1; node; node = node->next, n++)
		printf("node %d (%p) addr = %p, size = %ld\n", n, node, node->addr, node->size);
#endif
	png_alloc_free_all(); // also frees info and image data from PNG_decode

	return 0;
}

#endif

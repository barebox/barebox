// SPDX-License-Identifier: GPL-2.0-only
/**
 * Decompression wrapper for loadable resources
 *
 * This provides transparent decompression for loadables, allowing compressed
 * images to be used without requiring temporary files.
 */

#include <common.h>
#include <loadable.h>
#include <malloc.h>
#include <uncompress.h>
#include <linux/minmax.h>

/**
 * struct decompress_loadable_priv - private data for decompressing loadable
 * @inner: wrapped compressed loadable (owned by wrapper)
 */
struct decompress_loadable_priv {
	struct loadable *inner;
};

/**
 * struct decompress_ctx - context passed to fill/flush callbacks
 * @inner: inner loadable to read compressed data from
 * @compressed_pos: current read position in compressed stream
 * @dest_buf: destination buffer for decompressed data
 * @dest_size: size of destination buffer
 * @skip_bytes: bytes to skip at start (for offset handling)
 * @bytes_written: total bytes written to dest_buf so far
 * @done: early termination flag (buffer full)
 */
struct decompress_ctx {
	struct loadable *inner;
	loff_t compressed_pos;

	void *dest_buf;
	size_t dest_size;
	loff_t skip_bytes;
	size_t bytes_written;
	bool done;
};

static struct decompress_ctx *current_ctx;

/**
 * decompress_fill() - callback to read compressed data from inner loadable
 * @buf: buffer to fill with compressed data
 * @len: maximum bytes to read
 *
 * Return: number of bytes read, or negative errno on error
 */
static long decompress_fill(void *buf, unsigned long len)
{
	struct decompress_ctx *ctx = current_ctx;
	ssize_t ret;

	ret = loadable_extract_into_buf(ctx->inner, buf, len,
					ctx->compressed_pos,
					LOADABLE_EXTRACT_PARTIAL);
	if (ret < 0)
		return ret;

	ctx->compressed_pos += ret;
	return ret;
}

/**
 * decompress_flush() - callback to write decompressed data to output buffer
 * @buf: buffer containing decompressed data
 * @len: number of bytes in buffer
 *
 * Handles offset skipping and early termination when output buffer is full.
 *
 * Return: @len on success to continue, 0 to signal early termination
 */
static long decompress_flush(void *buf, unsigned long len)
{
	struct decompress_ctx *ctx = current_ctx;
	size_t space_left, to_copy;

	/* Already got enough data - signal early termination */
	if (ctx->done || ctx->bytes_written >= ctx->dest_size) {
		ctx->done = true;
		return 0;  /* Returning != len signals abort to decompressors */
	}

	/* Skip bytes for offset handling */
	if (ctx->skip_bytes > 0) {
		if (len <= ctx->skip_bytes) {
			ctx->skip_bytes -= len;
			return len;  /* Continue decompressing */
		}
		buf = (char *)buf + ctx->skip_bytes;
		len -= ctx->skip_bytes;
		ctx->skip_bytes = 0;
	}

	/* Copy to destination buffer */
	space_left = ctx->dest_size - ctx->bytes_written;
	to_copy = min_t(size_t, len, space_left);

	memcpy(ctx->dest_buf + ctx->bytes_written, buf, to_copy);
	ctx->bytes_written += to_copy;

	/* Check if we've filled the buffer */
	if (ctx->bytes_written >= ctx->dest_size) {
		ctx->done = true;
		return 0;  /* Signal completion */
	}

	return len;
}

/**
 * decompress_error() - error callback that suppresses early termination errors
 *
 * When the flush callback signals early termination via ctx->done,
 * the decompressor may report a "write error". This is expected and not
 * a real error, so suppress it
 */
static void decompress_error(char *msg)
{
	struct decompress_ctx *ctx = current_ctx;

	if (ctx && ctx->done)
		return;

	uncompress_err_stdout(msg);
}

static int decompress_loadable_get_info(struct loadable *l,
					struct loadable_info *info)
{
	/* Cannot know decompressed size without decompressing */
	info->final_size = LOADABLE_SIZE_UNKNOWN;
	return 0;
}

/**
 * decompress_loadable_extract_into_buf() - extract decompressed data to buffer
 * @l: loadable wrapper
 * @load_addr: destination buffer
 * @size: size of destination buffer
 * @offset: offset within decompressed stream to start from
 * @flags: extraction flags (LOADABLE_EXTRACT_PARTIAL)
 *
 * Decompresses data from the inner loadable to the provided buffer.
 * When offset is zero, partial reads are allowed (streaming).
 * When offset is non-zero, the full requested size must be available.
 *
 * Return: number of bytes written on success, negative errno on error
 */
static ssize_t decompress_loadable_extract_into_buf(struct loadable *l,
						    void *load_addr,
						    size_t size,
						    loff_t offset,
						    unsigned flags)
{
	struct decompress_loadable_priv *priv = l->priv;
	int ret;

	struct decompress_ctx ctx = {
		.inner = priv->inner,
		.compressed_pos = 0,
		.dest_buf = load_addr,
		.dest_size = size,
		.skip_bytes = offset,
		.bytes_written = 0,
		.done = false,
	};

	current_ctx = &ctx;

	ret = uncompress(NULL, 0, decompress_fill, decompress_flush,
			 NULL, NULL, decompress_error);

	current_ctx = NULL;

	/* Early termination (ctx.done) is not an error */
	if (ret != 0 && !ctx.done)
		return ret;

	/*
	 * LOADABLE_EXTRACT_PARTIAL handling:
	 * - If offset is zero, partial reads are always allowed (streaming)
	 * - If offset is non-zero, partial reads require the flag
	 */
	if (offset != 0 && !(flags & LOADABLE_EXTRACT_PARTIAL) &&
	    ctx.bytes_written < size && !ctx.done)
		return -ENOSPC;

	return ctx.bytes_written;
}

static void decompress_loadable_release(struct loadable *l)
{
	struct decompress_loadable_priv *priv = l->priv;

	loadable_release(&priv->inner);
	free(priv);
}

static const struct loadable_ops decompress_loadable_ops = {
	.get_info = decompress_loadable_get_info,
	.extract_into_buf = decompress_loadable_extract_into_buf,
	.release = decompress_loadable_release,
};

/**
 * loadable_decompress() - wrap a loadable with transparent decompression
 * @l: compressed loadable (ownership transferred to wrapper)
 *
 * Creates a wrapper loadable that transparently decompresses data from
 * the inner loadable. The wrapper takes ownership of the inner loadable
 * and will release it when the wrapper is released.
 *
 * Supported compression formats depend on kernel configuration:
 * gzip, bzip2, lzo, lz4, xz, zstd.
 *
 * Return: wrapper loadable on success, or the original loadable on error
 */
struct loadable *loadable_decompress(struct loadable *l)
{
	struct loadable *wrapper;
	struct decompress_loadable_priv *priv;

	if (IS_ERR_OR_NULL(l))
		return l;

	wrapper = xzalloc(sizeof(*wrapper));
	priv = xzalloc(sizeof(*priv));

	priv->inner = l;

	wrapper->name = xasprintf("Decompress(%s)", l->name ?: "unnamed");
	wrapper->type = l->type;
	wrapper->ops = &decompress_loadable_ops;
	wrapper->priv = priv;
	loadable_init(wrapper);

	return wrapper;
}

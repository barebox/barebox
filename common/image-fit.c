/*
 * Copyright (C) Jan Lübbe, 2014
 *
 * This code is inspired by the U-Boot FIT image code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) "FIT: " fmt
#include <common.h>
#include <init.h>
#include <bootm.h>
#include <libfile.h>
#include <fdt.h>
#include <digest.h>
#include <of.h>
#include <fs.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <linux/err.h>
#include <stringlist.h>
#include <rsa.h>
#include <image-fit.h>

#define FDT_MAX_DEPTH 32
#define FDT_MAX_PATH_LEN 200

#define CHECK_LEVEL_NONE 0
#define CHECK_LEVEL_HASH 1
#define CHECK_LEVEL_SIG 2
#define CHECK_LEVEL_MAX 3

static uint32_t dt_struct_advance(struct fdt_header *f, uint32_t dt, int size)
{
	dt += size;
	dt = ALIGN(dt, 4);

	if (dt > f->off_dt_struct + f->size_dt_struct)
		return 0;

	return dt;
}

static char *dt_string(struct fdt_header *f, char *strstart, uint32_t ofs)
{
	if (ofs > f->size_dt_strings)
		return NULL;
	else
		return strstart + ofs;
}

static int of_read_string_list(struct device_node *np, const char *name, struct string_list *sl)
{
	struct property *prop;
	const char *s;

	of_property_for_each_string(np, name, prop, s) {
		string_list_add(sl, s);
	}

	return prop ? 0 : -EINVAL;
}

static int fit_digest(const void *fit, struct digest *digest,
		      struct string_list *inc_nodes, struct string_list *exc_props,
		      uint32_t hashed_strings_start, uint32_t hashed_strings_size)
{
	const struct fdt_header *fdt = fit;
	uint32_t dt_struct;
	void *dt_strings;
	struct fdt_header f = {};
	int stack[FDT_MAX_DEPTH];
	char path[FDT_MAX_PATH_LEN];
	char *end;
	uint32_t tag;
	int start = -1;
	int depth = -1;
	int want = 0;

	f.totalsize = fdt32_to_cpu(fdt->totalsize);
	f.off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
	f.size_dt_struct = fdt32_to_cpu(fdt->size_dt_struct);
	f.off_dt_strings = fdt32_to_cpu(fdt->off_dt_strings);
	f.size_dt_strings = fdt32_to_cpu(fdt->size_dt_strings);

	if (hashed_strings_start > f.size_dt_strings ||
	    hashed_strings_size > f.size_dt_strings ||
	    hashed_strings_start + hashed_strings_size > f.size_dt_strings) {
		pr_err("%s: hashed-strings too large\n", __func__);
		return -EINVAL;
	}

	dt_struct = f.off_dt_struct;
	dt_strings = (void *)fdt + f.off_dt_strings;

	end = path;
	*end = '\0';

	do {
		const struct fdt_property *fdt_prop;
		const struct fdt_node_header *fnh;
		const char *name;
		int include = 0;
		int stop_at = 0;
		int offset = dt_struct;
		int maxlen, len;

		tag = be32_to_cpu(*(uint32_t *)(fit + dt_struct));

		switch (tag) {
		case FDT_BEGIN_NODE:
			fnh = fit + dt_struct;
			name = fnh->name;
			maxlen = (unsigned long)fdt + f.off_dt_struct +
				f.size_dt_struct - (unsigned long)name;

			len = strnlen(name, maxlen + 1);
			if (len > maxlen)
				return -ESPIPE;

			dt_struct = dt_struct_advance(&f, dt_struct,
						      sizeof(struct fdt_node_header) + len + 1);

			depth++;
			if (depth == FDT_MAX_DEPTH)
				return -ESPIPE;
			if (end - path + 2 + len >= FDT_MAX_PATH_LEN)
				return -ESPIPE;
			if (end != path + 1)
				*end++ = '/';
			strcpy(end, name);
			end += len;
			stack[depth] = want;
			if (want == 1)
				stop_at = offset;
			if (string_list_contains(inc_nodes, path))
				want = 2;
			else if (want)
				want--;
			else
				stop_at = offset;
			include = want;

			break;

		case FDT_END_NODE:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);

			include = want;
			want = stack[depth--];
			while (end > path && *--end != '/')
				;
			*end = '\0';

			break;

		case FDT_PROP:
			fdt_prop = fit + dt_struct;
			len = fdt32_to_cpu(fdt_prop->len);

			name = dt_string(&f, dt_strings, fdt32_to_cpu(fdt_prop->nameoff));
			if (!name)
				return -ESPIPE;

			dt_struct = dt_struct_advance(&f, dt_struct,
						      sizeof(struct fdt_property) + len);

			include = want >= 2;
			stop_at = offset;
			if (string_list_contains(exc_props, name))
				include = 0;

			break;

		case FDT_NOP:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);

			include = want >= 2;
			stop_at = offset;

			break;

		case FDT_END:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);

			include = 1;

			break;

		default:
			pr_err("%s: Unknown tag 0x%08X\n", __func__, tag);
			return -EINVAL;
		}

		if (!dt_struct)
			return -ESPIPE;

		pr_debug("%s: include %d, want %d, offset 0x%x, len 0x%x\n",
			 path, include, want, offset, dt_struct-offset);

		if (include && start == -1)
			start = offset;

		if (!include && start != -1) {
			pr_debug("region: 0x%p+0x%x\n", fit + start, offset - start);
			digest_update(digest, fit + start, offset - start);
			start = -1;
		}
	} while (tag != FDT_END);

	pr_debug("region: 0x%p+0x%x\n", fit + start, dt_struct - start);
	digest_update(digest, fit + start, dt_struct - start);

	pr_debug("strings: 0x%p+0x%x\n", dt_strings+hashed_strings_start, hashed_strings_size);
	digest_update(digest, dt_strings + hashed_strings_start, hashed_strings_size);

	return 0;
}

static struct digest *fit_alloc_digest(struct device_node *sig_node,
				       enum hash_algo *algo_out)
{
	struct digest *digest;
	enum hash_algo algo;
	const char *algo_name;

	if (of_property_read_string(sig_node, "algo", &algo_name)) {
		pr_err("algo property not found\n");
		return ERR_PTR(-EINVAL);
	}

	if (strcmp(algo_name, "sha1,rsa2048") == 0) {
		algo = HASH_ALGO_SHA1;
	} else if (strcmp(algo_name, "sha256,rsa2048") == 0) {
		algo = HASH_ALGO_SHA256;
	} else if (strcmp(algo_name, "sha256,rsa4096") == 0) {
		algo = HASH_ALGO_SHA256;
	} else	{
		pr_err("unknown algo %s\n", algo_name);
		return ERR_PTR(-EINVAL);
	}

	digest = digest_alloc_by_algo(algo);
	if (!digest) {
		pr_err("unsupported algo %s\n", algo_name);
		return ERR_PTR(-EINVAL);
	}

	digest_init(digest);

	*algo_out = algo;

	return digest;
}

static int fit_check_rsa_signature(struct device_node *sig_node,
				   enum hash_algo algo, void *hash)
{
	struct rsa_public_key key = {};
	const char *key_name;
	char *key_path;
	struct device_node *key_node;
	int sig_len;
	const char *sig_value;
	int ret;

	sig_value = of_get_property(sig_node, "value", &sig_len);
	if (!sig_value) {
		pr_err("signature value not found in %s\n", sig_node->full_name);
		return -EINVAL;
	}

	if (of_property_read_string(sig_node, "key-name-hint", &key_name)) {
		pr_err("key name not found in %s\n", sig_node->full_name);
		return -EINVAL;
	}
	key_path = xasprintf("/signature/key-%s", key_name);
	key_node = of_find_node_by_path(key_path);
	if (!key_node) {
		pr_info("failed to find key node %s\n", key_path);
		free(key_path);
		return -ENOENT;
	}
	free(key_path);

	ret = rsa_of_read_key(key_node, &key);
	if (ret) {
		pr_info("failed to read key in %s\n", key_node->full_name);
		return -ENOENT;
	}

	ret = rsa_verify(&key, sig_value, sig_len, hash, algo);
	if (ret)
		pr_err("image signature BAD\n");
	else
		pr_info("image signature OK\n");

	return ret;
}

/*
 * The consistency of the FTD structure was already checked by of_unflatten_dtb()
 */
static int fit_verify_signature(struct device_node *sig_node, const void *fit)
{
	uint32_t hashed_strings_start, hashed_strings_size;
	struct string_list inc_nodes, exc_props;
	struct digest *digest;
	void *hash;
	enum hash_algo algo = 0;
	int ret;

	if (of_property_read_u32_index(sig_node, "hashed-strings", 0,
	    &hashed_strings_start)) {
		pr_err("hashed-strings start not found in %s\n", sig_node->full_name);
		return -EINVAL;
	}
	if (of_property_read_u32_index(sig_node, "hashed-strings", 1,
	    &hashed_strings_size)) {
		pr_err("hashed-strings size not found in %s\n", sig_node->full_name);
		return -EINVAL;
	}

	string_list_init(&inc_nodes);
	string_list_init(&exc_props);

	if (of_read_string_list(sig_node, "hashed-nodes", &inc_nodes)) {
		pr_err("hashed-nodes property not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_sl;
	}

	string_list_add(&exc_props, "data");

	digest = fit_alloc_digest(sig_node, &algo);
	if (IS_ERR(digest)) {
		ret = PTR_ERR(digest);
		goto out_sl;
	}

	ret = fit_digest(fit, digest, &inc_nodes, &exc_props, hashed_strings_start,
			 hashed_strings_size);
	hash = xzalloc(digest_length(digest));
	digest_final(digest, hash);

	ret = fit_check_rsa_signature(sig_node, algo, hash);
	if (ret)
		goto out_free_hash;

	ret = 0;

 out_free_hash:
	free(hash);
	digest_free(digest);
 out_sl:
	string_list_free(&inc_nodes);
	string_list_free(&exc_props);
 
	return ret;
}

static int fit_verify_hash(struct fit_handle *handle, struct device_node *image,
			   const void *data, int data_len)
{
	struct digest *d;
	const char *algo;
	const char *value_read;
	char *value_calc;
	int hash_len, ret;
	struct device_node *hash;

	switch (handle->verify) {
	case BOOTM_VERIFY_NONE:
		return 0;
	case BOOTM_VERIFY_AVAILABLE:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	hash = of_get_child_by_name(image, "hash@1");
	if (!hash) {
		if (ret)
			pr_err("image %s does not have hashes\n",
			       image->full_name);
		return ret;
	}

	value_read = of_get_property(hash, "value", &hash_len);
	if (!value_read) {
		pr_err("%s: \"value\" property not found\n", hash->full_name);
		return -EINVAL;
	}

	if (of_property_read_string(hash, "algo", &algo)) {
		pr_err("%s: \"algo\" property not found\n", hash->full_name);
		return -EINVAL;
	}

	d = digest_alloc(algo);
	if (!d) {
		pr_err("%s: unsupported algo %s\n", hash->full_name, algo);
		return -EINVAL;
	}

	if (hash_len != digest_length(d)) {
		pr_err("%s: invalid hash length %d\n", hash->full_name, hash_len);
		ret = -EINVAL;
		goto err_digest_free;
	}

	value_calc = xmalloc(hash_len);

	digest_init(d);
	digest_update(d, data, data_len);
	digest_final(d, value_calc);

	if (memcmp(value_read, value_calc, hash_len)) {
		pr_info("%s: hash BAD\n", hash->full_name);
		ret =  -EBADMSG;
	} else {
		pr_info("%s: hash OK\n", hash->full_name);
		ret = 0;
	}

	free(value_calc);

err_digest_free:
	digest_free(d);

	return ret;
}

static int fit_image_verify_signature(struct fit_handle *handle,
				      struct device_node *image,
				      const void *data, int data_len)
{
	struct digest *digest;
	struct device_node *sig_node;
	enum hash_algo algo = 0;
	void *hash;
	int ret;

	if (!IS_ENABLED(CONFIG_FITIMAGE_SIGNATURE))
		return 0;

	switch (handle->verify) {
	case BOOTM_VERIFY_NONE:
		return 0;
	case BOOTM_VERIFY_AVAILABLE:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	sig_node = of_get_child_by_name(image, "signature@1");
	if (!sig_node) {
		pr_err("Image %s has no signature\n", image->full_name);
		return ret;
	}

	digest = fit_alloc_digest(sig_node, &algo);
	if (IS_ERR(digest))
		return PTR_ERR(digest);

	digest_update(digest, data, data_len);
	hash = xzalloc(digest_length(digest));
	digest_final(digest, hash);

	ret = fit_check_rsa_signature(sig_node, algo, hash);

	free(hash);

	digest_free(digest);

	return ret;
}

int fit_has_image(struct fit_handle *handle, void *configuration,
		  const char *name)
{
	const char *unit;
	struct device_node *conf_node = configuration;

	if (!conf_node)
		return -EINVAL;

	if (of_property_read_string(conf_node, name, &unit))
		return 0;

	return 1;
}

/**
 * fit_open_image - Open an image in a FIT image
 * @handle: The FIT image handle
 * @name: The name of the image to open
 * @outdata: The returned image
 * @outsize: Size of the returned image
 *
 * Open an image in a FIT image. The returned image is freed during fit_close().
 * @configuration holds the cookie returned from fit_open_configuration() if
 * the image is opened as part of a configuration, or NULL if the image is
 * opened without a configuration. If @configuration is NULL then the RSA
 * signature of the image is checked if desired, if @configuration is non NULL,
 * then only the hash is checked (because opening the configuration already
 * checks the RSA signature of all involved nodes).
 *
 * Return: 0 for success, negative error code otherwise
 */
int fit_open_image(struct fit_handle *handle, void *configuration,
		   const char *name, const void **outdata,
		   unsigned long *outsize)
{
	struct device_node *image;
	const char *unit, *type = NULL, *desc= "(no description)";
	const void *data;
	int data_len;
	int ret = 0;
	struct device_node *conf_node = configuration;

	if (conf_node) {
		if (of_property_read_string(conf_node, name, &unit)) {
			pr_err("No image named '%s'\n", name);
			return -ENOENT;
		}
	} else {
		unit = name;
	}

	image = of_get_child_by_name(handle->images, unit);
	if (!image)
		return -ENOENT;

	of_property_read_string(image, "description", &desc);
	pr_info("image '%s': '%s'\n", unit, desc);

	of_property_read_string(image, "type", &type);
	if (!type) {
		pr_err("No \"type\" property found in %s\n", image->full_name);
		return -EINVAL;
	}

	data = of_get_property(image, "data", &data_len);
	if (!data) {
		pr_err("data not found\n");
		return -EINVAL;
	}

	if (conf_node)
		ret = fit_verify_hash(handle, image, data, data_len);
	else
		ret = fit_image_verify_signature(handle, image, data, data_len);

	if (ret < 0)
		return ret;

	*outdata = data;
	*outsize = data_len;

	return 0;
}

static int fit_config_verify_signature(struct fit_handle *handle, struct device_node *conf_node)
{
	struct device_node *sig_node;
	int ret = -EINVAL;

	if (!IS_ENABLED(CONFIG_FITIMAGE_SIGNATURE))
		return 0;

	switch (handle->verify) {
	case BOOTM_VERIFY_NONE:
	case BOOTM_VERIFY_HASH:
		return 0;
	case BOOTM_VERIFY_SIGNATURE:
		ret = -EINVAL;
		break;
	case BOOTM_VERIFY_AVAILABLE:
		ret = 0;
		break;
	}

	for_each_child_of_node(conf_node, sig_node) {
		if (handle->verbose)
			of_print_nodes(sig_node, 0);
		ret = fit_verify_signature(sig_node, handle->fit);
		if (ret < 0)
			return ret;
	}

	if (ret < 0) {
		pr_err("configuration '%s' does not have a signature\n",
		       conf_node->full_name);
		return ret;
	}

	return ret;
}

static int fit_find_compatible_unit(struct device_node *conf_node,
				    const char **unit)
{
	struct device_node *child = NULL;
	struct device_node *barebox_root;
	const char *machine;
	int ret;

	barebox_root = of_get_root_node();
	if (!barebox_root)
		goto default_unit;

	ret = of_property_read_string(barebox_root, "compatible", &machine);
	if (ret)
		return -ENOENT;

	for_each_child_of_node(conf_node, child) {
		if (of_device_is_compatible(child, machine)) {
			*unit = child->name;
			pr_info("matching unit '%s' found\n", *unit);
			return 0;
		}
	}

default_unit:
	pr_info("No match found. Trying default.\n");
	if (of_property_read_string(conf_node, "default", unit) == 0)
		return 0;

	return -ENOENT;
}

/**
 * fit_open_configuration - open a FIT configuration
 * @handle: The FIT image handle
 * @name: The name of the configuration
 *
 * This opens a FIT configuration and eventually checks the signature
 * depending on the verify mode the FIT image is opened with.
 *
 * Return: If successful a pointer to a valid configuration node,
 *         otherwise a ERR_PTR()
 */
void *fit_open_configuration(struct fit_handle *handle, const char *name)
{
	struct device_node *conf_node = handle->configurations;
	const char *unit, *desc = "(no description)";
	int ret;

	if (!conf_node)
		return ERR_PTR(-ENOENT);

	if (name) {
		unit = name;
	} else {
		ret = fit_find_compatible_unit(conf_node, &unit);
		if (ret) {
			pr_info("Couldn't get a valid configuration. Aborting.\n");
			return ERR_PTR(ret);
		}
	}

	conf_node = of_get_child_by_name(conf_node, unit);
	if (!conf_node) {
		pr_err("configuration '%s' not found\n", unit);
		return ERR_PTR(-ENOENT);
	}

	of_property_read_string(conf_node, "description", &desc);
	pr_info("configuration '%s': %s\n", unit, desc);

	ret = fit_config_verify_signature(handle, conf_node);
	if (ret)
		return ERR_PTR(ret);

	return conf_node;
}

static int fit_do_open(struct fit_handle *handle)
{
	const char *desc = "(no description)";
	struct device_node *root;

	root = of_unflatten_dtb_const(handle->fit);
	if (IS_ERR(root))
		return PTR_ERR(root);

	handle->root = root;

	handle->images = of_get_child_by_name(handle->root, "images");
	if (!handle->images)
		return -ENOENT;

	handle->configurations = of_get_child_by_name(handle->root,
						      "configurations");

	of_property_read_string(handle->root, "description", &desc);
	pr_info("Opened FIT image: %s\n", desc);

	return 0;
}

/**
 * fit_open_buf - open a FIT image from a buffer
 * @buf:	The buffer containing the FIT image
 * @size:	Size of the FIT image
 * @verbose:	If true, be more verbose
 * @verify:	The verify mode
 *
 * This opens a FIT image found in buf. The returned handle is used as
 * context for the other FIT functions.
 *
 * Return: A handle to a FIT image or a ERR_PTR
 */
struct fit_handle *fit_open_buf(const void *buf, size_t size, bool verbose,
				enum bootm_verify verify)
{
	struct fit_handle *handle;
	int ret;

	handle = xzalloc(sizeof(struct fit_handle));

	handle->verbose = verbose;
	handle->fit = buf;
	handle->size = size;
	handle->verify = verify;

	ret = fit_do_open(handle);
	if (ret) {
		fit_close(handle);
		return ERR_PTR(ret);
	}

	return handle;
}

/**
 * fit_open - open a FIT image
 * @filename:	The filename of the FIT image
 * @verbose:	If true, be more verbose
 * @verify:	The verify mode
 *
 * This opens a FIT image found in @filename. The returned handle is used as
 * context for the other FIT functions.
 *
 * Return: A handle to a FIT image or a ERR_PTR
 */
struct fit_handle *fit_open(const char *filename, bool verbose,
			    enum bootm_verify verify)
{
	struct fit_handle *handle;
	int ret;

	handle = xzalloc(sizeof(struct fit_handle));

	handle->verbose = verbose;
	handle->verify = verify;

	ret = read_file_2(filename, &handle->size, &handle->fit_alloc,
			  FILESIZE_MAX);
	if (ret) {
		pr_err("unable to read %s: %s\n", filename, strerror(-ret));
		return ERR_PTR(ret);
	}

	handle->fit = handle->fit_alloc;

	ret = fit_do_open(handle);
	if (ret) {
		fit_close(handle);
		return ERR_PTR(ret);
	}

	return handle;
}

void fit_close(struct fit_handle *handle)
{
	if (handle->root)
		of_delete_node(handle->root);

	free(handle->fit_alloc);
	free(handle);
}

#ifdef CONFIG_SANDBOX
static int do_bootm_sandbox_fit(struct image_data *data)
{
	struct fit_handle *handle;
	int ret;
	void *kernel;
	unsigned long kernel_size;

	handle = fit_open(data->os_file, data->verbose);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	ret = fit_open_configuration(handle, data->os_part);
	if (ret)
		goto out;

	ret = 0;
out:
	fit_close(handle);

	return ret;
}

static struct image_handler sandbox_fit_handler = {
	.name = "FIT image",
	.bootm = do_bootm_sandbox_fit,
	.filetype = filetype_oftree,
};

static int sandbox_fit_register(void)
{
	return register_image_handler(&sandbox_fit_handler);
}
late_initcall(sandbox_fit_register);
#endif

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

static int fit_digest(void *fit, struct digest *digest,
		      struct string_list *inc_nodes, struct string_list *exc_props,
		      uint32_t hashed_strings_start, uint32_t hashed_strings_size)
{
	struct fdt_header *fdt = fit;
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

/*
 * The consistency of the FTD structure was already checked by of_unflatten_dtb()
 */
static int fit_verify_signature(struct device_node *sig_node, void *fit)
{
	uint32_t hashed_strings_start, hashed_strings_size;
	struct string_list inc_nodes, exc_props;
	struct rsa_public_key key = {};
	struct digest *digest;
	int sig_len;
	const char *algo_name, *key_name, *sig_value;
	char *key_path;
	struct device_node *key_node;
	enum hash_algo algo;
	void *hash;
	int ret;

	if (of_property_read_string(sig_node, "algo", &algo_name)) {
		pr_err("algo not found\n");
		ret = -EINVAL;
		goto out;
	}
	if (strcmp(algo_name, "sha1,rsa2048") == 0) {
		algo = HASH_ALGO_SHA1;
	} else if (strcmp(algo_name, "sha256,rsa4096") == 0) {
		algo = HASH_ALGO_SHA256;
	} else	{
		pr_err("unknown algo %s\n", algo_name);
		ret = -EINVAL;
		goto out;
	}
	digest = digest_alloc_by_algo(algo);
	if (!digest) {
		pr_err("unsupported algo %s\n", algo_name);
		ret = -EINVAL;
		goto out;
	}

	sig_value = of_get_property(sig_node, "value", &sig_len);
	if (!sig_value) {
		pr_err("signature value not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_free_digest;
	}

	if (of_property_read_string(sig_node, "key-name-hint", &key_name)) {
		pr_err("key name not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_free_digest;
	}
	key_path = xasprintf("/signature/key-%s", key_name);
	key_node = of_find_node_by_path(key_path);
	free(key_path);
	if (!key_node) {
		pr_info("failed to find key node\n");
		ret = -ENOENT;
		goto out_free_digest;
	}

	ret = rsa_of_read_key(key_node, &key);
	if (ret) {
		pr_info("failed to read key in %s\n", key_node->full_name);
		ret = -ENOENT;
		goto out_free_digest;
	}

	if (of_property_read_u32_index(sig_node, "hashed-strings", 0, &hashed_strings_start)) {
		pr_err("hashed-strings start not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_free_digest;
	}
	if (of_property_read_u32_index(sig_node, "hashed-strings", 1, &hashed_strings_size)) {
		pr_err("hashed-strings size not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_free_digest;
	}

	string_list_init(&inc_nodes);
	string_list_init(&exc_props);

	if (of_read_string_list(sig_node, "hashed-nodes", &inc_nodes)) {
		pr_err("hashed-nodes property not found in %s\n", sig_node->full_name);
		ret = -EINVAL;
		goto out_sl;
	}

	string_list_add(&exc_props, "data");

	digest_init(digest);
	ret = fit_digest(fit, digest, &inc_nodes, &exc_props, hashed_strings_start, hashed_strings_size);
	hash = xzalloc(digest_length(digest));
	digest_final(digest, hash);

	ret = rsa_verify(&key, sig_value, sig_len, hash, algo);
	if (ret) {
		pr_info("image signature BAD\n");
		ret = -EBADMSG;
	} else {
		pr_info("image signature OK\n");
		ret = 0;
	}

	free(hash);
 out_sl:
	string_list_free(&inc_nodes);
	string_list_free(&exc_props);
 out_free_digest:
	digest_free(digest);
 out:
	return ret;
}

static int fit_verify_hash(struct device_node *hash, const void *data, int data_len)
{
	struct digest *d;
	const char *algo;
	const char *value_read;
	char *value_calc;
	int hash_len, ret;

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

static int fit_open_image(struct fit_handle *handle, const char *unit, const void **outdata,
	unsigned long *outsize)
{
	struct device_node *image = NULL, *hash;
	const char *type = NULL, *desc= "(no description)";
	const void *data;
	int data_len;
	int ret = 0;

	image = of_get_child_by_name(handle->root, "images");
	if (!image)
		return -ENOENT;

	image = of_get_child_by_name(image, unit);
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

	if (handle->verify > BOOTM_VERIFY_NONE) {
		if (handle->verify == BOOTM_VERIFY_AVAILABLE)
			ret = 0;
		else
			ret = -EINVAL;
		for_each_child_of_node(image, hash) {
			if (handle->verbose)
				of_print_nodes(hash, 0);
			ret = fit_verify_hash(hash, data, data_len);
			if (ret < 0)
				return ret;
		}

		if (ret < 0) {
			pr_err("image '%s': '%s' does not have hashes\n", unit, desc);
			return ret;
		}
	}

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

static int fit_open_configuration(struct fit_handle *handle, const char *name)
{
	struct device_node *conf_node = NULL;
	const char *unit, *desc = "(no description)";
	int ret;

	conf_node = of_get_child_by_name(handle->root, "configurations");
	if (!conf_node)
		return -ENOENT;

	if (name) {
		unit = name;
	} else if (of_property_read_string(conf_node, "default", &unit)) {
		unit = "conf@1";
	}

	conf_node = of_get_child_by_name(conf_node, unit);
	if (!conf_node) {
		pr_err("configuration '%s' not found\n", unit);
		return -ENOENT;
	}

	of_property_read_string(conf_node, "description", &desc);
	pr_info("configuration '%s': %s\n", unit, desc);

	ret = fit_config_verify_signature(handle, conf_node);
	if (ret)
		return ret;

	if (of_property_read_string(conf_node, "kernel", &unit) == 0) {
		ret = fit_open_image(handle, unit, &handle->kernel, &handle->kernel_size);
		if (ret)
			return ret;
	} else {
		return -ENOENT;
	}

	if (of_property_read_string(conf_node, "fdt", &unit) == 0) {
		ret = fit_open_image(handle, unit, &handle->oftree, &handle->oftree_size);
		if (ret)
			return ret;
	}

	if (of_property_read_string(conf_node, "ramdisk", &unit) == 0) {
		ret = fit_open_image(handle, unit, &handle->initrd, &handle->initrd_size);
		if (ret)
			return ret;
	}

	return 0;
}

struct fit_handle *fit_open(const char *filename, const char *config, bool verbose,
			    enum bootm_verify verify)
{
	struct fit_handle *handle = NULL;
	const char *desc = "(no description)";
	int ret;

	handle = xzalloc(sizeof(struct fit_handle));

	handle->verbose = verbose;

	ret = read_file_2(filename, &handle->size, &handle->fit, FILESIZE_MAX);
	if (ret) {
		pr_err("unable to read %s: %s\n", filename, strerror(-ret));
		goto err;
	}

	handle->root = of_unflatten_dtb(handle->fit);
	if (IS_ERR(handle->root)) {
		ret = PTR_ERR(handle->root);
		goto err;
	}

	handle->verify = verify;

	of_property_read_string(handle->root, "description", &desc);
	pr_info("'%s': %s\n", filename, desc);

	ret = fit_open_configuration(handle, config);
	if (ret)
		goto err;

	return handle;
 err:
	if (handle->root)
		of_delete_node(handle->root);
	free(handle->fit);
	free(handle);

	return ERR_PTR(ret);
}

void fit_close(struct fit_handle *handle)
{
	if (handle->root)
		of_delete_node(handle->root);
	if (handle->fit)
		free(handle->fit);
	free(handle);
}

#ifdef CONFIG_SANDBOX
static int do_bootm_sandbox_fit(struct image_data *data)
{
	struct fit_handle *handle;
	handle = fit_open(data->os_file, data->os_part, data->verbose);
	if (handle)
		fit_close(handle);
	return 0;
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

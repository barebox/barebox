// SPDX-License-Identifier: GPL-2.0-only
/*
 * Functions for working with device tree overlays
 *
 * Copyright (C) 2012 Pantelis Antoniou <panto@antoniou-consulting.com>
 * Copyright (C) 2012 Texas Instruments Inc.
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */
#define pr_fmt(fmt) "of_overlay: " fmt

#include <bootm.h>
#include <common.h>
#include <of.h>
#include <errno.h>
#include <globalvar.h>
#include <image-fit.h>
#include <magicvar.h>
#include <string.h>
#include <libfile.h>
#include <fs.h>
#include <libbb.h>
#include <fnmatch.h>

static struct device_node *find_target(struct device_node *root,
				       struct device_node *fragment)
{
	struct device_node *node;
	const char *path;
	u32 phandle;
	int ret;

	ret = of_property_read_u32(fragment, "target", &phandle);
	if (!ret) {
		node = of_find_node_by_phandle_from(phandle, root);
		if (!node)
			pr_err("fragment %pOF: phandle 0x%x not found\n",
			       fragment, phandle);
		return node;
	}

	ret = of_property_read_string(fragment, "target-path", &path);
	if (!ret) {
		node = of_find_node_by_path_from(root, path);
		if (!node)
			pr_err("fragment %pOF: path '%s' not found\n",
			       fragment, path);
		return node;
	}

	pr_err("fragment %pOF: no target property\n", fragment);

	return NULL;
}

static int of_overlay_apply(struct device_node *target,
			    const struct device_node *overlay)
{
	struct device_node *child;
	struct device_node *target_child;
	struct property *prop;
	int err;

	if (target == NULL || overlay == NULL)
		return -EINVAL;

	list_for_each_entry(prop, &overlay->properties, list) {
		if (of_prop_cmp(prop->name, "name") == 0)
			continue;

		if (of_prop_cmp(prop->name, "phandle") == 0)
			target->phandle = be32_to_cpup(prop->value);

		err = of_set_property(target, prop->name, prop->value,
				      prop->length, true);
		if (err)
			return err;
	}

	for_each_child_of_node(overlay, child) {
		target_child = of_get_child_by_name(target, child->name);
		if (!target_child)
			target_child = of_new_node(target, child->name);
		if (!target_child)
			return -ENOMEM;

		err = of_overlay_apply(target_child, child);
		if (err)
			return err;
	}

	return 0;
}

static char *of_overlay_fix_path(struct device_node *root,
				 struct device_node *overlay, const char *path)
{
	struct device_node *fragment;
	struct device_node *target;
	const char *path_tail;
	const char *prefix;

	fragment = of_find_node_by_path_from(overlay, path);
	while ((fragment = of_get_parent(fragment)) != NULL) {
		if (of_get_child_by_name(fragment, "__overlay__"))
			break;
	}
	if (!fragment) {
		pr_info("could not find __overlay__ node\n");
		return NULL;
	}

	target = find_target(root, fragment);
	if (!target)
		return NULL;

	prefix = of_get_child_by_name(fragment, "__overlay__")->full_name;
	path_tail = path + strlen(prefix);

	return basprintf("%pOF%s", target, path_tail);
}

static int of_overlay_apply_symbols(struct device_node *root,
				    struct device_node *overlay)
{
	const char *old_path;
	char *new_path;
	struct property *prop;
	struct device_node *root_symbols;
	struct device_node *overlay_symbols;

	root_symbols = of_get_child_by_name(root, "__symbols__");
	overlay_symbols = of_get_child_by_name(overlay, "__symbols__");

	if (!overlay_symbols) {
		pr_debug("overlay doesn't have a __symbols__ node\n");
		return 0;
	}

	if (!root_symbols) {
		pr_info("root doesn't have a __symbols__ node\n");
		return 0;
	}

	list_for_each_entry(prop, &overlay_symbols->properties, list) {
		if (of_prop_cmp(prop->name, "name") == 0)
			continue;

		old_path = of_property_get_value(prop);
		new_path = of_overlay_fix_path(root, overlay, old_path);
		if (!new_path)
			return -EINVAL;

		pr_debug("add symbol %s with new path %s\n",
			 prop->name, new_path);
		of_property_write_string(root_symbols, prop->name, new_path);
		free(new_path);
	}

	return 0;
}

static int of_overlay_apply_fragment(struct device_node *root,
				     struct device_node *fragment)
{
	struct device_node *target;
	struct device_node *overlay;

	overlay = of_get_child_by_name(fragment, "__overlay__");
	if (!overlay)
		return 0;

	target = find_target(root, fragment);
	if (!target)
		return -EINVAL;

	return of_overlay_apply(target, overlay);
}

static char *of_overlay_compatible;

/**
 * Apply the overlay on the passed devicetree root
 * @root: the devicetree onto which the overlay will be applied
 * @overlay: the devicetree to apply as an overlay
 */
int of_overlay_apply_tree(struct device_node *root,
			  struct device_node *overlay)
{
	struct device_node *resolved;
	struct device_node *fragment;
	int err = 0;

	resolved = of_resolve_phandles(root, overlay);
	if (!resolved)
		return -EINVAL;

	err = of_overlay_pre_load_firmware(root, resolved);
	if (err)
		goto out_err;

	/* Copy symbols from resolved overlay to base device tree */
	err = of_overlay_apply_symbols(root, resolved);
	if (err)
		goto out_err;

	/* Copy nodes and properties from resolved overlay to root */
	for_each_child_of_node(resolved, fragment) {
		err = of_overlay_apply_fragment(root, fragment);
		if (err)
			pr_warn("failed to apply %s\n", fragment->name);
	}

	/* We are patching the live tree, reload aliases */
	if (root == of_get_root_node())
		of_alias_scan();

out_err:
	of_delete_node(resolved);

	return err;
}

static char *of_overlay_filter;

static LIST_HEAD(of_overlay_filters);

static struct of_overlay_filter *of_overlay_find_filter(const char *name)
{
	struct of_overlay_filter *f;

	list_for_each_entry(f, &of_overlay_filters, list)
		if (!strcmp(f->name, name))
			return f;
	return NULL;
}

static bool of_overlay_matches_filter(const char *pattern, struct device_node *ovl)
{
	struct of_overlay_filter *filter;
	char *p, *path, *n;
	bool apply = false;
	bool have_pattern_filter = false;
	bool have_content_filter = false;

	p = path = strdup(of_overlay_filter);

	while ((n = strsep_unescaped(&p, " ", NULL))) {
		int score = 0;

		if (!*n)
			continue;

		filter = of_overlay_find_filter(n);
		if (!filter) {
			pr_err("Ignoring unknown filter %s\n", n);
			continue;
		}

		if (filter->filter_pattern || filter->filter_filename)
			have_pattern_filter = true;
		if (filter->filter_content)
			have_content_filter = true;

		if (pattern) {
			if ((filter->filter_pattern &&
			     filter->filter_pattern(filter, kbasename(pattern))) ||
			    (filter->filter_filename &&
			     filter->filter_filename(filter, kbasename(pattern))))
				score++;
		} else {
			score++;
		}

		if (ovl) {
			if (filter->filter_content &&
			    filter->filter_content(filter, ovl))
				score++;
		} else {
			score++;
		}

		if (score == 2) {
			apply = true;
			break;
		}
	}

	free(path);

	/* No filter found at all, no match */
	if (!have_pattern_filter && !have_content_filter)
		return false;

	/* Want to match pattern, but we do not have a filename_filter */
	if (pattern && !have_pattern_filter)
		return true;

	/* Want to match content, but we do not have a content_filter */
	if (ovl && !have_content_filter)
		return true;

	if (apply)
		pr_debug("pattern %s, overlay %p: match against filter %s\n",
			 pattern ?: "<NONE>",
			 ovl, filter->name);
	else
		pr_debug("pattern %s, overlay %p: no match\n",
			 pattern ?: "<NONE>", ovl);

	return apply;
}

int of_overlay_apply_file(struct device_node *root, const char *filename,
			  bool filter)
{
	struct device_node *ovl;
	int ret;

	if (filter && !of_overlay_matches_filter(filename, NULL))
		return 0;

	ovl = of_read_file(filename);
	if (IS_ERR(ovl))
		return PTR_ERR(ovl);

	if (filter && !of_overlay_matches_filter(NULL, ovl))
		return 0;

	ret = of_overlay_apply_tree(root, ovl);
	if (ret == -ENODEV)
		pr_debug("Not applied %s (not compatible)\n", filename);
	else if (ret)
		pr_err("Cannot apply %s: %pe\n", filename, ERR_PTR(ret));
	else
		pr_info("Applied %s\n", filename);

	of_delete_node(ovl);

	return ret;
}

int of_overlay_apply_dtbo(struct device_node *root, const void *dtbo)
{
	struct device_node *overlay;
	int ret;

	overlay = of_unflatten_dtb(dtbo, INT_MAX);
	ret = of_overlay_apply_tree(root, overlay);
	of_delete_node(overlay);

	return ret;
}

static int of_overlay_fixup(struct device_node *root, void *data)
{
	struct device_node *overlay = data;

	return of_overlay_apply_tree(root, overlay);
}

/**
 * Iterate the overlay and call process for each fragment
 *
 * If process() fails for any fragment, the function will stop to process
 * other fragments and return the error of the failed process() call.
 */
int of_process_overlay(struct device_node *root,
		       struct device_node *overlay,
		       int (*process)(struct device_node *target,
				      struct device_node *overlay, void *data),
		       void *data)
{
	struct device_node *fragment;
	int err = 0;

	for_each_child_of_node(overlay, fragment) {
		struct device_node *ovl;
		struct device_node *target;

		ovl = of_get_child_by_name(fragment, "__overlay__");
		if (!ovl)
			continue;

		target = find_target(root, fragment);
		if (!target)
			pr_debug("cannot find target for fragment %s\n",
				 fragment->name);

		err = process(target, ovl, data);
		if (err) {
			pr_warn("failed to process overlay for %s\n",
				target ? target->name : "unknown");
			break;
		}
	}

	return err;
}

/**
 * Register a devicetree overlay
 *
 * The overlay is not applied to the live device tree, but registered as fixup
 * for the fixed up device tree. Therefore, drivers relying on the overlay
 * must use the fixed device tree.
 *
 * The fixup takes ownership of the overlay.
 */
int of_register_overlay(struct device_node *overlay)
{
	return of_register_fixup(of_overlay_fixup, overlay);
}

static char *of_overlay_pattern;
static char *of_overlay_path;
static char *of_overlay_basedir;

/**
 * of_overlay_set_basedir - set the overlay basedir
 * @path: The new overlay basedir
 *
 * This specifies the base directory where overlay files are expected. By
 * default this is the root directory, but it is overwritten by blspec to
 * point to the rootfs of the about-to-be-booted system.
 */
void of_overlay_set_basedir(const char *path)
{
	free(of_overlay_basedir);
	of_overlay_basedir = strdup(path);
}

static int of_overlay_apply_dir(struct device_node *root, const char *dirname,
				bool filter)
{
	int ret = 0;
	DIR *dir;

	if (!dirname || !*dirname)
		return 0;

	pr_debug("Applying overlays from %s\n", dirname);

	dir = opendir(dirname);
	if (!dir)
		return -errno;

	while (1) {
		struct dirent *ent;
		char *filename;

		ent = readdir(dir);
		if (!ent)
			break;

		if (!strcmp(dir->d.d_name, ".") || !strcmp(dir->d.d_name, ".."))
			continue;

		filename = basprintf("%s/%s", dirname, dir->d.d_name);

		of_overlay_apply_file(root, filename, filter);

		free(filename);
	}

	closedir(dir);

	return ret;
}

static int of_overlay_apply_fit(struct device_node *root, struct fit_handle *fit,
				struct device_node *config)
{
	const char *name = config->name;
	struct device_node *overlay;
	unsigned long ovl_sz;
	const void *ovl;
	int ret;

	pr_debug("Process FIT config \"%s\"\n", name);

	if (!of_overlay_matches_filter(name, NULL))
		return 0;

	ret = fit_open_image(fit, config, "fdt", &ovl, &ovl_sz);
	if (ret)
		return ret;

	overlay = of_unflatten_dtb(ovl, ovl_sz);
	if (IS_ERR(overlay))
		return PTR_ERR(overlay);

	if (!of_overlay_matches_filter(NULL, overlay)) {
		ret = 0;
		goto out;
	}

	ret = of_overlay_apply_tree(root, overlay);
	if (ret == -ENODEV)
		pr_debug("Not applied %s (not compatible)\n", name);
	else if (ret)
		pr_err("Cannot apply %s: %s\n", name, strerror(-ret));
	else
		pr_info("Applied %s\n", name);

out:
	of_delete_node(overlay);

	return ret;
}

static bool of_overlay_valid_config(struct fit_handle *fit,
				    struct device_node *config)
{
	/*
	 * Either kernel or firmware is marked as mandatory by U-Boot
	 * (doc/usage/fit/source_file_format.rst) except for overlays
	 * (doc/usage/fit/overlay-fdt-boot.rst). Therefore we need to ensure
	 * that only "fdt" config nodes are recognized as overlay config node.
	 */
	if (!fit_has_image(fit, config, "fdt") ||
	    fit_has_image(fit, config, "kernel") ||
	    fit_has_image(fit, config, "firmware"))
		return false;

	return true;
}

static int of_overlay_global_fixup_fit(struct device_node *root,
				       const char *fit_path, loff_t fit_size)
{
	enum bootm_verify verify = bootm_get_verify_mode();
	struct device_node *conf_node;
	struct fit_handle *fit;
	int ret;

	if (!IS_ENABLED(CONFIG_FITIMAGE)) {
		pr_err("FIT based overlay handling requested while CONFIG_FITIMAGE=n\n");
		return -EINVAL;
	}

	fit = fit_open(fit_path, 0, verify, fit_size);
	if (IS_ERR(fit)) {
		pr_err("Loading FIT image %s failed with: %pe\n", fit_path, fit);
		return PTR_ERR(fit);
	}

	for_each_child_of_node(fit->configurations, conf_node) {
		if (!of_overlay_valid_config(fit, conf_node))
			continue;

		ret = fit_config_verify_signature(fit, conf_node);
		if (ret)
			goto out;

		ret = of_overlay_apply_fit(root, fit, conf_node);
		if (ret)
			goto out;
	}

out:
	fit_close(fit);
	return ret;
}

static int of_overlay_global_fixup(struct device_node *root, void *data)
{
	struct stat s;
	char *dir;
	int ret;

	if (isempty(of_overlay_path))
		return 0;

	if (*of_overlay_path == '/')
		dir = xstrdup(of_overlay_path);
	else
		dir = concat_path_file(of_overlay_basedir, of_overlay_path);


	if (stat(dir, &s)) {
		pr_err("Cannot stat global.of.overlay.path (%s): %m\n", dir);
		ret = -errno;
		goto out;
	}

	if (S_ISDIR(s.st_mode)) {
		ret = of_overlay_apply_dir(root, dir, true);
		goto out;
	}

	/* Assume a FIT image if of_overlay_path points to a file */
	ret = of_overlay_global_fixup_fit(root, dir, s.st_size);

out:
	free(dir);
	return ret;
}

/**
 * of_overlay_register_filter - register a new overlay filter
 * @filter: The new filter
 *
 * Register a new overlay filter. A filter can either match on
 * a pattern or on the content of an overlay, but not on both.
 * If that's desired two filters have to be registered.
 *
 * @return: 0 for success, negative error code otherwise
 */
int of_overlay_register_filter(struct of_overlay_filter *filter)
{
	if ((filter->filter_pattern || filter->filter_filename) &&
	    filter->filter_content)
		return -EINVAL;

	list_add_tail(&filter->list, &of_overlay_filters);

	return 0;
}

/**
 * of_overlay_filter_pattern - A filter that matches on the filename or
 *			       FIT config-node name of an overlay
 * @f: The filter
 * @pattern: The filename or FIT config-node name of the overlay
 *
 * This filter matches when the filename or FIT config-node name matches one of
 * the patterns given in global.of.overlay.pattern. global.of.overlay.pattern
 * shall contain a space separated list of wildcard patterns.
 *
 * @return: True when the overlay shall be applied, false otherwise.
 */
static bool of_overlay_filter_pattern(struct of_overlay_filter *f,
				      const char *pattern)
{
	char *p, *path, *n;
	int ret;
	bool apply;

	p = path = strdup(of_overlay_pattern);

	while ((n = strsep_unescaped(&p, " ", NULL))) {
		if (!*n)
			continue;

		ret = fnmatch(n, pattern, 0);

		if (!ret) {
			apply = true;
			goto out;
		}
	}

	apply = false;
out:
	free(path);

	return apply;
}

static struct of_overlay_filter of_overlay_pattern_filter = {
	.name = "pattern",
	.filter_pattern = of_overlay_filter_pattern,
};

static bool of_overlay_filter_filename(struct of_overlay_filter *f,
				       const char *filename)
{
	pr_warn("'filepattern' filter is marked as deprecated, convert to 'pattern' filter\n");
	return of_overlay_filter_pattern(f, filename);
}

static struct of_overlay_filter of_overlay_filepattern_filter = {
	.name = "filepattern",
	.filter_pattern = of_overlay_filter_filename,
};

/**
 * of_overlay_filter_compatible - A filter that matches on the compatible of
 *                                an overlay
 * @f: The filter
 * @ovl: The overlay
 *
 * This filter matches when the compatible of an overlay matches to one
 * of the compatibles given in global.of.overlay.compatible. When the
 * overlay doesn't contain a compatible entry it is considered matching.
 * Also when no compatibles are given in global.of.overlay.compatible
 * all overlays will match.
 *
 * @return: True when the overlay shall be applied, false otherwise.
 */
static bool of_overlay_filter_compatible(struct of_overlay_filter *f,
					struct device_node *ovl)
{
	char *p, *n, *compatibles;
	bool res = false;

	if (!of_overlay_compatible || !*of_overlay_compatible)
		return true;
	if (!of_find_property(ovl, "compatible", NULL))
		return true;

	p = compatibles = xstrdup(of_overlay_compatible);

	while ((n = strsep_unescaped(&p, " ", NULL))) {
		if (!*n)
			continue;

		if (of_device_is_compatible(ovl, n)) {
			res = true;
			break;
		}
	}

	free(compatibles);

	return res;
}

static struct of_overlay_filter of_overlay_compatible_filter = {
	.name = "compatible",
	.filter_content = of_overlay_filter_compatible,
};

static int of_overlay_init(void)
{
	of_overlay_pattern = strdup("*");
	of_overlay_filter = strdup("pattern compatible");
	of_overlay_set_basedir("/");

	globalvar_add_simple_string("of.overlay.compatible", &of_overlay_compatible);
	globalvar_add_simple_string("of.overlay.pattern", &of_overlay_pattern);
	globalvar_add_simple_string("of.overlay.filter", &of_overlay_filter);
	globalvar_add_simple_string("of.overlay.path", &of_overlay_path);

	globalvar_alias_deprecated("of.overlay.filepattern", "of.overlay.pattern");
	globalvar_alias_deprecated("of.overlay.dir", "of.overlay.path");

	of_overlay_register_filter(&of_overlay_pattern_filter);
	of_overlay_register_filter(&of_overlay_filepattern_filter);
	of_overlay_register_filter(&of_overlay_compatible_filter);

	of_register_fixup(of_overlay_global_fixup, NULL);

	return 0;
}
device_initcall(of_overlay_init);

BAREBOX_MAGICVAR(global.of.overlay.compatible, "space separated list of compatibles an overlay must match");
BAREBOX_MAGICVAR(global.of.overlay.pattern, "space separated list of filename or fit config-node name patterns an overlay must match");
BAREBOX_MAGICVAR(global.of.overlay.path, "Path to look for dt overlays or a path to a FIT image");
BAREBOX_MAGICVAR(global.of.overlay.filter, "space separated list of filters");

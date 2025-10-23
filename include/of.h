/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __OF_H
#define __OF_H

#include <fdt.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/err.h>
#include <asm/byteorder.h>

/* Default string compare functions */
#define of_compat_cmp(s1, s2, l)	strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)		strcmp((s1), (s2))
#define of_node_cmp(s1, s2)		strcasecmp((s1), (s2))

#define OF_BAD_ADDR      ((u64)-1)

typedef u32 phandle;

struct property {
	const char *name;
	int length;
	void *value;
	const void *value_const;
	struct list_head list;
};

struct device_node {
	const char *name;
	char *full_name;

	struct list_head properties;
	struct device_node *parent;
	struct list_head children;
	struct list_head parent_list;
	struct list_head list;
	phandle phandle;
	struct device *dev;
};

struct of_device_id {
	const char *compatible;
	const void *data;
};

#define MAX_PHANDLE_ARGS 8
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};

struct of_phandle_iterator {
	/* Common iterator information */
	const char *cells_name;
	int cell_count;
	const struct device_node *parent;

	/* List size information */
	const __be32 *list_end;
	const __be32 *phandle_end;

	/* Current position state */
	const __be32 *cur;
	uint32_t cur_count;
	phandle phandle;
	struct device_node *node;
};

#define OF_MAX_RESERVE_MAP	16
struct of_reserve_map {
	uint64_t start[OF_MAX_RESERVE_MAP];
	uint64_t end[OF_MAX_RESERVE_MAP];
	int num_entries;
};

int of_add_reserve_entry(resource_size_t start, resource_size_t end);
void of_clean_reserve_map(void);
void fdt_add_reserve_map(void *fdt);
void fdt_print_reserve_map(const void *fdt);

int fdt_machine_is_compatible(const struct fdt_header *fdt, size_t fdt_size, const char *compat);


struct device;
struct driver;
struct resource;

void of_fix_tree(struct device_node *);

bool of_match(struct device *dev, const struct driver *drv);

int of_add_initrd(struct device_node *root, resource_size_t start,
		resource_size_t end);

/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
	u64 r = 0;
	for (; size--; cell++)
		r = (r << 32) | be32_to_cpu(*cell);
	return r;
}

/* Helper to write a big number; size is in cells (not bytes) */
static inline void of_write_number(void *__cell, u64 val, int size)
{
	__be32 *cell = __cell;

	while (size--) {
		cell[size] = cpu_to_be32(val);
		val >>= 32;
	}
}

static inline const void *of_property_get_value(const struct property *pp)
{
	return pp->value ? pp->value : pp->value_const;
}

static inline struct device_node *of_node_get(struct device_node *node)
{
	return node;
}
static inline void of_node_put(struct device_node *node) { }

void of_print_property(const void *data, int len);
void of_print_cmdline(struct device_node *root);

void of_print_nodes(struct device_node *node, int indent, unsigned maxpropsize);
void of_print_properties(struct device_node *node, unsigned maxpropsize);
int of_diff(struct device_node *a, struct device_node *b, int indent);
int of_probe(void);
struct device_node *of_unflatten_dtb(const void *fdt, int size);
struct device_node *of_unflatten_dtb_const(const void *infdt, int size);

int of_fixup_reserved_memory(struct device_node *node, void *data);

struct cdev;

/* Maximum score returned by of_device_is_compatible() */
#define OF_DEVICE_COMPATIBLE_MAX_SCORE	(INT_MAX / 2)

#if IS_ENABLED(CONFIG_OFTREE) && IN_PROPER
extern struct device_node *of_read_file(const char *filename);
extern struct of_reserve_map *of_get_reserve_map(void);
extern int of_bus_n_addr_cells(struct device_node *np);
extern int of_n_addr_cells(struct device_node *np);
extern int of_bus_n_size_cells(struct device_node *np);
extern int of_n_size_cells(struct device_node *np);
extern int __of_parse_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name, int cell_count,
	int index, struct of_phandle_args *out_args);
extern bool of_node_name_eq(const struct device_node *np, const char *name);
extern size_t of_node_has_prefix(const struct device_node *np, const char *prefix);

extern struct property *of_find_property(const struct device_node *np,
					const char *name, int *lenp);
extern const void *of_get_property(const struct device_node *np,
				const char *name, int *lenp);
extern struct device_node *of_get_cpu_node(int cpu, unsigned int *thread);

extern int of_set_property(struct device_node *node, const char *p,
			const void *val, int len, int create);
extern int of_append_property(struct device_node *np, const char *p,
			      const void *val, int len);
extern int of_prepend_property(struct device_node *np, const char *name,
			       const void *val, int len);
extern struct property *of_new_property(struct device_node *node,
				const char *name, const void *data, int len);
extern struct property *of_new_property_const(struct device_node *node,
					      const char *name,
					      const void *data, int len);
extern struct property *__of_new_property(struct device_node *node,
					  const char *name, void *data, int len);
extern void of_delete_property(struct property *pp);
extern struct property *of_rename_property(struct device_node *np,
					   const char *old_name, const char *new_name);
extern struct property *of_copy_property(const struct device_node *src,
					 const char *propname,
					 struct device_node *dst);

extern struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name);
extern struct device_node *of_find_node_by_name_address(struct device_node *from,
	const char *name);
extern struct device_node *of_find_node_by_path_from(struct device_node *from,
						const char *path);
extern struct device_node *of_find_node_by_path(const char *path);
extern struct device_node *of_find_node_by_phandle(phandle phandle);
extern struct device_node *of_find_node_by_phandle_from(phandle phandle,
	struct device_node *root);
extern struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type);
extern struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compat);
extern const struct of_device_id *of_match_node(
	const struct of_device_id *matches, const struct device_node *node);
extern struct device_node *of_find_matching_node_and_match(
	struct device_node *from,
	const struct of_device_id *matches,
	const struct of_device_id **match);
extern struct device_node *of_find_node_with_property(
	struct device_node *from, const char *prop_name);

extern struct device_node *of_new_node(struct device_node *parent,
				const char *name);
extern struct device_node *of_create_node(struct device_node *root,
					const char *path);
extern void of_merge_nodes(struct device_node *np, const struct device_node *other);
extern struct device_node *of_copy_node(struct device_node *parent,
				const struct device_node *other);
extern struct device_node *of_dup(const struct device_node *root);
extern void of_delete_node(struct device_node *node);

extern int of_alias_from_compatible(const struct device_node *node, char *alias, int len);
extern const char *of_get_machine_compatible(void);
extern int of_machine_is_compatible(const char *compat);
extern int of_device_is_compatible(const struct device_node *device,
		const char *compat);
extern bool of_node_is_fixed_partitions(const struct device_node *np);
extern int of_device_is_available(const struct device_node *device);
extern bool of_device_is_big_endian(const struct device_node *device);

extern struct device_node *of_get_parent(const struct device_node *node);
extern struct device_node *of_get_next_available_child(
	const struct device_node *node, struct device_node *prev);
struct device_node *of_get_next_child(const struct device_node *node,
	struct device_node *prev);
extern int of_get_child_count(const struct device_node *parent);
extern int of_get_available_child_count(const struct device_node *parent);
extern struct device_node *of_get_compatible_child(const struct device_node *parent,
					const char *compatible);
extern struct device_node *of_get_child_by_name(const struct device_node *node,
					const char *name);
extern struct device_node *of_get_child_by_name_stem(const struct device_node *node,
					const char *name);
extern char *of_get_reproducible_name(struct device_node *node);
extern struct device_node *of_get_node_by_reproducible_name(struct device_node *dstroot,
							    struct device_node *srcnp);

extern struct device_node *of_find_node_by_reproducible_name(struct device_node
							     *from,
							     const char *name);
extern int of_property_read_u32_index(const struct device_node *np,
				       const char *propname,
				       u32 index, u32 *out_value);
extern int of_property_count_elems_of_size(const struct device_node *np,
				const char *propname, int elem_size);
extern int of_property_read_u8_array(const struct device_node *np,
			const char *propname, u8 *out_values, size_t sz);
extern int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz);
extern int of_property_read_u32_array(const struct device_node *np,
				      const char *propname,
				      u32 *out_values,
				      size_t sz);
extern int of_property_read_u64(const struct device_node *np,
				const char *propname, u64 *out_value);
extern int of_property_read_variable_u8_array(const struct device_node *np,
					      const char *propname, u8 *out_values,
					      size_t sz_min, size_t sz_max);
extern int of_property_read_variable_u16_array(const struct device_node *np,
					       const char *propname, u16 *out_values,
					       size_t sz_min, size_t sz_max);
extern int of_property_read_variable_u32_array(const struct device_node *np,
					       const char *propname,
					       u32 *out_values,
					       size_t sz_min,
					       size_t sz_max);
extern int of_property_read_variable_u64_array(const struct device_node *np,
					const char *propname,
					u64 *out_values,
					size_t sz_min,
					size_t sz_max);

extern int of_property_read_string(struct device_node *np,
				   const char *propname,
				   const char **out_string);
extern int of_property_match_string(const struct device_node *np,
				    const char *propname,
				    const char *string);
extern int of_property_read_string_helper(const struct device_node *np,
					      const char *propname,
					      const char **out_strs, size_t sz, int index);

extern const __be32 *of_prop_next_u32(const struct property *prop,
				const __be32 *cur, u32 *pu);
extern const char *of_prop_next_string(const struct property *prop, const char *cur);

extern int of_property_write_bool(struct device_node *np,
				const char *propname, const bool value);
extern int of_property_write_u8_array(struct device_node *np,
				const char *propname, const u8 *values,
				size_t sz);
extern int of_property_write_u16_array(struct device_node *np,
				const char *propname, const u16 *values,
				size_t sz);
extern int of_property_write_u32_array(struct device_node *np,
				const char *propname, const u32 *values,
				size_t sz);
extern int of_property_write_u64_array(struct device_node *np,
				const char *propname, const u64 *values,
				size_t sz);
extern int of_property_write_string(struct device_node *np, const char *propname,
				    const char *value);
extern int of_property_write_strings(struct device_node *np, const char *propname,
				    ...) __attribute__((__sentinel__));
int of_property_sprintf(struct device_node *np, const char *propname, const char *fmt, ...)
	__printf(3, 4);

extern struct device_node *of_parse_phandle(const struct device_node *np,
					    const char *phandle_name,
					    int index);
extern struct device_node *of_parse_phandle_from(const struct device_node *np,
					    struct device_node *root,
					    const char *phandle_name,
					    int index);
/**
 * of_parse_phandle_with_args() - Find a node pointed by phandle in a list
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 * @index:	index of a phandle to parse out
 * @out_args:	optional pointer to output arguments structure (will be filled)
 *
 * This function is useful to parse lists of phandles and their arguments.
 * Returns 0 on success and fills out_args, on error returns appropriate
 * errno value.
 *
 * Caller is responsible to call of_node_put() on the returned out_args->np
 * pointer.
 *
 * Example::
 *
 *  phandle1: node1 {
 *	#list-cells = <2>;
 *  };
 *
 *  phandle2: node2 {
 *	#list-cells = <1>;
 *  };
 *
 *  node3 {
 *	list = <&phandle1 1 2 &phandle2 3>;
 *  };
 *
 * To get a device_node of the ``node2`` node you may call this:
 * of_parse_phandle_with_args(node3, "list", "#list-cells", 1, &args);
 */
static inline int of_parse_phandle_with_args(const struct device_node *np,
					     const char *list_name,
					     const char *cells_name,
					     int index,
					     struct of_phandle_args *out_args)
{
	int cell_count = -1;

	/* If cells_name is NULL we assume a cell count of 0 */
	if (!cells_name)
		cell_count = 0;

	return __of_parse_phandle_with_args(np, list_name, cells_name,
					    cell_count, index, out_args);
}

/**
 * of_parse_phandle_with_optional_args() - Find a node pointed by phandle in a list
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 * @index:	index of a phandle to parse out
 * @out_args:	optional pointer to output arguments structure (will be filled)
 *
 * Same as of_parse_phandle_with_args() except that if the cells_name property
 * is not found, cell_count of 0 is assumed.
 *
 * This is used to useful, if you have a phandle which didn't have arguments
 * before and thus doesn't have a '#*-cells' property but is now migrated to
 * having arguments while retaining backwards compatibility.
 */
static inline int of_parse_phandle_with_optional_args(const struct device_node *np,
						      const char *list_name,
						      const char *cells_name,
						      int index,
						      struct of_phandle_args *out_args)
{
	return __of_parse_phandle_with_args(np, list_name, cells_name,
					    0, index, out_args);
}

extern int of_count_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name);

/* phandle iterator functions */
extern int of_phandle_iterator_init(struct of_phandle_iterator *it,
				    const struct device_node *np,
				    const char *list_name,
				    const char *cells_name,
				    int cell_count);

extern int of_phandle_iterator_next(struct of_phandle_iterator *it);
extern int of_phandle_iterator_args(struct of_phandle_iterator *it,
				    uint32_t *args,
				    int size);

extern void of_alias_scan(void);
extern int of_alias_get_id(struct device_node *np, const char *stem);
extern int of_alias_get_id_from(struct device_node *root, struct device_node *np,
				const char *stem);
extern int of_alias_get_highest_id(const char *stem);
extern const char *of_alias_get(struct device_node *np);
extern int of_modalias_node(struct device_node *node, char *modalias, int len);

extern const char *of_property_get_alias_from(struct device_node *root,
					      const char *np_name, const char *propname,
					      int index);

extern const char *of_parse_phandle_and_get_alias_from(struct device_node *root,
						       const char *np_name, const char *phandle_name,
						       int index);

extern struct device_node *of_get_root_node(void);
extern struct fdt_header *of_get_flattened_tree(const struct device_node *node, bool fixup);
extern int of_set_root_node(struct device_node *node);
extern int barebox_register_of(struct device_node *root);
extern int barebox_register_fdt(const void *dtb);

extern struct device *of_platform_root_device;

extern struct device *of_platform_device_create(struct device_node *np,
						struct device *parent);
extern void of_platform_device_dummy_drv(struct device *dev);
extern int of_platform_populate(struct device_node *root,
				const struct of_device_id *matches,
				struct device *parent);
extern struct device *of_find_device_by_node(struct device_node *np);
extern struct device *of_device_enable_and_register(struct device_node *np);
extern struct device *of_device_enable_and_register_by_name(const char *name);
extern struct device *of_device_enable_and_register_by_alias(
							const char *alias);

extern int of_device_ensure_probed(struct device_node *np);
extern int of_device_ensure_probed_by_alias(const char *alias);
extern int of_devices_ensure_probed_by_property(const char *property_name);
extern int of_devices_ensure_probed_by_name(const char *name);
extern int of_devices_ensure_probed_by_dev_id(const struct of_device_id *ids);
extern int of_partition_ensure_probed(struct device_node *np);

struct cdev *of_parse_partition(struct cdev *cdev, struct device_node *node);
int of_parse_partitions(struct cdev *cdev, struct device_node *node);
int of_fixup_partitions(struct device_node *np, struct cdev *cdev);
int of_partitions_register_fixup(struct cdev *cdev);
struct device_node *of_find_node_by_chosen(const char *propname,
					   const char **options);
struct device_node *of_get_stdoutpath(unsigned int *);
int of_device_is_stdout_path(struct device *dev, unsigned int *baudrate);
const char *of_get_model(void);
void *of_flatten_dtb(struct device_node *node);
int of_add_memory(struct device_node *node, bool dump);
int of_add_memory_bank(struct device_node *node, bool dump, int r,
		u64 base, u64 size);
struct device *of_find_device_by_node_path(const char *path);
#define OF_FIND_PATH_FLAGS_BB 1		/* return .bb device if available */
int of_find_path(struct device_node *node, const char *propname, char **outpath, unsigned flags);
struct cdev *of_cdev_find(struct device_node *node);
int of_find_path_by_node(struct device_node *node, char **outpath, unsigned flags);
struct device_node *of_find_node_by_devpath(struct device_node *root, const char *path);
int of_register_fixup(int (*fixup)(struct device_node *, void *), void *context);
int of_unregister_fixup(int (*fixup)(struct device_node *, void *), void *context);

struct of_fixup {
	int (*fixup)(struct device_node *, void *);
	void *context;
	struct list_head list;
	bool disabled;
};

extern struct list_head of_fixup_list;

int of_register_set_status_fixup(const char *node, bool status);
struct device_node *of_find_node_by_alias(struct device_node *root,
		const char *alias);
struct device_node *of_find_node_by_path_or_alias(struct device_node *root,
		const char *str);
int of_autoenable_device_by_path(char *path);
int of_autoenable_i2c_by_component(char *path);
int of_prepend_machine_compatible(struct device_node *root, const char *compat);

static inline const char *of_node_full_name(const struct device_node *np)
{
	return np ? np->full_name : "<no-node>";
}

#else
static inline struct device_node *of_read_file(const char *filename)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct of_reserve_map *of_get_reserve_map(void)
{
	return NULL;
}

static inline bool of_node_name_eq(const struct device_node *np, const char *name)
{
	return false;
}

static inline size_t of_node_has_prefix(const struct device_node *np, const char *prefix)
{
	return 0;
}

static inline int of_parse_partitions(struct cdev *cdev,
					  struct device_node *node)
{
	return -EINVAL;
}

static inline int of_fixup_partitions(struct device_node *np, struct cdev *cdev)
{
	return -ENOSYS;
}

static inline int of_partitions_register_fixup(struct cdev *cdev)
{
	return -ENOSYS;
}

static inline struct device_node *of_find_node_by_chosen(const char *propname,
							 const char **options)
{
	return NULL;
}

static inline struct device_node *of_get_stdoutpath(unsigned int *rate)
{
	return NULL;
}

static inline int of_device_is_stdout_path(struct device *dev,
					   unsigned int *baudrate)
{
	return 0;
}

static inline const char *of_get_model(void)
{
	return NULL;
}

static inline void *of_flatten_dtb(struct device_node *node)
{
	return NULL;
}

static inline int of_add_memory(struct device_node *node, bool dump)
{
	return -EINVAL;
}

static inline struct device_node *of_get_root_node(void)
{
	return NULL;
}

static inline struct fdt_header *of_get_flattened_tree(const struct device_node *node, bool fixup)
{
	return NULL;
}

static inline int of_set_root_node(struct device_node *node)
{
	return -ENOSYS;
}

static inline int barebox_register_of(struct device_node *root)
{
	return -ENOSYS;
}

static inline struct device *of_platform_device_create(struct device_node *np,
							 struct device *parent)
{
	return NULL;
}

static inline void of_platform_device_dummy_drv(struct device *dev)
{
}

static inline int of_device_ensure_probed(struct device_node *np)
{
	return 0;
}

static inline int of_device_ensure_probed_by_alias(const char *alias)
{
	return 0;
}

static inline int of_devices_ensure_probed_by_property(const char *property_name)
{
	return 0;
}

static inline int
of_devices_ensure_probed_by_dev_id(const struct of_device_id *ids)
{
	return 0;
}

static inline int of_partition_ensure_probed(struct device_node *np)
{
	return 0;
}

static inline int of_bus_n_addr_cells(struct device_node *np)
{
	return 0;
}

static inline int of_n_addr_cells(struct device_node *np)
{
	return 0;
}

static inline int of_bus_n_size_cells(struct device_node *np)
{
	return 0;
}

static inline int of_n_size_cells(struct device_node *np)
{
	return 0;
}

static inline struct device_node *of_get_parent(const struct device_node *node)
{
	return NULL;
}

static inline struct device_node *of_get_next_available_child(
		const struct device_node *node, struct device_node *prev)
{
	return NULL;
}

static inline struct device_node *of_get_next_child(const struct device_node *node,
	struct device_node *prev)
{
	return NULL;
}

static inline int of_get_child_count(const struct device_node *parent)
{
	return -ENOSYS;
}

static inline int of_get_available_child_count(const struct device_node *parent)
{
	return -ENOSYS;
}

static inline struct device_node *of_get_compatible_child(const struct device_node *parent,
					const char *compatible)
{
	return NULL;
}

static inline struct device_node *of_get_child_by_name(
			const struct device_node *node, const char *name)
{
	return NULL;
}

static inline struct device_node *of_get_child_by_name_stem(
			const struct device_node *node, const char *name)
{
	return NULL;
}

static inline char *of_get_reproducible_name(struct device_node *node)
{
	return NULL;
}

static inline struct device_node *
of_find_node_by_reproducible_name(struct device_node *from, const char *name)
{
	return NULL;
}


static inline struct device_node *of_get_node_by_reproducible_name(struct device_node *dstroot,
								  struct device_node *srcnp)
{
	return NULL;
}

static inline struct property *of_find_property(const struct device_node *np,
						const char *name,
						int *lenp)
{
	return NULL;
}

static inline const void *of_get_property(const struct device_node *np,
				const char *name, int *lenp)
{
	return NULL;
}

static inline struct device_node *of_get_cpu_node(int cpu,
					unsigned int *thread)
{
	return NULL;
}

static inline int of_set_property(struct device_node *node, const char *p,
			const void *val, int len, int create)
{
	return -ENOSYS;
}

static inline int of_append_property(struct device_node *np, const char *p,
				      const void *val, int len)
{
	return -ENOSYS;
}

static inline int of_prepend_property(struct device_node *np, const char *name,
				      const void *val, int len)
{
	return -ENOSYS;
}

static inline struct property *of_new_property(struct device_node *node,
				const char *name, const void *data, int len)
{
	return NULL;
}

static inline struct property *__of_new_property(struct device_node *node,
					  const char *name, void *data, int len)
{
	return NULL;
}

static inline struct property *of_copy_property(const struct device_node *src,
						const char *propname,
						struct device_node *dst)
{
	return NULL;
}

static inline void of_delete_property(struct property *pp)
{
}

static inline struct property *of_rename_property(struct device_node *np,
						  const char *old_name, const char *new_name)
{
	return NULL;
}

static inline int of_property_read_u32_index(const struct device_node *np,
				const char *propname, u32 index, u32 *out_value)
{
	return -ENOSYS;
}

static inline int of_property_count_elems_of_size(const struct device_node *np,
			const char *propname, int elem_size)
{
	return -ENOSYS;
}

static inline int of_property_read_u8_array(const struct device_node *np,
				const char *propname, u8 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u32_array(const struct device_node *np,
			const char *propname, u32 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u64(const struct device_node *np,
				const char *propname, u64 *out_value)
{
	return -ENOSYS;
}

static inline  int of_property_read_variable_u8_array(const struct device_node *np,
					      const char *propname, u8 *out_values,
					      size_t sz_min, size_t sz_max)
{
	return -ENOSYS;
}

static inline  int of_property_read_variable_u16_array(const struct device_node *np,
					       const char *propname, u16 *out_values,
					       size_t sz_min, size_t sz_max)
{
	return -ENOSYS;
}

static inline  int of_property_read_variable_u32_array(const struct device_node *np,
					       const char *propname,
					       u32 *out_values,
					       size_t sz_min,
					       size_t sz_max)
{
	return -ENOSYS;
}

static inline int of_property_read_variable_u64_array(const struct device_node *np,
					const char *propname,
					u64 *out_values,
					size_t sz_min,
					size_t sz_max)
{
	return -ENOSYS;
}

static inline int of_property_read_string(struct device_node *np,
				const char *propname, const char **out_string)
{
	return -ENOSYS;
}

static inline int of_property_match_string(const struct device_node *np,
				const char *propname, const char *string)
{
	return -ENOSYS;
}

static inline int of_property_read_string_helper(const struct device_node *np,
						 const char *propname,
						 const char **out_strs, size_t sz, int index)
{
	return -ENOSYS;
}

static inline int __of_parse_phandle_with_args(const struct device_node *np,
					       const char *list_name,
					       const char *cells_name,
					       int cell_count,
					       int index,
					       struct of_phandle_args *out_args)
{
	return -ENOSYS;
}

static inline const __be32 *of_prop_next_u32(const struct property *prop,
					const __be32 *cur, u32 *pu)
{
	return 0;
}

static inline const char *of_prop_next_string(const struct property *prop,
					const char *cur)
{
	return NULL;
}

static inline int of_property_write_bool(struct device_node *np,
				const char *propname, const bool value)
{
	return -ENOSYS;
}

static inline int of_property_write_u8_array(struct device_node *np,
			const char *propname, const u8 *values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_write_u16_array(struct device_node *np,
			const char *propname, const u16 *values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_write_u32_array(struct device_node *np,
			const char *propname, const u32 *values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_write_u64_array(struct device_node *np,
			const char *propname, const u64 *values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_write_string(struct device_node *np, const char *propname,
				    const char *value)
{
	return -ENOSYS;
}

static inline struct device_node *of_parse_phandle(const struct device_node *np,
					    const char *phandle_name, int index)
{
	return NULL;
}

static inline struct device_node *of_parse_phandle_from(const struct device_node *np,
					    struct device_node *root,
					    const char *phandle_name, int index)
{
	return NULL;
}

static inline int of_parse_phandle_with_args(const struct device_node *np,
		const char *list_name, const char *cells_name, int index,
		struct of_phandle_args *out_args)
{
	return -ENOSYS;
}

static inline int of_count_phandle_with_args(const struct device_node *np,
				const char *list_name, const char *cells_name)
{
	return -ENOSYS;
}

static inline int of_phandle_iterator_init(struct of_phandle_iterator *it,
					   const struct device_node *np,
					   const char *list_name,
					   const char *cells_name,
					   int cell_count)
{
	return -ENOSYS;
}

static inline int of_phandle_iterator_next(struct of_phandle_iterator *it)
{
	return -ENOSYS;
}

static inline int of_phandle_iterator_args(struct of_phandle_iterator *it,
					   uint32_t *args,
					   int size)
{
	return 0;
}

static inline struct device_node *of_find_node_by_path_from(
	struct device_node *from, const char *path)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_path(const char *path)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_name_address(struct device_node *from,
	const char *name)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_phandle(phandle phandle)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_phandle_from(phandle phandle,
	struct device_node *root)
{
	return NULL;
}

static inline struct device_node *of_find_compatible_node(
						struct device_node *from,
						const char *type,
						const char *compat)
{
	return NULL;
}

static inline const struct of_device_id *of_match_node(
	const struct of_device_id *matches, const struct device_node *node)
{
	return NULL;
}

static inline struct device_node *of_find_matching_node_and_match(
					struct device_node *from,
					const struct of_device_id *matches,
					const struct of_device_id **match)
{
	return NULL;
}

static inline struct device_node *of_find_node_with_property(
			struct device_node *from, const char *prop_name)
{
	return NULL;
}

static inline struct device_node *of_new_node(struct device_node *parent,
				const char *name)
{
	return NULL;
}

static inline struct device_node *of_create_node(struct device_node *root,
					const char *path)
{
	return NULL;
}

static inline struct device_node *of_dup(const struct device_node *root)
{
	return NULL;
}

static inline void of_delete_node(struct device_node *node)
{
}

static inline int of_machine_is_compatible(const char *compat)
{
	return 0;
}

static inline int of_device_is_compatible(const struct device_node *device,
		const char *compat)
{
	return 0;
}

static inline bool of_node_is_fixed_partitions(const struct device_node *device)
{
	return false;
}

static inline int of_device_is_available(const struct device_node *device)
{
	return 0;
}

static inline bool of_device_is_big_endian(const struct device_node *device)
{
	return false;
}

static inline void of_alias_scan(void)
{
}

static inline int of_alias_get_id(struct device_node *np, const char *stem)
{
	return -ENOSYS;
}

static inline int of_alias_get_id_from(struct device_node *root, struct device_node *np,
				       const char *stem)
{
	return -ENOSYS;
}

static inline int of_alias_get_highest_id(const char *stem)
{
	return -ENOSYS;
}

static inline const char *of_alias_get(struct device_node *np)
{
	return NULL;
}

static inline int of_modalias_node(struct device_node *node, char *modalias,
				int len)
{
	return -ENOSYS;
}

static inline const char *of_property_get_alias_from(struct device_node *root,
						     const char *np_name, const char *propname,
						     int index)
{
	return NULL;
}

static inline const char *of_parse_phandle_and_get_alias_from(struct device_node *root,
							      const char *np_name, const char *phandle_name,
							      int index)
{
	return NULL;
}

static inline int of_platform_populate(struct device_node *root,
				const struct of_device_id *matches,
				struct device *parent)
{
	return -ENOSYS;
}

static inline struct device *of_find_device_by_node(struct device_node *np)
{
	return NULL;
}

static inline struct device *of_device_enable_and_register(
				struct device_node *np)
{
	return NULL;
}

static inline struct device *of_device_enable_and_register_by_name(
				const char *name)
{
	return NULL;
}

static inline struct device *of_device_enable_and_register_by_alias(
				const char *alias)
{
	return NULL;
}

static inline struct cdev *of_cdev_find(struct device_node *node)
{
	return ERR_PTR(-ENOSYS);
}

static inline int of_register_fixup(int (*fixup)(struct device_node *, void *),
				void *context)
{
	return -ENOSYS;
}

static inline struct device_node *of_find_node_by_alias(
				struct device_node *root, const char *alias)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_path_or_alias(
				struct device_node *root, const char *str)
{
	return NULL;
}

static inline int of_autoenable_i2c_by_path(char *path)
{
	return -ENODEV;
}

static inline int of_autoenable_i2c_by_component(char *path)
{
	return -ENODEV;
}

static inline int of_prepend_machine_compatible(struct device_node *root,
					 const char *compat)
{
	return -ENODEV;
}

static inline const char *of_node_full_name(const struct device_node *np)
{
	return "<no-node>";
}

#endif

#define for_each_property_of_node(dn, pp) \
	list_for_each_entry(pp, &dn->properties, list)
#define for_each_node_by_name(dn, name) \
	for (dn = of_find_node_by_name(NULL, name); dn; \
	     dn = of_find_node_by_name(dn, name))
#define for_each_node_by_name_address(dn, name) \
	for (dn = of_find_node_by_name_address(NULL, name); dn; \
	     dn = of_find_node_by_name_address(dn, name))
#define for_each_node_by_type(dn, type) \
	for (dn = of_find_node_by_type(NULL, type); dn; \
	     dn = of_find_node_by_type(dn, type))
#define for_each_node_by_name_from(dn, root, name) \
	for (dn = of_find_node_by_name(root, name); dn; \
	     dn = of_find_node_by_name(dn, name))
#define for_each_node_by_name_address_from(dn, root, name) \
	for (dn = of_find_node_by_name_address(root, name); dn; \
	     dn = of_find_node_by_name_address(dn, name))
/* Iterate over compatible nodes starting from given root */
#define for_each_compatible_node_from(dn, root, type, compatible) \
	for (dn = of_find_compatible_node(root, type, compatible); dn; \
	     dn = of_find_compatible_node(dn, type, compatible))
/* Iterate over compatible nodes in default device tree */
#define for_each_compatible_node(dn, type, compatible) \
        for_each_compatible_node_from(dn, NULL, type, compatible)
static inline struct device_node *of_find_matching_node(
	struct device_node *from,
	const struct of_device_id *matches)
{
	return of_find_matching_node_and_match(from, matches, NULL);
}
#define for_each_matching_node_from(dn, root, matches) \
	for (dn = of_find_matching_node(root, matches); dn; \
	     dn = of_find_matching_node(dn, matches))
#define for_each_matching_node(dn, matches) \
	for_each_matching_node_from(dn, NULL, matches)
#define for_each_matching_node_and_match(dn, matches, match) \
	for (dn = of_find_matching_node_and_match(NULL, matches, match); \
	     dn; dn = of_find_matching_node_and_match(dn, matches, match))
#define for_each_node_with_property(dn, prop_name) \
	for (dn = of_find_node_with_property(NULL, prop_name); dn; \
	     dn = of_find_node_with_property(dn, prop_name))

#define for_each_child_of_node_safe(parent, tmp, child) \
	list_for_each_entry_safe(child, tmp, &parent->children, parent_list)
#define for_each_child_of_node(parent, child) \
	list_for_each_entry(child, &parent->children, parent_list)
#define for_each_available_child_of_node(parent, child) \
	for (child = of_get_next_available_child(parent, NULL); child != NULL; \
	     child = of_get_next_available_child(parent, child))

/**
 * of_property_read_string_array() - Read an array of strings from a multiple
 * strings property.
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_strs:	output array of string pointers.
 * @sz:		number of array elements to read.
 *
 * Search for a property in a device tree node and retrieve a list of
 * terminated string values (pointer to data, not a copy) in that property.
 *
 * If @out_strs is NULL, the number of strings in the property is returned.
 */
static inline int of_property_read_string_array(const struct device_node *np,
						const char *propname, const char **out_strs,
						size_t sz)
{
	return of_property_read_string_helper(np, propname, out_strs, sz, 0);
}

/**
 * of_property_count_strings() - Find and return the number of strings from a
 * multiple strings property.
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 *
 * Search for a property in a device tree node and retrieve the number of null
 * terminated string contain in it. Returns the number of strings on
 * success, -EINVAL if the property does not exist, -ENODATA if property
 * does not have a value, and -EILSEQ if the string is not null-terminated
 * within the length of the property data.
 */
static inline int of_property_count_strings(const struct device_node *np,
					    const char *propname)
{
	return of_property_read_string_helper(np, propname, NULL, 0, 0);
}

/**
 * of_property_read_string_index() - Find and read a string from a multiple
 * strings property.
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @index:	index of the string in the list of strings
 * @out_string:	pointer to null terminated return string, modified only if
 *		return value is 0.
 *
 * Search for a property in a device tree node and retrieve a null
 * terminated string value (pointer to data, not a copy) in the list of strings
 * contained in that property.
 * Returns 0 on success, -EINVAL if the property does not exist, -ENODATA if
 * property does not have a value, and -EILSEQ if the string is not
 * null-terminated within the length of the property data.
 *
 * The out_string pointer is modified only if a valid string can be decoded.
 */
static inline int of_property_read_string_index(const struct device_node *np,
						const char *propname,
						int index, const char **output)
{
	int rc = of_property_read_string_helper(np, propname, output, 1, index);
	return rc < 0 ? rc : 0;
}

/**
 * of_property_read_bool - Findfrom a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 *
 * Search for a boolean property in a device node. Usage on non-boolean
 * property types is deprecated.

 * Return: true if the property exist false otherwise.
 */
static inline bool of_property_read_bool(const struct device_node *np,
					 const char *propname)
{
	struct property *prop = of_find_property(np, propname, NULL);

	return prop ? true : false;
}

/**
 * of_property_present - Test if a property is present in a node
 * @np:		device node to search for the property.
 * @propname:	name of the property to be searched.
 *
 * Test for a property present in a device node.
 *
 * Return: true if the property exists false otherwise.
 */
static inline bool of_property_present(const struct device_node *np, const char *propname)
{
	return of_property_read_bool(np, propname);
}

static inline int of_property_read_u8(const struct device_node *np,
				       const char *propname,
				       u8 *out_value)
{
	return of_property_read_u8_array(np, propname, out_value, 1);
}

static inline int of_property_read_u16(const struct device_node *np,
				       const char *propname,
				       u16 *out_value)
{
	return of_property_read_u16_array(np, propname, out_value, 1);
}

static inline int of_property_read_u32(const struct device_node *np,
				       const char *propname,
				       u32 *out_value)
{
	return of_property_read_u32_array(np, propname, out_value, 1);
}

static inline int of_property_read_s32(const struct device_node *np,
				       const char *propname,
				       s32 *out_value)
{
	return of_property_read_u32(np, propname, (u32*) out_value);
}

#define of_for_each_phandle(it, err, np, ln, cn, cc)			\
	for (of_phandle_iterator_init((it), (np), (ln), (cn), (cc)),	\
	     err = of_phandle_iterator_next(it);			\
	     err == 0;							\
	     err = of_phandle_iterator_next(it))


/**
 * of_property_read_u64_array - Find and read an array of 64 bit integers
 * from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_values:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 64-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_values is modified only if a valid u64 value can be decoded.
 */
static inline int of_property_read_u64_array(const struct device_node *np,
					     const char *propname,
					     u64 *out_values, size_t sz)
{
	int ret = of_property_read_variable_u64_array(np, propname, out_values,
						      sz, 0);
	if (ret >= 0)
		return 0;
	else
		return ret;
}

/*
 * struct property *prop;
 * const __be32 *p;
 * u32 u;
 *
 * of_property_for_each_u32(np, "propname", prop, p, u)
 *         printk("U32 value: %x\n", u);
 */
#define of_property_for_each_u32(np, propname, prop, p, u)	\
	for (prop = of_find_property(np, propname, NULL),	\
		p = of_prop_next_u32(prop, NULL, &u);		\
		p;						\
		p = of_prop_next_u32(prop, p, &u))

/*
 * struct property *prop;
 * const char *s;
 *
 * of_property_for_each_string(np, "propname", prop, s)
 *         printk("String value: %s\n", s);
 */
#define of_property_for_each_string(np, propname, prop, s)	\
	for (prop = of_find_property(np, propname, NULL),	\
		s = of_prop_next_string(prop, NULL);		\
		s;						\
		s = of_prop_next_string(prop, s))

static inline int of_property_write_u8(struct device_node *np,
				       const char *propname, u8 value)
{
	return of_property_write_u8_array(np, propname, &value, 1);
}

static inline int of_property_write_u16(struct device_node *np,
					const char *propname, u16 value)
{
	return of_property_write_u16_array(np, propname, &value, 1);
}

static inline int of_property_write_u32(struct device_node *np,
					const char *propname,
					u32 value)
{
	return of_property_write_u32_array(np, propname, &value, 1);
}

static inline int of_property_write_u64(struct device_node *np,
					const char *propname,
					u64 value)
{
	return of_property_write_u64_array(np, propname, &value, 1);
}

static inline void of_delete_property_by_name(struct device_node *np, const char *name)
{
	of_delete_property(of_find_property(np, name, NULL));
}

static inline const char *of_property_get_alias(const char *np_name, const char *propname)
{
	return of_property_get_alias_from(NULL, np_name, propname, 0);
}

static inline const char *of_parse_phandle_and_get_alias(const char *np_name, const char *phandle_name)
{
	return of_parse_phandle_and_get_alias_from(NULL, np_name, phandle_name, 0);
}

extern const struct of_device_id of_default_bus_match_table[];

int of_device_enable(struct device_node *node);
int of_device_enable_path(const char *path);
int of_device_enable_by_alias(const char *alias);
int of_device_disable(struct device_node *node);
int of_device_disable_path(const char *path);
int of_device_disable_by_alias(const char *alias);

static inline int of_devices_ensure_probed_by_compatible(const char *compatible)
{
	struct of_device_id match_id[] = {
		{ .compatible = compatible, },
		{ /* sentinel */ },
	};

	return of_devices_ensure_probed_by_dev_id(match_id);
}

phandle of_get_tree_max_phandle(struct device_node *root);
phandle of_node_create_phandle(struct device_node *node);
int of_set_property_to_child_phandle(struct device_node *node, char *prop_name);

static inline struct device_node *of_find_root_node(struct device_node *node)
{
	while (node->parent)
		node = node->parent;

	return node;
}

/*
 * Get the fixed fdt. This function uses the fdt input pointer
 * if provided or the barebox internal devicetree if not.
 * It increases the size of the tree and applies the registered
 * fixups.
 */
static inline struct fdt_header *of_get_fixed_tree(const struct device_node *node)
{
	return of_get_flattened_tree(node, true);
}

static inline struct fdt_header *of_get_fixed_tree_for_boot(const struct device_node *node)
{
	if (!IS_ENABLED(CONFIG_BOOTM_OFTREE_FALLBACK) && !node)
		return NULL;

	return of_get_fixed_tree(node);
}

static inline struct device_node *of_dup_root_node_for_boot(void)
{
	if (!IS_ENABLED(CONFIG_BOOTM_OFTREE_FALLBACK))
		return NULL;

	return of_dup(of_get_root_node());
}

struct of_overlay_filter {
	bool (*filter_filename)(struct of_overlay_filter *, const char *filename); /* deprecated */
	bool (*filter_pattern)(struct of_overlay_filter *, const char *pattern);
	bool (*filter_content)(struct of_overlay_filter *, struct device_node *);
	const char *name;
	struct list_head list;
};

#ifdef CONFIG_OF_OVERLAY
struct device_node *of_resolve_phandles(struct device_node *root,
					const struct device_node *overlay);
int of_overlay_apply_tree(struct device_node *root,
			  struct device_node *overlay);
int of_overlay_apply_file(struct device_node *root, const char *filename,
			  bool filter);
int of_overlay_apply_dtbo(struct device_node *root,
			  const void *dtbo);
int of_register_overlay(struct device_node *overlay);
int of_process_overlay(struct device_node *root,
		    struct device_node *overlay,
		    int (*process)(struct device_node *target,
				   struct device_node *overlay, void *data),
		    void *data);

int of_overlay_pre_load_firmware(struct device_node *root, struct device_node *overlay);
int of_overlay_load_firmware(void);
void of_overlay_load_firmware_clear(void);
void of_overlay_set_basedir(const char *path);
int of_overlay_register_filter(struct of_overlay_filter *);
#else
static inline struct device_node *of_resolve_phandles(struct device_node *root,
					const struct device_node *overlay)
{
	return NULL;
}

static inline int of_overlay_apply_tree(struct device_node *root,
					struct device_node *overlay)
{
	return -ENOSYS;
}

static inline int of_overlay_apply_file(struct device_node *root,
					const char *filename, bool filter)
{
	return -ENOSYS;
}

static inline int of_overlay_apply_dtbo(struct device_node *root,
					const void *dtbo)
{
	return -ENOSYS;
}

static inline int of_register_overlay(struct device_node *overlay)
{
	return -ENOSYS;
}

static inline int of_process_overlay(struct device_node *root,
				     struct device_node *overlay,
				     int (*process)(struct device_node *target,
						    struct device_node *overlay, void *data),
				     void *data)
{
	return -ENOSYS;
}

static inline int of_overlay_pre_load_firmware(struct device_node *root,
					       struct device_node *overlay)
{
	return -ENOSYS;
}

static inline int of_overlay_load_firmware(void)
{
	return 0;
}

static inline void of_overlay_load_firmware_clear(void)
{
}

static inline void of_overlay_set_basedir(const char *path)
{
}

#endif

#endif /* __OF_H */

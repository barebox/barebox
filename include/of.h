#ifndef __OF_H
#define __OF_H

#include <fdt.h>
#include <errno.h>
#include <asm/byteorder.h>

#define OF_BAD_ADDR      ((u64)-1)

typedef u32 phandle;

struct property {
	char *name;
	int length;
	void *value;
	struct list_head list;
};

struct device_node {
	char *name;
	char *full_name;

	struct list_head properties;
	struct device_node *parent;
	struct list_head children;
	struct list_head parent_list;
	struct list_head list;
	struct resource *resource;
	struct device_d *device;
	struct list_head phandles;
	phandle phandle;
};

struct of_device_id {
	char *compatible;
	unsigned long data;
};

#define OF_MAX_RESERVE_MAP	16
struct of_reserve_map {
	uint64_t start[OF_MAX_RESERVE_MAP];
	uint64_t end[OF_MAX_RESERVE_MAP];
	int num_entries;
};

int of_add_reserve_entry(resource_size_t start, resource_size_t end);
struct of_reserve_map *of_get_reserve_map(void);
void of_clean_reserve_map(void);
void fdt_add_reserve_map(void *fdt);

struct driver_d;

int of_fix_tree(struct device_node *);
int of_register_fixup(int (*fixup)(struct device_node *));

int of_match(struct device_d *dev, struct driver_d *drv);

int of_add_initrd(struct device_node *root, resource_size_t start,
		resource_size_t end);

int of_n_addr_cells(struct device_node *np);
int of_n_size_cells(struct device_node *np);

struct property *of_find_property(const struct device_node *node, const char *name);

struct device_node *of_find_node_by_path(struct device_node *root, const char *path);

struct device_node *of_find_child_by_name(struct device_node *node, const char *name);

struct fdt_header *fdt_get_tree(void);

struct fdt_header *of_get_fixed_tree(struct device_node *node);

#define device_node_for_nach_child(node, child) \
	list_for_each_entry(child, &node->children, parent_list)

/*
 * Iterate over all nodes of a tree. As a devicetree does not
 * have a dedicated list head, the start node (usually the root
 * node) will not be iterated over.
 */
#define of_tree_for_each_node(node, root) \
	list_for_each_entry(node, &root->list, list)

/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
	u64 r = 0;
	while (size--)
		r = (r << 32) | be32_to_cpu(*(cell++));
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

int of_property_read_u32_array(const struct device_node *np,
			       const char *propname, u32 *out_values,
			       size_t sz);

static inline int of_property_read_u32(const struct device_node *np,
				       const char *propname,
				       u32 *out_value)
{
	return of_property_read_u32_array(np, propname, out_value, 1);
}

int of_property_write_u32_array(struct device_node *np,
				const char *propname, const u32 *values,
				size_t sz);

static inline int of_property_write_u32(struct device_node *np,
					const char *propname,
					u32 value)
{
	return of_property_write_u32_array(np, propname, &value, 1);
}

const void *of_get_property(const struct device_node *np, const char *name,
			 int *lenp);

int of_parse_phandles_with_args(struct device_node *np, const char *list_name,
				const char *cells_name, int index,
				struct device_node **out_node,
				const void **out_args);

int of_get_named_gpio(struct device_node *np,
                                   const char *propname, int index);

struct device_node *of_find_node_by_phandle(phandle phandle);
void of_print_property(const void *data, int len);

int of_device_is_compatible(const struct device_node *device,
		const char *compat);

int of_machine_is_compatible(const char *compat);

u64 of_translate_address(struct device_node *node, const __be32 *in_addr);

#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1

void of_print_nodes(struct device_node *node, int indent);
int of_probe(void);
int of_parse_dtb(struct fdt_header *fdt);
void of_free(struct device_node *node);
struct device_node *of_unflatten_dtb(struct device_node *root, void *fdt);
struct device_node *of_new_node(struct device_node *parent, const char *name);
struct property *of_new_property(struct device_node *node, const char *name,
		const void *data, int len);
void of_delete_property(struct property *pp);

int of_property_read_string(struct device_node *np, const char *propname,
				const char **out_string);
int of_set_property(struct device_node *node, const char *p, const void *val, int len,
		int create);
struct device_node *of_create_node(struct device_node *root, const char *path);

struct device_node *of_get_root_node(void);
int of_set_root_node(struct device_node *);

#ifdef CONFIG_OFTREE
int of_parse_partitions(const char *cdevname,
			    struct device_node *node);

int of_alias_get_id(struct device_node *np, const char *stem);
int of_device_is_stdout_path(struct device_d *dev);
const char *of_get_model(void);
void *of_flatten_dtb(struct device_node *node);
int of_add_memory(struct device_node *node, bool dump);
#else
static inline int of_parse_partitions(const char *cdevname,
					  struct device_node *node)
{
	return -EINVAL;
}

static inline int of_alias_get_id(struct device_node *np, const char *stem)
{
	return -ENOENT;
}

static inline int of_device_is_stdout_path(struct device_d *dev)
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
#endif

#endif /* __OF_H */

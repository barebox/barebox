#include <common.h>
#include <environment.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <libfdt.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>
#include <boot.h>

#define MAX_LEVEL	32		/* how deeply nested we will go */

static int is_printable_string(const void *data, int len)
{
	const char *s = data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero */
	if (s[len - 1] != '\0')
		return 0;

	/* printable or a null byte (concatenated strings) */
	while (((*s == '\0') || isprint(*s)) && (len > 0)) {
		/*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

/*
 * Print the property in the best format, a heuristic guess.  Print as
 * a string, concatenated strings, a byte, word, double word, or (if all
 * else fails) it is printed as a stream of bytes.
 */
static void print_data(const void *data, int len)
{
	int j;

	/* no data, don't print */
	if (len == 0)
		return;

	/*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
	if (is_printable_string(data, len)) {
		puts("\"");
		j = 0;
		while (j < len) {
			if (j > 0)
				puts("\", \"");
			puts(data);
			j    += strlen(data) + 1;
			data += strlen(data) + 1;
		}
		puts("\"");
		return;
	}

	if ((len % 4) == 0) {
		const u32 *p;

		printf("<");
		for (j = 0, p = data; j < len/4; j ++)
			printf("0x%x%s", fdt32_to_cpu(p[j]), j < (len/4 - 1) ? " " : "");
		printf(">");
	} else { /* anything else... hexdump */
		const u8 *s;

		printf("[");
		for (j = 0, s = data; j < len; j++)
			printf("%02x%s", s[j], j < len - 1 ? " " : "");
		printf("]");
	}
}

static void printf_indent(int level, const char *fmt, ...)
{
	va_list args;

	printf("%*s", level * 8, "");

	va_start (args, fmt);
	vprintf(fmt, args);
	va_end (args);
}

int fdt_print(struct fdt_header *working_fdt, const char *pathp)
{
	const void *nodep;	/* property node pointer */
	int  nodeoffset;	/* node offset from libfdt */
	int  nextoffset;	/* next node offset from libfdt */
	uint32_t tag;		/* tag */
	int  len;		/* length of the property */
	int  level = 0;		/* keep track of nesting level */
	const struct fdt_property *fdt_prop;

	nodeoffset = fdt_path_offset(working_fdt, pathp);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(nodeoffset));
		return 1;
	}

	while (level >= 0) {
		tag = fdt_next_tag(working_fdt, nodeoffset, &nextoffset);
		switch (tag) {
		case FDT_BEGIN_NODE:
			pathp = fdt_get_name(working_fdt, nodeoffset, NULL);
			if (pathp == NULL)
				pathp = "/* NULL pointer error */";
			if (*pathp == '\0')
				pathp = "/";	/* root is nameless */
			printf_indent(level, "%s {\n",pathp);
			level++;
			if (level >= MAX_LEVEL) {
				printf("Nested too deep, aborting.\n");
				return 1;
			}
			break;
		case FDT_END_NODE:
			level--;
			printf_indent(level, "};\n");
			if (level == 0) {
				level = -1;		/* exit the loop */
			}
			break;
		case FDT_PROP:
			fdt_prop = fdt_offset_ptr(working_fdt, nodeoffset,
					sizeof(*fdt_prop));
			pathp    = fdt_string(working_fdt,
					fdt32_to_cpu(fdt_prop->nameoff));
			len      = fdt32_to_cpu(fdt_prop->len);
			nodep    = fdt_prop->data;
			if (len < 0) {
				printf("libfdt fdt_getprop(): %s\n",
					fdt_strerror(len));
				return 1;
			} else if (len == 0) {
				/* the property has no value */
				printf_indent(level, "%s;\n", pathp);
			} else {
				printf_indent(level, "%s = ", pathp);
				print_data(nodep, len);
				printf(";\n");
			}
			break;
		case FDT_NOP:
			printf_indent(level, "/* NOP */\n");
			break;
		case FDT_END:
			return 1;
		default:
			printf("Unknown tag 0x%08X\n", tag);
			return 1;
		}
		nodeoffset = nextoffset;
	}
	return 0;
}

/**
 * fdt_find_and_setprop: Find a node and set it's property
 *
 * @fdt: ptr to device tree
 * @node: path of node
 * @prop: property name
 * @val: ptr to new value
 * @len: length of new property value
 * @create: flag to create the property if it doesn't exist
 *
 * Convenience function to directly set a property given the path to the node.
 */
int fdt_find_and_setprop(struct fdt_header *fdt, const char *node,
		const char *prop, const void *val, int len, int create)
{
	int nodeoff = fdt_path_offset(fdt, node);

	if (nodeoff < 0)
		return nodeoff;

	if ((!create) && (fdt_get_property(fdt, nodeoff, prop, NULL) == NULL))
		return 0; /* create flag not set; so exit quietly */

	return fdt_setprop(fdt, nodeoff, prop, val, len);
}

void do_fixup_by_path(struct fdt_header *fdt, const char *path,
		const char *prop, const void *val, int len, int create)
{
	int rc = fdt_find_and_setprop(fdt, path, prop, val, len, create);
	if (rc)
		printf("Unable to update property %s:%s, err=%s\n",
			path, prop, fdt_strerror(rc));
}

void do_fixup_by_path_u32(struct fdt_header *fdt, const char *path,
		const char *prop, u32 val, int create)
{
	val = cpu_to_fdt32(val);
	do_fixup_by_path(fdt, path, prop, &val, sizeof(val), create);
}

int fdt_get_path_or_create(struct fdt_header *fdt, const char *path)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset (fdt, path);
	if (nodeoffset < 0) {
		nodeoffset = fdt_add_subnode(fdt, 0, path + 1);
		if (nodeoffset < 0) {
			printf("WARNING: could not create %s %s.\n",
					path, fdt_strerror(nodeoffset));
                        return -EINVAL;
                }
        }

	return nodeoffset;
}

struct fdt_header *barebox_fdt;

static int of_fixup_bootargs(struct fdt_header *fdt)
{
	int nodeoffset;
	const char *str;
	int err;

	nodeoffset = fdt_get_path_or_create(fdt, "/chosen");
	if (nodeoffset < 0)
		return nodeoffset;

	str = linux_bootargs_get();
	if (str) {
		err = fdt_setprop(fdt, nodeoffset,
				"bootargs", str, strlen(str)+1);
		if (err < 0)
			printf("WARNING: could not set bootargs %s.\n",
					fdt_strerror(err));
        }

	return 0;
}

static int of_register_bootargs_fixup(void)
{
	return of_register_fixup(of_fixup_bootargs);
}
late_initcall(of_register_bootargs_fixup);

struct of_fixup {
	int (*fixup)(struct fdt_header *);
	struct list_head list;
};

static LIST_HEAD(of_fixup_list);

int of_register_fixup(int (*fixup)(struct fdt_header *))
{
	struct of_fixup *of_fixup = xzalloc(sizeof(*of_fixup));

	of_fixup->fixup = fixup;

	list_add_tail(&of_fixup->list, &of_fixup_list);

	return 0;
}

int of_fix_tree(struct fdt_header *fdt)
{
	struct of_fixup *of_fixup;
	int ret;

	list_for_each_entry(of_fixup, &of_fixup_list, list) {
		ret = of_fixup->fixup(fdt);
		if (ret)
			return ret;
	}

	return 0;
}

struct fdt_header *of_get_fixed_tree(void)
{
	int ret;

	if (!barebox_fdt)
		return NULL;
	ret = of_fix_tree(barebox_fdt);
	if (ret)
		return NULL;
	return barebox_fdt;
}

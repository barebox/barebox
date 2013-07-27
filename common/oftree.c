#include <common.h>
#include <environment.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
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
void of_print_property(const void *data, int len)
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

void of_print_cmdline(struct device_node *root)
{
	struct device_node *node = of_find_node_by_path_from(root, "/chosen");
	const char *cmdline;

	if (!node) {
		printf("commandline: no /chosen node\n");
		return;
	}

	cmdline = of_get_property(node, "bootargs", NULL);

	printf("commandline: %s\n", cmdline);
}

static int of_fixup_bootargs(struct device_node *root)
{
	struct device_node *node;
	const char *str;
	int err;

	str = linux_bootargs_get();
	if (!str)
		return 0;

	node = of_create_node(root, "/chosen");
	if (!node)
		return -ENOMEM;


	err = of_set_property(node, "bootargs", str, strlen(str) + 1, 1);

	return err;
}

static int of_register_bootargs_fixup(void)
{
	return of_register_fixup(of_fixup_bootargs);
}
late_initcall(of_register_bootargs_fixup);

struct of_fixup {
	int (*fixup)(struct device_node *);
	struct list_head list;
};

static LIST_HEAD(of_fixup_list);

int of_register_fixup(int (*fixup)(struct device_node *))
{
	struct of_fixup *of_fixup = xzalloc(sizeof(*of_fixup));

	of_fixup->fixup = fixup;

	list_add_tail(&of_fixup->list, &of_fixup_list);

	return 0;
}

/*
 * Apply registered fixups for the given fdt. The fdt must have
 * enough free space to apply the fixups.
 */
int of_fix_tree(struct device_node *node)
{
	struct of_fixup *of_fixup;
	int ret;

	list_for_each_entry(of_fixup, &of_fixup_list, list) {
		ret = of_fixup->fixup(node);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Get the fixed fdt. This function uses the fdt input pointer
 * if provided or the barebox internal devicetree if not.
 * It increases the size of the tree and applies the registered
 * fixups.
 */
struct fdt_header *of_get_fixed_tree(struct device_node *node)
{
	int ret;
	struct fdt_header *fdt;

	if (!node) {
		node = of_get_root_node();
		if (!node)
			return NULL;
	}

	ret = of_fix_tree(node);
	if (ret)
		return NULL;

	fdt = of_flatten_dtb(node);
	if (!fdt)
		return NULL;

	return fdt;
}

#include <dlfcn.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint64_t qemu_plugin_id_t;
typedef struct qemu_info_t qemu_info_t;
struct qemu_plugin_tb;

int qemu_plugin_version = 7;

extern uint64_t qemu_plugin_tb_vaddr(const struct qemu_plugin_tb *block);
extern void qemu_plugin_register_vcpu_tb_trans_cb(qemu_plugin_id_t id,
	void (*callback)(struct qemu_plugin_tb *, void *), void *userdata);
extern void qemu_plugin_register_vcpu_tb_exec_cb(struct qemu_plugin_tb *block,
	void (*callback)(unsigned int, void *), int flags, void *userdata);

static __thread uint64_t previous_pc;
static unsigned char *coverage;
static FILE *trace;

static void trace_block(unsigned int cpu_index, void *userdata)
{
	uintptr_t pc = (uintptr_t)userdata;
	uint32_t current = (pc ^ (pc >> 4)) & 0xffff;
	uint32_t edge = (uint32_t)previous_pc ^ current;

	(void)cpu_index;
	if (coverage && ++coverage[edge] == 0)
		coverage[edge] = 1;
	previous_pc = current >> 1;
}

static void translate_block(struct qemu_plugin_tb *block, void *userdata)
{
	void *pc = (void *)(uintptr_t)qemu_plugin_tb_vaddr(block);

	(void)userdata;
	if (trace)
		fprintf(trace, "%lx\n", (uintptr_t)pc);
	qemu_plugin_register_vcpu_tb_exec_cb(block, trace_block,
					  0, pc);
}

int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
			char **argv)
{
	(void)info;
	(void)argc;
	(void)argv;
	unsigned char **area = dlsym(RTLD_DEFAULT, "__afl_area_ptr");
	const char *trace_path = getenv("BAREBOX_COVERAGE");

	if (area)
		coverage = *area;
	if (trace_path)
		trace = fopen(trace_path, "a");
	if (!coverage && !trace)
		return -1;
	qemu_plugin_register_vcpu_tb_trans_cb(id, translate_block, NULL);
	return 0;
}

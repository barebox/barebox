// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <asm/stacktrace.h>
#include <asm/unwind.h>
#include <asm/sections.h>

void __aeabi_unwind_cpp_pr0(void);
void __aeabi_unwind_cpp_pr1(void);
void __aeabi_unwind_cpp_pr2(void);

/* Dummy functions to avoid linker complaints */
void __aeabi_unwind_cpp_pr0(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr0);

void __aeabi_unwind_cpp_pr1(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr1);

void __aeabi_unwind_cpp_pr2(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr2);

struct unwind_ctrl_block {
	unsigned long vrs[16];		/* virtual register set */
	const unsigned long *insn;	/* pointer to the current instructions word */
	int entries;			/* number of entries left to interpret */
	int byte;			/* current byte number in the instructions word */
};

enum regs {
	FP = 11,
	SP = 13,
	LR = 14,
	PC = 15
};

#define THREAD_SIZE               8192

extern const struct unwind_idx __start_unwind_idx[];
static const struct unwind_idx *__origin_unwind_idx;
extern const struct unwind_idx __stop_unwind_idx[];

/* Convert a prel31 symbol to an absolute address */
#define prel31_to_addr(ptr)				\
({							\
	/* sign-extend to 32 bits */			\
	long offset = (((long)*(ptr)) << 1) >> 1;	\
	(unsigned long)(ptr) + offset;			\
})

static inline int is_kernel_text(unsigned long addr)
{
	if ((addr >= (unsigned long)_stext && addr <= (unsigned long)_etext))
		return 1;
	return 0;
}

static void dump_backtrace_entry(unsigned long where, unsigned long from,
				 unsigned long frame)
{
#ifdef CONFIG_KALLSYMS
	eprintf("[<%08lx>] (%pS) from [<%08lx>] (%pS)\n", where, (void *)where, from, (void *)from);
#else
	eprintf("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
#endif
}

/*
 * Binary search in the unwind index. The entries are
 * guaranteed to be sorted in ascending order by the linker.
 *
 * start = first entry
 * origin = first entry with positive offset (or stop if there is no such entry)
 * stop - 1 = last entry
 */
static const struct unwind_idx *search_index(unsigned long addr,
					const struct unwind_idx *start,
					const struct unwind_idx *origin,
					const struct unwind_idx *stop)
{
	unsigned long addr_prel31;

	pr_debug("%s(%08lx, %p, %p, %p)\n",
			__func__, addr, start, origin, stop);

	/*
	 * only search in the section with the matching sign. This way the
	 * prel31 numbers can be compared as unsigned longs.
	 */
	if (addr < (unsigned long)start)
		/* negative offsets: [start; origin) */
		stop = origin;
	else
		/* positive offsets: [origin; stop) */
		start = origin;

	/* prel31 for address relavive to start */
	addr_prel31 = (addr - (unsigned long)start) & 0x7fffffff;

	while (start < stop - 1) {
		const struct unwind_idx *mid = start + ((stop - start) >> 1);

		/*
		 * As addr_prel31 is relative to start an offset is needed to
		 * make it relative to mid.
		 */
		if (addr_prel31 - ((unsigned long)mid - (unsigned long)start) <
				mid->addr_offset)
			stop = mid;
		else {
			/* keep addr_prel31 relative to start */
			addr_prel31 -= ((unsigned long)mid -
					(unsigned long)start);
			start = mid;
		}
	}

	if (likely(start->addr_offset <= addr_prel31))
		return start;
	else {
		eprintf("unwind: Unknown symbol address %08lx\n", addr);
		return NULL;
	}
}

static const struct unwind_idx *unwind_find_origin(
		const struct unwind_idx *start, const struct unwind_idx *stop)
{
	pr_debug("%s(%p, %p)\n", __func__, start, stop);
	while (start < stop - 1) {
		const struct unwind_idx *mid = start + ((stop - start) >> 1);

		if (mid->addr_offset >= 0x40000000)
			/* negative offset */
			start = mid;
		else
			/* positive offset */
			stop = mid;
	}

	pr_debug("%s -> %p\n", __func__, stop);
	return stop;
 }

static const struct unwind_idx *unwind_find_idx(unsigned long addr)
{
	const struct unwind_idx *idx = NULL;

	pr_debug("%s(%08lx)\n", __func__, addr);

	if (is_kernel_text(addr)) {
		if (unlikely(!__origin_unwind_idx))
			__origin_unwind_idx =
				unwind_find_origin(__start_unwind_idx,
						__stop_unwind_idx);

		/* main unwind table */
		idx = search_index(addr, __start_unwind_idx,
					__origin_unwind_idx,
					__stop_unwind_idx);
	} else {
		/* module unwinding not supported */
	}

	pr_debug("%s: idx = %p\n", __func__, idx);
	return idx;
}

static unsigned long unwind_get_byte(struct unwind_ctrl_block *ctrl)
{
	unsigned long ret;

	if (ctrl->entries <= 0) {
		eprintf("unwind: Corrupt unwind table\n");
		return 0;
	}

	ret = (*ctrl->insn >> (ctrl->byte * 8)) & 0xff;

	if (ctrl->byte == 0) {
		ctrl->insn++;
		ctrl->entries--;
		ctrl->byte = 3;
	} else
		ctrl->byte--;

	return ret;
}

/*
 * Execute the current unwind instruction.
 */
static int unwind_exec_insn(struct unwind_ctrl_block *ctrl)
{
	unsigned long insn = unwind_get_byte(ctrl);

	pr_debug("%s: insn = %08lx\n", __func__, insn);

	if ((insn & 0xc0) == 0x00)
		ctrl->vrs[SP] += ((insn & 0x3f) << 2) + 4;
	else if ((insn & 0xc0) == 0x40)
		ctrl->vrs[SP] -= ((insn & 0x3f) << 2) + 4;
	else if ((insn & 0xf0) == 0x80) {
		unsigned long mask;
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int load_sp, reg = 4;

		insn = (insn << 8) | unwind_get_byte(ctrl);
		mask = insn & 0x0fff;
		if (mask == 0) {
			eprintf("unwind: 'Refuse to unwind' instruction %04lx\n",
				   insn);
			return -URC_FAILURE;
		}

		/* pop R4-R15 according to mask */
		load_sp = mask & (1 << (13 - 4));
		while (mask) {
			if (mask & 1)
				ctrl->vrs[reg] = *vsp++;
			mask >>= 1;
			reg++;
		}
		if (!load_sp)
			ctrl->vrs[SP] = (unsigned long)vsp;
	} else if ((insn & 0xf0) == 0x90 &&
		   (insn & 0x0d) != 0x0d)
		ctrl->vrs[SP] = ctrl->vrs[insn & 0x0f];
	else if ((insn & 0xf0) == 0xa0) {
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int reg;

		/* pop R4-R[4+bbb] */
		for (reg = 4; reg <= 4 + (insn & 7); reg++)
			ctrl->vrs[reg] = *vsp++;
		if (insn & 0x80)
			ctrl->vrs[14] = *vsp++;
		ctrl->vrs[SP] = (unsigned long)vsp;
	} else if (insn == 0xb0) {
		if (ctrl->vrs[PC] == 0)
			ctrl->vrs[PC] = ctrl->vrs[LR];
		/* no further processing */
		ctrl->entries = 0;
	} else if (insn == 0xb1) {
		unsigned long mask = unwind_get_byte(ctrl);
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int reg = 0;

		if (mask == 0 || mask & 0xf0) {
			eprintf("unwind: Spare encoding %04lx\n",
			       (insn << 8) | mask);
			return -URC_FAILURE;
		}

		/* pop R0-R3 according to mask */
		while (mask) {
			if (mask & 1)
				ctrl->vrs[reg] = *vsp++;
			mask >>= 1;
			reg++;
		}
		ctrl->vrs[SP] = (unsigned long)vsp;
	} else if (insn == 0xb2) {
		unsigned long uleb128 = unwind_get_byte(ctrl);

		ctrl->vrs[SP] += 0x204 + (uleb128 << 2);
	} else {
		eprintf("unwind: Unhandled instruction %02lx\n", insn);
		return -URC_FAILURE;
	}

	pr_debug("%s: fp = %08lx sp = %08lx lr = %08lx pc = %08lx\n", __func__,
		 ctrl->vrs[FP], ctrl->vrs[SP], ctrl->vrs[LR], ctrl->vrs[PC]);

	return URC_OK;
}

/*
 * Unwind a single frame starting with *sp for the symbol at *pc. It
 * updates the *pc and *sp with the new values.
 */
int unwind_frame(struct stackframe *frame)
{
	unsigned long high, low;
	const struct unwind_idx *idx;
	struct unwind_ctrl_block ctrl;

	/* only go to a higher address on the stack */
	low = frame->sp;
	high = ALIGN(low, THREAD_SIZE);

	pr_debug("%s(pc = %08lx lr = %08lx sp = %08lx)\n", __func__,
		 frame->pc, frame->lr, frame->sp);

	if (!is_kernel_text(frame->pc))
		return -URC_FAILURE;

	idx = unwind_find_idx(frame->pc);
	if (!idx) {
		eprintf("unwind: Index not found %08lx\n", frame->pc);
		return -URC_FAILURE;
	}

	ctrl.vrs[FP] = frame->fp;
	ctrl.vrs[SP] = frame->sp;
	ctrl.vrs[LR] = frame->lr;
	ctrl.vrs[PC] = 0;

	if (idx->insn == 1)
		/* can't unwind */
		return -URC_FAILURE;
	else if ((idx->insn & 0x80000000) == 0)
		/* prel31 to the unwind table */
		ctrl.insn = (unsigned long *)prel31_to_addr(&idx->insn);
	else if ((idx->insn & 0xff000000) == 0x80000000)
		/* only personality routine 0 supported in the index */
		ctrl.insn = &idx->insn;
	else {
		eprintf("unwind: Unsupported personality routine %08lx in the index at %p\n",
			   idx->insn, idx);
		return -URC_FAILURE;
	}

	/* check the personality routine */
	if ((*ctrl.insn & 0xff000000) == 0x80000000) {
		ctrl.byte = 2;
		ctrl.entries = 1;
	} else if ((*ctrl.insn & 0xff000000) == 0x81000000) {
		ctrl.byte = 1;
		ctrl.entries = 1 + ((*ctrl.insn & 0x00ff0000) >> 16);
	} else {
		eprintf("unwind: Unsupported personality routine %08lx at %p\n",
			   *ctrl.insn, ctrl.insn);
		return -URC_FAILURE;
	}

	while (ctrl.entries > 0) {
		int urc = unwind_exec_insn(&ctrl);
		if (urc < 0)
			return urc;
		if (ctrl.vrs[SP] < low || ctrl.vrs[SP] >= high)
			return -URC_FAILURE;
	}

	if (ctrl.vrs[PC] == 0)
		ctrl.vrs[PC] = ctrl.vrs[LR];

	/* check for infinite loop */
	if (frame->pc == ctrl.vrs[PC])
		return -URC_FAILURE;

	frame->fp = ctrl.vrs[FP];
	frame->sp = ctrl.vrs[SP];
	frame->lr = ctrl.vrs[LR];
	frame->pc = ctrl.vrs[PC];

	return URC_OK;
}

void unwind_backtrace(struct pt_regs *regs)
{
	struct stackframe frame;
	register unsigned long current_sp asm ("sp");

	pr_debug("%s\n", __func__);

	if (regs) {
		frame.fp = regs->ARM_fp;
		frame.sp = regs->ARM_sp;
		frame.lr = regs->ARM_lr;
		/* PC might be corrupted, use LR in that case. */
		frame.pc = is_kernel_text(regs->ARM_pc)
			 ? regs->ARM_pc : regs->ARM_lr;
	} else {
		frame.sp = current_sp;
		frame.lr = (unsigned long)__builtin_return_address(0);
		frame.pc = (unsigned long)unwind_backtrace;
	}

	while (1) {
		int urc;
		unsigned long where = frame.pc;

		urc = unwind_frame(&frame);
		if (urc < 0)
			break;
		dump_backtrace_entry(where, frame.pc, frame.sp - 4);
	}
}

void dump_stack(void)
{
	unwind_backtrace(NULL);
}

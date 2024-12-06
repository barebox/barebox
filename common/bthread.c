/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 *
 * ASAN bookkeeping based on Qemu coroutine-ucontext.c
 */

/* To avoid future issues; fortify doesn't like longjmp up the call stack */
#ifndef __NO_FORTIFY
#define __NO_FORTIFY
#endif

#include <common.h>
#include <bthread.h>
#include <asm/setjmp.h>
#include <linux/overflow.h>

static struct bthread {
	void (*threadfn)(void *);
	void *data;
	char *name;
	jmp_buf jmp_buf;
	void *stack;
	u32 stack_size;
	struct list_head list;
	u8 awake :1;
	u8 should_stop :1;
	u8 should_clean :1;
	u8 has_stopped :1;
} main_thread = {
	.list = LIST_HEAD_INIT(main_thread.list),
	.name = "main",
	.awake = true,
};

struct bthread *current = &main_thread;

/*
 * When using ASAN, it needs to be told when we switch stacks.
 */
static void bthread_finish_switch_fiber(void *stack_save)
{
	finish_switch_fiber(stack_save,
			    &main_thread.stack, &main_thread.stack_size);
}

static void bthread_start_switch_fiber(void **stack_save)
{
	start_switch_fiber(stack_save,
			   current->stack, current->stack_size);
}

static void __noreturn bthread_trampoline(void)
{
	bthread_finish_switch_fiber(NULL);

	bthread_reschedule();

	current->threadfn(current->data);

	bthread_suspend(current);
	current->has_stopped = true;

	current = &main_thread;
	bthread_start_switch_fiber(NULL);
	longjmp(current->jmp_buf, 1);
}

bool bthread_is_main(struct bthread *bthread)
{
	return bthread == &main_thread;
}

static void bthread_free(struct bthread *bthread)
{
	if (!bthread)
		return;
	free(bthread->stack);
	free(bthread->name);
	free(bthread);
}

const char *bthread_name(struct bthread *bthread)
{
	return bthread->name;
}

struct bthread *bthread_create(void (*threadfn)(void *), void *data,
			       const char *namefmt, ...)
{
	struct bthread *bthread;
	va_list ap;
	int len;

	bthread = calloc(1, sizeof(*bthread));
	if (!bthread)
		goto err;

	bthread->stack = memalign(16, CONFIG_STACK_SIZE);
	if (!bthread->stack)
		goto err;

	bthread->stack_size = CONFIG_STACK_SIZE;
	bthread->threadfn = threadfn;
	bthread->data = data;

	va_start(ap, namefmt);
	len = vasprintf(&bthread->name, namefmt, ap);
	va_end(ap);

	if (len < 0)
		goto err;

	list_add_tail(&bthread->list, &current->list);

	/* set up bthread context with the new stack */
	initjmp(bthread->jmp_buf, bthread_trampoline,
		bthread->stack + CONFIG_STACK_SIZE);

	return bthread;
err:
	bthread_free(bthread);
	return NULL;
}

void *bthread_data(struct bthread *bthread)
{
	return bthread->data;
}

void bthread_wake(struct bthread *bthread)
{
	bthread->awake = true;
}

void bthread_suspend(struct bthread *bthread)
{
	bthread->awake = false;
}

void bthread_cancel(struct bthread *bthread)
{
	bthread->should_stop = true;
	bthread->should_clean = true;
}

void __bthread_stop(struct bthread *bthread)
{
	bthread->should_stop = true;

	pr_debug("waiting for %s to stop\n", bthread->name);

	while (!bthread->has_stopped)
		bthread_reschedule();

	list_del(&bthread->list);
	bthread_free(bthread);
}

int bthread_should_stop(void)
{
	if (bthread_is_main(current))
		return -EINTR;
	bthread_reschedule();
	return current->should_stop;
}

void bthread_info(void)
{
	struct bthread *bthread;

	printf("Registered barebox threads:\n%s\n", current->name);

	list_for_each_entry(bthread, &current->list, list)
		printf("%s\n", bthread->name);
}

void bthread_reschedule(void)
{
	struct bthread *next, *tmp;

	if (current == list_next_entry(current, list))
		return;

	list_for_each_entry_safe(next, tmp, &current->list, list) {
		if (next->awake) {
			pr_debug("switch %s -> %s\n", current->name, next->name);
			bthread_schedule(next);
			return;
		}
		if (next->has_stopped && next->should_clean) {
			pr_debug("deleting %s\n", next->name);
			list_del(&next->list);
			bthread_free(next);
		}
	}
}

void bthread_schedule(struct bthread *to)
{
	struct bthread *from = current;
	void *stack_save = NULL;
	int ret;

	ret = setjmp(from->jmp_buf);
	if (ret == 0) {
		current = to;
		bthread_start_switch_fiber(&stack_save);
		longjmp(to->jmp_buf, 1);
	}

	bthread_finish_switch_fiber(stack_save);
}

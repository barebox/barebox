/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ASAN bookkeeping based on Qemu coroutine-ucontext.c
 */

#ifndef __ASM_GENERIC_SETJMP_H_
#define __ASM_GENERIC_SETJMP_H_

#ifndef finish_switch_fiber
#define finish_switch_fiber finish_switch_fiber
static inline void finish_switch_fiber(void *fake_stack_save,
                                       void **initial_bottom,
                                       unsigned *initial_stack_size)
{
}
#endif

#ifndef start_switch_fiber
#define start_switch_fiber start_switch_fiber
static inline void start_switch_fiber(void **fake_stack_save,
                                      const void *bottom, unsigned size)
{
}

#endif

#endif

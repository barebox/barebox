/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file is included both in arch/sandbox/os along with
 * system headers and outside with barebox headers, so we avoid
 * including any other headers here.
 */

#ifndef __SETJMP_H_
#define __SETJMP_H_

struct jmp_buf_data {
	unsigned char opaque[512] __attribute__((__aligned__(16)));
};

typedef struct jmp_buf_data jmp_buf[1];

int setjmp(jmp_buf jmp) __attribute__((returns_twice));
void longjmp(jmp_buf jmp, int ret) __attribute__((noreturn));

int initjmp(jmp_buf jmp, void __attribute__((noreturn)) (*func)(void), void *stack_top);

#define start_switch_fiber start_switch_fiber
#define finish_switch_fiber finish_switch_fiber

void __sanitizer_start_switch_fiber(void **fake_stack_save,
                                    const void *bottom, size_t size)
    __attribute__((visibility("default")));

void __sanitizer_finish_switch_fiber(void *fake_stack_save,
                                     const void **bottom_old, size_t *size_old)
    __attribute__((visibility("default")));

static inline void finish_switch_fiber(void *fake_stack_save,
                                       void **initial_bottom,
                                       unsigned *initial_stack_size)
{
#ifdef CONFIG_ARCH_HAS_ASAN_FIBER_API
    const void *bottom_old;
    size_t size_old;

    __sanitizer_finish_switch_fiber(fake_stack_save,
                                    &bottom_old, &size_old);

    if (initial_bottom && !*initial_bottom) {
        *initial_bottom = (void *)bottom_old;
        *initial_stack_size = size_old;
    }
#endif
}

static inline void start_switch_fiber(void **fake_stack_save,
                                      const void *bottom, unsigned size)
{
#ifdef CONFIG_ARCH_HAS_ASAN_FIBER_API
    __sanitizer_start_switch_fiber(fake_stack_save, bottom, size);
#endif
}

#endif

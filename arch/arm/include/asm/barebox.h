#ifndef _BAREBOX_H_
#define _BAREBOX_H_	1

#ifdef CONFIG_ARM_UNWIND
#define ARCH_HAS_STACK_DUMP
#endif

#ifdef CONFIG_CPU_V8
#define ARCH_HAS_STACK_DUMP
#endif

#ifdef CONFIG_ARM_EXCEPTIONS
#define ARCH_HAS_DATA_ABORT_MASK
#endif

#endif	/* _BAREBOX_H_ */

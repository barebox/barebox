// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2019 Ahmad Fatoum

void __sanitizer_print_stack_trace(void);

void dump_stack(void);
void dump_stack(void)
{
	__sanitizer_print_stack_trace();
}

// SPDX-License-Identifier: GPL-2.0+

#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <mach/imx/errata.h>

#ifdef CONFIG_CPU_V8

extern char early_imx8m_vectors[];

void erratum_050350_imx8m(void)
{
	void *addr;

	if (current_el() != 3)
		return;

	addr = runtime_address(early_imx8m_vectors);

	asm volatile("msr vbar_el3, %0" : : "r" (addr) : "cc");
	asm volatile("msr daifclr, #4;isb");
}

#endif /* CONFIG_CPU_V8 */

#include <asm/asm.h>

	.section .data
EXPORT(input_data)
	.incbin "arch/mips/pbl/piggy.gzip"
EXPORT(input_data_end)

#include <asm/asm.h>

	.section .data
EXPORT(input_data)
	.incbin	"arch/mips/pbl/piggy.shipped"
EXPORT(input_data_end)

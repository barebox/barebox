#include <asm/asm.h>

	.section .data
EXPORT(input_data)
	.incbin	"arch/mips/pbl/piggy.lz4"
EXPORT(input_data_end)

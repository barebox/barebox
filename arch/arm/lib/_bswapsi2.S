#include <linux/linkage.h>
#include <asm/assembler.h>

ENTRY(__bswapsi2)
#if __LINUX_ARM_ARCH__ <= 5
	eor	r1, r0, r0, ror #16
	bic	r1, r1, #0xff0000
	mov	r0, r0, ror #8
	eor	r0, r0, r1, lsr #8
#else
	rev	r0, r0
#endif
	bx	lr
ENDPROC(__bswapsi2)

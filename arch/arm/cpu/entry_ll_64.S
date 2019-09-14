#include <linux/linkage.h>
#include <asm/sections.h>

/*
 * x0: memory base
 * x1: memory size
 * x2: board data
 * x3: new value for SP
 */
.section .text.__barebox_arm_entry
ENTRY(__barebox_arm_entry)
	mov	sp, x3
	/*
	 * arm_early_mmu_cache_invalidate is jsut a call to
	 * v8_invalidate_icache_all() which doesn't clobber x0, x1 or x2
 	 */
	bl	arm_early_mmu_cache_invalidate
#if IS_ENABLED(CONFIG_PBL_IMAGE)
	b	barebox_pbl_start
#else
	b	barebox_non_pbl_start
#endif
ENDPROC(__barebox_arm_entry)
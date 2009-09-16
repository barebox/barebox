#include <common.h>
#include <init.h>
#include <asm/mmu.h>

static unsigned long *ttb;

void arm_create_section(unsigned long virt, unsigned long phys, int size_m,
		unsigned int flags)
{
	int i;

	phys >>= 20;
	virt >>= 20;

	for (i = size_m; i > 0; i--, virt++, phys++)
		ttb[virt] = (phys << 20) | flags;

	asm volatile (
		"mov r0, #0;"
		"mcr p15, 0, r0, c7, c6, 0;" /* flush d-cache */
		"mcr p15, 0, r0, c8, c7, 0;" /* flush i+d-TLBs */
		:
		:
		: "r0","memory" /* clobber list */
	);
}

/*
 * Prepare MMU for usage and create a flat mapping. Board
 * code is responsible to remap the SDRAM cached
 */
void mmu_init(void)
{
	int i;

	ttb = xzalloc(0x8000);
	ttb = (void *)(((unsigned long)ttb + 0x4000) & ~0x3fff);

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/* create a flat mapping */
	arm_create_section(0, 0, 4096, PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT);
}

/*
 * enable the MMU. Should be called after mmu_init()
 */
void mmu_enable(void)
{
	asm volatile (
		"mrc p15, 0, r1, c1, c0, 0;"
		"orr r1, r1, #0x0007;"  /* enable MMU + Dcache */
		"mcr p15, 0, r1, c1, c0, 0"
		:
		:
		: "r1" /* Clobber list */
        );
}

/*
 * Clean and invalide caches, disable MMU
 */
void mmu_disable(void)
{
	asm volatile (
		"nop; "
		"nop; "
		"nop; "
		"nop; "
		"nop; "
		"nop; "
		/* test, clean and invalidate cache */
		"1:	mrc	p15, 0, r15, c7, c14, 3;" 
		"	bne	1b;"
		"	mov	pc, lr;"
		"	mov r0, #0x0;"
		"	mcr p15, 0, r0, c7, c10, 4;" /* drain the write buffer   */
		"	mcr p15, 0, r1, c7, c6, 0;"  /* clear data cache         */
		"	mrc p15, 0, r1, c1, c0, 0;"
		"	bic r1, r1, #0x0007;"        /* disable MMU + DCache     */
		"	mcr p15, 0, r1, c1, c0, 0;"
		"	mcr p15, 0, r0, c7, c6, 0;" /* flush d-cache             */
		"	mcr p15, 0, r0, c8, c7, 0;" /* flush i+d-TLBs            */
		:
		:
		: "r0" /* Clobber list */
	);
}

/*
 * For boards which need coherent memory for DMA. The idea
 * is simple: Setup a uncached section containing your SDRAM
 * and call setup_dma_coherent() with the offset between the
 * cached and the uncached section. dma_alloc_coherent() then
 * works using normal malloc but returns the corresponding
 * pointer in the uncached area.
 */
static unsigned long dma_coherent_offset;

void setup_dma_coherent(unsigned long offset)
{
	dma_coherent_offset = offset;
}

void *dma_alloc_coherent(size_t size)
{
	void *mem;

	mem = malloc(size);
	if (mem)
		return mem + dma_coherent_offset;

	return NULL; 
}

unsigned long virt_to_phys(void *virt)
{
	return (unsigned long)virt - dma_coherent_offset;
}

void *phys_to_virt(unsigned long phys)
{
	return (void *)(phys + dma_coherent_offset);
}

void dma_free_coherent(void *mem)
{
	free(mem - dma_coherent_offset);
}


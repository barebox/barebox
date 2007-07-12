#include <common.h>
#include <command.h>

/* read co-processor 15, register #1 (control register) */
static unsigned long read_p15_c1 (void)
{
	unsigned long value;

	__asm__ __volatile__(
		"mrc	p15, 0, %0, c1, c0, 0   @ read control reg\n"
		: "=r" (value)
		:
		: "memory");

#ifdef MMU_DEBUG
	printf ("p15/c1 is = %08lx\n", value);
#endif
	return value;
}

/* write to co-processor 15, register #1 (control register) */
static void write_p15_c1 (unsigned long value)
{
#ifdef MMU_DEBUG
	printf ("write %08lx to p15/c1\n", value);
#endif
	__asm__ __volatile__(
		"mcr	p15, 0, %0, c1, c0, 0   @ write it back\n"
		:
		: "r" (value)
		: "memory");

	read_p15_c1 ();
}

static void cp_delay (void)
{
	volatile int i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < 100; i++);
}

#define C1_MMU		(1<<0)		/* mmu off/on */
#define C1_ALIGN	(1<<1)		/* alignment faults off/on */
#define C1_DC		(1<<2)		/* dcache off/on */
#define C1_BIG_ENDIAN	(1<<7)		/* big endian off/on */
#define C1_SYS_PROT	(1<<8)		/* system protection */
#define C1_ROM_PROT	(1<<9)		/* ROM protection */
#define C1_IC		(1<<12)		/* icache off/on */
#define C1_HIGH_VECTORS (1<<13)		/* location of vectors: low/high addresses */

void icache_enable (void)
{
	ulong reg;

	reg = read_p15_c1 ();		/* get control reg. */
	cp_delay ();
	write_p15_c1 (reg | C1_IC);
}

void icache_disable (void)
{
	ulong reg;

	reg = read_p15_c1 ();
	cp_delay ();
	write_p15_c1 (reg & ~C1_IC);
}

int icache_status (void)
{
	return (read_p15_c1 () & C1_IC) != 0;
}

/*
 * this function is called just before we call linux
 * it prepares the processor for linux
 */
int cleanup_before_linux (void)
{
	int i;

	/*
	 * we never enable dcache so we do not need to disable
	 * it. Linux can be called with icache enabled, so just
	 * do nothing here
	 */

	/* flush I/D-cache */
	i = 0;
	asm ("mcr p15, 0, %0, c7, c7, 0": :"r" (i));
	return (0);
}

#ifdef CONFIG_USE_IRQ
static int cpu_init (void)
{
	/*
	 * setup up stacks if necessary
	 */
	IRQ_STACK_START = _u_boot_start - CFG_MALLOC_LEN - CFG_GBL_DATA_SIZE - 4;
	FIQ_STACK_START = IRQ_STACK_START - CONFIG_STACKSIZE_IRQ;
	return 0;
}

core_initcall(cpu_init);
#endif

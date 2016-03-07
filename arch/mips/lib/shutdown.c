/**
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
#include <init.h>
#include <asm/cache.h>

static void arch_shutdown(void)
{
	flush_cache_all();
}
archshutdown_exitcall(arch_shutdown);

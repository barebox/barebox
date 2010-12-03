
#include <common.h>
#include <clock.h>
#include <init.h>
#include <asm/blackfin.h>
#include <asm/cpu/cdef_LPBlackfin.h>

static ulong get_vco(void)
{
	ulong msel;
	ulong vco;

	msel = (*pPLL_CTL >> 9) & 0x3F;
	if (0 == msel)
		msel = 64;

	vco = CONFIG_CLKIN_HZ;
	vco >>= (1 & *pPLL_CTL);	/* DF bit */
	vco = msel * vco;
	return vco;
}

/* Get the Core clock */
ulong get_cclk(void)
{
	ulong csel, ssel;
	if (*pPLL_STAT & 0x1)
		return CONFIG_CLKIN_HZ;

	ssel = *pPLL_DIV;
	csel = ((ssel >> 4) & 0x03);
	ssel &= 0xf;
	if (ssel && ssel < (1 << csel))	/* SCLK > CCLK */
		return get_vco() / ssel;
	return get_vco() >> csel;
}

/* Get the System clock */
ulong get_sclk(void)
{
	ulong ssel;

	if (*pPLL_STAT & 0x1)
		return CONFIG_CLKIN_HZ;

	ssel = (*pPLL_DIV & 0xf);

	return get_vco() / ssel;
}

static uint64_t blackfin_clocksource_read(void)
{
	return ~(*pTCOUNT);
}

static struct clocksource cs = {
	.read	= blackfin_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int clocksource_init (void)
{
	*pTCNTL = 0x1;
	*pTSCALE = 0x0;
	*pTCOUNT  = ~0;
	*pTPERIOD = ~0;
	*pTCNTL = 0x7;
	asm("CSYNC;");

        cs.mult = clocksource_hz2mult(get_cclk(), cs.shift);

        init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);


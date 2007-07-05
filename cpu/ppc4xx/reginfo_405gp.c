#include <common.h>

void reginfo(void)
{
	printf ("\n405GP registers; MSR=%08x\n",mfmsr());
	printf ("\nUniversal Interrupt Controller Regs\n"
	    "uicsr    uicsrs   uicer    uiccr    uicpr    uictr    uicmsr   uicvr    uicvcr"
	    "\n"
	    "%08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
	mfdcr(uicsr),
	mfdcr(uicsrs),
	mfdcr(uicer),
	mfdcr(uiccr),
	mfdcr(uicpr),
	mfdcr(uictr),
	mfdcr(uicmsr),
	mfdcr(uicvr),
	mfdcr(uicvcr));

	puts ("\nMemory (SDRAM) Configuration\n"
	    "besra    besrsa   besrb    besrsb   bear     mcopt1   rtr      pmit\n");

	mtdcr(memcfga,mem_besra); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_besrsa);	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_besrb); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_besrsb); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_bear); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_mcopt1); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_rtr); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_pmit); 	printf ("%08x ", mfdcr(memcfgd));

	puts ("\n"
	    "mb0cf    mb1cf    mb2cf    mb3cf    sdtr1    ecccf    eccerr\n");
	mtdcr(memcfga,mem_mb0cf); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_mb1cf); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_mb2cf); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_mb3cf); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_sdtr1); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_ecccf); 	printf ("%08x ", mfdcr(memcfgd));
	mtdcr(memcfga,mem_eccerr); 	printf ("%08x ", mfdcr(memcfgd));

	printf ("\n\n"
	    "DMA Channels\n"
	    "dmasr    dmasgc   dmaadr\n"
	    "%08x %08x %08x\n"
	    "dmacr_0  dmact_0  dmada_0  dmasa_0  dmasb_0\n"
	    "%08x %08x %08x %08x %08x\n"
	    "dmacr_1  dmact_1  dmada_1  dmasa_1  dmasb_1\n"
	    "%08x %08x %08x %08x %08x\n",
	mfdcr(dmasr),  mfdcr(dmasgc),mfdcr(dmaadr),
	mfdcr(dmacr0), mfdcr(dmact0),mfdcr(dmada0), mfdcr(dmasa0), mfdcr(dmasb0),
	mfdcr(dmacr1), mfdcr(dmact1),mfdcr(dmada1), mfdcr(dmasa1), mfdcr(dmasb1));

	printf (
	    "dmacr_2  dmact_2  dmada_2  dmasa_2  dmasb_2\n"	"%08x %08x %08x %08x %08x\n"
	    "dmacr_3  dmact_3  dmada_3  dmasa_3  dmasb_3\n"	"%08x %08x %08x %08x %08x\n",
	mfdcr(dmacr2), mfdcr(dmact2),mfdcr(dmada2), mfdcr(dmasa2), mfdcr(dmasb2),
	mfdcr(dmacr3), mfdcr(dmact3),mfdcr(dmada3), mfdcr(dmasa3), mfdcr(dmasb3) );

	puts ("\n"
	    "External Bus\n"
	    "pbear    pbesr0   pbesr1   epcr\n");
	mtdcr(ebccfga,pbear); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pbesr0); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pbesr1); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,epcr); 	printf ("%08x ", mfdcr(ebccfgd));

	puts ("\n"
	    "pb0cr    pb0ap    pb1cr    pb1ap    pb2cr    pb2ap    pb3cr    pb3ap\n");
	mtdcr(ebccfga,pb0cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb0ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb1cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb1ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb2cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb2ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb3cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb3ap); 	printf ("%08x ", mfdcr(ebccfgd));

	puts ("\n"
	    "pb4cr    pb4ap    pb5cr    bp5ap    pb6cr    pb6ap    pb7cr    pb7ap\n");
	mtdcr(ebccfga,pb4cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb4ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb5cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb5ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb6cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb6ap); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb7cr); 	printf ("%08x ", mfdcr(ebccfgd));
	mtdcr(ebccfga,pb7ap); 	printf ("%08x ", mfdcr(ebccfgd));

	puts ("\n\n");
}
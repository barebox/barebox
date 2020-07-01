#ifndef __MACH_SAMA5D2_LL__
#define __MACH_SAMA5D2_LL__

#include <mach/sama5d2.h>
#include <mach/at91_pmc_ll.h>
#include <mach/iomux.h>
#include <mach/debug_ll.h>
#include <mach/early_udelay.h>
#include <mach/ddramc.h>

#include <common.h>

void sama5d2_lowlevel_init(void);

static inline void sama5d2_pmc_enable_periph_clock(int clk)
{
	at91_pmc_sam9x5_enable_periph_clock(SAMA5D2_BASE_PMC, clk);
}

/* requires relocation */
static inline void sama5d2_udelay_init(unsigned int msc)
{
	early_udelay_init(SAMA5D2_BASE_PMC, SAMA5D2_BASE_PITC,
			  SAMA5D2_ID_PIT, msc, AT91_PMC_LL_SAMA5D2);
}


void sama5d2_ddr2_init(struct at91_ddramc_register *ddramc_reg_config);

static inline int sama5d2_pmc_enable_generic_clock(unsigned int periph_id,
						   unsigned int clk_source,
						   unsigned int div)
{
	return at91_pmc_enable_generic_clock(SAMA5D2_BASE_PMC,
					     SAMA5D2_BASE_SFR,
					     periph_id, clk_source, div,
					     AT91_PMC_LL_SAMA5D2);
}

static inline int sama5d2_dbgu_setup_ll(unsigned dbgu_id,
					unsigned pin, unsigned periph,
					unsigned mck)
{
	unsigned mask, bank, pio_id;
	void __iomem *dbgu_base, *pio_base;

	mask = pin_to_mask(pin);
	bank = pin_to_bank(pin);

	switch (dbgu_id) {
	case SAMA5D2_ID_UART0:
		dbgu_base = SAMA5D2_BASE_UART0;
		break;
	case SAMA5D2_ID_UART1:
		dbgu_base = SAMA5D2_BASE_UART1;
		break;
	case SAMA5D2_ID_UART2:
		dbgu_base = SAMA5D2_BASE_UART2;
		break;
	case SAMA5D2_ID_UART3:
		dbgu_base = SAMA5D2_BASE_UART3;
		break;
	case SAMA5D2_ID_UART4:
		dbgu_base = SAMA5D2_BASE_UART4;
		break;
	default:
		return -EINVAL;
	}

	pio_base = sama5d2_pio_map_bank(bank, &pio_id);
	if (!pio_base)
		return -EINVAL;

	sama5d2_pmc_enable_periph_clock(pio_id);

	at91_mux_pio4_set_periph(pio_base, mask, periph);

	sama5d2_pmc_enable_periph_clock(dbgu_id);

	at91_dbgu_setup_ll(dbgu_base, mck / 2, CONFIG_BAUDRATE);

	return 0;
}

struct sama5d2_uart_pinmux {
	void __iomem *base;
	u8 id, dtxd, periph;
};

#define SAMA5D2_UART(idx, pio, periph) (struct sama5d2_uart_pinmux) {  \
	SAMA5D2_BASE_UART##idx, SAMA5D2_ID_UART##idx, \
	AT91_PIN_##pio, AT91_MUX_PERIPH_##periph }

static inline void __iomem *sama5d2_resetup_uart_console(unsigned mck)
{
	struct sama5d2_uart_pinmux pinmux;

	/* Table 48-2 I/O Lines and 16.4.4 Boot Configuration Word */

	switch (FIELD_GET(SAMA5D2_BOOTCFG_UART, sama5d2_bootcfg())) {
	case 0: /* UART_1_IOSET_1 */
		pinmux = SAMA5D2_UART(1, PD3,  A);
		break;
	case 1: /* UART_0_IOSET_1 */
		pinmux = SAMA5D2_UART(0, PB27, C);
		break;
	case 2: /* UART_1_IOSET_2 */
		pinmux = SAMA5D2_UART(1, PC8,  E);
		break;
	case 3: /* UART_2_IOSET_1 */
		pinmux = SAMA5D2_UART(2, PD5,  B);
		break;
	case 4: /* UART_2_IOSET_2 */
		pinmux = SAMA5D2_UART(2, PD24, A);
		break;
	case 5: /* UART_2_IOSET_3 */
		pinmux = SAMA5D2_UART(2, PD20, C);
		break;
	case 6: /* UART_3_IOSET_1 */
		pinmux = SAMA5D2_UART(3, PC13, D);
		break;
	case 7: /* UART_3_IOSET_2 */
		pinmux = SAMA5D2_UART(3, PD0,  C);
		break;
	case 8: /* UART_3_IOSET_3 */
		pinmux = SAMA5D2_UART(3, PB12, C);
		break;
	case 9: /* UART_4_IOSET_1 */
		pinmux = SAMA5D2_UART(4, PB4,  A);
		break;
	default:
		return NULL;
	}

	sama5d2_dbgu_setup_ll(pinmux.id, pinmux.dtxd, pinmux.periph, mck);
	return pinmux.base;
}

#endif

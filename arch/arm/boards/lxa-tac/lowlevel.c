// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <debug_ll.h>
#include <mach/stm32mp/entry.h>
#include <mach/stm32mp/stm32.h>
#include <soc/stm32/gpio.h>

extern char __dtb_z_stm32mp157c_lxa_tac_gen1_start[];
extern char __dtb_z_stm32mp157c_lxa_tac_gen2_start[];
extern char __dtb_z_stm32mp153c_lxa_tac_gen3_start[];

/*
 * Major board generation is set via traces in copper
 * Minor board generation can be changed via resistors.
 * The revision is available on GPIOs:
 * [PZ0, PZ1, PZ2, PZ3, PZ6, PZ7]
 */
#define BOARD_GEN(major, minor)	(((major) << 2) | minor)
#define BOARD_GEN1		BOARD_GEN(0, 0)
#define BOARD_GEN2		BOARD_GEN(1, 0)
#define BOARD_GEN3		BOARD_GEN(2, 0)

static const int board_rev_pins[] = {0, 1, 2, 3, 6, 7};

static u32 get_board_rev(void)
{
	u32 board_rev = 0;

	/* Enable GPIOZ bank */
	setbits_le32(STM32MP15_RCC_MP_AHB5ENSETR, BIT(0));

	for (size_t i = 0; i < ARRAY_SIZE(board_rev_pins); i++) {
		int pin = board_rev_pins[i];

		__stm32_pmx_gpio_input(STM32MP15_GPIOZ_BASE, pin);
		board_rev |= __stm32_pmx_gpio_get(STM32MP15_GPIOZ_BASE, pin) << i;
	}

	return board_rev;
}

static void noinline select_fdt_and_start(void *fdt)
{
	putc_ll('>');

	switch (get_board_rev()) {
	case BOARD_GEN1:
		fdt = runtime_address(__dtb_z_stm32mp157c_lxa_tac_gen1_start);
		break;
	case BOARD_GEN2:
		fdt = runtime_address(__dtb_z_stm32mp157c_lxa_tac_gen2_start);
		break;
	case BOARD_GEN3:
		fdt = runtime_address(__dtb_z_stm32mp153c_lxa_tac_gen3_start);
		break;
	}

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_stm32mp15xc_lxa_tac, r0, r1, r2)
{
	stm32mp_cpu_lowlevel_init();

	/*
	 * stm32mp_cpu_lowlevel_init sets up a stack. Do the remaining
	 * init in a non-naked function. Register r2 points to the fdt
	 * from the FIP image which can be used as a default.
	 */
	select_fdt_and_start((void *)r2);
}

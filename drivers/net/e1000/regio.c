#include <common.h>

#include "e1000.h"

static uint32_t e1000_true_offset(struct e1000_hw *hw, uint32_t reg)
{
	if (reg & E1000_MIGHT_BE_REMAPPED) {
		if (hw->mac_type == e1000_igb) {
			switch (reg) {
			case E1000_EEWR:
				reg = E1000_I210_EEWR;
				break;
			case E1000_PHY_CTRL:
				reg = E1000_I210_PHY_CTRL;
				break;
			case E1000_EEMNGCTL:
				reg = E1000_I210_EEMNGCTL;
				break;
			case E1000_FLSWCTL:
				reg = E1000_I210_FLSWCTL;
				break;
			case E1000_FLSWCNT:
				reg = E1000_I210_FLSWCNT;
				break;
			case E1000_FLSWDATA:
				reg = E1000_I210_FLSWDATA;
				break;
			}
		}
		reg &= ~E1000_MIGHT_BE_REMAPPED;
	}

	return reg;
}

void e1000_write_reg(struct e1000_hw *hw, uint32_t reg, uint32_t value)
{
	reg = e1000_true_offset(hw, reg);
	writel(value, hw->hw_addr + reg);
}

uint32_t e1000_read_reg(struct e1000_hw *hw, uint32_t reg)
{
	reg = e1000_true_offset(hw, reg);
	return readl(hw->hw_addr + reg);
}

void e1000_write_reg_array(struct e1000_hw *hw, uint32_t base,
			   uint32_t idx, uint32_t value)
{
	writel(value, hw->hw_addr + base + idx * sizeof(uint32_t));
}

uint32_t e1000_read_reg_array(struct e1000_hw *hw, uint32_t base, uint32_t idx)
{
	return readl(hw->hw_addr + base + idx * sizeof(uint32_t));
}

void e1000_write_flush(struct e1000_hw *hw)
{
	e1000_read_reg(hw, E1000_STATUS);
}

int e1000_poll_reg(struct e1000_hw *hw, uint32_t reg, uint32_t mask,
		   uint32_t value, uint64_t timeout)
{
	const uint64_t start = get_time_ns();

	do {
		const uint32_t v = e1000_read_reg(hw, reg);

		if ((v & mask) == value)
			return 0;

	} while (!is_timeout(start, timeout));

	return -ETIMEDOUT;
}

#include <common.h>

#include "e1000.h"

void e1000_write_reg(struct e1000_hw *hw, uint32_t reg, uint32_t value)
{
	writel(value, hw->hw_addr + reg);
}

uint32_t e1000_read_reg(struct e1000_hw *hw, uint32_t reg)
{
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

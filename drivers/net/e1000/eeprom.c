#include <common.h>
#include <init.h>
#include <malloc.h>

#include "e1000.h"



/******************************************************************************
 * Raises the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code
 * eecd - EECD's current value
 *****************************************************************************/
static void e1000_raise_ee_clk(struct e1000_hw *hw, uint32_t *eecd)
{
	/* Raise the clock input to the EEPROM (by setting the SK bit), and then
	 * wait 50 microseconds.
	 */
	*eecd = *eecd | E1000_EECD_SK;
	e1000_write_reg(hw, E1000_EECD, *eecd);
	e1000_write_flush(hw);
	udelay(50);
}

/******************************************************************************
 * Lowers the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code
 * eecd - EECD's current value
 *****************************************************************************/
static void e1000_lower_ee_clk(struct e1000_hw *hw, uint32_t *eecd)
{
	/* Lower the clock input to the EEPROM (by clearing the SK bit), and then
	 * wait 50 microseconds.
	 */
	*eecd = *eecd & ~E1000_EECD_SK;
	e1000_write_reg(hw, E1000_EECD, *eecd);
	e1000_write_flush(hw);
	udelay(50);
}

/******************************************************************************
 * Shift data bits out to the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * data - data to send to the EEPROM
 * count - number of bits to shift out
 *****************************************************************************/
static void e1000_shift_out_ee_bits(struct e1000_hw *hw, uint16_t data, uint16_t count)
{
	uint32_t eecd;
	uint32_t mask;

	/* We need to shift "count" bits out to the EEPROM. So, value in the
	 * "data" parameter will be shifted out to the EEPROM one bit at a time.
	 * In order to do this, "data" must be broken down into bits.
	 */
	mask = 0x01 << (count - 1);
	eecd = e1000_read_reg(hw, E1000_EECD);
	eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
	do {
		/* A "1" is shifted out to the EEPROM by setting bit "DI" to a "1",
		 * and then raising and then lowering the clock (the SK bit controls
		 * the clock input to the EEPROM).  A "0" is shifted out to the EEPROM
		 * by setting "DI" to "0" and then raising and then lowering the clock.
		 */
		eecd &= ~E1000_EECD_DI;

		if (data & mask)
			eecd |= E1000_EECD_DI;

		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);

		udelay(50);

		e1000_raise_ee_clk(hw, &eecd);
		e1000_lower_ee_clk(hw, &eecd);

		mask = mask >> 1;

	} while (mask);

	/* We leave the "DI" bit set to "0" when we leave this routine. */
	eecd &= ~E1000_EECD_DI;
	e1000_write_reg(hw, E1000_EECD, eecd);
}

/******************************************************************************
 * Shift data bits in from the EEPROM
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static uint16_t e1000_shift_in_ee_bits(struct e1000_hw *hw, uint16_t count)
{
	uint32_t eecd;
	uint32_t i;
	uint16_t data;

	/* In order to read a register from the EEPROM, we need to shift 'count'
	 * bits in from the EEPROM. Bits are "shifted in" by raising the clock
	 * input to the EEPROM (setting the SK bit), and then reading the
	 * value of the "DO" bit.  During this "shifting in" process the
	 * "DI" bit should always be clear.
	 */

	eecd = e1000_read_reg(hw, E1000_EECD);

	eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
	data = 0;

	for (i = 0; i < count; i++) {
		data = data << 1;
		e1000_raise_ee_clk(hw, &eecd);

		eecd = e1000_read_reg(hw, E1000_EECD);

		eecd &= ~(E1000_EECD_DI);
		if (eecd & E1000_EECD_DO)
			data |= 1;

		e1000_lower_ee_clk(hw, &eecd);
	}

	return data;
}

/******************************************************************************
 * Returns EEPROM to a "standby" state
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void e1000_standby_eeprom(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd;

	eecd = e1000_read_reg(hw, E1000_EECD);

	if (eeprom->type == e1000_eeprom_microwire) {
		eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);

		/* Clock high */
		eecd |= E1000_EECD_SK;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);

		/* Select EEPROM */
		eecd |= E1000_EECD_CS;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);

		/* Clock low */
		eecd &= ~E1000_EECD_SK;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);
	} else if (eeprom->type == e1000_eeprom_spi) {
		/* Toggle CS to flush commands */
		eecd |= E1000_EECD_CS;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);
		eecd &= ~E1000_EECD_CS;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(eeprom->delay_usec);
	}
}

/***************************************************************************
* Description:     Determines if the onboard NVM is FLASH or EEPROM.
*
* hw - Struct containing variables accessed by shared code
****************************************************************************/
static bool e1000_is_onboard_nvm_eeprom(struct e1000_hw *hw)
{
	uint32_t eecd = 0;

	DEBUGFUNC();

	if (hw->mac_type == e1000_ich8lan)
		return false;

	if (hw->mac_type == e1000_82573 || hw->mac_type == e1000_82574) {
		eecd = e1000_read_reg(hw, E1000_EECD);

		/* Isolate bits 15 & 16 */
		eecd = ((eecd >> 15) & 0x03);

		/* If both bits are set, device is Flash type */
		if (eecd == 0x03)
			return false;
	}
	return true;
}

/******************************************************************************
 * Prepares EEPROM for access
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Lowers EEPROM clock. Clears input pin. Sets the chip select pin. This
 * function should be called before issuing a command to the EEPROM.
 *****************************************************************************/
static int32_t e1000_acquire_eeprom(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd, i = 0;

	DEBUGFUNC();

	if (e1000_swfw_sync_acquire(hw, E1000_SWFW_EEP_SM))
		return -E1000_ERR_SWFW_SYNC;
	eecd = e1000_read_reg(hw, E1000_EECD);

	/* Request EEPROM Access */
	if (hw->mac_type > e1000_82544 && hw->mac_type != e1000_82573 &&
			hw->mac_type != e1000_82574) {
		eecd |= E1000_EECD_REQ;
		e1000_write_reg(hw, E1000_EECD, eecd);
		eecd = e1000_read_reg(hw, E1000_EECD);
		while ((!(eecd & E1000_EECD_GNT)) &&
			(i < E1000_EEPROM_GRANT_ATTEMPTS)) {
			i++;
			udelay(5);
			eecd = e1000_read_reg(hw, E1000_EECD);
		}
		if (!(eecd & E1000_EECD_GNT)) {
			eecd &= ~E1000_EECD_REQ;
			e1000_write_reg(hw, E1000_EECD, eecd);
			dev_dbg(hw->dev, "Could not acquire EEPROM grant\n");
			return -E1000_ERR_EEPROM;
		}
	}

	/* Setup EEPROM for Read/Write */

	if (eeprom->type == e1000_eeprom_microwire) {
		/* Clear SK and DI */
		eecd &= ~(E1000_EECD_DI | E1000_EECD_SK);
		e1000_write_reg(hw, E1000_EECD, eecd);

		/* Set CS */
		eecd |= E1000_EECD_CS;
		e1000_write_reg(hw, E1000_EECD, eecd);
	} else if (eeprom->type == e1000_eeprom_spi) {
		/* Clear SK and CS */
		eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
		e1000_write_reg(hw, E1000_EECD, eecd);
		udelay(1);
	}

	return E1000_SUCCESS;
}

/******************************************************************************
 * Sets up eeprom variables in the hw struct.  Must be called after mac_type
 * is configured.  Additionally, if this is ICH8, the flash controller GbE
 * registers must be mapped, or this will crash.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t e1000_init_eeprom_params(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd;
	int32_t ret_val = E1000_SUCCESS;
	uint16_t eeprom_size;

	if (hw->mac_type == e1000_igb)
		eecd = e1000_read_reg(hw, E1000_I210_EECD);
	else
		eecd = e1000_read_reg(hw, E1000_EECD);

	DEBUGFUNC();

	switch (hw->mac_type) {
	case e1000_82542_rev2_0:
	case e1000_82542_rev2_1:
	case e1000_82543:
	case e1000_82544:
		eeprom->type = e1000_eeprom_microwire;
		eeprom->word_size = 64;
		eeprom->opcode_bits = 3;
		eeprom->address_bits = 6;
		eeprom->delay_usec = 50;
		eeprom->use_eerd = false;
		eeprom->use_eewr = false;
	break;
	case e1000_82540:
	case e1000_82545:
	case e1000_82545_rev_3:
	case e1000_82546:
	case e1000_82546_rev_3:
		eeprom->type = e1000_eeprom_microwire;
		eeprom->opcode_bits = 3;
		eeprom->delay_usec = 50;
		if (eecd & E1000_EECD_SIZE) {
			eeprom->word_size = 256;
			eeprom->address_bits = 8;
		} else {
			eeprom->word_size = 64;
			eeprom->address_bits = 6;
		}
		eeprom->use_eerd = false;
		eeprom->use_eewr = false;
		break;
	case e1000_82541:
	case e1000_82541_rev_2:
	case e1000_82547:
	case e1000_82547_rev_2:
		if (eecd & E1000_EECD_TYPE) {
			eeprom->type = e1000_eeprom_spi;
			eeprom->opcode_bits = 8;
			eeprom->delay_usec = 1;
			if (eecd & E1000_EECD_ADDR_BITS) {
				eeprom->page_size = 32;
				eeprom->address_bits = 16;
			} else {
				eeprom->page_size = 8;
				eeprom->address_bits = 8;
			}
		} else {
			eeprom->type = e1000_eeprom_microwire;
			eeprom->opcode_bits = 3;
			eeprom->delay_usec = 50;
			if (eecd & E1000_EECD_ADDR_BITS) {
				eeprom->word_size = 256;
				eeprom->address_bits = 8;
			} else {
				eeprom->word_size = 64;
				eeprom->address_bits = 6;
			}
		}
		eeprom->use_eerd = false;
		eeprom->use_eewr = false;
		break;
	case e1000_82571:
	case e1000_82572:
		eeprom->type = e1000_eeprom_spi;
		eeprom->opcode_bits = 8;
		eeprom->delay_usec = 1;
		if (eecd & E1000_EECD_ADDR_BITS) {
			eeprom->page_size = 32;
			eeprom->address_bits = 16;
		} else {
			eeprom->page_size = 8;
			eeprom->address_bits = 8;
		}
		eeprom->use_eerd = false;
		eeprom->use_eewr = false;
		break;
	case e1000_82573:
	case e1000_82574:
		eeprom->type = e1000_eeprom_spi;
		eeprom->opcode_bits = 8;
		eeprom->delay_usec = 1;
		if (eecd & E1000_EECD_ADDR_BITS) {
			eeprom->page_size = 32;
			eeprom->address_bits = 16;
		} else {
			eeprom->page_size = 8;
			eeprom->address_bits = 8;
		}
		if (e1000_is_onboard_nvm_eeprom(hw) == false) {
			eeprom->use_eerd = true;
			eeprom->use_eewr = true;

			eeprom->type = e1000_eeprom_flash;
			eeprom->word_size = 2048;

		/* Ensure that the Autonomous FLASH update bit is cleared due to
		 * Flash update issue on parts which use a FLASH for NVM. */
			eecd &= ~E1000_EECD_AUPDEN;
			e1000_write_reg(hw, E1000_EECD, eecd);
		}
		break;
	case e1000_80003es2lan:
		eeprom->type = e1000_eeprom_spi;
		eeprom->opcode_bits = 8;
		eeprom->delay_usec = 1;
		if (eecd & E1000_EECD_ADDR_BITS) {
			eeprom->page_size = 32;
			eeprom->address_bits = 16;
		} else {
			eeprom->page_size = 8;
			eeprom->address_bits = 8;
		}
		eeprom->use_eerd = true;
		eeprom->use_eewr = false;
		break;
	case e1000_igb:
		/* i210 has 4k of iNVM mapped as EEPROM */
		eeprom->type = e1000_eeprom_invm;
		eeprom->opcode_bits = 8;
		eeprom->delay_usec = 1;
		eeprom->page_size = 32;
		eeprom->address_bits = 16;
		eeprom->use_eerd = true;
		eeprom->use_eewr = false;
		break;
	default:
		break;
	}

	if (eeprom->type == e1000_eeprom_spi ||
	    eeprom->type == e1000_eeprom_invm) {
		/* eeprom_size will be an enum [0..8] that maps
		 * to eeprom sizes 128B to
		 * 32KB (incremented by powers of 2).
		 */
		if (hw->mac_type <= e1000_82547_rev_2) {
			/* Set to default value for initial eeprom read. */
			eeprom->word_size = 64;
			ret_val = e1000_read_eeprom(hw, EEPROM_CFG, 1,
					&eeprom_size);
			if (ret_val)
				return ret_val;
			eeprom_size = (eeprom_size & EEPROM_SIZE_MASK)
				>> EEPROM_SIZE_SHIFT;
			/* 256B eeprom size was not supported in earlier
			 * hardware, so we bump eeprom_size up one to
			 * ensure that "1" (which maps to 256B) is never
			 * the result used in the shifting logic below. */
			if (eeprom_size)
				eeprom_size++;
		} else {
			eeprom_size = (uint16_t)((eecd &
				E1000_EECD_SIZE_EX_MASK) >>
				E1000_EECD_SIZE_EX_SHIFT);
		}

		eeprom->word_size = 1 << (eeprom_size + EEPROM_WORD_SIZE_SHIFT);
	}
	return ret_val;
}

/******************************************************************************
 * Polls the status bit (bit 1) of the EERD to determine when the read is done.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static int32_t e1000_poll_eerd_eewr_done(struct e1000_hw *hw, int eerd)
{
	uint32_t attempts = 100000;
	uint32_t i, reg = 0;
	int32_t done = E1000_ERR_EEPROM;

	for (i = 0; i < attempts; i++) {
		if (eerd == E1000_EEPROM_POLL_READ) {
			if (hw->mac_type == e1000_igb)
				reg = e1000_read_reg(hw, E1000_I210_EERD);
			else
				reg = e1000_read_reg(hw, E1000_EERD);
		} else {
			if (hw->mac_type == e1000_igb)
				reg = e1000_read_reg(hw, E1000_I210_EEWR);
			else
				reg = e1000_read_reg(hw, E1000_EEWR);
		}

		if (reg & E1000_EEPROM_RW_REG_DONE) {
			done = E1000_SUCCESS;
			break;
		}
		udelay(5);
	}

	return done;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM using the EERD register.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
static int32_t e1000_read_eeprom_eerd(struct e1000_hw *hw,
			uint16_t offset,
			uint16_t words,
			uint16_t *data)
{
	uint32_t i, eerd = 0;
	int32_t error = 0;

	for (i = 0; i < words; i++) {
		eerd = ((offset+i) << E1000_EEPROM_RW_ADDR_SHIFT) +
			E1000_EEPROM_RW_REG_START;

		if (hw->mac_type == e1000_igb)
			e1000_write_reg(hw, E1000_I210_EERD, eerd);
		else
			e1000_write_reg(hw, E1000_EERD, eerd);

		error = e1000_poll_eerd_eewr_done(hw, E1000_EEPROM_POLL_READ);

		if (error)
			break;

		if (hw->mac_type == e1000_igb) {
			data[i] = (e1000_read_reg(hw, E1000_I210_EERD) >>
				E1000_EEPROM_RW_REG_DATA);
		} else {
			data[i] = (e1000_read_reg(hw, E1000_EERD) >>
				E1000_EEPROM_RW_REG_DATA);
		}

	}

	return error;
}

static void e1000_release_eeprom(struct e1000_hw *hw)
{
	uint32_t eecd;

	DEBUGFUNC();

	eecd = e1000_read_reg(hw, E1000_EECD);

	if (hw->eeprom.type == e1000_eeprom_spi) {
		eecd |= E1000_EECD_CS;  /* Pull CS high */
		eecd &= ~E1000_EECD_SK; /* Lower SCK */

		e1000_write_reg(hw, E1000_EECD, eecd);

		udelay(hw->eeprom.delay_usec);
	} else if (hw->eeprom.type == e1000_eeprom_microwire) {
		/* cleanup eeprom */

		/* CS on Microwire is active-high */
		eecd &= ~(E1000_EECD_CS | E1000_EECD_DI);

		e1000_write_reg(hw, E1000_EECD, eecd);

		/* Rising edge of clock */
		eecd |= E1000_EECD_SK;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(hw->eeprom.delay_usec);

		/* Falling edge of clock */
		eecd &= ~E1000_EECD_SK;
		e1000_write_reg(hw, E1000_EECD, eecd);
		e1000_write_flush(hw);
		udelay(hw->eeprom.delay_usec);
	}

	/* Stop requesting EEPROM access */
	if (hw->mac_type > e1000_82544) {
		eecd &= ~E1000_EECD_REQ;
		e1000_write_reg(hw, E1000_EECD, eecd);
	}
}
/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static int32_t e1000_spi_eeprom_ready(struct e1000_hw *hw)
{
	uint16_t retry_count = 0;
	uint8_t spi_stat_reg;

	DEBUGFUNC();

	/* Read "Status Register" repeatedly until the LSB is cleared.  The
	 * EEPROM will signal that the command has been completed by clearing
	 * bit 0 of the internal status register.  If it's not cleared within
	 * 5 milliseconds, then error out.
	 */
	retry_count = 0;
	do {
		e1000_shift_out_ee_bits(hw, EEPROM_RDSR_OPCODE_SPI,
			hw->eeprom.opcode_bits);
		spi_stat_reg = (uint8_t)e1000_shift_in_ee_bits(hw, 8);
		if (!(spi_stat_reg & EEPROM_STATUS_RDY_SPI))
			break;

		udelay(5);
		retry_count += 5;

		e1000_standby_eeprom(hw);
	} while (retry_count < EEPROM_MAX_RETRY_SPI);

	/* ATMEL SPI write time could vary from 0-20mSec on 3.3V devices (and
	 * only 0-5mSec on 5V devices)
	 */
	if (retry_count >= EEPROM_MAX_RETRY_SPI) {
		dev_dbg(hw->dev, "SPI EEPROM Status error\n");
		return -E1000_ERR_EEPROM;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 *****************************************************************************/
int32_t e1000_read_eeprom(struct e1000_hw *hw, uint16_t offset,
			  uint16_t words, uint16_t *data)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t i = 0;

	DEBUGFUNC();

	/* If eeprom is not yet detected, do so now */
	if (eeprom->word_size == 0)
		e1000_init_eeprom_params(hw);

	/* A check for invalid values:  offset too large, too many words,
	 * and not enough words.
	 */
	if ((offset >= eeprom->word_size) ||
		(words > eeprom->word_size - offset) ||
		(words == 0)) {
		dev_dbg(hw->dev, "\"words\" parameter out of bounds."
			"Words = %d, size = %d\n", offset, eeprom->word_size);
		return -E1000_ERR_EEPROM;
	}

	/* EEPROM's that don't use EERD to read require us to bit-bang the SPI
	 * directly. In this case, we need to acquire the EEPROM so that
	 * FW or other port software does not interrupt.
	 */
	if (e1000_is_onboard_nvm_eeprom(hw) == true &&
		hw->eeprom.use_eerd == false) {

		/* Prepare the EEPROM for bit-bang reading */
		if (e1000_acquire_eeprom(hw) != E1000_SUCCESS)
			return -E1000_ERR_EEPROM;
	}

	/* Eerd register EEPROM access requires no eeprom aquire/release */
	if (eeprom->use_eerd == true)
		return e1000_read_eeprom_eerd(hw, offset, words, data);

	/* Set up the SPI or Microwire EEPROM for bit-bang reading.  We have
	 * acquired the EEPROM at this point, so any returns should relase it */
	if (eeprom->type == e1000_eeprom_spi) {
		uint16_t word_in;
		uint8_t read_opcode = EEPROM_READ_OPCODE_SPI;

		if (e1000_spi_eeprom_ready(hw)) {
			e1000_release_eeprom(hw);
			return -E1000_ERR_EEPROM;
		}

		e1000_standby_eeprom(hw);

		/* Some SPI eeproms use the 8th address bit embedded in
		 * the opcode */
		if ((eeprom->address_bits == 8) && (offset >= 128))
			read_opcode |= EEPROM_A8_OPCODE_SPI;

		/* Send the READ command (opcode + addr)  */
		e1000_shift_out_ee_bits(hw, read_opcode, eeprom->opcode_bits);
		e1000_shift_out_ee_bits(hw, (uint16_t)(offset*2),
				eeprom->address_bits);

		/* Read the data.  The address of the eeprom internally
		 * increments with each byte (spi) being read, saving on the
		 * overhead of eeprom setup and tear-down.  The address
		 * counter will roll over if reading beyond the size of
		 * the eeprom, thus allowing the entire memory to be read
		 * starting from any offset. */
		for (i = 0; i < words; i++) {
			word_in = e1000_shift_in_ee_bits(hw, 16);
			data[i] = (word_in >> 8) | (word_in << 8);
		}
	} else if (eeprom->type == e1000_eeprom_microwire) {
		for (i = 0; i < words; i++) {
			/* Send the READ command (opcode + addr)  */
			e1000_shift_out_ee_bits(hw,
				EEPROM_READ_OPCODE_MICROWIRE,
				eeprom->opcode_bits);
			e1000_shift_out_ee_bits(hw, (uint16_t)(offset + i),
				eeprom->address_bits);

			/* Read the data.  For microwire, each word requires
			 * the overhead of eeprom setup and tear-down. */
			data[i] = e1000_shift_in_ee_bits(hw, 16);
			e1000_standby_eeprom(hw);
		}
	}

	/* End this read operation */
	e1000_release_eeprom(hw);

	return E1000_SUCCESS;
}

/******************************************************************************
 * Verifies that the EEPROM has a valid checksum
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Reads the first 64 16 bit words of the EEPROM and sums the values read.
 * If the the sum of the 64 16 bit words is 0xBABA, the EEPROM's checksum is
 * valid.
 *****************************************************************************/
int e1000_validate_eeprom_checksum(struct e1000_hw *hw)
{
	uint16_t i, checksum, checksum_reg;
	uint16_t buf[EEPROM_CHECKSUM_REG + 1];

	DEBUGFUNC();

	/* Read the EEPROM */
	if (e1000_read_eeprom(hw, 0, EEPROM_CHECKSUM_REG + 1, buf) < 0) {
		dev_err(&hw->edev.dev, "Unable to read EEPROM!\n");
		return -E1000_ERR_EEPROM;
	}

	/* Compute the checksum */
	checksum = 0;
	for (i = 0; i < EEPROM_CHECKSUM_REG; i++)
		checksum += buf[i];
	checksum = ((uint16_t)EEPROM_SUM) - checksum;
	checksum_reg = buf[i];

	/* Verify it! */
	if (checksum == checksum_reg)
		return 0;

	/* Hrm, verification failed, print an error */
	dev_err(&hw->edev.dev, "EEPROM checksum is incorrect!\n");
	dev_err(&hw->edev.dev, "  ...register was 0x%04hx, calculated 0x%04hx\n",
			checksum_reg, checksum);

	return -E1000_ERR_EEPROM;
}

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <linux/math64.h>
#include <linux/sizes.h>
#include <linux/mtd/spi-nor.h>

#include "e1000.h"

static int32_t e1000_read_eeprom_spi(struct e1000_hw *hw, uint16_t offset,
				     uint16_t words, uint16_t *data);
static int32_t e1000_read_eeprom_microwire(struct e1000_hw *hw, uint16_t offset,
					   uint16_t words, uint16_t *data);

static int32_t e1000_read_eeprom_eerd(struct e1000_hw *hw, uint16_t offset,
				      uint16_t words, uint16_t *data);
static int32_t e1000_spi_eeprom_ready(struct e1000_hw *hw);
static void e1000_release_eeprom_flash(struct e1000_hw *hw);


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

static int32_t
e1000_acquire_eeprom_spi_microwire_prologue(struct e1000_hw *hw)
{
	uint32_t eecd;

	if (e1000_swfw_sync_acquire(hw, E1000_SWFW_EEP_SM))
		return -E1000_ERR_SWFW_SYNC;

	eecd = e1000_read_reg(hw, E1000_EECD);

	/* Request EEPROM Access */
	if (hw->mac_type > e1000_82544  &&
	    hw->mac_type != e1000_82573 &&
	    hw->mac_type != e1000_82574) {
		int i = 0;

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

	return E1000_SUCCESS;
}

static void
e1000_release_eeprom_spi_microwire_epilogue(struct e1000_hw *hw)
{
	uint32_t eecd = e1000_read_reg(hw, E1000_EECD);

	/* Stop requesting EEPROM access */
	if (hw->mac_type > e1000_82544) {
		eecd &= ~E1000_EECD_REQ;
		e1000_write_reg(hw, E1000_EECD, eecd);
	}
}


static int32_t e1000_acquire_eeprom_spi(struct e1000_hw *hw)
{
	int32_t ret;
	uint32_t eecd;

	ret = e1000_acquire_eeprom_spi_microwire_prologue(hw);
	if (ret != E1000_SUCCESS)
		return ret;

	eecd = e1000_read_reg(hw, E1000_EECD);

	/* Clear SK and CS */
	eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
	e1000_write_reg(hw, E1000_EECD, eecd);
	udelay(1);

	return E1000_SUCCESS;
}

static void e1000_release_eeprom_spi(struct e1000_hw *hw)
{
	uint32_t eecd = e1000_read_reg(hw, E1000_EECD);

	eecd |= E1000_EECD_CS;  /* Pull CS high */
	eecd &= ~E1000_EECD_SK; /* Lower SCK */

	e1000_write_reg(hw, E1000_EECD, eecd);
	udelay(hw->eeprom.delay_usec);

	e1000_release_eeprom_spi_microwire_epilogue(hw);
}

static int32_t e1000_acquire_eeprom_microwire(struct e1000_hw *hw)
{
	int ret;
	uint32_t eecd;

	ret = e1000_acquire_eeprom_spi_microwire_prologue(hw);
	if (ret != E1000_SUCCESS)
		return ret;

	eecd = e1000_read_reg(hw, E1000_EECD);
	/* Clear SK and DI */
	eecd &= ~(E1000_EECD_DI | E1000_EECD_SK);
	e1000_write_reg(hw, E1000_EECD, eecd);

	/* Set CS */
	eecd |= E1000_EECD_CS;
	e1000_write_reg(hw, E1000_EECD, eecd);

	return E1000_SUCCESS;
}

static void e1000_release_eeprom_microwire(struct e1000_hw *hw)
{
	uint32_t eecd = e1000_read_reg(hw, E1000_EECD);

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


	e1000_release_eeprom_spi_microwire_epilogue(hw);
}

static int32_t e1000_acquire_eeprom_flash(struct e1000_hw *hw)
{
	return e1000_swfw_sync_acquire(hw, E1000_SWFW_EEP_SM);
}

static void e1000_release_eeprom_flash(struct e1000_hw *hw)
{
	if (e1000_swfw_sync_release(hw, E1000_SWFW_EEP_SM) < 0)
		dev_warn(hw->dev,
			 "Timeout while releasing SWFW_SYNC bits (0x%08x)\n",
			 E1000_SWFW_EEP_SM);
}

static int32_t e1000_acquire_eeprom(struct e1000_hw *hw)
{
	if (hw->eeprom.acquire)
		return hw->eeprom.acquire(hw);
	else
		return E1000_SUCCESS;
}

static void e1000_release_eeprom(struct e1000_hw *hw)
{
	if (hw->eeprom.release)
		hw->eeprom.release(hw);
}

static void e1000_eeprom_uses_spi(struct e1000_eeprom_info *eeprom,
				  uint32_t eecd)
{
	eeprom->type = e1000_eeprom_spi;
	eeprom->opcode_bits = 8;
	eeprom->delay_usec = 1;
	if (eecd & E1000_EECD_ADDR_BITS) {
		eeprom->address_bits = 16;
	} else {
		eeprom->address_bits = 8;
	}

	eeprom->acquire = e1000_acquire_eeprom_spi;
	eeprom->release = e1000_release_eeprom_spi;
	eeprom->read = e1000_read_eeprom_spi;
}

static void e1000_eeprom_uses_microwire(struct e1000_eeprom_info *eeprom,
					uint32_t eecd)
{
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

	eeprom->acquire = e1000_acquire_eeprom_microwire;
	eeprom->release = e1000_release_eeprom_microwire;
	eeprom->read = e1000_read_eeprom_microwire;
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

	eecd = e1000_read_reg(hw, E1000_EECD);

	DEBUGFUNC();

	switch (hw->mac_type) {
	case e1000_82542_rev2_0:
	case e1000_82542_rev2_1:
	case e1000_82543:
	case e1000_82544:
		e1000_eeprom_uses_microwire(eeprom, 0);
		break;

	case e1000_82540:
	case e1000_82545:
	case e1000_82545_rev_3:
	case e1000_82546:
	case e1000_82546_rev_3:
		e1000_eeprom_uses_microwire(eeprom, eecd);
		break;

	case e1000_82541:
	case e1000_82541_rev_2:
	case e1000_82547:
	case e1000_82547_rev_2:
		if (eecd & E1000_EECD_TYPE)
			e1000_eeprom_uses_spi(eeprom, eecd);
		else
			e1000_eeprom_uses_microwire(eeprom, eecd);
		break;

	case e1000_82571:
	case e1000_82572:
		e1000_eeprom_uses_spi(eeprom, eecd);
		break;

	case e1000_82573:
	case e1000_82574:
		if (e1000_is_onboard_nvm_eeprom(hw)) {
			e1000_eeprom_uses_spi(eeprom, eecd);
		} else {
			eeprom->read = e1000_read_eeprom_eerd;
			eeprom->type = e1000_eeprom_flash;
			eeprom->word_size = 2048;

			/*
			 * Ensure that the Autonomous FLASH update bit is cleared due to
			 * Flash update issue on parts which use a FLASH for NVM.
			 */
			eecd &= ~E1000_EECD_AUPDEN;
			e1000_write_reg(hw, E1000_EECD, eecd);
		}
		break;

	case e1000_80003es2lan:
		eeprom->type = e1000_eeprom_spi;
		eeprom->read = e1000_read_eeprom_eerd;
		break;

	case e1000_igb:
		if (eecd & E1000_EECD_I210_FLASH_DETECTED) {
			uint32_t fla;

			fla  = e1000_read_reg(hw, E1000_FLA);
			fla &= E1000_FLA_FL_SIZE_MASK;
			fla >>= E1000_FLA_FL_SIZE_SHIFT;

			if (fla) {
				eeprom->word_size = (SZ_64K << fla) / 2;
			} else {
				eeprom->word_size = 2048;
				dev_info(hw->dev, "Unprogrammed Flash detected, "
					 "limiting access to first 4KB\n");
			}

			eeprom->acquire = e1000_acquire_eeprom_flash;
			eeprom->release = e1000_release_eeprom_flash;
		}

		eeprom->type = e1000_eeprom_flash;
		eeprom->read = e1000_read_eeprom_eerd;
		break;

	default:
		break;
	}

	if (eeprom->type == e1000_eeprom_spi) {
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
		if (eerd == E1000_EEPROM_POLL_READ)
			reg = e1000_read_reg(hw, E1000_EERD);
		else
			reg = e1000_read_reg(hw, E1000_EEWR);

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

		e1000_write_reg(hw, E1000_EERD, eerd);

		error = e1000_poll_eerd_eewr_done(hw, E1000_EEPROM_POLL_READ);

		if (error)
			break;

		data[i] = (e1000_read_reg(hw, E1000_EERD) >>
			   E1000_EEPROM_RW_REG_DATA);
	}

	return error;
}

static int32_t e1000_read_eeprom_spi(struct e1000_hw *hw,
				     uint16_t offset,
				     uint16_t words,
				     uint16_t *data)
{
	unsigned int i;
	uint16_t word_in;
	uint8_t read_opcode = EEPROM_READ_OPCODE_SPI;

	if (e1000_spi_eeprom_ready(hw)) {
		e1000_release_eeprom(hw);
		return -E1000_ERR_EEPROM;
	}

	e1000_standby_eeprom(hw);

	/* Some SPI eeproms use the 8th address bit embedded in
	 * the opcode */
	if ((hw->eeprom.address_bits == 8) && (offset >= 128))
		read_opcode |= EEPROM_A8_OPCODE_SPI;

	/* Send the READ command (opcode + addr)  */
	e1000_shift_out_ee_bits(hw, read_opcode, hw->eeprom.opcode_bits);
	e1000_shift_out_ee_bits(hw, (uint16_t)(offset * 2),
				hw->eeprom.address_bits);

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

	return E1000_SUCCESS;
}

static int32_t e1000_read_eeprom_microwire(struct e1000_hw *hw,
					   uint16_t offset,
					   uint16_t words,
					   uint16_t *data)
{
	int i;
	for (i = 0; i < words; i++) {
		/* Send the READ command (opcode + addr)  */
		e1000_shift_out_ee_bits(hw,
					EEPROM_READ_OPCODE_MICROWIRE,
					hw->eeprom.opcode_bits);
		e1000_shift_out_ee_bits(hw, (uint16_t)(offset + i),
					hw->eeprom.address_bits);

		/* Read the data.  For microwire, each word requires
		 * the overhead of eeprom setup and tear-down. */
		data[i] = e1000_shift_in_ee_bits(hw, 16);
		e1000_standby_eeprom(hw);
	}

	return E1000_SUCCESS;
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

static int e1000_flash_mode_wait_for_idle(struct e1000_hw *hw)
{
	const int ret = e1000_poll_reg(hw, E1000_FLSWCTL, E1000_FLSWCTL_DONE,
				       E1000_FLSWCTL_DONE, SECOND);
	if (ret < 0)
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set (wait)\n");
	return ret;
}

static int e1000_flash_mode_check_command_valid(struct e1000_hw *hw)
{
	const uint32_t flswctl = e1000_read_reg(hw, E1000_FLSWCTL);
	if (!(flswctl & E1000_FLSWCTL_CMDV)) {
		dev_err(hw->dev, "FLSWCTL.CMDV was cleared\n");
		return -EIO;
	}

	return E1000_SUCCESS;
}

static void e1000_flash_cmd(struct e1000_hw *hw,
			    uint32_t cmd, uint32_t offset)
{
	uint32_t flswctl = e1000_read_reg(hw, E1000_FLSWCTL);
	flswctl &= ~E1000_FLSWCTL_CMD_ADDR_MASK;
	flswctl |= E1000_FLSWCTL_CMD(cmd) | E1000_FLSWCTL_ADDR(offset);
	e1000_write_reg(hw, E1000_FLSWCTL, flswctl);
}

static int e1000_flash_mode_read_chunk(struct e1000_hw *hw, loff_t offset,
				       size_t size, void *data)
{
	int ret;
	size_t chunk, residue = size;
	uint32_t flswdata;

	DEBUGFUNC();

	if (size > SZ_4K ||
	    E1000_FLSWCTL_ADDR(offset) != offset)
		return -EINVAL;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;

	e1000_write_reg(hw, E1000_FLSWCNT, size);
	e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_READ, offset);

	do {
		ret = e1000_flash_mode_check_command_valid(hw);
		if (ret < 0)
			return -EIO;

		chunk = min(sizeof(flswdata), residue);

		ret = e1000_poll_reg(hw, E1000_FLSWCTL,
				     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
				     SECOND);
		if (ret < 0) {
			dev_err(hw->dev,
				"Timeout waiting for FLSWCTL.DONE to be set (read)\n");
			return ret;
		}

		flswdata = e1000_read_reg(hw, E1000_FLSWDATA);
		/*
		 * Readl does le32_to_cpu, so we need to undo that
		 */
		flswdata = cpu_to_le32(flswdata);
		memcpy(data, &flswdata, chunk);

		data += chunk;
		residue -= chunk;
	} while (residue);

	return E1000_SUCCESS;
}

static int e1000_flash_mode_write_chunk(struct e1000_hw *hw, loff_t offset,
					size_t size, const void *data)
{
	int ret;
	size_t chunk, residue = size;
	uint32_t flswdata;

	if (size > 256 ||
	    E1000_FLSWCTL_ADDR(offset) != offset)
		return -EINVAL;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;


	e1000_write_reg(hw, E1000_FLSWCNT, size);
	e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_WRITE, offset);

	do {
		chunk = min(sizeof(flswdata), residue);
		memcpy(&flswdata, data, chunk);
		/*
		 * writel does cpu_to_le32, so we do the inverse in
		 * order to account for that
		 */
		flswdata = le32_to_cpu(flswdata);
		e1000_write_reg(hw, E1000_FLSWDATA, flswdata);

		ret = e1000_flash_mode_check_command_valid(hw);
		if (ret < 0)
			return -EIO;

		ret = e1000_poll_reg(hw, E1000_FLSWCTL,
				     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
				     SECOND);
		if (ret < 0) {
			dev_err(hw->dev,
				"Timeout waiting for FLSWCTL.DONE to be set (write)\n");
			return ret;
		}

		data += chunk;
		residue -= chunk;

	} while (residue);

	return E1000_SUCCESS;
}


static int e1000_flash_mode_erase_chunk(struct e1000_hw *hw, loff_t offset,
					size_t size)
{
	int ret;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;

	if (!size && !offset)
		e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_ERASE_DEVICE, 0);
	else
		e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_ERASE_SECTOR, offset);

	ret = e1000_flash_mode_check_command_valid(hw);
	if (ret < 0)
		return -EIO;

	ret = e1000_poll_reg(hw, E1000_FLSWCTL,
			     E1000_FLSWCTL_DONE | E1000_FLSWCTL_FLBUSY,
			     E1000_FLSWCTL_DONE,
			     10 * SECOND);
	if (ret < 0) {
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set (erase)\n");
		return ret;
	}

	return E1000_SUCCESS;
}

enum {
	E1000_FLASH_MODE_OP_READ = 0,
	E1000_FLASH_MODE_OP_WRITE = 1,
	E1000_FLASH_MODE_OP_ERASE = 2,
};


static int e1000_flash_mode_io(struct e1000_hw *hw, int op, size_t granularity,
			       loff_t offset, size_t size, void *data)
{
	int ret;
	size_t residue = size;

	do {
		const size_t chunk = min(granularity, residue);

		switch (op) {
		case E1000_FLASH_MODE_OP_READ:
			ret = e1000_flash_mode_read_chunk(hw, offset,
							  chunk, data);
			break;
		case E1000_FLASH_MODE_OP_WRITE:
			ret = e1000_flash_mode_write_chunk(hw, offset,
							   chunk, data);
			break;
		case E1000_FLASH_MODE_OP_ERASE:
			ret = e1000_flash_mode_erase_chunk(hw, offset,
							   chunk);
			break;
		default:
			return -ENOTSUPP;
		}

		if (ret < 0)
			return ret;

		offset += chunk;
		residue -= chunk;
		data += chunk;
	} while (residue);

	return E1000_SUCCESS;
}


static int e1000_flash_mode_read(struct e1000_hw *hw, loff_t offset,
				 size_t size, void *data)
{
	return e1000_flash_mode_io(hw,
				   E1000_FLASH_MODE_OP_READ, SZ_4K,
				   offset, size, data);
}

static int e1000_flash_mode_write(struct e1000_hw *hw, loff_t offset,
				  size_t size, const void *data)
{
	int ret;

	ret = e1000_flash_mode_io(hw,
				  E1000_FLASH_MODE_OP_WRITE, 256,
				  offset, size, (void *)data);
	if (ret < 0)
		return ret;

	ret = e1000_poll_reg(hw, E1000_FLSWCTL,
			     E1000_FLSWCTL_FLBUSY,
			     0,  SECOND);
	if (ret < 0)
		dev_err(hw->dev, "Timout while waiting for FLSWCTL.FLBUSY\n");

	return ret;
}

static int e1000_flash_mode_erase(struct e1000_hw *hw, loff_t offset,
				  size_t size)
{
	return e1000_flash_mode_io(hw,
				   E1000_FLASH_MODE_OP_ERASE, SZ_4K,
				   offset, size, NULL);
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
	int32_t ret;

	DEBUGFUNC();

	if (!e1000_eeprom_valid(hw))
		return -EINVAL;

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

	if (eeprom->read) {
		if (e1000_acquire_eeprom(hw) != E1000_SUCCESS)
			return -E1000_ERR_EEPROM;

		ret = eeprom->read(hw, offset, words, data);
		e1000_release_eeprom(hw);

		return ret;
	} else {
		return -ENOTSUPP;
	}
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

	/*
	 * If the EEPROM device content isn't valid there is no point in
	 * checking the signature.
	 */
	if (!e1000_eeprom_valid(hw))
		return 0;

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

static ssize_t e1000_invm_cdev_read(struct cdev *cdev, void *buf,
				    size_t count, loff_t offset, unsigned long flags)
{
	uint8_t n, bnr;
	uint32_t line;
	size_t chunk, residue = count;
	struct e1000_hw *hw = container_of(cdev, struct e1000_hw, invm.cdev);

	n = offset / sizeof(line);
	if (n > E1000_INVM_DATA_MAX_N)
		return -EINVAL;

	bnr = offset % sizeof(line);
	if (bnr) {
		/*
		 * if bnr in not zero it means we have a non 4-byte
		 * aligned start and need to do a partial read
		 */
		const uint8_t *bptr;

		bptr  = (uint8_t *)&line + bnr;
		chunk = min(bnr - sizeof(line), count);
		line  = e1000_read_reg(hw, E1000_INVM_DATA(n));
		line  = cpu_to_le32(line); /* to account for readl */
		memcpy(buf, bptr, chunk);

		goto start_adjusted;
	}

	do {
		if (n > E1000_INVM_DATA_MAX_N)
			return -EINVAL;

		chunk = min(sizeof(line), residue);
		line = e1000_read_reg(hw, E1000_INVM_DATA(n));
		line = cpu_to_le32(line); /* to account for readl */

		/*
		 * by using memcpy in conjunction with min should get
		 * dangling tail reads as well as aligned reads
		 */
		memcpy(buf, &line, chunk);

	start_adjusted:
		residue -= chunk;
		buf += chunk;
		n++;
	} while (residue);

	return count;
}

static int e1000_invm_program(struct e1000_hw *hw, u32 offset, u32 value,
			      unsigned int delay)
{
	int retries = 400;
	do {
		if ((e1000_read_reg(hw, offset) & value) == value)
			return E1000_SUCCESS;

		e1000_write_reg(hw, offset, value);

		if (delay) {
			udelay(delay);
		} else {
			int ret;

			if (e1000_read_reg(hw, E1000_INVM_PROTECT) &
			    E1000_INVM_PROTECT_WRITE_ERROR) {
				dev_err(hw->dev, "Error while writing to %x\n", offset);
				return -EIO;
			}

			ret = e1000_poll_reg(hw, E1000_INVM_PROTECT,
					     E1000_INVM_PROTECT_BUSY,
					     0,  SECOND);
			if (ret < 0) {
				dev_err(hw->dev,
					"Timeout while waiting for INVM_PROTECT.BUSY\n");
				return ret;
			}
		}
	} while (retries--);

	return -ETIMEDOUT;
}

static int e1000_invm_set_lock(struct param_d *param, void *priv)
{
	struct e1000_hw *hw = priv;

	if (hw->invm.line > 31)
		return -EINVAL;

	return e1000_invm_program(hw,
				  E1000_INVM_LOCK(hw->invm.line),
				  E1000_INVM_LOCK_BIT,
				  10);
}

static int e1000_invm_unlock(struct e1000_hw *hw)
{
	e1000_write_reg(hw, E1000_INVM_PROTECT, E1000_INVM_PROTECT_CODE);
	/*
	 * If we were successful at unlocking iNVM for programming we
	 * should see ALLOW_WRITE bit toggle to 1
	 */
	if (!(e1000_read_reg(hw, E1000_INVM_PROTECT) &
	      E1000_INVM_PROTECT_ALLOW_WRITE))
		return -EIO;
	else
		return E1000_SUCCESS;
}

static void e1000_invm_lock(struct e1000_hw *hw)
{
	e1000_write_reg(hw, E1000_INVM_PROTECT, 0);
}

static int e1000_invm_write_prepare(struct e1000_hw *hw)
{
	int ret;
	/*
	 * This needs to be done accorging to the datasheet p. 541 and
	 * p. 79
	*/
	e1000_write_reg(hw, E1000_PCIEMISC,
			E1000_PCIEMISC_RESERVED_PATTERN1 |
			E1000_PCIEMISC_DMA_IDLE          |
			E1000_PCIEMISC_RESERVED_PATTERN2);

	/*
	 * Needed for programming iNVM on devices with Flash with valid
	 * contents attached
	 */
	ret = e1000_poll_reg(hw, E1000_EEMNGCTL,
			     E1000_EEMNGCTL_CFG_DONE,
			     E1000_EEMNGCTL_CFG_DONE, SECOND);
	if (ret < 0) {
		dev_err(hw->dev,
			"Timeout while waiting for EEMNGCTL.CFG_DONE\n");
		return ret;
	}

	udelay(15);

	return E1000_SUCCESS;
}

static ssize_t e1000_invm_cdev_write(struct cdev *cdev, const void *buf,
				     size_t count, loff_t offset, unsigned long flags)
{
	int ret;
	uint8_t n, bnr;
	uint32_t line;
	size_t chunk, residue = count;
	struct e1000_hw *hw = container_of(cdev, struct e1000_hw, invm.cdev);

	ret = e1000_invm_write_prepare(hw);
	if (ret < 0)
		return ret;

	ret = e1000_invm_unlock(hw);
	if (ret < 0)
		goto exit;

	n = offset / sizeof(line);
	if (n > E1000_INVM_DATA_MAX_N) {
		ret = -EINVAL;
		goto exit;
	}

	bnr = offset % sizeof(line);
	if (bnr) {
		uint8_t *bptr;
		/*
		 * if bnr in not zero it means we have a non 4-byte
		 * aligned start and need to do a read-modify-write
		 * sequence
		 */

		/* Read */
		line = e1000_read_reg(hw, E1000_INVM_DATA(n));

		/* Modify */
		/*
		 * We need to ensure that line is LE32 in order for
		 * memcpy to copy byte from least significant to most
		 * significant, since that's how i210 will write the
		 * 32-bit word out to OTP
		 */
		line = cpu_to_le32(line);
		bptr  = (uint8_t *)&line + bnr;
		chunk = min(sizeof(line) - bnr, count);
		memcpy(bptr, buf, chunk);
		line = le32_to_cpu(line);

		/* Jumping inside of the loop to take care of the
		 * Write */
		goto start_adjusted;
	}

	do {
		if (n > E1000_INVM_DATA_MAX_N) {
			ret = -EINVAL;
			goto exit;
		}

		chunk = min(sizeof(line), residue);
		if (chunk != sizeof(line)) {
			/*
			 * If chunk is smaller that sizeof(line), which
			 * should be 4 bytes, we have a "dangling"
			 * chunk and we should read the unchanged
			 * portion of the 4-byte word from iNVM and do
			 * a read-modify-write sequence
			 */
			line = e1000_read_reg(hw, E1000_INVM_DATA(n));
		}

		line = cpu_to_le32(line);
		memcpy(&line, buf, chunk);
		line = le32_to_cpu(line);

	start_adjusted:
		/*
		 * iNVM is organized in 32 64-bit lines and each of
		 * those lines can be locked to prevent any further
		 * modification, so for every i-th 32-bit word we need
		 * to check INVM_LINE[i/2] register to see if that word
		 * can be modified
		 */
		if (e1000_read_reg(hw, E1000_INVM_LOCK(n / 2)) &
		    E1000_INVM_LOCK_BIT) {
			dev_err(hw->dev, "line %d is locked\n", n / 2);
			ret = -EIO;
			goto exit;
		}

		ret = e1000_invm_program(hw,
					 E1000_INVM_DATA(n),
					 line,
					 0);
		if (ret < 0)
			goto exit;

		residue -= chunk;
		buf += chunk;
		n++;
	} while (residue);

	ret = E1000_SUCCESS;
exit:
	e1000_invm_lock(hw);
	return ret;
}

static struct file_operations e1000_invm_ops = {
	.read	= e1000_invm_cdev_read,
	.write	= e1000_invm_cdev_write,
	.lseek	= dev_lseek_default,
};

static ssize_t e1000_eeprom_cdev_read(struct cdev *cdev, void *buf,
				      size_t count, loff_t offset, unsigned long flags)
{
	struct e1000_hw *hw = container_of(cdev, struct e1000_hw, eepromcdev);
	int32_t ret;

	/*
	 * The eeprom interface works on 16 bit words which gives a nice excuse
	 * for being lazy and not implementing unaligned reads.
	 */
	if (offset & 1 || count == 1)
		return -EIO;

	ret = e1000_read_eeprom(hw, offset / 2, count / 2, buf);
	if (ret)
		return -EIO;
	else
		return (count / 2) * 2;
};

static struct file_operations e1000_eeprom_ops = {
	.read = e1000_eeprom_cdev_read,
	.lseek = dev_lseek_default,
};

static int e1000_mtd_read_or_write(bool read,
				   struct mtd_info *mtd, loff_t off, size_t len,
				   size_t *retlen, u_char *buf)
{
	int ret;
	struct e1000_hw *hw = container_of(mtd, struct e1000_hw, mtd);

	DEBUGFUNC();

	if (e1000_acquire_eeprom(hw) == E1000_SUCCESS) {
		if (read)
			ret = e1000_flash_mode_read(hw, off,
						    len, buf);
		else
			ret = e1000_flash_mode_write(hw, off,
						     len, buf);
		if (ret == E1000_SUCCESS)
			*retlen = len;

		e1000_release_eeprom(hw);
	} else {
		ret = -E1000_ERR_EEPROM;
	}

	return ret;

}

static int e1000_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
				 size_t *retlen, u_char *buf)
{
	return e1000_mtd_read_or_write(true,
				       mtd, from, len, retlen, buf);
}

static int e1000_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			   size_t *retlen, const u_char *buf)
{
	return e1000_mtd_read_or_write(false,
				       mtd, to, len, retlen, (u_char *)buf);
}

static int e1000_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	uint32_t rem;
	struct e1000_hw *hw = container_of(mtd, struct e1000_hw, mtd);
	int ret;

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	ret = e1000_acquire_eeprom(hw);
	if (ret != E1000_SUCCESS)
		goto fail;

	/*
	 * If mtd->size is 4096 it means we are dealing with
	 * unprogrammed flash and we don't really know its size to
	 * make an informed decision wheither to erase the whole chip or
	 * just a number of its sectors
	 */
	if (mtd->size > SZ_4K &&
	    instr->len == mtd->size)
		ret = e1000_flash_mode_erase(hw, 0, 0);
	else
		ret = e1000_flash_mode_erase(hw,
					     instr->addr, instr->len);

	e1000_release_eeprom(hw);

	if (ret < 0)
		goto fail;

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;

fail:
	instr->state = MTD_ERASE_FAILED;
	return ret;
}

static int e1000_mtd_sr_rmw(struct mtd_info *mtd, u8 mask, u8 val)
{
	struct e1000_hw *hw = container_of(mtd, struct e1000_hw, mtd);
	uint32_t flswdata;
	int ret;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;

	e1000_write_reg(hw, E1000_FLSWCNT, 1);
	e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_RDSR, 0);

	ret = e1000_flash_mode_check_command_valid(hw);
	if (ret < 0)
		return -EIO;

	ret = e1000_poll_reg(hw, E1000_FLSWCTL,
			     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
			     SECOND);
	if (ret < 0) {
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set (RDSR)\n");
		return ret;
	}

	flswdata = e1000_read_reg(hw, E1000_FLSWDATA);

	flswdata = (flswdata & ~mask) | val;

	e1000_write_reg(hw, E1000_FLSWCNT, 1);
	e1000_flash_cmd(hw, E1000_FLSWCTL_CMD_WRSR, 0);

	ret = e1000_flash_mode_check_command_valid(hw);
	if (ret < 0)
		return -EIO;

	e1000_write_reg(hw, E1000_FLSWDATA, flswdata);

	ret = e1000_poll_reg(hw, E1000_FLSWCTL,
			     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
			     SECOND);
	if (ret < 0) {
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set (WRSR)\n");
	}

	return ret;
}

/*
 * The available spi nor devices are very different in how the block protection
 * bits affect which sectors to be protected. So take the simple approach and
 * only use BP[012] = b000 (unprotected) and BP[012] = b111 (protected).
 */
#define SR_BPALL (SR_BP0 | SR_BP1 | SR_BP2)

static int e1000_mtd_lock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return e1000_mtd_sr_rmw(mtd, SR_BPALL, SR_BPALL);
}

static int e1000_mtd_unlock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return e1000_mtd_sr_rmw(mtd, SR_BPALL, 0x0);
}

int e1000_register_invm(struct e1000_hw *hw)
{
	int ret;
	u16 word;
	struct param_d *p;

	if (e1000_eeprom_valid(hw)) {
		ret = e1000_read_eeprom(hw, 0x0a, 1, &word);
		if (ret < 0)
			return ret;

		if (word & (1 << 15))
			dev_warn(hw->dev, "iNVM lockout mechanism is active\n");
	}

	hw->invm.cdev.dev = hw->dev;
	hw->invm.cdev.ops = &e1000_invm_ops;
	hw->invm.cdev.priv = hw;
	hw->invm.cdev.name = xasprintf("e1000-invm%d", hw->dev->id);
	hw->invm.cdev.size = 4 * (E1000_INVM_DATA_MAX_N + 1);

	ret = devfs_create(&hw->invm.cdev);
	if (ret < 0)
		return ret;

	strcpy(hw->invm.dev.name, "invm");
	hw->invm.dev.id = hw->dev->id;
	hw->invm.dev.parent = hw->dev;
	ret = register_device(&hw->invm.dev);
	if (ret < 0) {
		devfs_remove(&hw->invm.cdev);
		return ret;
	}

	p = dev_add_param_int(&hw->invm.dev, "lock", e1000_invm_set_lock,
			      NULL, &hw->invm.line, "%u", hw);
	if (IS_ERR(p)) {
		unregister_device(&hw->invm.dev);
		devfs_remove(&hw->invm.cdev);
		ret = PTR_ERR(p);
	}

	return ret;
}

int e1000_eeprom_valid(struct e1000_hw *hw)
{
	uint32_t valid_mask = E1000_EECD_FLASH_IN_USE |
			      E1000_EECD_AUTO_RD | E1000_EECD_EE_PRES;

	if (hw->mac_type != e1000_igb)
		return 1;

	/*
	 * If there is no flash in use or AUTO_RD or EE_PRES are not set in
	 * EECD, the shadow RAM is invalid (and in practise seems to contain
	 * the contents of iNVM).
	 */
	if ((e1000_read_reg(hw, E1000_EECD) & valid_mask) != valid_mask)
		return 0;

	return 1;
}

/*
 * This function has a wrong name for historic reasons, it doesn't add an
 * eeprom, but the flash (if available) that is used to simulate the eeprom.
 * Also a device that represents the invm is registered here (if available).
 */
int e1000_register_eeprom(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd;
	int ret;

	if (hw->mac_type != e1000_igb)
		return E1000_SUCCESS;

	eecd = e1000_read_reg(hw, E1000_EECD);

	if (eecd & E1000_EECD_AUTO_RD) {
		if (eecd & E1000_EECD_EE_PRES) {
			if (eecd & E1000_EECD_FLASH_IN_USE) {
				uint32_t fla = e1000_read_reg(hw, E1000_FLA);
				dev_info(hw->dev,
					 "Hardware programmed from flash (%ssecure)\n",
					 fla & E1000_FLA_LOCKED ? "" : "un");
			} else {
				dev_info(hw->dev, "Hardware programmed from iNVM\n");
			}
		} else {
			dev_warn(hw->dev, "Shadow RAM invalid\n");
		}
	} else {
		/*
		 * I never saw this case in practise and I'm unsure how
		 * to handle that. Maybe just wait until the hardware is
		 * up enough that this bit is set?
		 */
		dev_err(hw->dev, "Flash Auto-Read not done\n");
	}

	if (e1000_eeprom_valid(hw)) {
		hw->eepromcdev.dev = hw->dev;
		hw->eepromcdev.ops = &e1000_eeprom_ops;
		hw->eepromcdev.name = xasprintf("e1000-eeprom%d",
						hw->dev->id);
		hw->eepromcdev.size = 0x1000;

		ret = devfs_create(&hw->eepromcdev);
		if (ret < 0)
			return ret;
	}

	if (eecd & E1000_EECD_I210_FLASH_DETECTED) {
		hw->mtd.parent = hw->dev;
		hw->mtd.read = e1000_mtd_read;
		hw->mtd.write = e1000_mtd_write;
		hw->mtd.erase = e1000_mtd_erase;
		hw->mtd.lock = e1000_mtd_lock;
		hw->mtd.unlock = e1000_mtd_unlock;
		hw->mtd.size = eeprom->word_size * 2;
		hw->mtd.writesize = 1;
		hw->mtd.subpage_sft = 0;

		hw->mtd.eraseregions = xzalloc(sizeof(struct mtd_erase_region_info));
		hw->mtd.erasesize = SZ_4K;
		hw->mtd.eraseregions[0].erasesize = SZ_4K;
		hw->mtd.eraseregions[0].numblocks = hw->mtd.size / SZ_4K;
		hw->mtd.numeraseregions = 1;

		hw->mtd.flags = MTD_CAP_NORFLASH;
		hw->mtd.type = MTD_NORFLASH;

		ret = add_mtd_device(&hw->mtd, "e1000-nor",
				     DEVICE_ID_DYNAMIC);
		if (ret)
			goto out_eeprom;
	}

	ret = e1000_register_invm(hw);
	if (ret < 0)
		goto out_mtd;

	return E1000_SUCCESS;

out_mtd:
	if (eecd & E1000_EECD_I210_FLASH_DETECTED)
		del_mtd_device(&hw->mtd);
out_eeprom:
	if (e1000_eeprom_valid(hw))
		devfs_remove(&hw->eepromcdev);

	return ret;
}

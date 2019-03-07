#ifndef I2C_IMX_H
#define I2C_IMX_H

/*
 * IMX I2C registers:
 * the I2C register offset is different between SoCs, to provide support for
 * all these chips, split the register offset into a fixed base address and a
 * variable shift value, then the full register offset will be calculated by:
 * reg_off = reg_base_addr << reg_shift
 */
#define FSL_I2C_IADR	0x00	/* i2c slave address */
#define FSL_I2C_IFDR	0x01	/* i2c frequency divider */
#define FSL_I2C_I2CR	0x02	/* i2c control */
#define FSL_I2C_I2SR	0x03	/* i2c status */
#define FSL_I2C_I2DR	0x04	/* i2c transfer data */
#define FSL_I2C_DFSRR	0x05	/* i2c digital filter sampling rate */

#define IMX_I2C_REGSHIFT	2
#define VF610_I2C_REGSHIFT	0

/* Bits of FSL I2C registers */
#define I2SR_RXAK	0x01
#define I2SR_IIF	0x02
#define I2SR_SRW	0x04
#define I2SR_IAL	0x10
#define I2SR_IBB	0x20
#define I2SR_IAAS	0x40
#define I2SR_ICF	0x80
#define I2CR_RSTA	0x04
#define I2CR_TXAK	0x08
#define I2CR_MTX	0x10
#define I2CR_MSTA	0x20
#define I2CR_IIEN	0x40
#define I2CR_IEN	0x80

/*
 * register bits different operating codes definition:
 *
 * 1) I2SR: Interrupt flags clear operation differ between SoCs:
 * - write zero to clear(w0c) INT flag on i.MX,
 * - but write one to clear(w1c) INT flag on Vybrid.
 *
 * 2) I2CR: I2C module enable operation also differ between SoCs:
 * - set I2CR_IEN bit enable the module on i.MX,
 * - but clear I2CR_IEN bit enable the module on Vybrid.
 */
#define I2SR_CLR_OPCODE_W0C	0x0
#define I2SR_CLR_OPCODE_W1C	(I2SR_IAL | I2SR_IIF)
#define I2CR_IEN_OPCODE_0	0x0
#define I2CR_IEN_OPCODE_1	I2CR_IEN

#endif /* I2C_IMX_H */

#ifndef __INCLUDE_SPI_H
#define __INCLUDE_SPI_H

struct spi_board_info {
	char	*name;
	int 	max_speed_hz;
	int	bus_num;
	int	chip_select;
};

struct spi_master {
};

struct spi_transfer {
	/* it's ok if tx_buf == rx_buf (right?)
	 * for MicroWire, one buffer must be null
	 * buffers must work with dma_*map_single() calls, unless
	 *   spi_message.is_dma_mapped reports a pre-existing mapping
	 */
	const void	*tx_buf;
	void		*rx_buf;
	unsigned	len;

	unsigned	cs_change:1;
	u8		bits_per_word;
	u16		delay_usecs;
	u32		speed_hz;

	struct list_head transfer_list;
};

int spi_register_boardinfo(struct spi_board_info *info, int num);
int spi_register_master(struct spi_master *master);

#endif /* __INCLUDE_SPI_H */

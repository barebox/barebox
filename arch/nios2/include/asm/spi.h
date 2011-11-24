#ifndef __ALTERA_SPI_H_
#define __ALTERA_SPI_H_

#include <spi/spi.h>

struct spi_altera_master {
	int	num_chipselect;
	int	spi_mode;
	int	databits;
	int	speed;
	int	bus_num;
};

struct altera_spi {
	struct spi_master	master;
	int			databits;
	int			speed;
	int			mode;
	void __iomem		*regs;
};

#endif /*__ALTERA_SPI_H_*/

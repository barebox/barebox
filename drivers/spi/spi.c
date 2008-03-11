#include <common.h>
#include <spi/spi.h>

int spi_register_boardinfo(struct spi_board_info *info, int num)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int spi_register_master(struct spi_master *master)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

#ifndef MTD_OMAP_GPMC_DECODE_BCH_H
#define MTD_OMAP_GPMC_DECODE_BCH_H

int omap_gpmc_decode_bch(int select_4_8, unsigned char *ecc, unsigned int *err_loc);

#endif /* MTD_OMAP_GPMC_DECODE_BCH_H */
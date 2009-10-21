
#ifndef __DM9000_H__
#define __DM9000_H__

#define DM9000_WIDTH_8		1
#define DM9000_WIDTH_16		2
#define DM9000_WIDTH_32		3

struct dm9000_platform_data {
	unsigned long iobase;
	unsigned long iodata;
	int buswidth;
	int srom;
};

#endif /* __DM9000_H__ */


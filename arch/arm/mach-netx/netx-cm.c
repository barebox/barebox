#include <common.h>
#include <mach/netx-regs.h>
#include <mach/netx-cm.h>

#define I2C_CTRL_ENABLE		(1<<0)
#define I2C_CTRL_SPEED_25	(0<<1)
#define I2C_CTRL_SPEED_50	(1<<1)
#define I2C_CTRL_SPEED_100	(2<<1)
#define I2C_CTRL_SPEED_200	(3<<1)
#define I2C_CTRL_SPEED_400	(4<<1)
#define I2C_CTRL_SPEED_600	(5<<1)
#define I2C_CTRL_SPEED_800	(6<<1)
#define I2C_CTRL_SPEED_1000	(7<<1)
#define I2C_CTRL_SLAVEID(id)	(((id) & 0x7f) << 4)

#define I2C_DATA_SEND_STOP	(1<<8)
#define I2C_DATA_READ		(1<<9)
#define I2C_DATA_SEND_START	(1<<10)
#define I2C_DATA_BUSY		(1<<11)
#define I2C_DATA_EXECUTE	(1<<11)
#define I2C_DATA_RDF		(1<<12)

/* INS */
#define AT88_WRITE_USER_ZONE	0xb0
#define AT88_READ_USER_ZONE	0xb2
#define AT88_SYSTEM_WRITE	0xb4
#define AT88_SYSTEM_READ	0xb6

/* P1 */
#define AT88_READ_CONFIG_ZONE	0x00
#define AT88_SET_USER_ZONE	0x03
#define SEND_START (1<<0)
#define SEND_STOP  (1<<1)
#define IGNORE_RDF (1<<2)

#define MAX_USER_ZONE_SIZE 128

/*
 *  netx i2c functions
 */
static inline void i2c_set_slaveid(uchar id)
{
	unsigned int val;
	val = NETX_I2C_CTRL_REG;
	val &= 0xf;
	val |= I2C_CTRL_SLAVEID(id);
	NETX_I2C_CTRL_REG = val;
}

static inline uchar i2c_read_byte(int flags)
{
	unsigned int val = I2C_DATA_EXECUTE | I2C_DATA_READ;

	if(flags & SEND_START)
		val |= I2C_DATA_SEND_START;
	if(flags & SEND_STOP)
		val |= I2C_DATA_SEND_STOP;

	NETX_I2C_DATA_REG = val;

	while(NETX_I2C_DATA_REG & I2C_DATA_BUSY);

	return NETX_I2C_DATA_REG & 0xff;
}

static inline void i2c_write_byte(uchar byte, int flags)
{
	unsigned int val = byte;

	if(flags & SEND_START)
		val |= I2C_DATA_SEND_START;
	if(flags & SEND_STOP)
		val |= I2C_DATA_SEND_STOP;
	val |= I2C_DATA_EXECUTE;

	NETX_I2C_DATA_REG = val;

	while(NETX_I2C_DATA_REG & I2C_DATA_BUSY);
}

void i2c_init (int speed)
{
	unsigned int val;

	switch(speed) {
		case 25000:
			val = I2C_CTRL_SPEED_25;
			break;
		case 50000:
			val = I2C_CTRL_SPEED_50;
			break;
		case 100000:
			val = I2C_CTRL_SPEED_100;
			break;
		case 200000:
			val = I2C_CTRL_SPEED_200;
			break;
		case 400000:
			val = I2C_CTRL_SPEED_400;
			break;
		case 600000:
			val = I2C_CTRL_SPEED_600;
			break;
		case 800000:
			val = I2C_CTRL_SPEED_800;
			break;
		case 1000000:
			val = I2C_CTRL_SPEED_1000;
			break;
		default:
			printf("unsupported speed %d. defaulting to 100kHz\n",speed);
			val = I2C_CTRL_SPEED_100;
			break;
	}

	NETX_I2C_CTRL_REG = val | I2C_CTRL_ENABLE;

	i2c_write_byte(0xff, 0);

	udelay(2000);
}

/*
 *  at88SCxxxx CryptoMemory functions
 */
struct at88_parm {
	char *name;
	int zones;
	int zone_size;
	uchar atr[8];
};

struct at88_parm at88_parm_table[] = {
	{ .name = "at88sc0104c", .zones =  4, .zone_size = 32,  .atr = { 0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x01 } },
	{ .name = "at88sc0204c", .zones =  4, .zone_size = 64,  .atr = { 0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x02 } },
	{ .name = "at88sc0404c", .zones =  4, .zone_size = 128, .atr = { 0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x04 } },
	{ .name = "at88sc0808c", .zones =  8, .zone_size = 128, .atr = { 0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x08 } },
	{ .name = "at88sc1616c", .zones = 16, .zone_size = 128, .atr = { 0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x16 } },
};
#define parm_size (sizeof(at88_parm_table) / sizeof(struct at88_parm))

struct at88_parm *at88_parm = NULL;

int set_user_zone(uchar zone)
{
	if(zone >= at88_parm->zones)
		return -1;

	i2c_set_slaveid(AT88_SYSTEM_WRITE >> 1);
	i2c_write_byte(AT88_SET_USER_ZONE, SEND_START);
	i2c_write_byte(zone, 0);
	i2c_write_byte(8, SEND_STOP);

	return 0;
}

/*
 *  We chose the easy way here and read/write whole zones at once
 */
int read_user_zone(char *buf)
{
	int i;

	i2c_set_slaveid(AT88_READ_USER_ZONE >> 1);
	i2c_write_byte(0, SEND_START);
	i2c_write_byte(0, 0);
	i2c_write_byte(at88_parm->zone_size, 0);

	for(i=0; i < at88_parm->zone_size; i++)
		buf[i] = i2c_read_byte( i==at88_parm->zone_size - 1 ? SEND_STOP : 0);

	return 0;
}

#define BLK_SIZE 16
int write_user_zone(char *buf)
{
	int i,block;

	for(block=0; block < at88_parm->zone_size/16; block++) {
		i2c_set_slaveid(AT88_WRITE_USER_ZONE >> 1);
		i2c_write_byte(0, SEND_START);
		i2c_write_byte(block * BLK_SIZE, 0);
		i2c_write_byte(BLK_SIZE, 0);

		for(i=0; i<BLK_SIZE; i++)
			i2c_write_byte( buf[block * BLK_SIZE + i], i == BLK_SIZE - 1 ? SEND_STOP : 0);

		udelay(16000);
	}

	return 0;
}

struct netx_cm_userarea netx_cm_userarea;

struct netx_cm_userarea* netx_cm_get_userarea(void)
{
	set_user_zone(1);
	if( read_user_zone((char *)&netx_cm_userarea.area_1) )
		return NULL;

	set_user_zone(2);
	if( read_user_zone((char *)&netx_cm_userarea.area_2) )
		return NULL;

	set_user_zone(3);
	if( read_user_zone((char *)&netx_cm_userarea.area_2) )
		return NULL;

	return &netx_cm_userarea;
}

int netx_cm_write_userarea(struct netx_cm_userarea *area)
{
	set_user_zone(1);
	if( write_user_zone( (char *)&area->area_1 ) )
		return -1;

	set_user_zone(2);
	if( write_user_zone( (char *)&area->area_2 ) )
		return -1;

	set_user_zone(3);
	if( write_user_zone( (char *)&area->area_2 ) )
		return -1;
	return 0;
}


unsigned short crc16(unsigned short crc, unsigned int data)
{
	crc  = (crc >> 8) | ((crc & 0xff) << 8);
	crc ^= data;
	crc ^= (crc & 0xff) >> 4;
	crc ^= (crc & 0x0f) << 12;
	crc ^= ((crc & 0xff) << 4) << 1;

	return crc;
}


#define ETH_MAC_4321         0x1564
#define ETH_MAC_65           0x1568

int netx_cm_init(void)
{
	int i;
	char buf[MAX_USER_ZONE_SIZE];
	struct netx_cm_userarea *area;

	i2c_init(100000);

	i2c_set_slaveid(AT88_SYSTEM_READ >> 1);
	i2c_write_byte(AT88_READ_CONFIG_ZONE, SEND_START);
	i2c_write_byte(0, 0); /* adr */
	i2c_write_byte(8, 0); /* len */

	for(i=0;i<8;i++)
		buf[i] = i2c_read_byte( i==7 ? SEND_STOP : 0 );

	for(i=0; i<parm_size; i++) {
		if(!memcmp(buf,at88_parm_table[i].atr,8)) {
			at88_parm = &at88_parm_table[i];
			break;
		}
	}

	if(!at88_parm) {
		printf("no crypto flash found\n");
		debug("unrecognized atr: ");
		for(i=0; i<8; i++)
			debug("0x%02x ",buf[i]);
		printf("\n");
		return -1;
	}

	printf("%s crypto flash found\n",at88_parm->name);

	area = netx_cm_get_userarea();

	for(i=0;i<4;i++) {
		printf("xc%d mac: %02x:%02x:%02x:%02x:%02x:%02x\n", i,
			area->area_1.mac[i][0],
			area->area_1.mac[i][1],
			area->area_1.mac[i][2],
			area->area_1.mac[i][3],
			area->area_1.mac[i][4],
			area->area_1.mac[i][5]);

		XPEC_REG(i, XPEC_RAM_START + ETH_MAC_4321) =
			area->area_1.mac[i][0] |
			area->area_1.mac[i][1]<<8 |
			area->area_1.mac[i][2]<<16 |
			area->area_1.mac[i][3]<<24;

		XPEC_REG(i, XPEC_RAM_START + ETH_MAC_65) =
			area->area_1.mac[i][4] |
			area->area_1.mac[i][5]<<8;
	}

	for(i=0; i<6; i++)
		gd->bd->bi_enetaddr[i] = area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][i];


	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][0],
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][1],
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][2],
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][3],
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][4],
			area->area_1.mac[CONFIG_DRIVER_NET_NETX_XCNO][5]);

	setenv("ethaddr", buf);

	return 0;
}

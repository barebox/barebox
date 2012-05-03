#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <libgen.h>
#include <getopt.h>

struct netx_block_normal {
	uint32_t sdram_general_ctrl;	/* SDRam General control value */
	uint32_t sdram_timing_ctrl;	/* SDRam Timing control register value */
	uint32_t reserved[3];
};

struct netx_block_expbus {
		uint32_t exp_bus_reg;	/* Expension bus register value (EXPBus Bootmode) */
		uint32_t io_reg_mode0;	/* IORegmode0 register value (EXPBus Bootmode) */
		uint32_t io_reg_mode1;	/* IORegmode1 register value (EXPBus Bootmode) */
		uint32_t if_conf1;	/* IfConfig1 register value (EXPBus Bootmode) */
		uint32_t if_conf2;	/* IfConfig2 register value (EXPBus Bootmode) */
};

struct netx_bootblock {
	uint32_t cookie;     /* Cookie identifying bus width and valid bootblock */
# define MAGICCOOKIE_8BIT      0xF8BEAF08  /* Cookie used for  8Bit Flashes */
# define MAGICCOOKIE_16BIT     0xF8BEAF16  /* Cookie used for 16Bit Flashes */
# define MAGICCOOKIE_32BIT     0xF8BEAF32  /* Cookie used for 32Bit Flashes */

	union {
		uint32_t mem_ctrl;        /* Parallel/Serial Flash Mode for setting up timing parameters */
		uint32_t speed;          /* I2C/SPI Mode for identifying speed of device */
		uint32_t reserved;       /* PCI/DPM mode */
	} ctrl;

	uint32_t appl_entrypoint;   /* Entrypoint to application after relocation */
	uint32_t appl_checksum;     /* Checksum of application (DWORD sum over application) */
	uint32_t appl_size;         /* size of application in DWORDs */
	uint32_t appl_start_addr;    /* Relocation address of application */
	uint32_t signature;        /* Bootblock signature ('NETX') */
# define NETX_IDENTIFICATION   0x5854454E /* Valid signature 'N' 'E' 'T' 'X' */

	union {
		struct netx_block_normal normal;
		struct netx_block_expbus expbus;
	} config;

	uint32_t misc_asic_ctrl;            /* ASIC CTRL register value */
	uint32_t UserParameter;	/* Serial number or user parameter */
	uint32_t SourceType;	/* 1 = parallel falsh at the SRAM bus */
# define ST_PFLASH 1
# define ST_SFLASH 2
# define ST_SEEPROM 3

	uint32_t boot_checksum;            /* Bootblock checksum (complement of DWORD sum over bootblock) */
};

void print_usage(char *prg)
{
        fprintf(stderr, "Usage: %s [Options]\n"
                        "Options:\n"
			" -i, --infile=FILE      input file\n"
			" -o  --outfile=FILE     outputfile\n"
			" -m  --memctrl=REG      Memory Control register value\n"
	                " -s, --sdramctrl=REG    SDRAM Control regster value\n"
                        " -t, --sdramtimctrl=REG SDRAM Timing Control regster value\n"
                        " -e, --entrypoint=ADR   Application entrypoint\n"
                        " -c, --cookie=BITS      Cookie to use (8|16|32)\n"
			" -h, --help            this help\n",
				prg);
}

int main(int argc, char *argv[])
{
	struct netx_bootblock *nb;
	int fd;
	struct stat s;
	int opt;
	unsigned char *buf;
	int bytes, err, barebox_size, ofs, i;
	uint32_t *ptr;
	uint32_t checksum = 0;
	uint32_t memctrl = 0, sdramctrl = 0, sdramtimctrl = 0, entrypoint = 0, cookie = 0;
	char *infile = NULL, *outfile = NULL;

	struct option long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "infile", required_argument, 0, 'i'},
		{ "outfile", required_argument, 0, 'o'},
		{ "memctrl", required_argument, 0, 'm'},
		{ "sdramctrl", required_argument, 0, 's' },
		{ "sdramtimctrl", required_argument, 0, 't' },
		{ "entrypoint", required_argument, 0, 'e' },
		{ "cookie", required_argument, 0 , 'c' },
		{ 0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "hi:o:m:s:t:e:c:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'h':
				print_usage(basename(argv[0]));
				exit(0);
			case 'i':
				infile = optarg;
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'm':
				memctrl = strtoul(optarg, NULL, 0);
				break;
			case 's':
				sdramctrl = strtoul(optarg, NULL, 0);
				break;
			case 't':
				sdramtimctrl = strtoul(optarg, NULL, 0);
				break;
			case 'e':
				entrypoint = strtoul(optarg, NULL, 0);
				break;
			case 'c':
				cookie = strtoul(optarg, NULL, 0);
				break;
		}
	}

	if(!infile) {
		printf("no input filename supplied\n");
		exit(1);
	}

	if(!outfile) {
		printf("no outpu filename supplied\n");
		exit(1);
	}

	switch (cookie) {
		case 8:
			cookie = MAGICCOOKIE_8BIT;
			break;
		case 16:
			cookie = MAGICCOOKIE_16BIT;
			break;
		case 32:
			cookie = MAGICCOOKIE_32BIT;
			break;
		default:
			fprintf(stderr, "invalid coookie size %d\n",cookie);
	}

	fd = open(infile,O_RDONLY);
	if(fd < 0) {
		perror("open");
		exit(1);
	}

	if( fstat(fd, &s) < 0) {
		perror("fstat");
		exit(1);
	}

	barebox_size = s.st_size;
	printf("found barebox image. size: %d bytes. Using entrypoint 0x%08x\n",barebox_size,entrypoint);

	buf = malloc(barebox_size + sizeof(struct netx_bootblock) + 4);
	if(!buf) {
		perror("malloc");
		exit(1);
	}
	memset(buf, 0, barebox_size + sizeof(struct netx_bootblock) + 4);

	nb = (struct netx_bootblock *)buf;

	nb->cookie = cookie;
	nb->ctrl.mem_ctrl = memctrl;
	nb->appl_entrypoint = entrypoint;
	nb->appl_size = (barebox_size >> 2);

	nb->appl_start_addr = entrypoint;
	nb->signature = NETX_IDENTIFICATION;
	nb->config.normal.sdram_general_ctrl = sdramctrl;
	nb->config.normal.sdram_timing_ctrl = sdramtimctrl;
	nb->SourceType = ST_PFLASH;

	ofs = sizeof(struct netx_bootblock);
	bytes = barebox_size;

	while(bytes) {
		err = read(fd, buf + ofs, bytes);
		if( err < 0 ) {
			perror("read");
			exit(1);
		}
		bytes -= err;
		ofs += err;
	}

	close(fd);

	/* calculate application checksum */
	ptr = (uint32_t *)(buf + sizeof(struct netx_bootblock));

	checksum = 0;

	for( i = 0; i < nb->appl_size; i++) {
		checksum += *ptr++;
	}

	nb->appl_checksum = checksum;
	printf("application checksum: 0x%08x\n",nb->appl_checksum);

	/* calculate bootblock checksum */
	ptr = (uint32_t *)buf;
	checksum = 0;
	for( i = 0; i < (sizeof(struct netx_bootblock) >> 2); i++)
		checksum += *ptr++;
	nb->boot_checksum = -1 * checksum;

	fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP | S_IROTH);
	if(fd < 0) {
		perror("open");
		exit(1);
	}

	bytes = barebox_size + sizeof(struct netx_bootblock);
	ofs = 0;
	while(bytes) {
		err = write(fd, buf + ofs, bytes);
		if( err < 0) {
			perror("write");
			exit(1);
		}
		bytes -= err;
		ofs += err;
	}

	close(fd);
	free(buf);
	return 0;
}

#define BCM2835_MCI_SLOTISR_VER			0xfc

#define MIN_FREQ 400000
#define BLOCK_SHIFT			16

#define SDHCI_SPEC_100	0
#define SDHCI_SPEC_200	1
#define SDHCI_SPEC_300	2

#define CONTROL0_HISPEED	(1 << 2)
#define CONTROL0_4DATA		(1 << 1)

#define CONTROL1_DATARST	(1 << 26)
#define CONTROL1_CMDRST		(1 << 25)
#define CONTROL1_HOSTRST	(1 << 24)
#define CONTROL1_CLKSELPROG	(1 << 5)
#define CONTROL1_CLKENA		(1 << 2)
#define CONTROL1_CLK_STABLE	(1 << 1)
#define CONTROL1_INTCLKENA	(1 << 0)
#define CONTROL1_CLKMSB		6
#define CONTROL1_CLKLSB		8
#define CONTROL1_TIMEOUT	(0x0E << 16)

#define MAX_CLK_DIVIDER_V3	2046
#define MAX_CLK_DIVIDER_V2	256

/*this is only for mbox comms*/
#define BCM2835_MBOX_PHYSADDR				0x2000b880
#define BCM2835_MBOX_TAG_GET_CLOCK_RATE		0x00030002
#define BCM2835_MBOX_CLOCK_ID_EMMC			1
#define BCM2835_MBOX_STATUS_WR_FULL			0x80000000
#define BCM2835_MBOX_STATUS_RD_EMPTY		0x40000000
#define BCM2835_MBOX_PROP_CHAN				8
#define BCM2835_MBOX_TAG_VAL_LEN_RESPONSE	0x80000000

struct bcm2835_mbox_regs {
	u32 read;
	u32 rsvd0[5];
	u32 status;
	u32 config;
	u32 write;
};


struct bcm2835_mbox_hdr {
	u32 buf_size;
	u32 code;
};

struct bcm2835_mbox_tag_hdr {
	u32 tag;
	u32 val_buf_size;
	u32 val_len;
};

struct bcm2835_mbox_tag_get_clock_rate {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			u32 clock_id;
		} req;
		struct {
			u32 clock_id;
			u32 rate_hz;
		} resp;
	} body;
};

struct msg_get_clock_rate {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_clock_rate get_clock_rate;
	u32 end_tag;
};

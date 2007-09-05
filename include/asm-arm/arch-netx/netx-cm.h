
#ifndef __AT_CM_USERAREAS_H__
#define __AT_CM_USERAREAS_H__

int netx_cm_init(void);

struct netx_cm_userarea_1 {
        unsigned short signature;             /* configuration block signature */
        unsigned short version;               /* version information */
        unsigned short crc16;                 /* crc16 checksum over all 3 areas, including the reserved blocks */
        unsigned char mac[4][6];              /* mac addresses */
        unsigned char reserved[2];            /* reserved, must be 0 */
};

struct netx_cm_userarea_2 {
        unsigned long sdram_size;              /* sdram size in bytes */
        unsigned long sdram_control;           /* sdram control register value (sdram_general_ctrl) */
        unsigned long sdram_timing;            /* sdram timing register value (sdram_timing_ctrl) */
        unsigned char reserved0[20];           /* reserved, must be 0 */
};

struct netx_cm_userarea_3 {
        unsigned char reserved[32];          /* reserved, must be 0 */
};

struct netx_cm_userarea {
	struct netx_cm_userarea_1 area_1;
	struct netx_cm_userarea_2 area_2;
	struct netx_cm_userarea_3 area_3;
};

#endif  /* __AT_CM_USERAREAS_H__ */

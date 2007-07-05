#ifndef __CFI_FLASH_H
#define __CFI_FLASH_H

struct cfi_platform_data {
        flash_info_t finfo;
};

int flash_init (void);

#endif /* __CFI_FLASH_H */


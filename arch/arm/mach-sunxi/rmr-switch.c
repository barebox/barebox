#include "../include/asm/sysreg.h"
#include "../include/asm/system.h"

#define SUNXI_RVBAR_ADDR 0x017000A0
#define SUNXI_FEL_STASH_ADDR 0x00012000

void to_32bit_sunxi() {

read_sysreg_s(SUNXI_FEL_STASH_ADDR);
}



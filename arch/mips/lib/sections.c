#include <common.h>
#include <asm/sections.h>
#include <linux/types.h>

char _text[0] __attribute__((section("._text")));
char __bss_start[0] __attribute__((section(".__bss_start")));
char __bss_stop[0] __attribute__((section(".__bss_stop")));
char __image_start[0] __attribute__((section(".__image_start")));
char __image_end[0] __attribute__((section(".__image_end")));

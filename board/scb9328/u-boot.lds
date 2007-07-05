
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
 . = 0x00000000;

 . = ALIGN(4);
 .text :
 {
  cpu/arm920t/start.o (.text)
  *(.text)
 }

 . = ALIGN(4);
 .rodata : { *(.rodata) }

 . = ALIGN(4);
 .data : { *(.data) }

 . = ALIGN(4);
 .got : { *(.got) }

 . = .;
 __u_boot_cmd_start = .;
 .u_boot_cmd : { *(.u_boot_cmd) }
 __u_boot_cmd_end = .;

 __u_boot_initcalls_start = .;
 .u_boot_initcalls : { *(.initcall.0) *(.initcall.1) *(.initcall.2) *(.initcall.3) *(.initcall.4) *(.initcall.5) *(.initcall.6) *(.initcall.7) }
 __u_boot_initcalls_end = .;

 . = ALIGN(4);
 __bss_start = .;
 .bss : { *(.bss) }
 _end = .;
}

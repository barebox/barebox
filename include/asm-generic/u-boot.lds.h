
#define INITCALLS			\
  	KEEP(*(.initcall.0))			\
  	KEEP(*(.initcall.1))			\
  	KEEP(*(.initcall.2))			\
  	KEEP(*(.initcall.3))			\
  	KEEP(*(.initcall.4))			\
  	KEEP(*(.initcall.5))			\
  	KEEP(*(.initcall.6))			\
  	KEEP(*(.initcall.7))

#define U_BOOT_CMDS	KEEP(*(SORT_BY_NAME(.u_boot_cmd*)))

#define U_BOOT_SYMS	KEEP(*(__usymtab))

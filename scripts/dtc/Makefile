# scripts/dtc makefile

hostprogs-y	:= dtc fdtget
always		:= $(hostprogs-y)

dtc-objs	:= dtc.o flattree.o fstree.o data.o livetree.o treesource.o \
		   srcpos.o checks.o util.o
dtc-objs	+= dtc-lexer.lex.o dtc-parser.tab.o

libfdt-objs	= fdt.o fdt_ro.o fdt_strerror.o fdt_wip.o fdt_overlay.o
libfdt-objs	+= fdt_empty_tree.o fdt_rw.o fdt_sw.o

fdtget-objs    += fdtget.o $(libfdt-objs) util.o

# Source files need to get at the userspace version of libfdt_env.h to compile

HOSTCFLAGS_DTC := -I$(src)

HOSTCFLAGS_checks.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_data.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_dtc.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_flattree.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fstree.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_livetree.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_srcpos.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_treesource.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_util.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdtget.o := $(HOSTCFLAGS_DTC)

HOSTCFLAGS_fdt.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_ro.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_strerror.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_wip.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_empty_tree.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_rw.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_sw.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_fdt_overlay.o := $(HOSTCFLAGS_DTC)

HOSTCFLAGS_dtc-lexer.lex.o := $(HOSTCFLAGS_DTC)
HOSTCFLAGS_dtc-parser.tab.o := $(HOSTCFLAGS_DTC)

# dependencies on generated files need to be listed explicitly
$(obj)/dtc-lexer.lex.o: $(obj)/dtc-parser.tab.h

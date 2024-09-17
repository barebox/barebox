# SPDX-License-Identifier: GPL-2.0-only

obj-y			+= bcd.o
obj-y			+= term.o
obj-$(CONFIG_BOOTSTRAP)	+= bootstrap/
obj-pbl-y		+= ctype.o
obj-y			+= rbtree.o
obj-y			+= display_options.o
obj-y			+= string.o
obj-$(CONFIG_VERSION_CMP)	+= strverscmp.o
obj-y			+= strtox.o
obj-y			+= kstrtox.o
obj-y			+= vsprintf.o
obj-$(CONFIG_KASAN)	+= kasan/
obj-pbl-$(CONFIG_STACKPROTECTOR)	+= stackprot.o
pbl-$(CONFIG_PBL_CONSOLE) += vsprintf.o
obj-y			+= misc.o
obj-$(CONFIG_PARAMETER)	+= parameter.o
obj-y			+= xfuncs.o
obj-y			+= getopt.o
obj-y			+= readkey.o
obj-y			+= kfifo.o
obj-y			+= libbb.o
obj-y			+= libgen.o
obj-$(CONFIG_JSMN)	+= jsmn.o
obj-$(CONFIG_BLOBGEN)	+= blobgen.o
obj-y			+= stringlist.o
obj-y			+= cmdlinepart.o
obj-y			+= recursive_action.o
obj-y			+= make_directory.o
obj-y			+= arith.o
obj-$(CONFIG_IDR)	+= idr.o
obj-y			+= math/
obj-y			+= uuid.o
obj-$(CONFIG_XXHASH)	+= xxhash.o
obj-$(CONFIG_BZLIB)	+= decompress_bunzip2.o
obj-$(CONFIG_ZLIB)	+= decompress_inflate.o zlib_inflate/
obj-$(CONFIG_XZ_DECOMPRESS) += decompress_unxz.o xz/
obj-$(CONFIG_CMDLINE_EDITING)	+= readline.o
obj-$(CONFIG_SIMPLE_READLINE)	+= readline_simple.o
obj-$(CONFIG_FNMATCH)		+= fnmatch.o
obj-$(CONFIG_GENERIC_FIND_NEXT_BIT) += find_next_bit.o
obj-$(CONFIG_GLOB)	+= glob.o
obj-y			+= notifier.o
obj-y			+= random.o
obj-$(CONFIG_IMAGE_SPARSE) += image-sparse.o
obj-y			+= lzo/
obj-$(CONFIG_LZ4_DECOMPRESS) += lz4/
obj-$(CONFIG_ZSTD_DECOMPRESS) += zstd/
obj-y			+= show_progress.o
obj-$(CONFIG_LZO_DECOMPRESS)		+= decompress_unlzo.o
obj-$(CONFIG_LZ4_DECOMPRESS) += decompress_unlz4.o
obj-$(CONFIG_ZSTD_DECOMPRESS) += decompress_unzstd.o
obj-$(CONFIG_PROCESS_ESCAPE_SEQUENCE)	+= process_escape_sequence.o
obj-$(CONFIG_UNCOMPRESS)	+= uncompress.o
obj-$(CONFIG_BCH)	+= bch.o
obj-$(CONFIG_BASE64)	+= base64.o
obj-$(CONFIG_BITREV)	+= bitrev.o
obj-$(CONFIG_QSORT)	+= qsort.o
obj-$(CONFIG_LIBSCAN)	+= libscan.o
obj-$(CONFIG_LIBUBIGEN)	+= libubigen.o
obj-y			+= gui/
obj-$(CONFIG_XYMODEM)	+= xymodem.o
obj-y			+= unlink-recursive.o
obj-$(CONFIG_STMP_DEVICE) += stmp-device.o
obj-y			+= wchar.o
obj-y			+= libfile.o
obj-y			+= bitmap.o
obj-y			+= gcd.o
obj-y			+= bsearch.o
obj-pbl-y		+= hexdump.o
obj-$(CONFIG_FONTS)	+= fonts/
obj-$(CONFIG_BAREBOX_LOGO)     += logo/
obj-y			+= reed_solomon/
obj-$(CONFIG_RATP)	+= ratp.o
obj-$(CONFIG_DEBUG_LIST) += list_debug.o
obj-y			+= list_sort.o
obj-y			+= refcount.o
obj-y			+= parseopt.o
obj-y			+= clz_ctz.o
obj-$(CONFIG_CRC_CCITT) += crc-ccitt.o
obj-$(CONFIG_CRC8)	+= crc8.o
obj-$(CONFIG_NLS)	+= nls_base.o
obj-$(CONFIG_FSL_QE_FIRMWARE) += fsl-qe-firmware.o
obj-$(CONFIG_UBSAN)	+= ubsan.o
obj-$(CONFIG_GENERIC_ALLOCATOR) += genalloc.o

# GCC library routines
obj-$(CONFIG_GENERIC_LIB_ASHLDI3) += ashldi3.o
obj-$(CONFIG_GENERIC_LIB_ASHRDI3) += ashrdi3.o
obj-$(CONFIG_GENERIC_LIB_LSHRDI3) += lshrdi3.o
obj-$(CONFIG_GENERIC_LIB_MULDI3)  += muldi3.o

pbl-$(CONFIG_GENERIC_LIB_ASHLDI3) += ashldi3.o

UBSAN_SANITIZE_ubsan.o := n
KASAN_SANITIZE_ubsan.o := n
CFLAGS_ubsan.o := -fno-stack-protector

libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
	                      fdt_empty_tree.o
$(foreach file, $(libfdt_files), \
	$(eval CFLAGS_$(file) = -I $(srctree)/scripts/dtc/libfdt) \
	$(eval CFLAGS_$(file:%.o=%.pbl.o) = -I $(srctree)/scripts/dtc/libfdt))

obj-pbl-$(CONFIG_LIBFDT) += $(libfdt_files)

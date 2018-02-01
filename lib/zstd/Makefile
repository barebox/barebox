obj-$(CONFIG_ZSTD_DECOMPRESS) += zstd_decompress.o

ccflags-y += -O3

zstd_decompress-y := huf_decompress.o decompress.o
zstd_decompress-y += entropy_common.o fse_decompress.o zstd_common.o

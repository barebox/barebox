###
# scripts contains sources for various helper programs used throughout
# the kernel for the build process.
# ---------------------------------------------------------------------------
# kallsyms:      Find all symbols in barebox

hostprogs-$(CONFIG_KALLSYMS)     += kallsyms
hostprogs-y                      += bin2c
hostprogs-y                      += mkimage
hostprogs-y                      += bareboxenv
hostprogs-$(CONFIG_ARCH_NETX)    += gen_netx_image
hostprogs-$(CONFIG_ARCH_OMAP)    += omap_signGP

always		:= $(hostprogs-y) $(hostprogs-m)

subdir-y                     += mod

subdir-$(CONFIG_X86)             += setupmbr

# Let clean descend into subdirs
subdir-	+= basic kconfig setupmbr

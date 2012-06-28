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
hostprogs-$(CONFIG_ARCH_S5PCxx)  += s5p_cksum
hostprogs-$(CONFIG_ARCH_DAVINCI) += mkublheader

always		:= $(hostprogs-y) $(hostprogs-m)

subdir-y                     += mod

subdir-$(CONFIG_X86)             += setupmbr

# Let clean descend into subdirs
subdir-	+= basic kconfig setupmbr

quiet_cmd_csingle	= CC      $@
      cmd_csingle	= $(CC) -Wp,-MD,$(depfile) $(CFLAGS) -o $@ $<

obj-$(CONFIG_BAREBOXENV_TARGET)	+= bareboxenv-target

scripts/bareboxenv-target: scripts/bareboxenv.c FORCE
	$(call if_changed_dep,csingle)

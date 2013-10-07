#
# barebox image generation Makefile
#
# This Makefile generates multiple images from a common barebox image
# and different pbl (pre bootloader) images. Optionally the result is
# encapsulated in SoC (or SoC boot type) specific image formats.
#
# The basic idea here is that we generate a single barebox main binary. This
# is compressed and prepended with a self extractor, generated as barebox.x.
# barebox.x is then prepended with different board specific pbls. The pbls
# are generally named after their entrypoints. So a pcm038 specific pbl will
# generate the following files:
#
# start_imx27_pcm038.pbl - The ELF file, linked with the entrypoint start_imx27_pcm038
# start_imx27_pcm038.pblb - The raw binary of the above.
# start_imx27_pcm038.pblx - The pblb appended with barebox.x
# start_imx27_pcm038.pbl.map - The linker map file
# start_imx27_pcm038.pbl.s - the disassembled ELF, generated with:
#                            make images/start_imx27_pcm038.pbl.s
#
# Example Makefile snippets for the i.MX51 babbage board (for readability in opposite
# order):
#
## image-$(CONFIG_MACH_FREESCALE_MX51_PDK) += barebox-imx51-babbage.img
#
# For CONFIG_MACH_FREESCALE_MX51_PDK build barebox-imx51-babbage.img
#
## FILE_barebox-imx51-babbage.img = start_imx51_babbage.pblx.imximg
#
# barebox-imx51-babbage.img should be generated (copied) from
# start_imx51_babbage.pblx.imximg. This copy process is only done so that we
# can generate images with a sane name. So what we really need for this
# board is a i.MX specific image, a .imximg
#
## CFG_start_imx51_babbage.pblx.imximg = $(board)/freescale-mx51-pdk/flash-header.imxcfg
#
# The .imximg can be generated from a .pblx using a rule specified in Makefile.imx.
# The configfile needed for this image is specified with CFG_<filename> = <configfile>
#
## pblx-$(CONFIG_MACH_FREESCALE_MX51_PDK) += start_imx51_babbage
#
# For this image we need a pblx (self extracting barebox binary) with
# start_imx51_babbage as entrypoint. start_imx51_babbage will be used
# both as entrypoint and as filename
#

quiet_cmd_objcopy_bin = OBJCOPYB $@
      cmd_objcopy_bin = $(OBJCOPY) -O binary $(OBJCOPYFLAGS) $(OBJCOPYFLAGS_$(@F)) $< $@

pbl-lds := $(obj)/pbl.lds
extra-y += $(pbl-lds)

$(pbl-lds): $(obj)/../arch/$(ARCH)/lib/pbl.lds.S FORCE
	$(call if_changed_dep,cpp_lds_S)

quiet_cmd_elf__ ?= LD      $@
      cmd_elf__ ?= $(LD) $(LDFLAGS) -static --gc-sections -pie		\
		-e $(2) -Map $@.map $(LDFLAGS_$(@F)) -o $@		\
		-T $(pbl-lds)						\
		--start-group $(barebox-pbl-common) --end-group

PBL_CPPFLAGS	+= -fdata-sections -ffunction-sections

$(obj)/%.pbl: $(pbl-lds) $(barebox-pbl-common) FORCE
	$(call if_changed,elf__,$(*F))

$(obj)/%.pblb: $(obj)/%.pbl FORCE
	$(call if_changed,objcopy_bin,$(*F))

quiet_cmd_pblx ?= PBLX    $@
      cmd_pblx ?= cat $(obj)/$(patsubst %.pblx,%.pblb,$(2)) > $@; \
		  $(call size_append, $(obj)/barebox.x) >> $@; \
		  cat $(obj)/barebox.x >> $@

$(obj)/%.pblx: $(obj)/%.pblb $(obj)/barebox.x FORCE
	$(call if_changed,pblx,$(@F))

$(obj)/%.s: $(obj)/% FORCE
	$(call if_changed,disasm)

suffix_$(CONFIG_IMAGE_COMPRESSION_GZIP) = gzip
suffix_$(CONFIG_IMAGE_COMPRESSION_LZO)  = lzo
suffix_$(CONFIG_IMAGE_COMPRESSION_LZ4)	= lz4
suffix_$(CONFIG_IMAGE_COMPRESSION_NONE) = shipped

# barebox.z - compressed barebox binary
# ----------------------------------------------------------------
$(obj)/barebox.z: $(obj)/../barebox.bin FORCE
	$(call if_changed,$(suffix_y))

quiet_cmd_selfextract = COMP    $@
      cmd_selfextract = cat $(obj)/start_uncompress.pblb > $@; \
			$(call size_append, $<) >> $@; \
			cat $< >> $@

pblx-y += start_uncompress
# barebox.x - self extracting barebox binary
# ----------------------------------------------------------------
$(obj)/barebox.x: $(obj)/barebox.z $(obj)/start_uncompress.pblb FORCE
	$(call if_changed,selfextract)

# %.img - create a copy from another file
# ----------------------------------------------------------------
.SECONDEXPANSION:
$(obj)/%.img: $(obj)/$$(FILE_$$(@F))
	$(Q)if [ -z $(FILE_$(@F)) ]; then echo "FILE_$(@F) empty!"; false; fi
	$(call if_changed,shipped)

include $(srctree)/images/Makefile.imx
include $(srctree)/images/Makefile.mvebu
include $(srctree)/images/Makefile.socfpga
include $(srctree)/images/Makefile.tegra

targets += $(image-y) pbl.lds barebox.x barebox.z
targets += $(patsubst %,%.pblx,$(pblx-y))
targets += $(patsubst %,%.pblb,$(pblx-y))
targets += $(patsubst %,%.pbl,$(pblx-y))
targets += $(patsubst %,%.s,$(pblx-y))
targets += $(foreach m, $(image-y), $(FILE_$(m)))

SECONDARY: $(addprefix $(obj)/,$(targets))

images: $(addprefix $(obj)/, $(image-y)) FORCE
	@echo "images built:"
	@for i in $(image-y); do echo $$i; done

clean-files := *.pbl *.pblb *.pblx *.map start_*.imximg *.img barebox.z start_*.kwbimg \
	start_*.kwbuartimg *.socfpgaimg
clean-files += pbl.lds

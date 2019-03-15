
suffix_$(CONFIG_IMAGE_COMPRESSION_GZIP) = gzip
suffix_$(CONFIG_IMAGE_COMPRESSION_LZO)	= lzo
suffix_$(CONFIG_IMAGE_COMPRESSION_LZ4)	= lz4
suffix_$(CONFIG_IMAGE_COMPRESSION_XZKERN)	= xzkern
suffix_$(CONFIG_IMAGE_COMPRESSION_NONE)	= comp_copy

OBJCOPYFLAGS_zbarebox.bin = -O binary
piggy_o := piggy.$(suffix_y).o

targets := zbarebox.lds zbarebox zbarebox.bin zbarebox.S \
	   $(piggy_o) piggy.$(suffix_y)

# Make sure files are removed during clean
extra-y       += piggy.gzip piggy.lz4 piggy.lzo piggy.lzma piggy.xzkern piggy.shipped zbarebox.map

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
FIX_SIZE=-b
else
FIX_SIZE=
endif

$(obj)/zbarebox.bin:	$(obj)/zbarebox FORCE
	$(call if_changed,objcopy)
	$(call cmd,check_file_size,$@,$(CONFIG_BAREBOX_MAX_IMAGE_SIZE))
	$(Q)$(kecho) '  Barebox: fix size'
	$(Q)$(objtree)/scripts/fix_size -i -f $(objtree)/$@ $(FIX_SIZE)
	$(Q)$(kecho) '  Barebox: $@ is ready'

$(obj)/zbarebox.S: $(obj)/zbarebox FORCE
	$(call if_changed,disasm)

PBL_CPPFLAGS		+= -fdata-sections -ffunction-sections
LDFLAGS_zbarebox	:= -Map $(obj)/zbarebox.map --gc-sections
ifdef CONFIG_PBL_RELOCATABLE
LDFLAGS_zbarebox += -pie
else
LDFLAGS_zbarebox += -static
endif

LDFLAGS_zbarebox += $(call ld-option, --no-dynamic-linker)

zbarebox-common := $(barebox-pbl-common) $(obj)/$(piggy_o)
zbarebox-lds := $(obj)/zbarebox.lds

$(zbarebox-lds): $(obj)/../lib/pbl.lds.S FORCE
	$(call if_changed_dep,cpp_lds_S)

quiet_cmd_zbarebox__ ?= LD      $@
      cmd_zbarebox__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_zbarebox) -o $@ \
      -e pbl_start -T $(zbarebox-lds)                              \
      --start-group $(zbarebox-common) --end-group                 \
      $(filter-out $(zbarebox-lds) $(zbarebox-common) FORCE ,$^)

$(obj)/zbarebox: $(zbarebox-lds) $(zbarebox-common) FORCE
	$(call if_changed,zbarebox__)

$(obj)/piggy.$(suffix_y): $(obj)/../../../barebox.bin FORCE
	$(call if_changed,$(suffix_y))

$(obj)/$(piggy_o): $(obj)/piggy.$(suffix_y) FORCE

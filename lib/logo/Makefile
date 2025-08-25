# SPDX-License-Identifier: GPL-2.0-only

OPTS_barebox-logo-w64 = -resize 64
bblogo-$(CONFIG_BAREBOX_LOGO_64) += barebox-logo-w64

OPTS_barebox-logo-w240 = -resize 240
bblogo-$(CONFIG_BAREBOX_LOGO_240) += barebox-logo-w240

OPTS_barebox-logo-w320 = -resize 320
bblogo-$(CONFIG_BAREBOX_LOGO_320) += barebox-logo-w320

OPTS_barebox-logo-w400 = -resize 400
bblogo-$(CONFIG_BAREBOX_LOGO_400) += barebox-logo-w400

OPTS_barebox-logo-w640 = -resize 640
bblogo-$(CONFIG_BAREBOX_LOGO_640) += barebox-logo-w640

obj-y += $(patsubst %,%.bblogo.o,$(bblogo-y))
extra-y += $(patsubst %,%.bblogo,$(bblogo-y))
extra-y += $(patsubst %,%.bblogo.S,$(bblogo-y))
extra-y += $(patsubst %,%.bblogo.o,$(bblogo-y))

obj-$(CONFIG_BAREBOX_LOGO) += logo.o

quiet_cmd_logo_S = LOGO.S  $@
cmd_logo_S =							\
(								\
	echo '\#include <asm/barebox.lds.h>';			\
	echo '.section .note.GNU-stack,"",%progbits';		\
	echo '.section .bblogo.rodata.$(subst -,_,$(*F)),"a"';	\
	echo '.balign STRUCT_ALIGNMENT';			\
	echo '.global __bblogo_$(subst -,_,$(*F))_start';	\
	echo '__bblogo_$(subst -,_,$(*F))_start:';		\
	echo '.incbin "$<" ';					\
	echo '__bblogo_$(subst -,_,$(*F))_end:';		\
	echo '.global __bblogo_$(subst -,_,$(*F))_end';		\
	echo '.balign STRUCT_ALIGNMENT';			\
) > $@

%.bblogo.S: %.bblogo FORCE
	$(call if_changed,logo_S)

quiet_cmd_logo = LOGO     $@
      cmd_logo = cp $< $@

ifdef CONFIG_BAREBOX_LOGO_PNG
extra-y += $(patsubst %,%.png,$(bblogo-y))
%.bblogo: %.png FORCE
	$(call if_changed,logo)
endif
ifdef CONFIG_BAREBOX_LOGO_BMP
extra-y += $(patsubst %,%.bmp,$(bblogo-y))
%.bblogo: %.bmp FORCE
	$(call if_changed,logo)
endif
ifdef CONFIG_BAREBOX_LOGO_QOI
extra-y += $(patsubst %,%.png,$(bblogo-y))
extra-y += $(patsubst %,%.qoi,$(bblogo-y))
%.bblogo: %.qoi FORCE
	$(call if_changed,logo)
endif

CONVERTOPTS += -background none

quiet_cmd_bmp = BMP     $@
      cmd_bmp = convert $(OPTS_$(@F:.bmp=)) $(CONVERTOPTS) $< bmp:$@

%.bmp: $(srctree)/Documentation/barebox.svg FORCE
	$(call if_changed,bmp)

# export SELF_CALL=xxx works around issue in [1]. This is seen on Debian Trixie
# with Inkscape 1.4 (e7c3feb100, 2024-10-09)
# [1] https://gitlab.com/inkscape/inkscape/-/issues/4716
quiet_cmd_png = PNG     $@
      cmd_png =	export SELF_CALL=xxx ; convert $(OPTS_$(@F:.png=)) $(CONVERTOPTS) $< png:$@

%.png: $(srctree)/Documentation/barebox.svg FORCE
	$(call if_changed,png)

quiet_cmd_qoi = QOI     $@
      cmd_qoi =	$(objtree)/scripts/qoiconv $< $@

%.qoi: %.png FORCE
	$(call if_changed,qoi)

clean-files += *.png *.qoi *.bmp *.bblogo *.bblogo.S

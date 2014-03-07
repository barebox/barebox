dtb-$(CONFIG_ARCH_AM33XX) += \
	am335x-bone.dtb \
	am335x-boneblack.dtb \
	am335x-phytec-phycore.dtb
dtb-$(CONFIG_ARCH_IMX25) += imx25-karo-tx25.dtb
dtb-$(CONFIG_ARCH_IMX27) += imx27-phytec-phycard-s-rdk.dtb \
	imx27-phytec-phycard-s-som.dtb
dtb-$(CONFIG_ARCH_IMX51) += imx51-babbage.dtb \
	imx51-genesi-efika-sb.dtb
dtb-$(CONFIG_ARCH_IMX53) += imx53-mba53.dtb \
	imx53-qsb.dtb \
	imx53-qsrb.dtb \
	imx53-voipac-bsb.dtb
dtb-$(CONFIG_ARCH_IMX6) += imx6q-gk802.dtb \
	imx6dl-dfi-fs700-m60-6s.dtb \
	imx6q-dfi-fs700-m60-6q.dtb \
	imx6q-dmo-edmqmx6.dtb \
	imx6q-sabrelite.dtb \
	imx6dl-sabrelite.dtb \
	imx6q-sabresd.dtb \
	imx6dl-mba6x.dtb \
	imx6q-mba6x.dtb \
	imx6q-phytec-pbab01.dtb \
	imx6dl-hummingboard.dtb \
	imx6q-guf-santaro.dtb \
	imx6q-nitrogen6x.dtb \
	imx6dl-nitrogen6x.dtb \
	imx6q-udoo.dtb
dtb-$(CONFIG_ARCH_MVEBU) += dove-cubox-bb.dtb
dtb-$(CONFIG_ARCH_SOCFPGA) += socfpga_cyclone5_sockit.dtb \
	socfpga_cyclone5_socrates.dtb
dtb-$(CONFIG_ARCH_TEGRA) += \
	tegra20-colibri-iris.dtb \
	tegra20-paz00.dtb

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

pbl-$(CONFIG_MACH_BEAGLEBONE) += am335x-bone.dtb.o am335x-boneblack.dtb.o
pbl-$(CONFIG_MACH_EFIKA_MX_SMARTBOOK) += imx51-genesi-efika-sb.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX51_PDK) += imx51-babbage.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX53_LOCO) += imx53-qsb.dtb.o imx53-qsrb.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX53_VMX53) += imx53-voipac-bsb.dtb.o
pbl-$(CONFIG_MACH_DFI_FS700_M60) += imx6q-dfi-fs700-m60-6q.dtb.o imx6dl-dfi-fs700-m60-6s.dtb.o
pbl-$(CONFIG_MACH_PCM051) += am335x-phytec-phycore.dtb.o
pbl-$(CONFIG_MACH_PHYTEC_PFLA02) += imx6q-phytec-pbab01.dtb.o
pbl-$(CONFIG_MACH_REALQ7) += imx6q-dmo-edmqmx6.dtb.o
pbl-$(CONFIG_MACH_SOLIDRUN_CUBOX) += dove-cubox-bb.dtb.o
pbl-$(CONFIG_MACH_GK802) += imx6q-gk802.dtb.o
pbl-$(CONFIG_MACH_TORADEX_COLIBRI_T20_IRIS) += tegra20-colibri-iris.dtb.o
pbl-$(CONFIG_MACH_TOSHIBA_AC100) += tegra20-paz00.dtb.o
pbl-$(CONFIG_MACH_TQMA53) += imx53-mba53.dtb.o
pbl-$(CONFIG_MACH_TQMA6X) += imx6dl-mba6x.dtb.o imx6q-mba6x.dtb.o
pbl-$(CONFIG_MACH_TX25) += imx25-karo-tx25.dtb.o
pbl-$(CONFIG_MACH_SOCFPGA_EBV_SOCRATES) += socfpga_cyclone5_socrates.dtb.o
pbl-$(CONFIG_MACH_SOCFPGA_TERASIC_SOCKIT) += socfpga_cyclone5_sockit.dtb.o
pbl-$(CONFIG_MACH_SOLIDRUN_HUMMINGBOARD) += imx6dl-hummingboard.dtb.o
pbl-$(CONFIG_MACH_SABRELITE) += imx6q-sabrelite.dtb.o imx6dl-sabrelite.dtb.o
pbl-$(CONFIG_MACH_SABRESD) += imx6q-sabresd.dtb.o
pbl-$(CONFIG_MACH_GUF_SANTARO) += imx6q-guf-santaro.dtb.o
pbl-$(CONFIG_MACH_NITROGEN6X) += imx6q-nitrogen6x.dtb.o imx6dl-nitrogen6x.dtb.o
pbl-$(CONFIG_MACH_UDOO) += imx6q-udoo.dtb.o

.SECONDARY: $(obj)/$(BUILTIN_DTB).dtb.S
.SECONDARY: $(patsubst %,$(obj)/%.S,$(dtb-y))

targets += dtbs
targets += $(dtb-y)

extra-y += $(dtb-y)

clean-files := *.dtb *.dtb.S

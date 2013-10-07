dtb-$(CONFIG_ARCH_IMX51) += imx51-babbage.dtb \
	imx51-genesi-efika-sb.dtb
dtb-$(CONFIG_ARCH_IMX53) += imx53-qsb.dtb
dtb-$(CONFIG_ARCH_IMX6) += imx6q-gk802.dtb \
	imx6dl-dfi-fs700-m60-6s.dtb \
	imx6q-dfi-fs700-m60-6q.dtb \
	imx6q-dmo-realq7.dtb \
	imx6q-sabrelite.dtb \
	imx6q-sabresd.dtb \
	imx6dl-mba6x.dtb \
	imx6q-mba6x.dtb \
	imx6q-phytec-pbab01.dtb
dtb-$(CONFIG_ARCH_MVEBU) += dove-cubox.dtb
dtb-$(CONFIG_ARCH_SOCFPGA) += socfpga_cyclone5_sockit.dtb \
	socfpga_cyclone5_socrates.dtb
dtb-$(CONFIG_ARCH_TEGRA) += \
	tegra20-colibri-iris.dtb \
	tegra20-paz00.dtb

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

pbl-$(CONFIG_MACH_EFIKA_MX_SMARTBOOK) += imx51-genesi-efika-sb.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX51_PDK) += imx51-babbage.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX53_LOCO) += imx53-qsb.dtb.o
pbl-$(CONFIG_MACH_DFI_FS700_M60) += imx6q-dfi-fs700-m60-6q.dtb.o imx6dl-dfi-fs700-m60-6s.dtb.o
pbl-$(CONFIG_MACH_PHYTEC_PFLA02) += imx6q-phytec-pbab01.dtb.o
pbl-$(CONFIG_MACH_REALQ7) += imx6q-dmo-realq7.dtb.o
pbl-$(CONFIG_MACH_SOLIDRUN_CUBOX) += dove-cubox.dtb.o
pbl-$(CONFIG_MACH_GK802) += imx6q-gk802.dtb.o
pbl-$(CONFIG_MACH_TORADEX_COLIBRI_T20_IRIS) += tegra20-colibri-iris.dtb.o
pbl-$(CONFIG_MACH_TOSHIBA_AC100) += tegra20-paz00.dtb.o
pbl-$(CONFIG_MACH_TQMA6X) += imx6dl-mba6x.dtb.o imx6q-mba6x.dtb.o
pbl-$(CONFIG_MACH_SOCFPGA_EBV_SOCRATES) += socfpga_cyclone5_socrates.dtb.o
pbl-$(CONFIG_MACH_SOCFPGA_TERASIC_SOCKIT) += socfpga_cyclone5_sockit.dtb.o

.SECONDARY: $(obj)/$(BUILTIN_DTB).dtb.S
.SECONDARY: $(patsubst %,$(obj)/%.S,$(dtb-y))

targets += dtbs
targets += $(dtb-y)

extra-y += $(dtb-y)

clean-files := *.dtb *.dtb.S

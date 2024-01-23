#!/bin/sh
#
# Generate dummy firmware files for compile tests
#

FW_NXP_LPDDR4="
	firmware/lpddr4_pmu_train_1d_dmem.bin
	firmware/lpddr4_pmu_train_1d_imem.bin
	firmware/lpddr4_pmu_train_2d_dmem.bin
	firmware/lpddr4_pmu_train_2d_imem.bin
"

FW_NXP_DDR4="
	firmware/ddr4_dmem_1d.bin
	firmware/ddr4_dmem_2d.bin
	firmware/ddr4_imem_1d.bin
	firmware/ddr4_imem_2d.bin
"

FW_BL31="
	firmware/imx8mm-bl31.bin
	firmware/imx8mn-bl31.bin
	firmware/imx8mp-bl31.bin
	firmware/imx8mq-bl31.bin
	firmware/rk3568-bl31.bin
	firmware/rk3588-bl31.bin
	firmware/ls1028a-bl31.bin
"

FW_ROCKCHIP_SDRAM_INIT="
	arch/arm/boards/pine64-quartz64/sdram-init.bin
	arch/arm/boards/radxa-rock3/sdram-init.bin
	arch/arm/boards/rockchip-rk3568-bpi-r2pro/sdram-init.bin
	arch/arm/boards/rockchip-rk3568-evb/sdram-init.bin
	arch/arm/boards/radxa-rock5/sdram-init.bin
"

FW_MVEBU_BINARY0="
	arch/arm/boards/globalscale-mirabox/binary.0
	arch/arm/boards/lenovo-ix4-300d/binary.0
	arch/arm/boards/marvell-armada-xp-db/binary.0
	arch/arm/boards/marvell-armada-xp-gp/binary.0
	arch/arm/boards/netgear-rn104/binary.0
	arch/arm/boards/netgear-rn2120/binary.0
	arch/arm/boards/plathome-openblocks-ax3/binary.0
	arch/arm/boards/turris-omnia/binary.0
"

FW_NXP_LAYERSCAPE="
	firmware/fsl_fman_ucode_ls1046_r1.0_106_4_18.bin
	firmware/ppa-ls1046a.bin
"

FW="
	$FW_NXP_LPDDR4
	$FW_NXP_DDR4
	$FW_BL31
	$FW_ROCKCHIP_SDRAM_INIT
	$FW_MVEBU_BINARY0
	$FW_NXP_LAYERSCAPE
"

for i in $FW; do
	mkdir -p $(dirname $i)
	echo "Dummy firmware generated for $i" > $i
done

NXP i.MX8MN EVK Evaluation Board
================================

Board comes with either:

* 2GiB of LPDDR4 RAM
* 2GiB of DDR4 RAM

barebox supports both variants with the same image.

Downloading DDR PHY Firmware
----------------------------

As a part of DDR intialization routine NXP i.MX8MN EVK requires and
uses several binary firmware blobs that are distributed under a
separate EULA and cannot be included in Barebox. In order to obtain
them do the following::

 wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-8.12.bin
 chmod +x firmware-imx-8.12.bin
 ./firmware-imx-8.12.bin

Executing that file should produce a EULA acceptance dialog as well as
result in the following files:

- lpddr4_pmu_train_1d_dmem.bin
- lpddr4_pmu_train_1d_imem.bin
- lpddr4_pmu_train_2d_dmem.bin
- lpddr4_pmu_train_2d_imem.bin
- ddr4_dmem_1d_201810.bin
- ddr4_imem_1d_201810.bin
- ddr4_dmem_2d_201810.bin
- ddr4_imem_2d_201810.bin

As a last step of this process those files need to be placed in
"firmware/"::

  for f in lpddr4_pmu_train_1d_dmem.bin  \
           lpddr4_pmu_train_1d_imem.bin  \
	   lpddr4_pmu_train_2d_dmem.bin  \
	   lpddr4_pmu_train_2d_imem.bin; \
  do \
	   cp firmware-imx-8.0/firmware/ddr/synopsys/${f} \
	      firmware/${f}; \
  done

  for f in ddr4_dmem_1d_201810.bin  \
           ddr4_imem_1d_201810.bin  \
           ddr4_dmem_2d_201810.bin  \
           ddr4_imem_2d_201810.bin; \
  do \
	   cp firmware-imx-8.0/firmware/ddr/synopsys/${f} \
	      firmware/${f%_201810.bin}.bin; \
  done

Build barebox
=============

 make imx_v8_defconfig
 make

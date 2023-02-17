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
	   cp firmware-imx-8.12/firmware/ddr/synopsys/${f} \
	      firmware/${f}; \
  done

  for f in ddr4_dmem_1d_201810.bin  \
           ddr4_imem_1d_201810.bin  \
           ddr4_dmem_2d_201810.bin  \
           ddr4_imem_2d_201810.bin; \
  do \
	   cp firmware-imx-8.12/firmware/ddr/synopsys/${f} \
	      firmware/${f%_201810.bin}.bin; \
  done

Build barebox
=============

 make imx_v8_defconfig
 make

Installing barebox
==================

When the EVK is strapped to boot from eMMC, the i.MX8M bootrom will
consult the eMMC ext_csd register to determine whether to boot
from the active eMMC boot partition or from the user area.

The same barebox image written to the start of the SD-Card can
be written to the start of the eMMC user area. Power-fail-safe
installation to the eMMC boot partition requires special handling:

  - The barebox image must be written to the inactive boot partition,
    then afterwards, the newly written boot partition is activated
    (This is controlled by the barebox ``mmcX.boot`` variable).

  - The barebox image includes a 32KiB preamble that allows the image
    to be directly writable to the start of the SD-Card or eMMC user area.
    Unlike older i.MX8M, the i.MX8MN BootROM expects the bootloader to not
    start at an offset when booting from eMMC boot partitions, thus the first
    32KiB must be stripped.

The ``barebox_update`` command takes care of this and need just be
supplied a barebox image as argument.

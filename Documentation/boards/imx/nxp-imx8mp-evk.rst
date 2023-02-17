NXP i.MX8MP-EVK board
=====================

The board comes with:

* 6GiB of LPDDR4 RAM
* 32GiB eMMC

Not including booting via serial, the device can boot from either SD or eMMC.

Downloading DDR PHY firmware
----------------------------

As a part of DDR intialization routine NXP i.MX8MP EVK requires and
uses several binary firmware blobs that are distributed under a
separate EULA and cannot be included in Barebox. In order to obtain
them do the following::

 wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-8.7.bin
 chmod +x firmware-imx-8.7.bin
 ./firmware-imx-8.7.bin

Executing that file should produce a EULA acceptance dialog as well as
result in the following files:

- lpddr4_pmu_train_1d_dmem.bin
- lpddr4_pmu_train_1d_imem.bin
- lpddr4_pmu_train_2d_dmem.bin
- lpddr4_pmu_train_2d_imem.bin

As a last step of this process those files need to be placed in
"firmware/"::

  for f in lpddr4_pmu_train_1d_dmem.bin  \
           lpddr4_pmu_train_1d_imem.bin  \
	   lpddr4_pmu_train_2d_dmem.bin  \
	   lpddr4_pmu_train_2d_imem.bin; \
  do \
	   cp firmware-imx-8.7/firmware/ddr/synopsys/${f} \
	      firmware/${f}; \
  done

Get and Build the Trusted Firmware A
------------------------------------

Get TF-A from https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git/ and
checkout version v2.7::

  make PLAT=imx8mp bl31
  cp build/imx8mp/release/bl31.bin ${barebox_srctree}/imx8mp-bl31.bin

.. warning:: It is important to use a version >= v2.7 else your system
   might not boot.

Build Barebox
-------------

i.MX8MP-EVK support is contained in the imx_v8_defconfig to build it use::

  make imx_v8_defconfig
  make

Boot Configuration
------------------

The NXP i.MX8MP-EVK board has four switches responsible for configuring
bootsource/boot mode. The settings for the different boot sources are
printed on the board.

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
    Unlike older i.MX8M, the i.MX8MP BootROM expects the bootloader to not
    start at an offset when booting from eMMC boot partitions, thus the first
    32KiB must be stripped.

The ``barebox_update`` command takes care of this and need just be
supplied a barebox image as argument.

NXP i.MX8MP-EVK board
=====================

The board comes with:

* 6GiB of LPDDR4 RAM
* 32GiB eMMC

Not including booting via serial, the device can boot from either SD or eMMC.

Downloading DDR PHY firmware
----------------------------

As a part of DDR intialization routine NXP i.MX8MQ EVK requires and
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

Get and Build the ARM Trusted firmware
--------------------------------------

Get ATF from https://source.codeaurora.org/external/imx/imx-atf, branch
imx_5.4.3_2.0.0::

  make PLAT=imx8mp bl31
  cp build/imx8mp/release/bl31.bin ${barebox_srctree}/imx8mp-bl31.bin

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

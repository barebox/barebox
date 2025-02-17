.. SPDX-License-Identifier: GPL-2.0+

Allwinner sunxi
===============

Boot process
------------

On power-up Allwinner SoC starts in boot ROM, aka BROM, which will search
for a bootable image (eGON header): first from the SD card, then from eMMC.
If no image is found then the boot ROM will enter into FEL mode that can be
used for programming and recovery through USB.

Some board may have a button to enter FEL mode at startup. If not, another
way to enter FEL mode is to not have a valid bootable eGON image, this can
be achieved by erasing existing eGON image headers.

.. note::

   Currently Barebox cannot boot directly on Allwinner sunxi SoC and can
   only be used as a secondary bootloader, requiring u-boot to initialize
   the SDRAM controller and starting the TF-A and optionally Crust.


Building barebox second stage
-----------------------------

The generic ``barebox-dt-2nd.img`` armv8 image should enable all the needed
drivers.

To build the image do:

.. code-block:: sh

  export ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
  make multi_v8_defconfig
  make

Installing barebox second stage
-------------------------------

Copy the built ``barebox-dt-2nd.img`` image into the first fat partition
of the boot medium.

.. code-block:: sh

  # mount /dev/sdX1 /mnt/${mount_dir}
  # cp ${build_dir}/images/barebox-dt-2nd.img /mnt/${mount_dir}
  # umount /mnt/${mount_dir}


Booting barebox from U-Boot
---------------------------

Currently Barebox can only be started as a second stage, and relies on
U-Boot for the SDRAM initialisation and to start the TF-A.

.. code-block:: console

  # on u-boot console, `mmc 0:1` is the first partition on the sd card
  fatload mmc 0:1 0x42000000 barebox-dt-2nd.img
  fatload mmc 0:1 0x44000000 dtbs-lts/allwinner/sun50i-a64-pine64-plus.dtb
  booti 0x42000000 - 0x44000000

.. note::

   A FIT image might be built and used instead of directly using the `barebox-dt-2nd.img`,
   this hasn't been tested yet, but using a FIT image sound like a better option.

See also the general documentation on :ref:`second_stage`.

Building Arm Trusted Firmware (TF-A)
------------------------------------

.. note::

   This step is currently only needed when building U-Boot.
   This step is also documented in U-Boot documention.

Boards using a 64-bit Soc (A64, H5, H6, H616, R329) require the BL31 stage of
the Arm Trusted Firmware-A firmware. This provides the reference
implementation of secure software for Armv8-A, offering PSCI and SMCCC
services. Allwinner support is fully mainlined. To build bl31.bin:

.. code-block:: sh

  git clone https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git
  cd trusted-firmware-a
  make CROSS_COMPILE=aarch64-linux-gnu- PLAT=sun50i_a64 DEBUG=1
  cp build/sun50i_a64/debug/bl31.bin ${barebox_dir}/firmware/sun50i-a64-bl31.bin

The target platform (``PLAT=``) for A64 and H5 SoCs is sun50i_a64, for the H6
sun50i_h6, for the H616 sun50i_h616, and for the R329 sun50i_r329.

Building U-Boot image
---------------------

Please refere to the relevant U-Boot_ build documentation (or ``/doc/board/allwinner/sunxi.rst`` in u-boot src tree).

.. _U-Boot: https://docs.u-boot.org/en/latest/board/allwinner/sunxi.html#building-the-u-boot-image


Installing the first stage bootloader
-------------------------------------

Installing on a (micro-) SD card
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The bootable image, a eGON header followed by the actual image, must be
located at the fixed offset of 8192 bytes (8KB) from the start of the
disk (sector 16).

.. code-block:: sh

  # copy the bootable fsbl image (including eGON header) into disk sdX
  dd if=${fsbl.img} of=/dev/sdX bs=512 seek=16

QNAP TS-433 NAS
===============

barebox has support for TS-433 and TS-433eU. Further variants likely need
only the appropriate device tree compiled into barebox.

Building
--------

The build process needs two binary files which have to be copied from the
`rkbin https://github.com/rockchip-linux/rkbin` repository to the barebox source tree:

.. code-block:: sh

  cp $RKBIN/bin/rk35/rk3568_bl31_v1.45.elf firmware/rk3568-bl31.bin
  cp $RKBIN/bin/rk35/rk3568_ddr_1560MHz_v1.23.bin arch/arm/boards/qnap-tsx33/sdram-init.bin

With these barebox can be compiled as:

.. code-block:: sh

  make ARCH=arm rockchip_v8_defconfig
  make ARCH=arm

If cross-compiling, ``CROSS_COMPILE`` needs to be additionally set.

Flashing via USB
----------------

The front USB port can be used to bootstrap the device over
the maskrom protocol:

- start with a completely powered off machine
- remove at least the two left harddisk trays
- put a jumper on the 2-pin MaskROM header
- power on
- remove the jumper as it shorts the eMMC
- connect your PC and the front USB jack using a USB-A to USB-A cable

Afterwards, you should see the device with ``lsusb`` and be able to
load barebox into RAM and flash it to the eMMC using ``fastboot``:

.. code-block:: sh

  scripts/rk-usb-loader images/barebox-qnap-ts433.img # or *ts433eu.img
  fastboot flash bbu-emmc barebox-qnap-ts433.img

Booting Debian
--------------

Refer to https://wiki.debian.org/InstallingDebianOn/Qnap/TS-433
for information on how to install Debian.

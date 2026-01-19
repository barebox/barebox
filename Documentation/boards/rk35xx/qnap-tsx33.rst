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

Alternatively, if you enable barebox to boot an OS image with
UEFI, use:

.. code-block:: sh

  make ARCH=arm rockchip_v8_efiloader_defconfig
  make ARCH=arm

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

  scripts/rk-usb-loader images/barebox-qnap-ts433.img # or *-eu.img
  fastboot flash bbu-emmc barebox-qnap-ts433.img

Repartitioning and environment partition
----------------------------------------

If you are wiping the disk anyway to repartition it to fit the distro
that's going to be installed, it's recommended to add a barebox environment
partition for holding persistent bootloader configuration:

.. warning:: This will delete the vendor system on the eMMC!

.. code-block:: sh

   fastboot erase emmc     # wipes full eMMC including barebox!
   fastboot oem exec "createnv -f /dev/mmc0"
   fastboot flash bbu-emmc barebox-qnap-ts433.img # or *-eu.img

Known issues
------------

- eMMC can't be operated at HS200 under Linux, when it should be possible.
- eMMC accesses times out occasionally under barebox. If an error message
  is reported during flashing, retry the operation.
- second USB port doesn't work in barebox
- Having multiple images per variant could be avoided by having barebox
  read the EEPROM early enough to determine correct device tree

Booting Debian
--------------

Flash the Debian netinstall image to a USB stick and insert it into
the (first) front USB and power on the device after having flashed
barebox to the eMMC as per the previous section.

It should then boot into a GRUB menu and from there into the
Debian installer.

When the installer asks you at the end about whether to also install
GRUB to the removable media path, say **yes**.

After installation is done, remove the USB drive and the system
will automatically boot from the Debian system installed to the
eMMC.

Refer also to https://wiki.debian.org/InstallingDebianOn/Qnap/TS-433
for more information on how to install Debian.

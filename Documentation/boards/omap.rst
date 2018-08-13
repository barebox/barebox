Texas Instruments OMAP
======================

Texas Instruments OMAP SoCs have a two-stage boot process. The first stage is
known as Xload which only loads the second stage bootloader. barebox can act as
both the first and the second stage loader. To build as a first stage loader,
build the \*_xload_defconfig for your board; for second stage, build the normal
\*_defconfig for your board.

Bootstrapping a PandaBoard
--------------------------

The PandaBoard boots from SD card. The OMAP Boot ROM code loads a file named
'MLO' on a bootable FAT partition on this card. There are several howtos and
scripts on the net which describe how to prepare such a card (it needs
special partitioning). The same procedure can be used for barebox. With such a
card (assumed to be at /dev/sdc), the following can be used to build and install
barebox:

.. code-block:: console

  # mount -t fat /dev/sdc1 /mnt
  # make panda_xload_defconfig
  # make
  # cp barebox.bin.ift /mnt/MLO
  # make panda_defconfig
  # make
  # cp barebox.bin /mnt/barebox.bin
  # umount /mnt

Bootstrapping a BeagleBoard is the same with the corresponding BeagleBoard defconfigs.

Networking
----------

The original BeagleBoard does not have Ethernet (the newer BeagleBoard-xM does),
but a USB Ethernet dongle can be used for networking. The PandaBoard has an
integrated USB Ethernet converter which behaves exactly like an external dongle.
Barebox does not automatically detect USB devices as this would have bad effects
on boot time when USB is not needed.
So you have to use the [[commands:usb|usb]] command to trigger USB detection.
After this a network device should be present which can be used with the normal
[[commands:dhcp|dhcp]] and [[commands:tftp|tftp]] commands.

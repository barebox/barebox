Texas Instruments OMAP
======================

Texas Instruments OMAP SoCs have a two-stage boot process. The first stage is
known as Xload which only loads the second stage bootloader. barebox can act as
both the first and the second stage loader. To build it as a first stage
loader, enable ``CONFIG_OMAP_BUILD_IFT`` in the configuration; without this
option, the normal second stage images are built.

Bootstrapping a BeagleBoard
---------------------------

The BeagleBoard boots from SD card. The OMAP Boot ROM code loads a file named
'MLO' on a bootable FAT partition on this card. There are several howtos and
scripts on the net which describe how to prepare such a card (it needs
special partitioning). The same procedure can be used for barebox. With such a
card (assumed to be at /dev/sdc), the following can be used to build and
install barebox. barebox has to be built twice, once with
``CONFIG_OMAP_BUILD_IFT`` enabled for the first stage and once without it for
the second stage:

.. code-block:: console

  # mount -t fat /dev/sdc1 /mnt
  # make omap_defconfig
  # ./scripts/config --enable OMAP_BUILD_IFT
  # make olddefconfig
  # make
  # cp images/barebox-beagleboard-mlo.img /mnt/MLO
  # make omap_defconfig
  # make
  # cp images/barebox-beagleboard.img /mnt/barebox.bin
  # umount /mnt

Networking
----------

The original BeagleBoard does not have Ethernet (the newer BeagleBoard-xM does),
but a USB Ethernet dongle can be used for networking. The PandaBoard has an
integrated USB Ethernet converter which behaves exactly like an external dongle.
Barebox does not automatically detect USB devices as this would have bad effects
on boot time when USB is not needed.
So you have to use the :ref:`usb <command_usb>` command to trigger USB detection.
After this a network device should be present which can be used with the normal
:ref:`dhcp <command_dhcp>` and :ref:`tftp <command_tftp>` commands.

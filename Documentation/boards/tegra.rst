.. highlight:: console

Nvidia Tegra
============

Building
--------

All currently supported Tegra boards are integrated in a single
multi-image build (:ref:`multi_image`). Building is as easy as typing:

.. code-block:: sh

  make ARCH=arm tegra_v7_defconfig
  make ARCH=arm CROSS_COMPILE=arm-v7-compiler-

**NOTE:** replace the arm-v7-compiler- with your ARM v7 cross compiler.

Tegra images are specific to the bootsource. The build will generate images for
all combinations of bootsources and supported boards. You can find the
completed images in the ``images/`` subdirectory.

The naming scheme consists of the triplet tegracodename-boardname-bootsource

Kickstarting a board using USB
------------------------------

The tool needed to transfer and start a bootloader image to any Tegra board
using the USB boot mode is called TegraRCM. Most likely this isn't available
from your distributions repositories. You can get and install it by running the
following commands:

.. code-block:: sh

  git clone https://github.com/NVIDIA/tegrarcm.git
  cd tegrarcm
  ./autogen.sh
  make
  sudo make install

Connect the board to your host via the USB OTG port.
The next step is to bring the device into USB boot mode. On developer boards
this could normally be done by holding down a force recovery button (or setting
some jumper) while resetting the board. On other devices you are on your own
finding out how to achieve this.

The tegrarcm tool has 3 basic options:

.. code-block:: none

  --bct        : the BCT file needed for basic hardware init,
                 this can be found in the respective board directory
  --bootloader : the actual barebox image
                 use the -usbloader image
  --loadaddr   : start address of the barebox image
                 use 0x00108000 for Tegra20 aka Tegra2 devices
                 use 0x80108000 for all other Tegra devices

An example command line for the NVIDIA Beaver board looks like this:

.. code-block:: sh

  tegrarcm --bct arch/arm/boards/nvidia-beaver/beaver-2gb-emmc.bct \
           --bootloader images/barebox-tegra30-nvidia-beaver-usbloader.img \
           --loadaddr 0x80108000

You should now see barebox coming up on the serial console.

Writing barebox to the primary boot device
------------------------------------------

**NOTE:** this may change in the near future to work with the standard
barebox update mechanism (:ref:`update`).

Copy the image corresponding to the primary boot device for your board to a
SD-card and plug it into your board.

Within the barebox shell use the standard mount and cp commands to copy the
image to the boot device.

On the NVIDIA Beaver board this looks like this:

.. code-block:: sh

  barebox@NVIDIA Tegra30 Beaver evaluation board:/ mount -a
  mci0: detected SD card version 2.0
  mci0: registered disk0
  mci1: detected MMC card version 4.65
  mci1: registered disk1.boot0
  mci1: registered disk1.boot1
  mci1: registered disk1
  ext4 ext40: EXT2 rev 1, inode_size 128
  ext4 ext41: EXT2 rev 1, inode_size 256
  ext4 ext42: EXT2 rev 1, inode_size 256
  none on / type ramfs
  none on /dev type devfs
  /dev/disk0.0 on /mnt/disk0.0 type ext4
  /dev/disk0.1 on /mnt/disk0.1 type ext4
  /dev/disk1.1 on /mnt/disk1.1 type ext4
  barebox@NVIDIA Tegra30 Beaver evaluation board:/ cp /mnt/disk0.0/barebox-tegra30-nvidia-beaver-emmc.img /dev/disk1.boot0

That's it: barebox should come up after resetting the board.

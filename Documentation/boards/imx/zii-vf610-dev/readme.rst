ZII VF610 Based Boards
======================

Building Barebox
----------------

To build Barebox for ZII VF610 based boards do the following:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> mrproper
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> zii_vf610_dev_defconfig
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix>

Uploading Barebox via JTAG
--------------------------

Barebox can be bootstrapped via JTAG using OpenOCD (latest master) as
follows:

.. code-block:: sh

  cd barebox
  Documentation/boards/imx/zii-vf610-dev/bootstrap.sh

A custom OpenOCD binary and options can be specified as follows:

.. code-block:: sh

  OPENOCD="../openocd/src/openocd -s ../openocd/tcl " \
    Documentation/boards/imx/zii-vf610-dev/bootstrap.sh

Writing Barebox to NVM
----------------------

With exception of Dev boards, all of ZII's VF610 based boards should
come with eMMC. To permanently write Barebox to it do:

.. code-block:: sh

  barebox_update -t eMMC -y barebox.img

This should also automatically configure your board to boot that
image. Note that the original ZII stack's bootloader in eMMC should be
left intact. Barebox is configured to be programmed to one of the MMC boot
partitions, whereas the original bootloader is located in user partition.

To restore the board to booting using the original bootloader do:

.. code-block:: sh

  detect mmc0
  mmc0.boot=disabled

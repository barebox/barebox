Xilinx ZynqMP Ultrascale+
=========================

Barebox has support as a second stage boot loader for the Xilinx ZynqMP
Ultrascale+.

Image creation
--------------

Currently, Barebox only supports booting as a second stage boot loader from an
SD-card. It relies on the FSBL_ to initialize the base system including sdram
setup and pin muxing.

The ZynqMP defconfig supports the ZCU102/104/106 reference board. Use it to build the
Barebox image::

   make ARCH=arm64 zynqmp_defconfig
   make ARCH=arm64

.. note:: The resulting image ``images/barebox-zynqmp-zcuX.img`` is **not** an image
  that can directly be booted on the ZynqMP.

For a bootable BOOT.BIN image, you also need to build the FSBL_ and a ZynqMP
TF-A. Prepare these separately using the respective instructions.

Use bootgen_ or ``mkimage -T zynqmpbif`` from the U-boot tools to build the
final BOOT.BIN image that can be loaded by the ROM code. Check the
instructions for these tools how to prepare the BOOT.BIN image.

Create a FAT partition as the first partition of the SD card and copy the
produced BOOT.BIN into this partition.

.. _FSBL: https://github.com/Xilinx/embeddedsw/
.. _bootgen: https://github.com/Xilinx/bootgen

Booting Barebox
---------------

The FSBL loads the TF-A and Barebox, jumps to the TF-A, which will then return
to Barebox. Afterwards, you can use Barebox as usual.

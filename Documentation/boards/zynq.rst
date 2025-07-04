Xilinx Zynq 7000
================

Barebox has support for the Xilinx Zynq 7000.

Image creation
--------------

The Zynq defconfig supports the Avnet ZedBoard. Use it to build the Barebox image::

   make ARCH=arm zynq_defconfig
   make ARCH=arm

Create a FAT partition as the first partition of the SD card and copy the
produced image ``images/barebox-avnet-zedboard.img`` into this partition.
Rename the image to ``BOOT.bin`` which is the name the Primary Bootloader of the
Zynq 7000 expects for the next stage.

Bitstream loading
-----------------

The Zynq 7000 features an ARM Cortex-A9 processor (Processing System, PS)
alongside a Programmable Logic (PL) component that functions as an FPGA. Barebox
provides support for loading a bitstream into the PL through its firmware
interface.

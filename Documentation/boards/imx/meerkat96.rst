Meerkat 96
==========

The Meerkat96 is a single board computer based on an i.MX7D SoC by NXP,
featuring a dual core ARM Cortex-A7 at 1 GHz and a Cortex-M4 at 266MHz
and 512 MB DRAM. For further details on the board's features check the
manufacturers page at https://www.96boards.org/product/imx7-96

Serial console
--------------

UART6 of the i.MX7D is broken out to Pinheader J3, on the Silkscreen
the Pins are labeled with B (Ground), W (UART 6 TX) and G (UART 6 RX).
If you use the UART-To-USB-Converter provided with the board, you can
just connect the Black jumper to B, the White to W and the Green to G.

The UART uses 3.3V levels.

Building Barebox
----------------

To build Barebox for the meerkat96 board do the following:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> mrproper
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> imx_v7_defconfig
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix>

Bringup
-------

flash the resulting barebox-meerkat96.img to an sdcard at address 0.

Make sure the pmic is set to power-on state by setting the dipswitch
SW3 on the boards bottom side to 1-1 (i.e. all switches on, which is
the factory default).

Schematics
----------

Schematics are available at https://github.com/96boards/documentation/blob/master/consumer/imx7-96/hardware-docs/files/iMX7-96-schematics.pdf


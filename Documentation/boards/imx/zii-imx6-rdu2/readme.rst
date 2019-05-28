ZII i.MX6 RDU2 Boards
=====================

Building Barebox
----------------

To build Barebox for ZII RDU2 boards do the following:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> mrproper
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix> imx_v7_defconfig
  make ARCH=arm CROSS_COMPILE=<ARM toolchain prefix>

Uploading Barebox via JTAG
--------------------------

Barebox can be bootstrapped via JTAG using OpenOCD (latest master) as
follows:

.. code-block:: sh

  cd barebox
  Documentation/boards/imx/zii-imx6-rdu2/bootstrap.sh

A custom OpenOCD binary and options can be specified as follows:

.. code-block:: sh

  OPENOCD="../openocd/src/openocd -s ../openocd/tcl " \
    Documentation/boards/imx/zii-imx6-rdu2/bootstrap.sh

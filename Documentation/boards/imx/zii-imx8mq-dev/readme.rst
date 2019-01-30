ZII i.MX8MQ Based Boards
========================

Building Barebox
----------------

To build Barebox of ZII i.MX8MQ based board do the following:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=<AArch64 toolchain prefix> mrproper
  make ARCH=arm CROSS_COMPILE=<AArch64 toolchain prefix> imx_v8_defconfig
  make ARCH=arm CROSS_COMPILE=<AArch64 toolchain prefix>

Uploading Barebox via JTAG
--------------------------

Barebox can be bootstrapped via JTAG using OpenOCD (latest master) as
follows:

.. code-block:: sh

        cd barebox
        openocd -f Documentation/boards/imx/zii-imx8mq-dev/openocd.cfg --command "init; ddr_init; start_barebox"

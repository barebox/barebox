ZII i.MX7D Based Boards
=======================

Building Barebox
----------------

To build Barebox for ZII i.MX7 based boards do the following:

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
  Documentation/boards/imx/zii-imx7d-rpu2/bootstrap.sh

A custom OpenOCD binary and options can be specified as follows:

.. code-block:: sh

  OPENOCD="../openocd/src/openocd -s ../openocd/tcl " \
    Documentation/boards/imx/zii-imx7d-rpu2/bootstrap.sh


Disabling DSA in Embedeed Switch
--------------------------------

Booting the Linux kernel that the device ships with will re-configure the on-board
switch into DSA mode, which would make the Ethernet connection unusable in
Barebox. To undo that and re-configure the switch into dumb/pass-through
mode, do the following:

.. code-block:: sh

  memset -b -d /dev/switch-eeprom 0x00 0xff 4

Once that is done, power cycling the device should force the switch to
re-read the EEPROM and reconfigure itself.

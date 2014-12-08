Garz & Fricke based boards
==========================

Garz & Fricke have a variety of Freescale i.MX based boards. Most boards are
shipped with the redboot bootloader, newer boards are shipped with a combination
of a shim (U-Boot based) non interactive loader and a Linux based rescue system
called 'Flash-N-Go'.

Santaro (i.MX6)
---------------

This board comes with Flash-N-Go. It boots from the internal eMMC boot0 partition.
To put barebox on this board boot the system up in the Flash-N-Go system by holding
the user button pressed during startup. The barebox image can be transferred via
network or any other medium the Santaro supports. You can backup the original bootloader
with:

.. code-block:: sh
  cat /dev/mmcblk0boot0 > /mnt/mmcblk0boot0.backup
  cat /dev/mmcblk0boot1 > /mnt/mmcblk0boot1.backup

The original bootloader can be restored by copying the backup files back to the
device.

To install barebox on the board do:

.. code-block:: sh
  echo 0 > /sys/block/mmcblk0boot0/force_ro
  cat barebox-guf-santaro.img > /dev/mmcblk0boot0

The board will come up with barebox after restart. Should you end up with a nonworking
barebox or otherwise brick your board this way write a mail to s.hauer@pengutronix.de.
There's a way to put the board into SD/USB boot mode, but this requires opening the case.

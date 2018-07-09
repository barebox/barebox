Wandboard
=========

The Wandboard is a carrier board available with three different
System-on-Module options, the Wandboard Solo (i.MX6S, 512MiB DDR3),
the Wandboard Dual (i.MX6DL, 1GiB DDR3) and Wandboard Quad (i.MX6Q, 2GiB DDR3).

The device boots from the SD card slot on the System-on-Module board, it
will not boot from the slot on the carrier board.

To boot barebox on any wandboard, build ``imx_v7_defconfig``
and copy the barebox imx-image to the i.MX boot location of a SD card, e.g.::

        dd bs=1024 skip=1 seek=1 if=images/barebox-imx6-wandboard.img of=/dev/mmcblk0

Only one image exists, supporting all three Wandboard variants, barebox will
detect the Wandboard variant depending on the SoC variant.
This image is only usable for SD boot. It will not boot via imx-usb-loader.

Connect to the serial port using a null-modem cable to get console access.

For further documentation, including board schematics, see http://wandboard.org/.

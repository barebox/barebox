Digi CC-IMX6UL-SBC-PRO
======================

This board is based on the i.MX6UL SoC.

The SBC Pro is shipped with:

  * 256MiB NAND flash
  * 256MiB DDR3 SDRAM

see https://www.digi.com/products/embedded-systems/single-board-computers/connectcore-for-i-mx6ul-sbc-pro
for more information.

MAC addresses
-------------
The Digi modules save their MAC addresses not in the OCOTP nodes, but in the
U-Boot environment. It is advised to boot the board using the shipped U-Boot
Bootloader and to read out and save the MAC addresses for the board.
The environment variables which contain the addresses are `$ethaddr` and
`$eth1addr`.
The MAC addresses can than be persisted to the barebox environment by using

.. code-block:: sh

  nv dev.eth0.ethaddr=<eth0addr>
  nv dev.eth1.ethaddr=<eth1addr>

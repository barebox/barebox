Marvell Embedded Business Unit (mvebu)
======================================

Move of the Register Window
---------------------------

When an mvebu SoC comes up the internal registers are mapped at 0xd0000000 in
the address space. To make it possible to have more than 3.25 GiB of continuous
RAM in Linux this window is moved to 0xf1000000.
Unfortunately the register to configure the location of the registers is located
in this window, so there is no way to determine the location afterwards.

RAM initialisation
------------------

Traditionally the RAM initialisation happens with a binary blob that have to be
extracted from the vendor U-Boot::

  scripts/kwbimage -x -i /dev/mtdblock0 -o .

This creates among others a file "binary.0" that has to be put into the board
directory. For license reasons this is usually not included in the barebox
repository.

Note that in the meantime U-Boot has open source code to do the RAM
initialisation that could be taken.

Booting second stage
--------------------

Since ``v2017.04.0`` barebox can boot a barebox image even if the register
window is moved. This is implemented by writing the actual window position
into the image where it is then picked up by the second stage bootloader.

Booting from UART
-----------------

The mvebu SoCs support booting from UART. For this there is a tool available in
barebox called ``kwboot``. Quite some mvebu boards are reset once more when
they already started to read the first block of the image to boot. If you want
to boot such a board, use the parameter ``-n 15`` for ``kwboot``. (The number
might have to be adapted per board.)

mvebu boards
------------

Not all supported boards have a description here.

.. toctree::
  :glob:
  :numbered:
  :maxdepth: 1

  mvebu/*

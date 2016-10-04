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

This is currently not possible because barebox assumes the registers are mapped
at 0xd0000000 as is the case when the boot ROM gives control to the bootloader.

Booting from UART
-----------------

The mvebu SoCs support booting from UART. For this there is a tool available in
barebox called kwboot.

mvebu boards
------------

Not all supported boards have a description here.

.. toctree::
  :glob:
  :numbered:
  :maxdepth: 1

  mvebu/*

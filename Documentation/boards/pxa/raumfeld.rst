Raumfeld Speaker
================

The Raumfeld speakers are PXA303 based WLAN speakers, sold by Teufel as the
Speaker M, L, S and One: https://teufel.de/raumfeld-speaker-m-102181000

They differ in enclosure and speaker assembly only, so ``raumfeld_speaker_defconfig``
and one device tree cover all of them.

Images
------

``barebox-raumfeld-speaker.img``
  barebox proper, entered at 0xa0008000 with the hardware already up. This is
  what the first stage loads, and what to chainload with ``go`` when testing a
  new build over TFTP.

``barebox-raumfeld-speaker-nand.img``
  The image the Boot ROM boots: an NTIM header, the OBM that brings up
  pinmuxing, clocks and DRAM, and a copy of the image above for it to load.
  Write it to the ``barebox`` partition with ``barebox_update -t nand``.

Internal connectors
-------------------

CON2, serial console, 3.3V:

===  ==============
Pin  Signal
===  ==============
1    not identified
2    UART RXD
3    UART TXD
4    GPIO79
5    GPIO84
6    GPIO81
7    GPIO83
8    GND
===  ==============

CON3, JTAG:

===  ==========
Pin  Signal
===  ==========
1    GND
2    nSRST
3    TDO
4    TCK
5    TMS
6    TDI
7    nTRST
8    reset status
===  ==========

Pin 8 is an output and cannot be used to reset the board; nSRST is pin 2.

NAND partitioning
-----------------

.. warning:: barebox does not use the vendor partitioning and overwrites it.

The vendor bootloader lives in 640KiB at the start of NAND, followed by its
environment and a splash screen partition, with the rest UBI. barebox does not
fit in 640KiB, so it takes 2MiB, 1MiB for an environment of its own, and 16MiB
at the end of the device for everything the running system writes:

=========  ======  ===================
Offset     Size    Name
=========  ======  ===================
0x0000000  2MiB    barebox
0x0200000  1MiB    barebox-environment
0x0300000  109MiB  UBI
0x7000000  16MiB   Data
=========  ======  ===================

barebox fixes these into the kernel device tree, so the partitioning in the
kernel's own device tree does not take effect.

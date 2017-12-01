OpenRISC
========

or1ksim
-------

Compile or1ksim emulator::

 $ cd ~/
 $ git clone https://github.com/openrisc/or1ksim
 $ cd or1ksim
 $ ./configure
 $ make

Create minimal or1ksim.cfg file:

.. code-block:: none

 section cpu
   ver = 0x12
   cfgr = 0x20
   rev = 0x0001
 end

 section memory
   name = "RAM"
   type = unknown
   baseaddr = 0x00000000
   size = 0x02000000
   delayr = 1
   delayw = 2
 end

 section uart
   enabled = 1
   baseaddr = 0x90000000
   irq = 2
   16550 = 1
   /* channel = "tcp:10084" */
   channel = "xterm:"
 end

 section ethernet
   enabled = 1
   baseaddr = 0x92000000
   irq = 4
   rtx_type = "tap"
   tap_dev = "tap0"
 end

Run or1ksim::

 $ ~/or1ksim/sim -f or1ksim.cfg barebox

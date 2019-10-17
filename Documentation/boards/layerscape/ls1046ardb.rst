NXP LS1046A Reference Design Board
==================================

Boot DIP Switches
-----------------

Boot source selection happens via the the bottom most DIP switch (near the micro-usb port)::

     OFF -> ON
    +---------+
  1 |  O----  |
  2 |  O----  |
  3 |  ----O  |
  4 |  O----  |
  5 |  O----  |
  6 |  O----  |
  7 |  ----O  |  <---- Boot from QSPI (default)
  8 |  O----  |
    +---------+

     OFF -> ON
    +---------+
  1 |  O----  |
  2 |  O----  |
  3 |  ----O  |
  4 |  O----  |
  5 |  O----  |
  6 |  O----  |
  7 |  O----  |  <---- Boot from SDHC
  8 |  O----  |
    +---------+

Known Issues
------------

System reset may not complete if the CMSIS-DAP micro-usb is connected.

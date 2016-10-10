Netgear ReadyNAS 2120
=====================

This is a rack mountable 4 bay NAS using an Armada XP dual-core processor.

UART booting
------------

The first UART hides behind a sticker on 4 pins.

The machine seems to do two resets at power on which makes UART booting hard. A
trick to work around this is::

  scripts/kwboot -d /dev/ttyUSB0; kwboot -b images/barebox-netgear-rn2120.img -t /dev/ttyUSB0

This way the first window in which the CPU accepts the magic string is taken by
the first invokation which blocks until the second reset happens. The second
window is then hit with the image to boot. This is not 100% reliable but works
most of the time.

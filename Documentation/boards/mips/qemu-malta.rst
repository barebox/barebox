QEMU Malta
==========

QEMU run string::

  qemu-system-mips -nodefaults -M malta -m 256 \
      -nographic -serial stdio -monitor null \
      -bios barebox-flash-image

Also you can use GXemul::

  gxemul -Q -x -e maltabe -M 256 0xbfc00000:barebox-flash-image

Links
-----

  * http://www.linux-mips.org/wiki/Mips_Malta
  * http://www.qemu.org/
  * http://gxemul.sourceforge.net/

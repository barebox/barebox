QEMU Malta
==========

Big-endian mode
---------------

QEMU run string::

  qemu-system-mips -nodefaults -M malta -m 256 \
      -nographic -serial stdio -monitor null \
      -bios barebox-flash-image

Also you can use GXemul::

  gxemul -Q -x -e maltabe -M 256 0xbfc00000:barebox-flash-image


Little-endian mode
------------------

Running little-endian Malta is a bit tricky.
In little-endian mode the 32bit words in the boot flash image are swapped,
a neat trick which allows bi-endian firmware.

You have to swap words of ``zbarebox.bin`` image, e.g.::

  echo arch/mips/pbl/zbarebox.bin \
      | cpio --create \
      | cpio --extract --swap --unconditional

QEMU run string::

  qemu-system-mipsel -nodefaults -M malta -m 256 \
      -nographic -serial stdio -monitor null \
      -bios barebox-flash-image


Links
-----

  * http://www.linux-mips.org/wiki/Mips_Malta
  * http://www.qemu.org/
  * http://gxemul.sourceforge.net/

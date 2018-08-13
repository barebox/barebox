QEMU Malta
==========

Big-endian mode
---------------

QEMU run string:

.. code-block:: sh

  qemu-system-mips -nodefaults -M malta -m 256 \
      -nographic -serial stdio -monitor null \
      -bios barebox-flash-image


Little-endian mode
------------------

Running little-endian Malta is a bit tricky.
In little-endian mode the 32bit words in the boot flash image are swapped,
a neat trick which allows bi-endian firmware.

You have to swap words of ``zbarebox.bin`` image, e.g.:

.. code-block:: sh

  echo arch/mips/pbl/zbarebox.bin \
      | cpio --create \
      | cpio --extract --swap --unconditional

QEMU run string:

.. code-block:: sh

  qemu-system-mipsel -nodefaults -M malta -m 256 \
      -nographic -serial stdio -monitor null \
      -bios barebox-flash-image


Using GXemul
------------

GXemul supports MIPS Malta except PCI stuff.
You can use GXemul to run little-endian barebox (use gxemul-malta_defconfig).

N.B. There is no need to swap words in ``zbarebox.bin`` for little-endian GXemul!

GXemul run string:

.. code-block:: sh

  gxemul -Q -e malta -M 256 0xbfc00000:barebox-flash-image


Links
-----

  * http://www.linux-mips.org/wiki/Mips_Malta
  * http://www.qemu.org/
  * http://gxemul.sourceforge.net/

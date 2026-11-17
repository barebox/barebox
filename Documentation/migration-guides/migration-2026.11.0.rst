Release v2026.11.0
==================

Generated MAC addresses now depend on the network device name
-------------------------------------------------------------

With ``CONFIG_NET_ETHADDR_FROM_MACHINE_ID`` enabled, barebox derives the MAC
address of a network device without one from the machine ID and the
hostname. Network devices used to be told apart by adding their number to
the derived address, which only works as long as all of them are named
``ethX``. Now that network devices may be named otherwise, like ``usb0`` of
the new CDC EEM gadget, the device name is hashed in instead.

This changes the generated MAC address of every network device, so hosts
that knew a board by its previous address, e.g. a DHCP server handing out
a fixed lease, will see a new one. Addresses set by board code, NVMEM or
the environment are not affected.

Deep probe is now the default on STM32MP13 and STM32MP15
--------------------------------------------------------

barebox now enables deep probe for every ``st,stm32mp1xx`` compatible, even
over ``barebox,disable-deep-probe``. Report to the mailing list if that
breaks your board.

devboot artifacts follow the boot net naming
--------------------------------------------

devboot now looks up ``user-linux-<hostname>`` and
``user-initramfs-<hostname>`` instead of ``user-image-<hostname>`` and
``user-initrd-<hostname>``, so the same files work for ``boot net``.

FIT default configuration wins
------------------------------

When several FIT configurations match the barebox device tree equally
well, the one named by ``default`` is now chosen instead of the first

Device tree node names are compared case-sensitively
----------------------------------------------------

Lookups by node name, including unit address, and alias stems are now
case-sensitive, as in Linux.

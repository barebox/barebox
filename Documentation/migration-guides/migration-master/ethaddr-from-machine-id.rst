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

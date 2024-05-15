.. index:: pstore (filesystem)

*pstore* filesystem with RAM backend (RAMOOPS)
==============================================

Barebox supports the *pstore* filesystem known from the kernel. The main backend
implementation is RAM. All other backends are currently not implemented by
Barebox.

*pstore* is a filesystem to store kernel log or kernel panic messages. These
messages are stored by the kernel in a specified RAM area which is never
overwritten by any user. This data can be accessed after a reboot through
``/pstore`` in Barebox or the kernel. The *pstore* filesystem is automatically
mounted at boot:

.. code-block:: none

	none on / type ramfs
	none on /dev type devfs
	none on /pstore type pstore

*pstore* may add additional warnings during boot due to wrong ECCs (no data
written):

.. code-block:: none

  persistent_ram: found existing invalid buffer, size 791282217, start 1116786789
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 791282281, start 1133564005
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 791347753, start 1133564005
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 791347753, start 1133572197
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 774505001, start 1133564005
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 791282281, start 1133564005
  persistent_ram: uncorrectable error in header
  persistent_ram: found existing invalid buffer, size 791282217, start 1133564005
  pstore: Registered ramoops as persistent store backend
  ramoops: attached 0x200000@0x1fdf4000, ecc: 16/0

To use *pstore/RAMOOPS* both Barebox and Kernel have to be compiled with *pstore*
and RAM backend support. The kernel receives the parameters describing the
layout via devicetree or - as a fallback - over the kernel command line.
To ensure both worlds are using the same memory layout, the required
configuration data for the kernel is generated on-the-fly prior booting a kernel.
For the devicetree use case Barebox adapts the kernel's devicetree, for the
kernel command line fallback the variable ``global.linux.bootargs.ramoops`` is
created and its content used to build the kernel command line.

You can adapt the *pstore* parameters in Barebox menuconfig.

To see where the RAMOOPS area is located, you can execute the ``iomem`` command
in the Barebox shell. The RAMOOPS area is listed as 'persistent ram':

.. code-block:: none

  0x10000000 - 0x1fffffff (size 0x10000000) ram0
    0x247f59c0 - 0x2fbf59bf (size 0x0b400000) malloc space
    0x2fbf59c0 - 0x2fbffffe (size 0x0000a63f) board data
    0x2fc00000 - 0x2fc8619f (size 0x000861a0) barebox
    0x2fc861a0 - 0x2fca35ef (size 0x0001d450) barebox data
    0x2fca35f0 - 0x2fca9007 (size 0x00005a18) bss
    0x2fdd4000 - 0x2fdf3fff (size 0x00020000) ramoops:dump(0/4)
    0x2fdf4000 - 0x2fe13fff (size 0x00020000) ramoops:dump(1/4)
    0x2fe14000 - 0x2fe33fff (size 0x00020000) ramoops:dump(2/4)
    0x2fe34000 - 0x2fe53fff (size 0x00020000) ramoops:dump(3/4)
    0x2fe54000 - 0x2fe73fff (size 0x00020000) ramoops:dump(4/4)
    0x2fe74000 - 0x2fe93fff (size 0x00020000) ramoops:console
    0x2fe94000 - 0x2feb3fff (size 0x00020000) ramoops:ftrace
    0x2feb4000 - 0x2fed3fff (size 0x00020000) ramoops:pmsg
    0x2fee4000 - 0x2fee7fff (size 0x00004000) ttb
    0x2fee8000 - 0x2feeffff (size 0x00008000) stack

If the menu entry ``FS_PSTORE_CONSOLE`` is enabled, Barebox itself will add all
its own console output to the *ramoops:console* part, which enables the regular
userland later on to have access to the bootloaders output.

All pstore files that could be found are added to the /pstore directory. This is
a read-only filesystem with the only supported operation being unlinking:
This resets (zaps) the RAMOOPS area and recalculates the ECC.

Zapping is done automatically, if the menu entry ``FS_PSTORE_RAMOOPS_RO`` is
disabled. In this case, only the barebox log will be available to the kernel
and ramoops from previous boots will not survive.

The usual setup is to not zap any buffers, i.e. ``CONFIG_FS_PSTORE_RAMOOPS_RO=y``
and no manual unlinking of files in ``/pstore``.

In Linux, the pstore file system is often mounted at ``/sys/fs/pstore``.
This will often not contain older output as Linux frequently zaps ramoops
buffers to conserve the limited space.

To counteract this, userland needs to archive the pstore contents
after booting into Linux and then clear pstore. Systemd has built-in support
for this when compiled with ``-Dpstore=true``.

Logs (including barebox log messages if enabled) will then be written to
journal by default and are accessible via::

  journalctl -b -o verbose -a -t systemd-pstore

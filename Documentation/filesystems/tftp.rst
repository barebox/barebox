.. index:: tftp (filesystem)

.. _filesystems_tftp:

TFTP support
============

barebox has read/write support for the Trivial File Transfer Protocol.

TFTP is not designed as a filesystem. It does not have support for listing
directories. This means a :ref:`command_ls` to a TFTP-mounted path will show an empty
directory. Nevertheless, the files are there.

Example::

  mount -t tftp 192.168.23.4 /mnt/tftp

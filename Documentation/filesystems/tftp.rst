.. index:: tftp (filesystem)

.. _filesystems_tftp:

TFTP filesystem
===============

barebox has read/write support for the Trivial File Transfer Protocol (TFTP,
`RFC1350 <https://tools.ietf.org/html/rfc1350>`_).

TFTP is not designed as a filesystem. It does not have support for listing
directories. This means a :ref:`ls <command_ls>` to a TFTP-mounted path will
show an empty directory. Nevertheless, the files are there.

Example:

.. code-block:: console

  barebox:/ mount -t tftp 192.168.23.4 /mnt/tftp

In addition to the TFTP filesystem implementation, barebox does also have a
:ref:`tftp command <command_tftp>`.

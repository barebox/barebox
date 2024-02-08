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

RFC 7440 "windowsize" support
-----------------------------

barebox supports the tftp windowsize option for downloading files.  It
is not implemented for uploads.

Generally, this option greatly improves the download speed (factors
4-30 are not uncommon).  But choosing a too large windowsize can have
the opposite effect.  Performance depends on:

 - the network infrastructure: when the tftp server sends files with
   1Gb/s but there are components in the network (switches or the
   target nic) which support only 100 Mb/s, packets will be dropped.

   The lower the internal buffers of the bottleneck components, the
   lower the optimal window size.

   In practice (iMX8MP on a Netgear GS108Ev3 with a port configured to
   100 Mb/s) it had to be reduced to

   .. code-block:: console

     global tftp.windowsize=26

   for example.

 - the target network driver: datagrams from server will arive faster
   than they can be processed and must be buffered internally.  For
   example, the `fec-imx` driver reserves place for

   .. code-block:: c

     #define FEC_RBD_NUM		64

   packets before they are dropped

 - partially the workload: copying downloaded files to ram will be
   faster than burning them into flash.  Latter can consume internal
   buffers quicker so that windowsize might be reduced

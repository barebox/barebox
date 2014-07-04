.. index:: nfs (filesystem)

.. _filesystems_nfs:

NFS Support
===========

barebox has readonly support for NFSv3 in UDP mode.

Example::

   mount -t nfs 192.168.23.4:/home/user/nfsroot /mnt/nfs

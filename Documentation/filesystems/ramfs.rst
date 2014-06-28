.. index:: ramfs (filesystem)

RAM filesystem
==============

ramfs is a simple malloc-based filesystem. An instance of ramfs is
mounted during startup on /. The filesystem type passed to ``mount``
is ``ramfs``.

Example::

  mount none ramfs /somedir

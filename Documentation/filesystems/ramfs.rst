.. index:: ramfs (filesystem)

ram filesystem
==============

ramfs is a simple malloc based filesystem. An instance of ramfs is mounted during startup on /. The filesystemtype passed to mount is 'ramfs'

example::

  mount none ramfs /somedir

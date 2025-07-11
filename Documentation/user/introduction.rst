Introduction
============

This is the barebox user manual, which describes how to configure, compile
and run barebox on embedded systems.

barebox (just barebox, not *the* barebox) is a bootloader designed for
embedded systems. It runs on a variety of architectures including
x86, ARM, MIPS, PowerPC and others.

barebox aims to be a versatile and flexible bootloader, not only for
booting embedded Linux systems, but also for initial hardware bringup and
development. barebox is highly configurable to be suitable as a full-featured
development binary as well as for lean production systems.
Just like busybox is the Swiss Army Knife for embedded Linux,
barebox is the Swiss Army Knife for bare metal, hence the name.

.. _community:

Community
---------

For sending patches, asking for help and giving general feedback you are
always welcome to write an e-mail to the barebox mailing list at
`barebox@lists.infradead.org <mailto:barebox@lists.infradead.org>`_.
Most of the discussion of barebox takes place here:

http://lists.infradead.org/mailman/listinfo/barebox/

Mails sent to the barebox mailing list are archived on
`lore.barebox.org <https://lore.barebox.org/barebox/>`_ and
`lore.kernel.org <https://lore.kernel.org/barebox/>`_.

There's also an IRC channel, which is
`bridged to Matrix  <https://app.element.io/#/room/#barebox:matrix.org>`_:
#barebox on Libera Chat.

.. _feedback:

Sending Patches
---------------

barebox uses a similar patch process as the Linux kernel, so most of the
`Linux guide for submitting patches <https://www.kernel.org/doc/html/latest/process/submitting-patches.html>`_
also applies to barebox, except for the need to select your recipient;
all barebox patches go to the same list.

Patch series can be sent and fetched from the list using `b4 <https://pypi.org/project/b4/>`_ ::

   b4 shazam -M https://lore.barebox.org/$messageid # replace with link

Fixes should apply on master and new features on the next branch.
If a series fails to apply, ``b4`` can determine/guess the base
and have ``FETCH_HEAD`` point at it::

   b4 shazam -H https://lore.kernel.org/$messageid # URL can be omitted

CI tests are executed by Github Actions can be used by forking
`the project on Github <https://github.com/barebox/barebox>`.

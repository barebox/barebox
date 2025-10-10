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

Patch workflow
--------------

Patches are sent via email to the barebox mailing list, similarly to how
you would contribute to the Linux kernel.

Patch series on the mailing list can be fetched most easily with
`b4 <https://pypi.org/project/b4/>`_::

   b4 shazam -H https://lore.barebox.org/$messageid # replace with link

This will determine a suitable base, apply the patches onto it and point
the ``FETCH_HEAD`` ref at it.

For a guide on how to send patches, see :ref:`contributing`.

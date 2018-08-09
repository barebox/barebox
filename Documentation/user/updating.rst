
.. _update:

Updating barebox
================

Updating barebox is potentially a dangerous task. When the update fails,
the board may not start anymore and must be recovered. barebox has a special
command to make updating barebox easier and safer: :ref:`command_barebox_update`.
A board can register an update handler to the update command. The handler can
do additional checks before trying an update, e.g. it's possible
to check whether the new image actually is a barebox image.

Updating barebox can be as easy as:

.. code-block:: sh

  barebox_update /path/to/new/barebox.img

Multiple handlers can be registered to the update mechanism. Usually the device
barebox has been started from is registered as default (marked with a ``*``):

.. code-block:: console

  barebox:/ barebox_update -l
  registered update handlers:
  * mmc         -> /dev/mmc1
    spinor	-> /dev/m25p0

:ref:`command_barebox_update` requires board support, so it may not be
available for your board. It is recommended to implement it, but you can also
update barebox manually using :ref:`command_erase` and :ref:`command_cp`
commands. The exact commands are board specific.

**NOTE** barebox images can be enriched with metadata which can be used to check
if a given image is suitable for updating barebox, see :ref:`imd`.

Repairing existing boot images
------------------------------

Some SoCs allow to store multiple boot images on a device in order to
improve robustness. When an update handler supports it the handler can
repair and/or refresh an image from this redundant information. This is
done with the '-r' option to :ref:`command_barebox_update`.

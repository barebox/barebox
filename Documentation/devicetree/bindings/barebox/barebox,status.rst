barebox,status property
=======================

barebox interprets ``barebox,status`` the same as it does ``status``,
but gives the barebox-specific property precedence if both exist.

The purpose of this property is to keep the ``status`` property of the
upstream DT, imported from Linux, untouched.

Using ``barebox,status`` may be necessary to temporarily workaround
barebox drivers that misbehave on a given board; Disabling the driver
may be undesirable if it can handle other instances of the same device
on the board or if barebox is being built to support other boards
at the same time, where the driver functions correctly.

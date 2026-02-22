:orphan:

Clock mux rate selection
------------------------

The default rate selection for mux clocks has been aligned with Linux.
Previously, barebox mux clocks selected the parent whose rate was closest
to the requested rate. Now, the default behavior is to select the highest
parent rate that does not exceed the requested rate (round-down).

Drivers that need the old closest-rate behavior should set the
``CLK_MUX_ROUND_CLOSEST`` flag on the mux clock.

Round-down can also fail where the old code could not: when every parent
runs faster than the requested rate there is no candidate left, and
``clk_round_rate()`` and ``clk_set_rate()`` return ``-EINVAL`` instead of
settling on the closest parent.

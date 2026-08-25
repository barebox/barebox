:orphan:

clk_ops::round_rate removed
---------------------------

``struct clk_ops`` no longer has a ``round_rate`` callback; clock drivers
now implement ``determine_rate`` instead, as in Linux. A ``round_rate``
implementation translates mechanically::

  -static long foo_round_rate(struct clk_hw *hw, unsigned long rate,
  -                           unsigned long *prate)
  +static int foo_determine_rate(struct clk_hw *hw,
  +                              struct clk_rate_request *req)
   {
  -        return compute(rate, *prate);
  +        req->rate = compute(req->rate, req->best_parent_rate);
  +        return 0;
   }

Where ``round_rate`` wrote back a new parent rate through ``*prate``,
``determine_rate`` assigns ``req->best_parent_rate``; the framework
propagates that to the parent when ``CLK_SET_RATE_PARENT`` is set. A
``determine_rate`` can additionally pick a different parent by setting
``req->best_parent_hw``, which ``round_rate`` had no way to express.

``clk_set_rate()`` on a clock that cannot change its rate at all -- no
``set_rate``, no ``determine_rate`` and no ``CLK_SET_RATE_PARENT`` --
now returns 0 and leaves the rate alone, where it used to return
``-ENOSYS``. This matches Linux' ``clk_core_set_rate_nolock()``, which
bails out with 0 once the rounded rate equals the current one. Use
``clk_round_rate()`` to find out which rate a clock will settle on.

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

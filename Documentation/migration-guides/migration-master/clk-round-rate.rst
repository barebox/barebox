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

#include <common.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

struct clk_lcdif {
	struct clk clk;

	struct clk *frac, *div, *gate;
	const char *parent;
};

#define to_clk_lcdif(_hw) container_of(_hw, struct clk_lcdif, clk)

static int clk_lcdif_set_rate(struct clk *clk, unsigned long rate,
			    unsigned long unused)
{
	struct clk_lcdif *lcdif = to_clk_lcdif(clk);
	unsigned long frac, div, best_div = 1;
	int delta, best_delta = 0x7fffffff;
	unsigned long frate, rrate, best_frate;
	unsigned long parent_rate = clk_get_rate(clk_get_parent(lcdif->frac));

	best_frate = parent_rate;

	for (frac = 18; frac < 35; frac++) {
		frate = (parent_rate / frac) * 18;
		div = frate / rate;
		if (!div)
			div = 1;
		rrate = frate / div;
		delta = rate - rrate;
		if (abs(delta) < abs(best_delta)) {
			best_frate = frate;
			best_div = div;
			best_delta = delta;
		}
	}

	clk_set_rate(lcdif->frac, best_frate);
	best_frate = clk_get_rate(lcdif->frac);
	clk_set_rate(lcdif->div, (best_frate + best_div) / best_div);

	return 0;
}

static const struct clk_ops clk_lcdif_ops = {
	.set_rate	= clk_lcdif_set_rate,
};

struct clk *mxs_clk_lcdif(const char *name, struct clk *frac, struct clk *div,
			  struct clk *gate)
{
	struct clk_lcdif *lcdif;
	int ret;

	lcdif = xzalloc(sizeof(*lcdif));

	lcdif->parent = gate->name;
	lcdif->frac = frac;
	lcdif->div = div;
	lcdif->gate = gate;
	lcdif->clk.name = name;
	lcdif->clk.ops = &clk_lcdif_ops;
	lcdif->clk.parent_names = &lcdif->parent;
	lcdif->clk.num_parents = 1;

	ret = clk_register(&lcdif->clk);
	if (ret)
		return ERR_PTR(ret);

	return &lcdif->clk;
}

/*
 *  include/linux/clkdev.h
 *
 *  Copyright (C) 2008 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Helper for the clk API to assist looking up a struct clk.
 */
#ifndef __CLKDEV_H
#define __CLKDEV_H

struct clk;
struct device_d;

struct clk_lookup {
	struct list_head	node;
	unsigned long		physbase;
	const char		*dev_id;
	const char		*con_id;
	struct clk		*clk;
};

struct clk_lookup *clkdev_alloc(struct clk *clk, const char *con_id,
	const char *dev_fmt, ...);

void clkdev_add(struct clk_lookup *cl);
void clkdev_drop(struct clk_lookup *cl);

void clkdev_add_table(struct clk_lookup *, size_t);
int clk_add_alias(const char *, const char *, char *, struct device_d *);
int clk_register_clkdev(struct clk *, const char *, const char *, ...);

int clkdev_add_physbase(struct clk *clk, unsigned long base, const char *id);

#define CLKDEV_DEV_ID(_id, _clk)			\
	{						\
		.dev_id = _id,				\
		.clk = _clk,				\
	}

#define CLKDEV_CON_ID(_id, _clk)			\
	{						\
		.con_id = _id,				\
		.clk = _clk,				\
	}

#define CLKDEV_CON_DEV_ID(_con_id, _dev_id, _clk)	\
	{						\
		.con_id = _con_id,			\
		.dev_id = _dev_id,			\
		.clk = _clk,				\
	}

#endif

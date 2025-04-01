/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTSCAN_H
#define __BOOTSCAN_H

struct bootentries;
struct device;
struct cdev;

struct bootscanner {
	/** For debugging output */
	const char *name;

	/** Invoked for when scanning a file */
	int (*scan_file)(struct bootscanner *,
			 struct bootentries *, const char *);
	/** Invoked for when scanning a directory */
	int (*scan_directory)(struct bootscanner *,
			      struct bootentries *, const char *);
	/** Fallback: Invoked only when none of the above returned results */
	int (*scan_device)(struct bootscanner *,
			   struct bootentries *, struct device *);
};

int boot_scan_cdev(struct bootscanner *scanner,
		   struct bootentries *bootentries, struct cdev *cdev);

int bootentry_scan_generate(struct bootscanner *scanner,
			    struct bootentries *bootentries,
			    const char *name);

#endif

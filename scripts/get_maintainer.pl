#!/usr/bin/env perl
# SPDX-License-Identifier: GPL-2.0-only
# Dummy get_maintainer.pl script for checkpatch.pl to use

die "USAGE: get_maintainer.pl --status\n" unless grep /--status/, @ARGV;

print <<'EOT'
Sascha Hauer <s.hauer@pengutronix.de> (maintainer:BAREBOX)
barebox@lists.infradead.org (open list:BAREBOX)
Maintained
EOT

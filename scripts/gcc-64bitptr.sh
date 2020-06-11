#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only

cat << "END" | $@ -x c - -c -o /dev/null
int main(void)
{
	return sizeof(struct { int:-!(sizeof(void *) == 8); });
}
END

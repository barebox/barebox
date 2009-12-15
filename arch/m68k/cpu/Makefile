#
# (C) Copyright 2007 Carsten Schlote <schlote@vahanus.net>
# See file CREDITS for list of people who contributed to this project.
#
# This file is part of barebox.
#
# barebox is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# barebox is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with barebox.  If not, see <http://www.gnu.org/licenses/>.
#

obj-y += cpu.o
obj-y += interrupts.o

#
# Support code for early IO over BDM wigglers
#

# obj-y += early_init_support.o

#
# Support for relocated early initdata
#
obj-$(CONFIG_HAS_EARLY_INIT) += early_init_support.o

#
# Startup codes - try to merge them into single file!
#
obj-$(CONFIG_ARCH_MCF54xx) += start-mcfv4e.o
obj-$(CONFIG_ARCH_MCF5445x) += start-mcfv4m.o

start-mcfv4e.o : start-mcfv4e.s
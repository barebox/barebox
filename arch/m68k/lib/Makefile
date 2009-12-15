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

#
# Architecture dependant stubs and callbacks
#

obj-y	+= m68k-meminit.o

obj-$(CONFIG_CMD_BOOTM)	+= m68k-linuxboot.o

obj-$(CONFIG_MODULES) += m68k-module.o

extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds

/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FLOW_HALT_CPU_EVENTS		0x000
#define FLOW_MODE_NONE			0
#define FLOW_MODE_STOP			2

#define FLOW_CLUSTER_CONTROL		0x02c
#define FLOW_CLUSTER_CONTROL_ACTIVE_G	(0 << 0)
#define FLOW_CLUSTER_CONTROL_ACTIVE_LP	(1 << 0)

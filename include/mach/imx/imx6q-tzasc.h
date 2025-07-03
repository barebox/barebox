/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Per default all clocks are on, except for TZASC1/2 CG11/2, so enable them
 * before activate the modules (disable the bypass mode).
 */
wm 32 0x020c4070 0xffffffff
/* Disable TZASC1/2 bypass */
wm 32 0x020E0024 0x00000003

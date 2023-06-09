/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_SFR_DEFS_H
#define _ASM_KVX_SFR_DEFS_H

#include <linux/const.h>

/* Register file indices */
#define KVX_SFR_PC 0 /* Program Counter $pc $s0 */
#define KVX_SFR_PS 1 /* Processor State $ps $s1 */
#define KVX_SFR_PCR 2 /* Processing Identification $pcr $s2 */
#define KVX_SFR_RA 3 /* Return Address $ra $s3 */
#define KVX_SFR_CS 4 /* Compute Status $cs $s4 */
#define KVX_SFR_CSIT 5 /* Compute Status arithmetic interrupt $csit $s5 */
#define KVX_SFR_AESPC 6 /* Arithmetic Exception Saved PC $aespc $s6 */
#define KVX_SFR_LS 7 /* Loop Start Address $ls $s7 */
#define KVX_SFR_LE 8 /* Loop Exit Address $le $s8 */
#define KVX_SFR_LC 9 /* Loop Counter $lc $s9 */
#define KVX_SFR_IPE 10 /* Inter Processor Event $ipe $s10 */
#define KVX_SFR_MEN 11 /* Misc External Notifications $men $s11 */
#define KVX_SFR_PMC 12 /* Performance Monitor Control $pmc $s12 */
#define KVX_SFR_PM0 13 /* Performance Monitor 0 $pm0 $s13 */
#define KVX_SFR_PM1 14 /* Performance Monitor 1 $pm1 $s14 */
#define KVX_SFR_PM2 15 /* Performance Monitor 2 $pm2 $s15 */
#define KVX_SFR_PM3 16 /* Performance Monitor 3 $pm3 $s16 */
#define KVX_SFR_PMSA 17 /* Performance Monitor Saved Address $pmsa $s17 */
#define KVX_SFR_TCR 18 /* Timer Control $tcr $s18 */
#define KVX_SFR_T0V 19 /* Timer 0 value $t0v $s19 */
#define KVX_SFR_T1V 20 /* Timer 1 value $t1v $s20 */
#define KVX_SFR_T0R 21 /* Timer 0 reload value $t0r $s21 */
#define KVX_SFR_T1R 22 /* Timer 1 reload value $t1r $s22 */
#define KVX_SFR_WDV 23 /* Watchdog Value $wdv $s23 */
#define KVX_SFR_WDR 24 /* Watchdog Reload $wdr $s24 */
#define KVX_SFR_ILE 25 /* Interrupt Line Enable $ile $s25 */
#define KVX_SFR_ILL 26 /* Interrupt Line Level $ill $s26 */
#define KVX_SFR_ILR 27 /* Interrupt Line Request $ilr $s27 */
#define KVX_SFR_MMC 28 /* Memory Management Control $mmc $s28 */
#define KVX_SFR_TEL 29 /* TLB Entry Low $tel $s29 */
#define KVX_SFR_TEH 30 /* TLB Entry High $teh $s30 */
#define KVX_SFR_SYO 32 /* Syscall Owners $syo $s32 */
#define KVX_SFR_HTO 33 /* Hardware Trap Owners $hto $s33 */
#define KVX_SFR_ITO 34 /* Interrupt Owners $ito $s34 */
#define KVX_SFR_DO 35 /* Debug Owners $do $s35 */
#define KVX_SFR_MO 36 /* Miscellaneous Owners $mo $s36 */
#define KVX_SFR_PSO 37 /* PS register fields Owners $pso $s37 */
#define KVX_SFR_DC 40 /* OCE (Debug) Control $dc $s40 */
#define KVX_SFR_DBA0 41 /* Breakpoint Address 0 $dba0 $s41 */
#define KVX_SFR_DBA1 42 /* Breakpoint Address 1 $dba1 $s42 */
#define KVX_SFR_DWA0 43 /* Watchpoint Address 0 $dwa0 $s43 */
#define KVX_SFR_DWA1 44 /* Watchpoint Address 1 $dwa1 $s44 */
#define KVX_SFR_MES 45 /* Memory Error Status $mes $s45 */
#define KVX_SFR_WS 46 /* Wake-Up Status $ws $s46 */
#define KVX_SFR_SPC_PL0 64 /* Shadow PC PL 0 $spc_pl0 $s64 */
#define KVX_SFR_SPC_PL1 65 /* Shadow PC PL 1 $spc_pl1 $s65 */
#define KVX_SFR_SPC_PL2 66 /* Shadow PC PL 2 $spc_pl2 $s66 */
#define KVX_SFR_SPC_PL3 67 /* Shadow PC PL 3 $spc_pl3 $s67 */
#define KVX_SFR_SPS_PL0 68 /* Shadow PS PL 0 $sps_pl0 $s68 */
#define KVX_SFR_SPS_PL1 69 /* Shadow PS PL 1 $sps_pl1 $s69 */
#define KVX_SFR_SPS_PL2 70 /* Shadow PS PL 2 $sps_pl2 $s70 */
#define KVX_SFR_SPS_PL3 71 /* Shadow PS PL 3 $sps_pl3 $s71 */
#define KVX_SFR_EA_PL0 72 /* Effective Address PL0 $ea_pl0 $s72 */
#define KVX_SFR_EA_PL1 73 /* Effective Address PL1 $ea_pl1 $s73 */
#define KVX_SFR_EA_PL2 74 /* Effective Address PL2 $ea_pl2 $s74 */
#define KVX_SFR_EA_PL3 75 /* Effective Address PL3 $ea_pl3 $s75 */
#define KVX_SFR_EV_PL0 76 /* Exception Vector PL 0 $ev_pl0 $s76 */
#define KVX_SFR_EV_PL1 77 /* Exception Vector PL 1 $ev_pl1 $s77 */
#define KVX_SFR_EV_PL2 78 /* Exception Vector PL 2 $ev_pl2 $s78 */
#define KVX_SFR_EV_PL3 79 /* Exception Vector PL 3 $ev_pl3 $s79 */
#define KVX_SFR_SR_PL0 80 /* System Register PL 0 $sr_pl0 $s80 */
#define KVX_SFR_SR_PL1 81 /* System Register PL 1 $sr_pl1 $s81 */
#define KVX_SFR_SR_PL2 82 /* System Register PL 2 $sr_pl2 $s82 */
#define KVX_SFR_SR_PL3 83 /* System Register PL 3 $sr_pl3 $s83 */
#define KVX_SFR_ES_PL0 84 /* Exception Syndrome PL 0 $es_pl0 $s84 */
#define KVX_SFR_ES_PL1 85 /* Exception Syndrome PL 1 $es_pl1 $s85 */
#define KVX_SFR_ES_PL2 86 /* Exception Syndrome PL 2 $es_pl2 $s86 */
#define KVX_SFR_ES_PL3 87 /* Exception Syndrome PL 3 $es_pl3 $s87 */
#define KVX_SFR_SYOW 96 /* Alias to SYO register $syow $s96 */
#define KVX_SFR_HTOW 97 /* Alias to HTO register $htow $s97 */
#define KVX_SFR_ITOW 98 /* Alias to ITO register $itow $s98 */
#define KVX_SFR_DOW 99 /* Alias to DO register $dow $s99 */
#define KVX_SFR_MOW 100 /* Alias to MO register $mow $s100 */
#define KVX_SFR_PSOW 101 /* Alias to PSO register $psow $s101 */
#define KVX_SFR_SPC 128 /* Shadow PC alias on SPC_PL<i> $spc $s128 */
#define KVX_SFR_SPS 132 /* Shadow PS alias on PS_PL<i> $sps $s132 */
#define KVX_SFR_EA 136 /* Effective Address alias on EA_PL<i> $ea $s136 */
#define KVX_SFR_EV 140 /* Exception Vector alias on EV_PL<i> $ev $s140 */
#define KVX_SFR_SR 144 /* System Register alias on SR_PL<i> $sr $s144 */
#define KVX_SFR_ES 148 /* Exception Syndrome alias on ES_PL<i> $es $s148 */
#define KVX_SFR_VSFR0 256 /* Virtual SFR 0 $vsfr0 $s256 */
#define KVX_SFR_VSFR1 257 /* Virtual SFR 1 $vsfr1 $s257 */
#define KVX_SFR_VSFR2 258 /* Virtual SFR 2 $vsfr2 $s258 */
#define KVX_SFR_VSFR3 259 /* Virtual SFR 3 $vsfr3 $s259 */
#define KVX_SFR_VSFR4 260 /* Virtual SFR 4 $vsfr4 $s260 */
#define KVX_SFR_VSFR5 261 /* Virtual SFR 5 $vsfr5 $s261 */
#define KVX_SFR_VSFR6 262 /* Virtual SFR 6 $vsfr6 $s262 */
#define KVX_SFR_VSFR7 263 /* Virtual SFR 7 $vsfr7 $s263 */
#define KVX_SFR_VSFR8 264 /* Virtual SFR 8 $vsfr8 $s264 */
#define KVX_SFR_VSFR9 265 /* Virtual SFR 9 $vsfr9 $s265 */
#define KVX_SFR_VSFR10 266 /* Virtual SFR 10 $vsfr10 $s266 */
#define KVX_SFR_VSFR11 267 /* Virtual SFR 11 $vsfr11 $s267 */
#define KVX_SFR_VSFR12 268 /* Virtual SFR 12 $vsfr12 $s268 */
#define KVX_SFR_VSFR13 269 /* Virtual SFR 13 $vsfr13 $s269 */
#define KVX_SFR_VSFR14 270 /* Virtual SFR 14 $vsfr14 $s270 */
#define KVX_SFR_VSFR15 271 /* Virtual SFR 15 $vsfr15 $s271 */
#define KVX_SFR_VSFR16 272 /* Virtual SFR 16 $vsfr16 $s272 */
#define KVX_SFR_VSFR17 273 /* Virtual SFR 17 $vsfr17 $s273 */
#define KVX_SFR_VSFR18 274 /* Virtual SFR 18 $vsfr18 $s274 */
#define KVX_SFR_VSFR19 275 /* Virtual SFR 19 $vsfr19 $s275 */
#define KVX_SFR_VSFR20 276 /* Virtual SFR 20 $vsfr20 $s276 */
#define KVX_SFR_VSFR21 277 /* Virtual SFR 21 $vsfr21 $s277 */
#define KVX_SFR_VSFR22 278 /* Virtual SFR 22 $vsfr22 $s278 */
#define KVX_SFR_VSFR23 279 /* Virtual SFR 23 $vsfr23 $s279 */
#define KVX_SFR_VSFR24 280 /* Virtual SFR 24 $vsfr24 $s280 */
#define KVX_SFR_VSFR25 281 /* Virtual SFR 25 $vsfr25 $s281 */
#define KVX_SFR_VSFR26 282 /* Virtual SFR 26 $vsfr26 $s282 */
#define KVX_SFR_VSFR27 283 /* Virtual SFR 27 $vsfr27 $s283 */
#define KVX_SFR_VSFR28 284 /* Virtual SFR 28 $vsfr28 $s284 */
#define KVX_SFR_VSFR29 285 /* Virtual SFR 29 $vsfr29 $s285 */
#define KVX_SFR_VSFR30 286 /* Virtual SFR 30 $vsfr30 $s286 */
#define KVX_SFR_VSFR31 287 /* Virtual SFR 31 $vsfr31 $s287 */
#define KVX_SFR_VSFR32 288 /* Virtual SFR 32 $vsfr32 $s288 */
#define KVX_SFR_VSFR33 289 /* Virtual SFR 33 $vsfr33 $s289 */
#define KVX_SFR_VSFR34 290 /* Virtual SFR 34 $vsfr34 $s290 */
#define KVX_SFR_VSFR35 291 /* Virtual SFR 35 $vsfr35 $s291 */
#define KVX_SFR_VSFR36 292 /* Virtual SFR 36 $vsfr36 $s292 */
#define KVX_SFR_VSFR37 293 /* Virtual SFR 37 $vsfr37 $s293 */
#define KVX_SFR_VSFR38 294 /* Virtual SFR 38 $vsfr38 $s294 */
#define KVX_SFR_VSFR39 295 /* Virtual SFR 39 $vsfr39 $s295 */
#define KVX_SFR_VSFR40 296 /* Virtual SFR 40 $vsfr40 $s296 */
#define KVX_SFR_VSFR41 297 /* Virtual SFR 41 $vsfr41 $s297 */
#define KVX_SFR_VSFR42 298 /* Virtual SFR 42 $vsfr42 $s298 */
#define KVX_SFR_VSFR43 299 /* Virtual SFR 43 $vsfr43 $s299 */
#define KVX_SFR_VSFR44 300 /* Virtual SFR 44 $vsfr44 $s300 */
#define KVX_SFR_VSFR45 301 /* Virtual SFR 45 $vsfr45 $s301 */
#define KVX_SFR_VSFR46 302 /* Virtual SFR 46 $vsfr46 $s302 */
#define KVX_SFR_VSFR47 303 /* Virtual SFR 47 $vsfr47 $s303 */
#define KVX_SFR_VSFR48 304 /* Virtual SFR 48 $vsfr48 $s304 */
#define KVX_SFR_VSFR49 305 /* Virtual SFR 49 $vsfr49 $s305 */
#define KVX_SFR_VSFR50 306 /* Virtual SFR 50 $vsfr50 $s306 */
#define KVX_SFR_VSFR51 307 /* Virtual SFR 51 $vsfr51 $s307 */
#define KVX_SFR_VSFR52 308 /* Virtual SFR 52 $vsfr52 $s308 */
#define KVX_SFR_VSFR53 309 /* Virtual SFR 53 $vsfr53 $s309 */
#define KVX_SFR_VSFR54 310 /* Virtual SFR 54 $vsfr54 $s310 */
#define KVX_SFR_VSFR55 311 /* Virtual SFR 55 $vsfr55 $s311 */
#define KVX_SFR_VSFR56 312 /* Virtual SFR 56 $vsfr56 $s312 */
#define KVX_SFR_VSFR57 313 /* Virtual SFR 57 $vsfr57 $s313 */
#define KVX_SFR_VSFR58 314 /* Virtual SFR 58 $vsfr58 $s314 */
#define KVX_SFR_VSFR59 315 /* Virtual SFR 59 $vsfr59 $s315 */
#define KVX_SFR_VSFR60 316 /* Virtual SFR 60 $vsfr60 $s316 */
#define KVX_SFR_VSFR61 317 /* Virtual SFR 61 $vsfr61 $s317 */
#define KVX_SFR_VSFR62 318 /* Virtual SFR 62 $vsfr62 $s318 */
#define KVX_SFR_VSFR63 319 /* Virtual SFR 63 $vsfr63 $s319 */
#define KVX_SFR_VSFR64 320 /* Virtual SFR 64 $vsfr64 $s320 */
#define KVX_SFR_VSFR65 321 /* Virtual SFR 65 $vsfr65 $s321 */
#define KVX_SFR_VSFR66 322 /* Virtual SFR 66 $vsfr66 $s322 */
#define KVX_SFR_VSFR67 323 /* Virtual SFR 67 $vsfr67 $s323 */
#define KVX_SFR_VSFR68 324 /* Virtual SFR 68 $vsfr68 $s324 */
#define KVX_SFR_VSFR69 325 /* Virtual SFR 69 $vsfr69 $s325 */
#define KVX_SFR_VSFR70 326 /* Virtual SFR 70 $vsfr70 $s326 */
#define KVX_SFR_VSFR71 327 /* Virtual SFR 71 $vsfr71 $s327 */
#define KVX_SFR_VSFR72 328 /* Virtual SFR 72 $vsfr72 $s328 */
#define KVX_SFR_VSFR73 329 /* Virtual SFR 73 $vsfr73 $s329 */
#define KVX_SFR_VSFR74 330 /* Virtual SFR 74 $vsfr74 $s330 */
#define KVX_SFR_VSFR75 331 /* Virtual SFR 75 $vsfr75 $s331 */
#define KVX_SFR_VSFR76 332 /* Virtual SFR 76 $vsfr76 $s332 */
#define KVX_SFR_VSFR77 333 /* Virtual SFR 77 $vsfr77 $s333 */
#define KVX_SFR_VSFR78 334 /* Virtual SFR 78 $vsfr78 $s334 */
#define KVX_SFR_VSFR79 335 /* Virtual SFR 79 $vsfr79 $s335 */
#define KVX_SFR_VSFR80 336 /* Virtual SFR 80 $vsfr80 $s336 */
#define KVX_SFR_VSFR81 337 /* Virtual SFR 81 $vsfr81 $s337 */
#define KVX_SFR_VSFR82 338 /* Virtual SFR 82 $vsfr82 $s338 */
#define KVX_SFR_VSFR83 339 /* Virtual SFR 83 $vsfr83 $s339 */
#define KVX_SFR_VSFR84 340 /* Virtual SFR 84 $vsfr84 $s340 */
#define KVX_SFR_VSFR85 341 /* Virtual SFR 85 $vsfr85 $s341 */
#define KVX_SFR_VSFR86 342 /* Virtual SFR 86 $vsfr86 $s342 */
#define KVX_SFR_VSFR87 343 /* Virtual SFR 87 $vsfr87 $s343 */
#define KVX_SFR_VSFR88 344 /* Virtual SFR 88 $vsfr88 $s344 */
#define KVX_SFR_VSFR89 345 /* Virtual SFR 89 $vsfr89 $s345 */
#define KVX_SFR_VSFR90 346 /* Virtual SFR 90 $vsfr90 $s346 */
#define KVX_SFR_VSFR91 347 /* Virtual SFR 91 $vsfr91 $s347 */
#define KVX_SFR_VSFR92 348 /* Virtual SFR 92 $vsfr92 $s348 */
#define KVX_SFR_VSFR93 349 /* Virtual SFR 93 $vsfr93 $s349 */
#define KVX_SFR_VSFR94 350 /* Virtual SFR 94 $vsfr94 $s350 */
#define KVX_SFR_VSFR95 351 /* Virtual SFR 95 $vsfr95 $s351 */
#define KVX_SFR_VSFR96 352 /* Virtual SFR 96 $vsfr96 $s352 */
#define KVX_SFR_VSFR97 353 /* Virtual SFR 97 $vsfr97 $s353 */
#define KVX_SFR_VSFR98 354 /* Virtual SFR 98 $vsfr98 $s354 */
#define KVX_SFR_VSFR99 355 /* Virtual SFR 99 $vsfr99 $s355 */
#define KVX_SFR_VSFR100 356 /* Virtual SFR 100 $vsfr100 $s356 */
#define KVX_SFR_VSFR101 357 /* Virtual SFR 101 $vsfr101 $s357 */
#define KVX_SFR_VSFR102 358 /* Virtual SFR 102 $vsfr102 $s358 */
#define KVX_SFR_VSFR103 359 /* Virtual SFR 103 $vsfr103 $s359 */
#define KVX_SFR_VSFR104 360 /* Virtual SFR 104 $vsfr104 $s360 */
#define KVX_SFR_VSFR105 361 /* Virtual SFR 105 $vsfr105 $s361 */
#define KVX_SFR_VSFR106 362 /* Virtual SFR 106 $vsfr106 $s362 */
#define KVX_SFR_VSFR107 363 /* Virtual SFR 107 $vsfr107 $s363 */
#define KVX_SFR_VSFR108 364 /* Virtual SFR 108 $vsfr108 $s364 */
#define KVX_SFR_VSFR109 365 /* Virtual SFR 109 $vsfr109 $s365 */
#define KVX_SFR_VSFR110 366 /* Virtual SFR 110 $vsfr110 $s366 */
#define KVX_SFR_VSFR111 367 /* Virtual SFR 111 $vsfr111 $s367 */
#define KVX_SFR_VSFR112 368 /* Virtual SFR 112 $vsfr112 $s368 */
#define KVX_SFR_VSFR113 369 /* Virtual SFR 113 $vsfr113 $s369 */
#define KVX_SFR_VSFR114 370 /* Virtual SFR 114 $vsfr114 $s370 */
#define KVX_SFR_VSFR115 371 /* Virtual SFR 115 $vsfr115 $s371 */
#define KVX_SFR_VSFR116 372 /* Virtual SFR 116 $vsfr116 $s372 */
#define KVX_SFR_VSFR117 373 /* Virtual SFR 117 $vsfr117 $s373 */
#define KVX_SFR_VSFR118 374 /* Virtual SFR 118 $vsfr118 $s374 */
#define KVX_SFR_VSFR119 375 /* Virtual SFR 119 $vsfr119 $s375 */
#define KVX_SFR_VSFR120 376 /* Virtual SFR 120 $vsfr120 $s376 */
#define KVX_SFR_VSFR121 377 /* Virtual SFR 121 $vsfr121 $s377 */
#define KVX_SFR_VSFR122 378 /* Virtual SFR 122 $vsfr122 $s378 */
#define KVX_SFR_VSFR123 379 /* Virtual SFR 123 $vsfr123 $s379 */
#define KVX_SFR_VSFR124 380 /* Virtual SFR 124 $vsfr124 $s380 */
#define KVX_SFR_VSFR125 381 /* Virtual SFR 125 $vsfr125 $s381 */
#define KVX_SFR_VSFR126 382 /* Virtual SFR 126 $vsfr126 $s382 */
#define KVX_SFR_VSFR127 383 /* Virtual SFR 127 $vsfr127 $s383 */
#define KVX_SFR_VSFR128 384 /* Virtual SFR 128 $vsfr128 $s384 */
#define KVX_SFR_VSFR129 385 /* Virtual SFR 129 $vsfr129 $s385 */
#define KVX_SFR_VSFR130 386 /* Virtual SFR 130 $vsfr130 $s386 */
#define KVX_SFR_VSFR131 387 /* Virtual SFR 131 $vsfr131 $s387 */
#define KVX_SFR_VSFR132 388 /* Virtual SFR 132 $vsfr132 $s388 */
#define KVX_SFR_VSFR133 389 /* Virtual SFR 133 $vsfr133 $s389 */
#define KVX_SFR_VSFR134 390 /* Virtual SFR 134 $vsfr134 $s390 */
#define KVX_SFR_VSFR135 391 /* Virtual SFR 135 $vsfr135 $s391 */
#define KVX_SFR_VSFR136 392 /* Virtual SFR 136 $vsfr136 $s392 */
#define KVX_SFR_VSFR137 393 /* Virtual SFR 137 $vsfr137 $s393 */
#define KVX_SFR_VSFR138 394 /* Virtual SFR 138 $vsfr138 $s394 */
#define KVX_SFR_VSFR139 395 /* Virtual SFR 139 $vsfr139 $s395 */
#define KVX_SFR_VSFR140 396 /* Virtual SFR 140 $vsfr140 $s396 */
#define KVX_SFR_VSFR141 397 /* Virtual SFR 141 $vsfr141 $s397 */
#define KVX_SFR_VSFR142 398 /* Virtual SFR 142 $vsfr142 $s398 */
#define KVX_SFR_VSFR143 399 /* Virtual SFR 143 $vsfr143 $s399 */
#define KVX_SFR_VSFR144 400 /* Virtual SFR 144 $vsfr144 $s400 */
#define KVX_SFR_VSFR145 401 /* Virtual SFR 145 $vsfr145 $s401 */
#define KVX_SFR_VSFR146 402 /* Virtual SFR 146 $vsfr146 $s402 */
#define KVX_SFR_VSFR147 403 /* Virtual SFR 147 $vsfr147 $s403 */
#define KVX_SFR_VSFR148 404 /* Virtual SFR 148 $vsfr148 $s404 */
#define KVX_SFR_VSFR149 405 /* Virtual SFR 149 $vsfr149 $s405 */
#define KVX_SFR_VSFR150 406 /* Virtual SFR 150 $vsfr150 $s406 */
#define KVX_SFR_VSFR151 407 /* Virtual SFR 151 $vsfr151 $s407 */
#define KVX_SFR_VSFR152 408 /* Virtual SFR 152 $vsfr152 $s408 */
#define KVX_SFR_VSFR153 409 /* Virtual SFR 153 $vsfr153 $s409 */
#define KVX_SFR_VSFR154 410 /* Virtual SFR 154 $vsfr154 $s410 */
#define KVX_SFR_VSFR155 411 /* Virtual SFR 155 $vsfr155 $s411 */
#define KVX_SFR_VSFR156 412 /* Virtual SFR 156 $vsfr156 $s412 */
#define KVX_SFR_VSFR157 413 /* Virtual SFR 157 $vsfr157 $s413 */
#define KVX_SFR_VSFR158 414 /* Virtual SFR 158 $vsfr158 $s414 */
#define KVX_SFR_VSFR159 415 /* Virtual SFR 159 $vsfr159 $s415 */
#define KVX_SFR_VSFR160 416 /* Virtual SFR 160 $vsfr160 $s416 */
#define KVX_SFR_VSFR161 417 /* Virtual SFR 161 $vsfr161 $s417 */
#define KVX_SFR_VSFR162 418 /* Virtual SFR 162 $vsfr162 $s418 */
#define KVX_SFR_VSFR163 419 /* Virtual SFR 163 $vsfr163 $s419 */
#define KVX_SFR_VSFR164 420 /* Virtual SFR 164 $vsfr164 $s420 */
#define KVX_SFR_VSFR165 421 /* Virtual SFR 165 $vsfr165 $s421 */
#define KVX_SFR_VSFR166 422 /* Virtual SFR 166 $vsfr166 $s422 */
#define KVX_SFR_VSFR167 423 /* Virtual SFR 167 $vsfr167 $s423 */
#define KVX_SFR_VSFR168 424 /* Virtual SFR 168 $vsfr168 $s424 */
#define KVX_SFR_VSFR169 425 /* Virtual SFR 169 $vsfr169 $s425 */
#define KVX_SFR_VSFR170 426 /* Virtual SFR 170 $vsfr170 $s426 */
#define KVX_SFR_VSFR171 427 /* Virtual SFR 171 $vsfr171 $s427 */
#define KVX_SFR_VSFR172 428 /* Virtual SFR 172 $vsfr172 $s428 */
#define KVX_SFR_VSFR173 429 /* Virtual SFR 173 $vsfr173 $s429 */
#define KVX_SFR_VSFR174 430 /* Virtual SFR 174 $vsfr174 $s430 */
#define KVX_SFR_VSFR175 431 /* Virtual SFR 175 $vsfr175 $s431 */
#define KVX_SFR_VSFR176 432 /* Virtual SFR 176 $vsfr176 $s432 */
#define KVX_SFR_VSFR177 433 /* Virtual SFR 177 $vsfr177 $s433 */
#define KVX_SFR_VSFR178 434 /* Virtual SFR 178 $vsfr178 $s434 */
#define KVX_SFR_VSFR179 435 /* Virtual SFR 179 $vsfr179 $s435 */
#define KVX_SFR_VSFR180 436 /* Virtual SFR 180 $vsfr180 $s436 */
#define KVX_SFR_VSFR181 437 /* Virtual SFR 181 $vsfr181 $s437 */
#define KVX_SFR_VSFR182 438 /* Virtual SFR 182 $vsfr182 $s438 */
#define KVX_SFR_VSFR183 439 /* Virtual SFR 183 $vsfr183 $s439 */
#define KVX_SFR_VSFR184 440 /* Virtual SFR 184 $vsfr184 $s440 */
#define KVX_SFR_VSFR185 441 /* Virtual SFR 185 $vsfr185 $s441 */
#define KVX_SFR_VSFR186 442 /* Virtual SFR 186 $vsfr186 $s442 */
#define KVX_SFR_VSFR187 443 /* Virtual SFR 187 $vsfr187 $s443 */
#define KVX_SFR_VSFR188 444 /* Virtual SFR 188 $vsfr188 $s444 */
#define KVX_SFR_VSFR189 445 /* Virtual SFR 189 $vsfr189 $s445 */
#define KVX_SFR_VSFR190 446 /* Virtual SFR 190 $vsfr190 $s446 */
#define KVX_SFR_VSFR191 447 /* Virtual SFR 191 $vsfr191 $s447 */
#define KVX_SFR_VSFR192 448 /* Virtual SFR 192 $vsfr192 $s448 */
#define KVX_SFR_VSFR193 449 /* Virtual SFR 193 $vsfr193 $s449 */
#define KVX_SFR_VSFR194 450 /* Virtual SFR 194 $vsfr194 $s450 */
#define KVX_SFR_VSFR195 451 /* Virtual SFR 195 $vsfr195 $s451 */
#define KVX_SFR_VSFR196 452 /* Virtual SFR 196 $vsfr196 $s452 */
#define KVX_SFR_VSFR197 453 /* Virtual SFR 197 $vsfr197 $s453 */
#define KVX_SFR_VSFR198 454 /* Virtual SFR 198 $vsfr198 $s454 */
#define KVX_SFR_VSFR199 455 /* Virtual SFR 199 $vsfr199 $s455 */
#define KVX_SFR_VSFR200 456 /* Virtual SFR 200 $vsfr200 $s456 */
#define KVX_SFR_VSFR201 457 /* Virtual SFR 201 $vsfr201 $s457 */
#define KVX_SFR_VSFR202 458 /* Virtual SFR 202 $vsfr202 $s458 */
#define KVX_SFR_VSFR203 459 /* Virtual SFR 203 $vsfr203 $s459 */
#define KVX_SFR_VSFR204 460 /* Virtual SFR 204 $vsfr204 $s460 */
#define KVX_SFR_VSFR205 461 /* Virtual SFR 205 $vsfr205 $s461 */
#define KVX_SFR_VSFR206 462 /* Virtual SFR 206 $vsfr206 $s462 */
#define KVX_SFR_VSFR207 463 /* Virtual SFR 207 $vsfr207 $s463 */
#define KVX_SFR_VSFR208 464 /* Virtual SFR 208 $vsfr208 $s464 */
#define KVX_SFR_VSFR209 465 /* Virtual SFR 209 $vsfr209 $s465 */
#define KVX_SFR_VSFR210 466 /* Virtual SFR 210 $vsfr210 $s466 */
#define KVX_SFR_VSFR211 467 /* Virtual SFR 211 $vsfr211 $s467 */
#define KVX_SFR_VSFR212 468 /* Virtual SFR 212 $vsfr212 $s468 */
#define KVX_SFR_VSFR213 469 /* Virtual SFR 213 $vsfr213 $s469 */
#define KVX_SFR_VSFR214 470 /* Virtual SFR 214 $vsfr214 $s470 */
#define KVX_SFR_VSFR215 471 /* Virtual SFR 215 $vsfr215 $s471 */
#define KVX_SFR_VSFR216 472 /* Virtual SFR 216 $vsfr216 $s472 */
#define KVX_SFR_VSFR217 473 /* Virtual SFR 217 $vsfr217 $s473 */
#define KVX_SFR_VSFR218 474 /* Virtual SFR 218 $vsfr218 $s474 */
#define KVX_SFR_VSFR219 475 /* Virtual SFR 219 $vsfr219 $s475 */
#define KVX_SFR_VSFR220 476 /* Virtual SFR 220 $vsfr220 $s476 */
#define KVX_SFR_VSFR221 477 /* Virtual SFR 221 $vsfr221 $s477 */
#define KVX_SFR_VSFR222 478 /* Virtual SFR 222 $vsfr222 $s478 */
#define KVX_SFR_VSFR223 479 /* Virtual SFR 223 $vsfr223 $s479 */
#define KVX_SFR_VSFR224 480 /* Virtual SFR 224 $vsfr224 $s480 */
#define KVX_SFR_VSFR225 481 /* Virtual SFR 225 $vsfr225 $s481 */
#define KVX_SFR_VSFR226 482 /* Virtual SFR 226 $vsfr226 $s482 */
#define KVX_SFR_VSFR227 483 /* Virtual SFR 227 $vsfr227 $s483 */
#define KVX_SFR_VSFR228 484 /* Virtual SFR 228 $vsfr228 $s484 */
#define KVX_SFR_VSFR229 485 /* Virtual SFR 229 $vsfr229 $s485 */
#define KVX_SFR_VSFR230 486 /* Virtual SFR 230 $vsfr230 $s486 */
#define KVX_SFR_VSFR231 487 /* Virtual SFR 231 $vsfr231 $s487 */
#define KVX_SFR_VSFR232 488 /* Virtual SFR 232 $vsfr232 $s488 */
#define KVX_SFR_VSFR233 489 /* Virtual SFR 233 $vsfr233 $s489 */
#define KVX_SFR_VSFR234 490 /* Virtual SFR 234 $vsfr234 $s490 */
#define KVX_SFR_VSFR235 491 /* Virtual SFR 235 $vsfr235 $s491 */
#define KVX_SFR_VSFR236 492 /* Virtual SFR 236 $vsfr236 $s492 */
#define KVX_SFR_VSFR237 493 /* Virtual SFR 237 $vsfr237 $s493 */
#define KVX_SFR_VSFR238 494 /* Virtual SFR 238 $vsfr238 $s494 */
#define KVX_SFR_VSFR239 495 /* Virtual SFR 239 $vsfr239 $s495 */
#define KVX_SFR_VSFR240 496 /* Virtual SFR 240 $vsfr240 $s496 */
#define KVX_SFR_VSFR241 497 /* Virtual SFR 241 $vsfr241 $s497 */
#define KVX_SFR_VSFR242 498 /* Virtual SFR 242 $vsfr242 $s498 */
#define KVX_SFR_VSFR243 499 /* Virtual SFR 243 $vsfr243 $s499 */
#define KVX_SFR_VSFR244 500 /* Virtual SFR 244 $vsfr244 $s500 */
#define KVX_SFR_VSFR245 501 /* Virtual SFR 245 $vsfr245 $s501 */
#define KVX_SFR_VSFR246 502 /* Virtual SFR 246 $vsfr246 $s502 */
#define KVX_SFR_VSFR247 503 /* Virtual SFR 247 $vsfr247 $s503 */
#define KVX_SFR_VSFR248 504 /* Virtual SFR 248 $vsfr248 $s504 */
#define KVX_SFR_VSFR249 505 /* Virtual SFR 249 $vsfr249 $s505 */
#define KVX_SFR_VSFR250 506 /* Virtual SFR 250 $vsfr250 $s506 */
#define KVX_SFR_VSFR251 507 /* Virtual SFR 251 $vsfr251 $s507 */
#define KVX_SFR_VSFR252 508 /* Virtual SFR 252 $vsfr252 $s508 */
#define KVX_SFR_VSFR253 509 /* Virtual SFR 253 $vsfr253 $s509 */
#define KVX_SFR_VSFR254 510 /* Virtual SFR 254 $vsfr254 $s510 */
#define KVX_SFR_VSFR255 511 /* Virtual SFR 255 $vsfr255 $s511 */

/* Register field masks */

#define KVX_SFR_MEN_MEN_MASK _ULL(0xffff) /* Miscellaneous External Notifications */
#define KVX_SFR_MEN_MEN_SHIFT 0
#define KVX_SFR_MEN_MEN_WIDTH 16
#define KVX_SFR_MEN_MEN_WFXL_MASK _ULL(0xffff)
#define KVX_SFR_MEN_MEN_WFXL_CLEAR _ULL(0xffff)
#define KVX_SFR_MEN_MEN_WFXL_SET _ULL(0xffff00000000)

#define KVX_SFR_SYO_Q0_MASK _ULL(0x3) /* Quarter 0 syscalls 0 to 1023 owner */
#define KVX_SFR_SYO_Q0_SHIFT 0
#define KVX_SFR_SYO_Q0_WIDTH 2
#define KVX_SFR_SYO_Q0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SYO_Q0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SYO_Q0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SYO_Q1_MASK _ULL(0xc) /* Quarter 1 syscalls 1024 to 2047 owner */
#define KVX_SFR_SYO_Q1_SHIFT 2
#define KVX_SFR_SYO_Q1_WIDTH 2
#define KVX_SFR_SYO_Q1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_SYO_Q1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_SYO_Q1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_SYO_Q2_MASK _ULL(0x30) /* Quarter 2 syscalls 2048 to 3071 owner */
#define KVX_SFR_SYO_Q2_SHIFT 4
#define KVX_SFR_SYO_Q2_WIDTH 2
#define KVX_SFR_SYO_Q2_WFXL_MASK _ULL(0x30)
#define KVX_SFR_SYO_Q2_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_SYO_Q2_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_SYO_Q3_MASK _ULL(0xc0) /* Quarter 3 syscalls 3072 to 4095 owner */
#define KVX_SFR_SYO_Q3_SHIFT 6
#define KVX_SFR_SYO_Q3_WIDTH 2
#define KVX_SFR_SYO_Q3_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_SYO_Q3_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_SYO_Q3_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_SYOW_Q0_MASK _ULL(0x3) /* Quarter 0 syscalls 0 to 1023 owner */
#define KVX_SFR_SYOW_Q0_SHIFT 0
#define KVX_SFR_SYOW_Q0_WIDTH 2
#define KVX_SFR_SYOW_Q0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SYOW_Q0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SYOW_Q0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SYOW_Q1_MASK _ULL(0xc) /* Quarter 1 syscalls 1024 to 2047 owner */
#define KVX_SFR_SYOW_Q1_SHIFT 2
#define KVX_SFR_SYOW_Q1_WIDTH 2
#define KVX_SFR_SYOW_Q1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_SYOW_Q1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_SYOW_Q1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_SYOW_Q2_MASK _ULL(0x30) /* Quarter 2 syscalls 2048 to 3071 owner */
#define KVX_SFR_SYOW_Q2_SHIFT 4
#define KVX_SFR_SYOW_Q2_WIDTH 2
#define KVX_SFR_SYOW_Q2_WFXL_MASK _ULL(0x30)
#define KVX_SFR_SYOW_Q2_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_SYOW_Q2_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_SYOW_Q3_MASK _ULL(0xc0) /* Quarter 3 syscalls 3072 to 4095 owner */
#define KVX_SFR_SYOW_Q3_SHIFT 6
#define KVX_SFR_SYOW_Q3_WIDTH 2
#define KVX_SFR_SYOW_Q3_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_SYOW_Q3_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_SYOW_Q3_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_HTO_OPC_MASK _ULL(0x3) /* OPCode trap owner */
#define KVX_SFR_HTO_OPC_SHIFT 0
#define KVX_SFR_HTO_OPC_WIDTH 2
#define KVX_SFR_HTO_OPC_WFXL_MASK _ULL(0x3)
#define KVX_SFR_HTO_OPC_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_HTO_OPC_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_HTO_DMIS_MASK _ULL(0xc) /* Data MISalign access trap owner */
#define KVX_SFR_HTO_DMIS_SHIFT 2
#define KVX_SFR_HTO_DMIS_WIDTH 2
#define KVX_SFR_HTO_DMIS_WFXL_MASK _ULL(0xc)
#define KVX_SFR_HTO_DMIS_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_HTO_DMIS_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_HTO_PSYS_MASK _ULL(0x30) /* Program System Error trap owner */
#define KVX_SFR_HTO_PSYS_SHIFT 4
#define KVX_SFR_HTO_PSYS_WIDTH 2
#define KVX_SFR_HTO_PSYS_WFXL_MASK _ULL(0x30)
#define KVX_SFR_HTO_PSYS_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_HTO_PSYS_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_HTO_DSYS_MASK _ULL(0xc0) /* Data System Error trap owner */
#define KVX_SFR_HTO_DSYS_SHIFT 6
#define KVX_SFR_HTO_DSYS_WIDTH 2
#define KVX_SFR_HTO_DSYS_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_HTO_DSYS_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_HTO_DSYS_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_HTO_DECCG_MASK _ULL(0x300) /* Double ECC traps group owner */
#define KVX_SFR_HTO_DECCG_SHIFT 8
#define KVX_SFR_HTO_DECCG_WIDTH 2
#define KVX_SFR_HTO_DECCG_WFXL_MASK _ULL(0x300)
#define KVX_SFR_HTO_DECCG_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_HTO_DECCG_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_HTO_SECCG_MASK _ULL(0xc00) /* Single ECC traps group owner */
#define KVX_SFR_HTO_SECCG_SHIFT 10
#define KVX_SFR_HTO_SECCG_WIDTH 2
#define KVX_SFR_HTO_SECCG_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_HTO_SECCG_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_HTO_SECCG_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_HTO_NOMAP_MASK _ULL(0x3000) /* No mapping trap owner */
#define KVX_SFR_HTO_NOMAP_SHIFT 12
#define KVX_SFR_HTO_NOMAP_WIDTH 2
#define KVX_SFR_HTO_NOMAP_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_HTO_NOMAP_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_HTO_NOMAP_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_HTO_PROT_MASK _ULL(0xc000) /* PROTection trap owner */
#define KVX_SFR_HTO_PROT_SHIFT 14
#define KVX_SFR_HTO_PROT_WIDTH 2
#define KVX_SFR_HTO_PROT_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_HTO_PROT_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_HTO_PROT_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_HTO_W2CL_MASK _ULL(0x30000) /* Write to clean trap owner */
#define KVX_SFR_HTO_W2CL_SHIFT 16
#define KVX_SFR_HTO_W2CL_WIDTH 2
#define KVX_SFR_HTO_W2CL_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_HTO_W2CL_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_HTO_W2CL_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_HTO_A2CL_MASK _ULL(0xc0000) /* Atomic to clean trap owner */
#define KVX_SFR_HTO_A2CL_SHIFT 18
#define KVX_SFR_HTO_A2CL_WIDTH 2
#define KVX_SFR_HTO_A2CL_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_HTO_A2CL_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_HTO_A2CL_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_HTO_DE_MASK _ULL(0x300000) /* Double Exception trap owner */
#define KVX_SFR_HTO_DE_SHIFT 20
#define KVX_SFR_HTO_DE_WIDTH 2
#define KVX_SFR_HTO_DE_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_HTO_DE_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_HTO_DE_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_HTO_VSFR_MASK _ULL(0xc00000) /* Virtual SFR trap owner */
#define KVX_SFR_HTO_VSFR_SHIFT 22
#define KVX_SFR_HTO_VSFR_WIDTH 2
#define KVX_SFR_HTO_VSFR_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_HTO_VSFR_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_HTO_VSFR_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_HTO_PLO_MASK _ULL(0x3000000) /* Privilege Level Overflow trap owner */
#define KVX_SFR_HTO_PLO_SHIFT 24
#define KVX_SFR_HTO_PLO_WIDTH 2
#define KVX_SFR_HTO_PLO_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_HTO_PLO_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_HTO_PLO_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_HTOW_OPC_MASK _ULL(0x3) /* OPCode trap owner */
#define KVX_SFR_HTOW_OPC_SHIFT 0
#define KVX_SFR_HTOW_OPC_WIDTH 2
#define KVX_SFR_HTOW_OPC_WFXL_MASK _ULL(0x3)
#define KVX_SFR_HTOW_OPC_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_HTOW_OPC_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_HTOW_DMIS_MASK _ULL(0xc) /* Data MISalign access trap owner */
#define KVX_SFR_HTOW_DMIS_SHIFT 2
#define KVX_SFR_HTOW_DMIS_WIDTH 2
#define KVX_SFR_HTOW_DMIS_WFXL_MASK _ULL(0xc)
#define KVX_SFR_HTOW_DMIS_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_HTOW_DMIS_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_HTOW_PSYS_MASK _ULL(0x30) /* Program System Error trap owner */
#define KVX_SFR_HTOW_PSYS_SHIFT 4
#define KVX_SFR_HTOW_PSYS_WIDTH 2
#define KVX_SFR_HTOW_PSYS_WFXL_MASK _ULL(0x30)
#define KVX_SFR_HTOW_PSYS_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_HTOW_PSYS_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_HTOW_DSYS_MASK _ULL(0xc0) /* Data System Error trap owner */
#define KVX_SFR_HTOW_DSYS_SHIFT 6
#define KVX_SFR_HTOW_DSYS_WIDTH 2
#define KVX_SFR_HTOW_DSYS_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_HTOW_DSYS_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_HTOW_DSYS_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_HTOW_DECCG_MASK _ULL(0x300) /* Double ECC traps group owner */
#define KVX_SFR_HTOW_DECCG_SHIFT 8
#define KVX_SFR_HTOW_DECCG_WIDTH 2
#define KVX_SFR_HTOW_DECCG_WFXL_MASK _ULL(0x300)
#define KVX_SFR_HTOW_DECCG_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_HTOW_DECCG_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_HTOW_SECCG_MASK _ULL(0xc00) /* Single ECC traps group owner */
#define KVX_SFR_HTOW_SECCG_SHIFT 10
#define KVX_SFR_HTOW_SECCG_WIDTH 2
#define KVX_SFR_HTOW_SECCG_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_HTOW_SECCG_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_HTOW_SECCG_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_HTOW_NOMAP_MASK _ULL(0x3000) /* No mapping trap owner */
#define KVX_SFR_HTOW_NOMAP_SHIFT 12
#define KVX_SFR_HTOW_NOMAP_WIDTH 2
#define KVX_SFR_HTOW_NOMAP_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_HTOW_NOMAP_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_HTOW_NOMAP_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_HTOW_PROT_MASK _ULL(0xc000) /* PROTection trap owner */
#define KVX_SFR_HTOW_PROT_SHIFT 14
#define KVX_SFR_HTOW_PROT_WIDTH 2
#define KVX_SFR_HTOW_PROT_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_HTOW_PROT_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_HTOW_PROT_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_HTOW_W2CL_MASK _ULL(0x30000) /* Write to clean trap owner */
#define KVX_SFR_HTOW_W2CL_SHIFT 16
#define KVX_SFR_HTOW_W2CL_WIDTH 2
#define KVX_SFR_HTOW_W2CL_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_HTOW_W2CL_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_HTOW_W2CL_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_HTOW_A2CL_MASK _ULL(0xc0000) /* Atomic to clean trap owner */
#define KVX_SFR_HTOW_A2CL_SHIFT 18
#define KVX_SFR_HTOW_A2CL_WIDTH 2
#define KVX_SFR_HTOW_A2CL_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_HTOW_A2CL_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_HTOW_A2CL_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_HTOW_DE_MASK _ULL(0x300000) /* Double Exception trap owner */
#define KVX_SFR_HTOW_DE_SHIFT 20
#define KVX_SFR_HTOW_DE_WIDTH 2
#define KVX_SFR_HTOW_DE_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_HTOW_DE_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_HTOW_DE_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_HTOW_VSFR_MASK _ULL(0xc00000) /* Virtual SFR trap owner */
#define KVX_SFR_HTOW_VSFR_SHIFT 22
#define KVX_SFR_HTOW_VSFR_WIDTH 2
#define KVX_SFR_HTOW_VSFR_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_HTOW_VSFR_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_HTOW_VSFR_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_HTOW_PLO_MASK _ULL(0x3000000) /* Privilege Level Overflow trap owner */
#define KVX_SFR_HTOW_PLO_SHIFT 24
#define KVX_SFR_HTOW_PLO_WIDTH 2
#define KVX_SFR_HTOW_PLO_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_HTOW_PLO_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_HTOW_PLO_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_ITO_IT0_MASK _ULL(0x3) /* Interrupt 0 owner */
#define KVX_SFR_ITO_IT0_SHIFT 0
#define KVX_SFR_ITO_IT0_WIDTH 2
#define KVX_SFR_ITO_IT0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_ITO_IT0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_ITO_IT0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_ITO_IT1_MASK _ULL(0xc) /* Interrupt 1 owner */
#define KVX_SFR_ITO_IT1_SHIFT 2
#define KVX_SFR_ITO_IT1_WIDTH 2
#define KVX_SFR_ITO_IT1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_ITO_IT1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_ITO_IT1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_ITO_IT2_MASK _ULL(0x30) /* Interrupt 2 owner */
#define KVX_SFR_ITO_IT2_SHIFT 4
#define KVX_SFR_ITO_IT2_WIDTH 2
#define KVX_SFR_ITO_IT2_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ITO_IT2_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ITO_IT2_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ITO_IT3_MASK _ULL(0xc0) /* Interrupt 3 owner */
#define KVX_SFR_ITO_IT3_SHIFT 6
#define KVX_SFR_ITO_IT3_WIDTH 2
#define KVX_SFR_ITO_IT3_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ITO_IT3_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ITO_IT3_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ITO_IT4_MASK _ULL(0x300) /* Interrupt 4 owner */
#define KVX_SFR_ITO_IT4_SHIFT 8
#define KVX_SFR_ITO_IT4_WIDTH 2
#define KVX_SFR_ITO_IT4_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ITO_IT4_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ITO_IT4_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ITO_IT5_MASK _ULL(0xc00) /* Interrupt 5 owner */
#define KVX_SFR_ITO_IT5_SHIFT 10
#define KVX_SFR_ITO_IT5_WIDTH 2
#define KVX_SFR_ITO_IT5_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ITO_IT5_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ITO_IT5_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ITO_IT6_MASK _ULL(0x3000) /* Interrupt 6 owner */
#define KVX_SFR_ITO_IT6_SHIFT 12
#define KVX_SFR_ITO_IT6_WIDTH 2
#define KVX_SFR_ITO_IT6_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ITO_IT6_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ITO_IT6_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ITO_IT7_MASK _ULL(0xc000) /* Interrupt 7 owner */
#define KVX_SFR_ITO_IT7_SHIFT 14
#define KVX_SFR_ITO_IT7_WIDTH 2
#define KVX_SFR_ITO_IT7_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_ITO_IT7_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_ITO_IT7_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_ITO_IT8_MASK _ULL(0x30000) /* Interrupt 8 owner */
#define KVX_SFR_ITO_IT8_SHIFT 16
#define KVX_SFR_ITO_IT8_WIDTH 2
#define KVX_SFR_ITO_IT8_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_ITO_IT8_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_ITO_IT8_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_ITO_IT9_MASK _ULL(0xc0000) /* Interrupt 9 owner */
#define KVX_SFR_ITO_IT9_SHIFT 18
#define KVX_SFR_ITO_IT9_WIDTH 2
#define KVX_SFR_ITO_IT9_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_ITO_IT9_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_ITO_IT9_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_ITO_IT10_MASK _ULL(0x300000) /* Interrupt 10 owner */
#define KVX_SFR_ITO_IT10_SHIFT 20
#define KVX_SFR_ITO_IT10_WIDTH 2
#define KVX_SFR_ITO_IT10_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_ITO_IT10_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_ITO_IT10_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_ITO_IT11_MASK _ULL(0xc00000) /* Interrupt 11 owner */
#define KVX_SFR_ITO_IT11_SHIFT 22
#define KVX_SFR_ITO_IT11_WIDTH 2
#define KVX_SFR_ITO_IT11_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_ITO_IT11_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_ITO_IT11_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_ITO_IT12_MASK _ULL(0x3000000) /* Interrupt 12 owner */
#define KVX_SFR_ITO_IT12_SHIFT 24
#define KVX_SFR_ITO_IT12_WIDTH 2
#define KVX_SFR_ITO_IT12_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_ITO_IT12_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_ITO_IT12_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_ITO_IT13_MASK _ULL(0xc000000) /* Interrupt 13 owner */
#define KVX_SFR_ITO_IT13_SHIFT 26
#define KVX_SFR_ITO_IT13_WIDTH 2
#define KVX_SFR_ITO_IT13_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_ITO_IT13_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_ITO_IT13_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_ITO_IT14_MASK _ULL(0x30000000) /* Interrupt 14 owner */
#define KVX_SFR_ITO_IT14_SHIFT 28
#define KVX_SFR_ITO_IT14_WIDTH 2
#define KVX_SFR_ITO_IT14_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_ITO_IT14_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_ITO_IT14_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_ITO_IT15_MASK _ULL(0xc0000000) /* Interrupt 15 owner */
#define KVX_SFR_ITO_IT15_SHIFT 30
#define KVX_SFR_ITO_IT15_WIDTH 2
#define KVX_SFR_ITO_IT15_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_ITO_IT15_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ITO_IT15_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_ITO_IT16_MASK _ULL(0x300000000) /* Interrupt 16 owner */
#define KVX_SFR_ITO_IT16_SHIFT 32
#define KVX_SFR_ITO_IT16_WIDTH 2
#define KVX_SFR_ITO_IT16_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_ITO_IT16_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_ITO_IT16_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_ITO_IT17_MASK _ULL(0xc00000000) /* Interrupt 17 owner */
#define KVX_SFR_ITO_IT17_SHIFT 34
#define KVX_SFR_ITO_IT17_WIDTH 2
#define KVX_SFR_ITO_IT17_WFXM_MASK _ULL(0xc00000000)
#define KVX_SFR_ITO_IT17_WFXM_CLEAR _ULL(0xc)
#define KVX_SFR_ITO_IT17_WFXM_SET _ULL(0xc00000000)

#define KVX_SFR_ITO_IT18_MASK _ULL(0x3000000000) /* Interrupt 18 owner */
#define KVX_SFR_ITO_IT18_SHIFT 36
#define KVX_SFR_ITO_IT18_WIDTH 2
#define KVX_SFR_ITO_IT18_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_ITO_IT18_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_ITO_IT18_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_ITO_IT19_MASK _ULL(0xc000000000) /* Interrupt 19 owner */
#define KVX_SFR_ITO_IT19_SHIFT 38
#define KVX_SFR_ITO_IT19_WIDTH 2
#define KVX_SFR_ITO_IT19_WFXM_MASK _ULL(0xc000000000)
#define KVX_SFR_ITO_IT19_WFXM_CLEAR _ULL(0xc0)
#define KVX_SFR_ITO_IT19_WFXM_SET _ULL(0xc000000000)

#define KVX_SFR_ITO_IT20_MASK _ULL(0x30000000000) /* Interrupt 20 owner */
#define KVX_SFR_ITO_IT20_SHIFT 40
#define KVX_SFR_ITO_IT20_WIDTH 2
#define KVX_SFR_ITO_IT20_WFXM_MASK _ULL(0x30000000000)
#define KVX_SFR_ITO_IT20_WFXM_CLEAR _ULL(0x300)
#define KVX_SFR_ITO_IT20_WFXM_SET _ULL(0x30000000000)

#define KVX_SFR_ITO_IT21_MASK _ULL(0xc0000000000) /* Interrupt 21 owner */
#define KVX_SFR_ITO_IT21_SHIFT 42
#define KVX_SFR_ITO_IT21_WIDTH 2
#define KVX_SFR_ITO_IT21_WFXM_MASK _ULL(0xc0000000000)
#define KVX_SFR_ITO_IT21_WFXM_CLEAR _ULL(0xc00)
#define KVX_SFR_ITO_IT21_WFXM_SET _ULL(0xc0000000000)

#define KVX_SFR_ITO_IT22_MASK _ULL(0x300000000000) /* Interrupt 22 owner */
#define KVX_SFR_ITO_IT22_SHIFT 44
#define KVX_SFR_ITO_IT22_WIDTH 2
#define KVX_SFR_ITO_IT22_WFXM_MASK _ULL(0x300000000000)
#define KVX_SFR_ITO_IT22_WFXM_CLEAR _ULL(0x3000)
#define KVX_SFR_ITO_IT22_WFXM_SET _ULL(0x300000000000)

#define KVX_SFR_ITO_IT23_MASK _ULL(0xc00000000000) /* Interrupt 23 owner */
#define KVX_SFR_ITO_IT23_SHIFT 46
#define KVX_SFR_ITO_IT23_WIDTH 2
#define KVX_SFR_ITO_IT23_WFXM_MASK _ULL(0xc00000000000)
#define KVX_SFR_ITO_IT23_WFXM_CLEAR _ULL(0xc000)
#define KVX_SFR_ITO_IT23_WFXM_SET _ULL(0xc00000000000)

#define KVX_SFR_ITO_IT24_MASK _ULL(0x3000000000000) /* Interrupt 24 owner */
#define KVX_SFR_ITO_IT24_SHIFT 48
#define KVX_SFR_ITO_IT24_WIDTH 2
#define KVX_SFR_ITO_IT24_WFXM_MASK _ULL(0x3000000000000)
#define KVX_SFR_ITO_IT24_WFXM_CLEAR _ULL(0x30000)
#define KVX_SFR_ITO_IT24_WFXM_SET _ULL(0x3000000000000)

#define KVX_SFR_ITO_IT25_MASK _ULL(0xc000000000000) /* Interrupt 25 owner */
#define KVX_SFR_ITO_IT25_SHIFT 50
#define KVX_SFR_ITO_IT25_WIDTH 2
#define KVX_SFR_ITO_IT25_WFXM_MASK _ULL(0xc000000000000)
#define KVX_SFR_ITO_IT25_WFXM_CLEAR _ULL(0xc0000)
#define KVX_SFR_ITO_IT25_WFXM_SET _ULL(0xc000000000000)

#define KVX_SFR_ITO_IT26_MASK _ULL(0x30000000000000) /* Interrupt 26 owner */
#define KVX_SFR_ITO_IT26_SHIFT 52
#define KVX_SFR_ITO_IT26_WIDTH 2
#define KVX_SFR_ITO_IT26_WFXM_MASK _ULL(0x30000000000000)
#define KVX_SFR_ITO_IT26_WFXM_CLEAR _ULL(0x300000)
#define KVX_SFR_ITO_IT26_WFXM_SET _ULL(0x30000000000000)

#define KVX_SFR_ITO_IT27_MASK _ULL(0xc0000000000000) /* Interrupt 27 owner */
#define KVX_SFR_ITO_IT27_SHIFT 54
#define KVX_SFR_ITO_IT27_WIDTH 2
#define KVX_SFR_ITO_IT27_WFXM_MASK _ULL(0xc0000000000000)
#define KVX_SFR_ITO_IT27_WFXM_CLEAR _ULL(0xc00000)
#define KVX_SFR_ITO_IT27_WFXM_SET _ULL(0xc0000000000000)

#define KVX_SFR_ITO_IT28_MASK _ULL(0x300000000000000) /* Interrupt 28 owner */
#define KVX_SFR_ITO_IT28_SHIFT 56
#define KVX_SFR_ITO_IT28_WIDTH 2
#define KVX_SFR_ITO_IT28_WFXM_MASK _ULL(0x300000000000000)
#define KVX_SFR_ITO_IT28_WFXM_CLEAR _ULL(0x3000000)
#define KVX_SFR_ITO_IT28_WFXM_SET _ULL(0x300000000000000)

#define KVX_SFR_ITO_IT29_MASK _ULL(0xc00000000000000) /* Interrupt 29 owner */
#define KVX_SFR_ITO_IT29_SHIFT 58
#define KVX_SFR_ITO_IT29_WIDTH 2
#define KVX_SFR_ITO_IT29_WFXM_MASK _ULL(0xc00000000000000)
#define KVX_SFR_ITO_IT29_WFXM_CLEAR _ULL(0xc000000)
#define KVX_SFR_ITO_IT29_WFXM_SET _ULL(0xc00000000000000)

#define KVX_SFR_ITO_IT30_MASK _ULL(0x3000000000000000) /* Interrupt 30 owner */
#define KVX_SFR_ITO_IT30_SHIFT 60
#define KVX_SFR_ITO_IT30_WIDTH 2
#define KVX_SFR_ITO_IT30_WFXM_MASK _ULL(0x3000000000000000)
#define KVX_SFR_ITO_IT30_WFXM_CLEAR _ULL(0x30000000)
#define KVX_SFR_ITO_IT30_WFXM_SET _ULL(0x3000000000000000)

#define KVX_SFR_ITO_IT31_MASK _ULL(0xc000000000000000) /* Interrupt 31 owner */
#define KVX_SFR_ITO_IT31_SHIFT 62
#define KVX_SFR_ITO_IT31_WIDTH 2
#define KVX_SFR_ITO_IT31_WFXM_MASK _ULL(0xc000000000000000)
#define KVX_SFR_ITO_IT31_WFXM_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ITO_IT31_WFXM_SET _ULL(0xc000000000000000)

#define KVX_SFR_ILE_IT0_MASK _ULL(0x1) /* Interrupt 0 owner */
#define KVX_SFR_ILE_IT0_SHIFT 0
#define KVX_SFR_ILE_IT0_WIDTH 1
#define KVX_SFR_ILE_IT0_WFXL_MASK _ULL(0x1)
#define KVX_SFR_ILE_IT0_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_ILE_IT0_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_ILE_IT1_MASK _ULL(0x2) /* Interrupt 1 owner */
#define KVX_SFR_ILE_IT1_SHIFT 1
#define KVX_SFR_ILE_IT1_WIDTH 1
#define KVX_SFR_ILE_IT1_WFXL_MASK _ULL(0x2)
#define KVX_SFR_ILE_IT1_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_ILE_IT1_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_ILE_IT2_MASK _ULL(0x4) /* Interrupt 2 owner */
#define KVX_SFR_ILE_IT2_SHIFT 2
#define KVX_SFR_ILE_IT2_WIDTH 1
#define KVX_SFR_ILE_IT2_WFXL_MASK _ULL(0x4)
#define KVX_SFR_ILE_IT2_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_ILE_IT2_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_ILE_IT3_MASK _ULL(0x8) /* Interrupt 3 owner */
#define KVX_SFR_ILE_IT3_SHIFT 3
#define KVX_SFR_ILE_IT3_WIDTH 1
#define KVX_SFR_ILE_IT3_WFXL_MASK _ULL(0x8)
#define KVX_SFR_ILE_IT3_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_ILE_IT3_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_ILE_IT4_MASK _ULL(0x10) /* Interrupt 4 owner */
#define KVX_SFR_ILE_IT4_SHIFT 4
#define KVX_SFR_ILE_IT4_WIDTH 1
#define KVX_SFR_ILE_IT4_WFXL_MASK _ULL(0x10)
#define KVX_SFR_ILE_IT4_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_ILE_IT4_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_ILE_IT5_MASK _ULL(0x20) /* Interrupt 5 owner */
#define KVX_SFR_ILE_IT5_SHIFT 5
#define KVX_SFR_ILE_IT5_WIDTH 1
#define KVX_SFR_ILE_IT5_WFXL_MASK _ULL(0x20)
#define KVX_SFR_ILE_IT5_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_ILE_IT5_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_ILE_IT6_MASK _ULL(0x40) /* Interrupt 6 owner */
#define KVX_SFR_ILE_IT6_SHIFT 6
#define KVX_SFR_ILE_IT6_WIDTH 1
#define KVX_SFR_ILE_IT6_WFXL_MASK _ULL(0x40)
#define KVX_SFR_ILE_IT6_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_ILE_IT6_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_ILE_IT7_MASK _ULL(0x80) /* Interrupt 7 owner */
#define KVX_SFR_ILE_IT7_SHIFT 7
#define KVX_SFR_ILE_IT7_WIDTH 1
#define KVX_SFR_ILE_IT7_WFXL_MASK _ULL(0x80)
#define KVX_SFR_ILE_IT7_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_ILE_IT7_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_ILE_IT8_MASK _ULL(0x100) /* Interrupt 8 owner */
#define KVX_SFR_ILE_IT8_SHIFT 8
#define KVX_SFR_ILE_IT8_WIDTH 1
#define KVX_SFR_ILE_IT8_WFXL_MASK _ULL(0x100)
#define KVX_SFR_ILE_IT8_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_ILE_IT8_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_ILE_IT9_MASK _ULL(0x200) /* Interrupt 9 owner */
#define KVX_SFR_ILE_IT9_SHIFT 9
#define KVX_SFR_ILE_IT9_WIDTH 1
#define KVX_SFR_ILE_IT9_WFXL_MASK _ULL(0x200)
#define KVX_SFR_ILE_IT9_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_ILE_IT9_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_ILE_IT10_MASK _ULL(0x400) /* Interrupt 10 owner */
#define KVX_SFR_ILE_IT10_SHIFT 10
#define KVX_SFR_ILE_IT10_WIDTH 1
#define KVX_SFR_ILE_IT10_WFXL_MASK _ULL(0x400)
#define KVX_SFR_ILE_IT10_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_ILE_IT10_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_ILE_IT11_MASK _ULL(0x800) /* Interrupt 11 owner */
#define KVX_SFR_ILE_IT11_SHIFT 11
#define KVX_SFR_ILE_IT11_WIDTH 1
#define KVX_SFR_ILE_IT11_WFXL_MASK _ULL(0x800)
#define KVX_SFR_ILE_IT11_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_ILE_IT11_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_ILE_IT12_MASK _ULL(0x1000) /* Interrupt 12 owner */
#define KVX_SFR_ILE_IT12_SHIFT 12
#define KVX_SFR_ILE_IT12_WIDTH 1
#define KVX_SFR_ILE_IT12_WFXL_MASK _ULL(0x1000)
#define KVX_SFR_ILE_IT12_WFXL_CLEAR _ULL(0x1000)
#define KVX_SFR_ILE_IT12_WFXL_SET _ULL(0x100000000000)

#define KVX_SFR_ILE_IT13_MASK _ULL(0x2000) /* Interrupt 13 owner */
#define KVX_SFR_ILE_IT13_SHIFT 13
#define KVX_SFR_ILE_IT13_WIDTH 1
#define KVX_SFR_ILE_IT13_WFXL_MASK _ULL(0x2000)
#define KVX_SFR_ILE_IT13_WFXL_CLEAR _ULL(0x2000)
#define KVX_SFR_ILE_IT13_WFXL_SET _ULL(0x200000000000)

#define KVX_SFR_ILE_IT14_MASK _ULL(0x4000) /* Interrupt 14 owner */
#define KVX_SFR_ILE_IT14_SHIFT 14
#define KVX_SFR_ILE_IT14_WIDTH 1
#define KVX_SFR_ILE_IT14_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ILE_IT14_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ILE_IT14_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ILE_IT15_MASK _ULL(0x8000) /* Interrupt 15 owner */
#define KVX_SFR_ILE_IT15_SHIFT 15
#define KVX_SFR_ILE_IT15_WIDTH 1
#define KVX_SFR_ILE_IT15_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ILE_IT15_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ILE_IT15_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ILE_IT16_MASK _ULL(0x10000) /* Interrupt 16 owner */
#define KVX_SFR_ILE_IT16_SHIFT 16
#define KVX_SFR_ILE_IT16_WIDTH 1
#define KVX_SFR_ILE_IT16_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_ILE_IT16_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_ILE_IT16_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_ILE_IT17_MASK _ULL(0x20000) /* Interrupt 17 owner */
#define KVX_SFR_ILE_IT17_SHIFT 17
#define KVX_SFR_ILE_IT17_WIDTH 1
#define KVX_SFR_ILE_IT17_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ILE_IT17_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ILE_IT17_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ILE_IT18_MASK _ULL(0x40000) /* Interrupt 18 owner */
#define KVX_SFR_ILE_IT18_SHIFT 18
#define KVX_SFR_ILE_IT18_WIDTH 1
#define KVX_SFR_ILE_IT18_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_ILE_IT18_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_ILE_IT18_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_ILE_IT19_MASK _ULL(0x80000) /* Interrupt 19 owner */
#define KVX_SFR_ILE_IT19_SHIFT 19
#define KVX_SFR_ILE_IT19_WIDTH 1
#define KVX_SFR_ILE_IT19_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_ILE_IT19_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_ILE_IT19_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_ILE_IT20_MASK _ULL(0x100000) /* Interrupt 20 owner */
#define KVX_SFR_ILE_IT20_SHIFT 20
#define KVX_SFR_ILE_IT20_WIDTH 1
#define KVX_SFR_ILE_IT20_WFXL_MASK _ULL(0x100000)
#define KVX_SFR_ILE_IT20_WFXL_CLEAR _ULL(0x100000)
#define KVX_SFR_ILE_IT20_WFXL_SET _ULL(0x10000000000000)

#define KVX_SFR_ILE_IT21_MASK _ULL(0x200000) /* Interrupt 21 owner */
#define KVX_SFR_ILE_IT21_SHIFT 21
#define KVX_SFR_ILE_IT21_WIDTH 1
#define KVX_SFR_ILE_IT21_WFXL_MASK _ULL(0x200000)
#define KVX_SFR_ILE_IT21_WFXL_CLEAR _ULL(0x200000)
#define KVX_SFR_ILE_IT21_WFXL_SET _ULL(0x20000000000000)

#define KVX_SFR_ILE_IT22_MASK _ULL(0x400000) /* Interrupt 22 owner */
#define KVX_SFR_ILE_IT22_SHIFT 22
#define KVX_SFR_ILE_IT22_WIDTH 1
#define KVX_SFR_ILE_IT22_WFXL_MASK _ULL(0x400000)
#define KVX_SFR_ILE_IT22_WFXL_CLEAR _ULL(0x400000)
#define KVX_SFR_ILE_IT22_WFXL_SET _ULL(0x40000000000000)

#define KVX_SFR_ILE_IT23_MASK _ULL(0x800000) /* Interrupt 23 owner */
#define KVX_SFR_ILE_IT23_SHIFT 23
#define KVX_SFR_ILE_IT23_WIDTH 1
#define KVX_SFR_ILE_IT23_WFXL_MASK _ULL(0x800000)
#define KVX_SFR_ILE_IT23_WFXL_CLEAR _ULL(0x800000)
#define KVX_SFR_ILE_IT23_WFXL_SET _ULL(0x80000000000000)

#define KVX_SFR_ILE_IT24_MASK _ULL(0x1000000) /* Interrupt 24 owner */
#define KVX_SFR_ILE_IT24_SHIFT 24
#define KVX_SFR_ILE_IT24_WIDTH 1
#define KVX_SFR_ILE_IT24_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_ILE_IT24_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_ILE_IT24_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_ILE_IT25_MASK _ULL(0x2000000) /* Interrupt 25 owner */
#define KVX_SFR_ILE_IT25_SHIFT 25
#define KVX_SFR_ILE_IT25_WIDTH 1
#define KVX_SFR_ILE_IT25_WFXL_MASK _ULL(0x2000000)
#define KVX_SFR_ILE_IT25_WFXL_CLEAR _ULL(0x2000000)
#define KVX_SFR_ILE_IT25_WFXL_SET _ULL(0x200000000000000)

#define KVX_SFR_ILE_IT26_MASK _ULL(0x4000000) /* Interrupt 26 owner */
#define KVX_SFR_ILE_IT26_SHIFT 26
#define KVX_SFR_ILE_IT26_WIDTH 1
#define KVX_SFR_ILE_IT26_WFXL_MASK _ULL(0x4000000)
#define KVX_SFR_ILE_IT26_WFXL_CLEAR _ULL(0x4000000)
#define KVX_SFR_ILE_IT26_WFXL_SET _ULL(0x400000000000000)

#define KVX_SFR_ILE_IT27_MASK _ULL(0x8000000) /* Interrupt 27 owner */
#define KVX_SFR_ILE_IT27_SHIFT 27
#define KVX_SFR_ILE_IT27_WIDTH 1
#define KVX_SFR_ILE_IT27_WFXL_MASK _ULL(0x8000000)
#define KVX_SFR_ILE_IT27_WFXL_CLEAR _ULL(0x8000000)
#define KVX_SFR_ILE_IT27_WFXL_SET _ULL(0x800000000000000)

#define KVX_SFR_ILE_IT28_MASK _ULL(0x10000000) /* Interrupt 28 owner */
#define KVX_SFR_ILE_IT28_SHIFT 28
#define KVX_SFR_ILE_IT28_WIDTH 1
#define KVX_SFR_ILE_IT28_WFXL_MASK _ULL(0x10000000)
#define KVX_SFR_ILE_IT28_WFXL_CLEAR _ULL(0x10000000)
#define KVX_SFR_ILE_IT28_WFXL_SET _ULL(0x1000000000000000)

#define KVX_SFR_ILE_IT29_MASK _ULL(0x20000000) /* Interrupt 29 owner */
#define KVX_SFR_ILE_IT29_SHIFT 29
#define KVX_SFR_ILE_IT29_WIDTH 1
#define KVX_SFR_ILE_IT29_WFXL_MASK _ULL(0x20000000)
#define KVX_SFR_ILE_IT29_WFXL_CLEAR _ULL(0x20000000)
#define KVX_SFR_ILE_IT29_WFXL_SET _ULL(0x2000000000000000)

#define KVX_SFR_ILE_IT30_MASK _ULL(0x40000000) /* Interrupt 30 owner */
#define KVX_SFR_ILE_IT30_SHIFT 30
#define KVX_SFR_ILE_IT30_WIDTH 1
#define KVX_SFR_ILE_IT30_WFXL_MASK _ULL(0x40000000)
#define KVX_SFR_ILE_IT30_WFXL_CLEAR _ULL(0x40000000)
#define KVX_SFR_ILE_IT30_WFXL_SET _ULL(0x4000000000000000)

#define KVX_SFR_ILE_IT31_MASK _ULL(0x80000000) /* Interrupt 31 owner */
#define KVX_SFR_ILE_IT31_SHIFT 31
#define KVX_SFR_ILE_IT31_WIDTH 1
#define KVX_SFR_ILE_IT31_WFXL_MASK _ULL(0x80000000)
#define KVX_SFR_ILE_IT31_WFXL_CLEAR _ULL(0x80000000)
#define KVX_SFR_ILE_IT31_WFXL_SET _ULL(0x8000000000000000)

#define KVX_SFR_ILL_IT0_MASK _ULL(0x3) /* Interrupt 0 owner */
#define KVX_SFR_ILL_IT0_SHIFT 0
#define KVX_SFR_ILL_IT0_WIDTH 2
#define KVX_SFR_ILL_IT0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_ILL_IT0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_ILL_IT0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_ILL_IT1_MASK _ULL(0xc) /* Interrupt 1 owner */
#define KVX_SFR_ILL_IT1_SHIFT 2
#define KVX_SFR_ILL_IT1_WIDTH 2
#define KVX_SFR_ILL_IT1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_ILL_IT1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_ILL_IT1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_ILL_IT2_MASK _ULL(0x30) /* Interrupt 2 owner */
#define KVX_SFR_ILL_IT2_SHIFT 4
#define KVX_SFR_ILL_IT2_WIDTH 2
#define KVX_SFR_ILL_IT2_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ILL_IT2_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ILL_IT2_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ILL_IT3_MASK _ULL(0xc0) /* Interrupt 3 owner */
#define KVX_SFR_ILL_IT3_SHIFT 6
#define KVX_SFR_ILL_IT3_WIDTH 2
#define KVX_SFR_ILL_IT3_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ILL_IT3_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ILL_IT3_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ILL_IT4_MASK _ULL(0x300) /* Interrupt 4 owner */
#define KVX_SFR_ILL_IT4_SHIFT 8
#define KVX_SFR_ILL_IT4_WIDTH 2
#define KVX_SFR_ILL_IT4_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ILL_IT4_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ILL_IT4_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ILL_IT5_MASK _ULL(0xc00) /* Interrupt 5 owner */
#define KVX_SFR_ILL_IT5_SHIFT 10
#define KVX_SFR_ILL_IT5_WIDTH 2
#define KVX_SFR_ILL_IT5_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ILL_IT5_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ILL_IT5_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ILL_IT6_MASK _ULL(0x3000) /* Interrupt 6 owner */
#define KVX_SFR_ILL_IT6_SHIFT 12
#define KVX_SFR_ILL_IT6_WIDTH 2
#define KVX_SFR_ILL_IT6_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ILL_IT6_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ILL_IT6_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ILL_IT7_MASK _ULL(0xc000) /* Interrupt 7 owner */
#define KVX_SFR_ILL_IT7_SHIFT 14
#define KVX_SFR_ILL_IT7_WIDTH 2
#define KVX_SFR_ILL_IT7_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_ILL_IT7_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_ILL_IT7_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_ILL_IT8_MASK _ULL(0x30000) /* Interrupt 8 owner */
#define KVX_SFR_ILL_IT8_SHIFT 16
#define KVX_SFR_ILL_IT8_WIDTH 2
#define KVX_SFR_ILL_IT8_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_ILL_IT8_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_ILL_IT8_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_ILL_IT9_MASK _ULL(0xc0000) /* Interrupt 9 owner */
#define KVX_SFR_ILL_IT9_SHIFT 18
#define KVX_SFR_ILL_IT9_WIDTH 2
#define KVX_SFR_ILL_IT9_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_ILL_IT9_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_ILL_IT9_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_ILL_IT10_MASK _ULL(0x300000) /* Interrupt 10 owner */
#define KVX_SFR_ILL_IT10_SHIFT 20
#define KVX_SFR_ILL_IT10_WIDTH 2
#define KVX_SFR_ILL_IT10_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_ILL_IT10_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_ILL_IT10_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_ILL_IT11_MASK _ULL(0xc00000) /* Interrupt 11 owner */
#define KVX_SFR_ILL_IT11_SHIFT 22
#define KVX_SFR_ILL_IT11_WIDTH 2
#define KVX_SFR_ILL_IT11_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_ILL_IT11_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_ILL_IT11_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_ILL_IT12_MASK _ULL(0x3000000) /* Interrupt 12 owner */
#define KVX_SFR_ILL_IT12_SHIFT 24
#define KVX_SFR_ILL_IT12_WIDTH 2
#define KVX_SFR_ILL_IT12_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_ILL_IT12_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_ILL_IT12_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_ILL_IT13_MASK _ULL(0xc000000) /* Interrupt 13 owner */
#define KVX_SFR_ILL_IT13_SHIFT 26
#define KVX_SFR_ILL_IT13_WIDTH 2
#define KVX_SFR_ILL_IT13_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_ILL_IT13_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_ILL_IT13_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_ILL_IT14_MASK _ULL(0x30000000) /* Interrupt 14 owner */
#define KVX_SFR_ILL_IT14_SHIFT 28
#define KVX_SFR_ILL_IT14_WIDTH 2
#define KVX_SFR_ILL_IT14_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_ILL_IT14_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_ILL_IT14_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_ILL_IT15_MASK _ULL(0xc0000000) /* Interrupt 15 owner */
#define KVX_SFR_ILL_IT15_SHIFT 30
#define KVX_SFR_ILL_IT15_WIDTH 2
#define KVX_SFR_ILL_IT15_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_ILL_IT15_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ILL_IT15_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_ILL_IT16_MASK _ULL(0x300000000) /* Interrupt 16 owner */
#define KVX_SFR_ILL_IT16_SHIFT 32
#define KVX_SFR_ILL_IT16_WIDTH 2
#define KVX_SFR_ILL_IT16_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_ILL_IT16_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_ILL_IT16_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_ILL_IT17_MASK _ULL(0xc00000000) /* Interrupt 17 owner */
#define KVX_SFR_ILL_IT17_SHIFT 34
#define KVX_SFR_ILL_IT17_WIDTH 2
#define KVX_SFR_ILL_IT17_WFXM_MASK _ULL(0xc00000000)
#define KVX_SFR_ILL_IT17_WFXM_CLEAR _ULL(0xc)
#define KVX_SFR_ILL_IT17_WFXM_SET _ULL(0xc00000000)

#define KVX_SFR_ILL_IT18_MASK _ULL(0x3000000000) /* Interrupt 18 owner */
#define KVX_SFR_ILL_IT18_SHIFT 36
#define KVX_SFR_ILL_IT18_WIDTH 2
#define KVX_SFR_ILL_IT18_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_ILL_IT18_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_ILL_IT18_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_ILL_IT19_MASK _ULL(0xc000000000) /* Interrupt 19 owner */
#define KVX_SFR_ILL_IT19_SHIFT 38
#define KVX_SFR_ILL_IT19_WIDTH 2
#define KVX_SFR_ILL_IT19_WFXM_MASK _ULL(0xc000000000)
#define KVX_SFR_ILL_IT19_WFXM_CLEAR _ULL(0xc0)
#define KVX_SFR_ILL_IT19_WFXM_SET _ULL(0xc000000000)

#define KVX_SFR_ILL_IT20_MASK _ULL(0x30000000000) /* Interrupt 20 owner */
#define KVX_SFR_ILL_IT20_SHIFT 40
#define KVX_SFR_ILL_IT20_WIDTH 2
#define KVX_SFR_ILL_IT20_WFXM_MASK _ULL(0x30000000000)
#define KVX_SFR_ILL_IT20_WFXM_CLEAR _ULL(0x300)
#define KVX_SFR_ILL_IT20_WFXM_SET _ULL(0x30000000000)

#define KVX_SFR_ILL_IT21_MASK _ULL(0xc0000000000) /* Interrupt 21 owner */
#define KVX_SFR_ILL_IT21_SHIFT 42
#define KVX_SFR_ILL_IT21_WIDTH 2
#define KVX_SFR_ILL_IT21_WFXM_MASK _ULL(0xc0000000000)
#define KVX_SFR_ILL_IT21_WFXM_CLEAR _ULL(0xc00)
#define KVX_SFR_ILL_IT21_WFXM_SET _ULL(0xc0000000000)

#define KVX_SFR_ILL_IT22_MASK _ULL(0x300000000000) /* Interrupt 22 owner */
#define KVX_SFR_ILL_IT22_SHIFT 44
#define KVX_SFR_ILL_IT22_WIDTH 2
#define KVX_SFR_ILL_IT22_WFXM_MASK _ULL(0x300000000000)
#define KVX_SFR_ILL_IT22_WFXM_CLEAR _ULL(0x3000)
#define KVX_SFR_ILL_IT22_WFXM_SET _ULL(0x300000000000)

#define KVX_SFR_ILL_IT23_MASK _ULL(0xc00000000000) /* Interrupt 23 owner */
#define KVX_SFR_ILL_IT23_SHIFT 46
#define KVX_SFR_ILL_IT23_WIDTH 2
#define KVX_SFR_ILL_IT23_WFXM_MASK _ULL(0xc00000000000)
#define KVX_SFR_ILL_IT23_WFXM_CLEAR _ULL(0xc000)
#define KVX_SFR_ILL_IT23_WFXM_SET _ULL(0xc00000000000)

#define KVX_SFR_ILL_IT24_MASK _ULL(0x3000000000000) /* Interrupt 24 owner */
#define KVX_SFR_ILL_IT24_SHIFT 48
#define KVX_SFR_ILL_IT24_WIDTH 2
#define KVX_SFR_ILL_IT24_WFXM_MASK _ULL(0x3000000000000)
#define KVX_SFR_ILL_IT24_WFXM_CLEAR _ULL(0x30000)
#define KVX_SFR_ILL_IT24_WFXM_SET _ULL(0x3000000000000)

#define KVX_SFR_ILL_IT25_MASK _ULL(0xc000000000000) /* Interrupt 25 owner */
#define KVX_SFR_ILL_IT25_SHIFT 50
#define KVX_SFR_ILL_IT25_WIDTH 2
#define KVX_SFR_ILL_IT25_WFXM_MASK _ULL(0xc000000000000)
#define KVX_SFR_ILL_IT25_WFXM_CLEAR _ULL(0xc0000)
#define KVX_SFR_ILL_IT25_WFXM_SET _ULL(0xc000000000000)

#define KVX_SFR_ILL_IT26_MASK _ULL(0x30000000000000) /* Interrupt 26 owner */
#define KVX_SFR_ILL_IT26_SHIFT 52
#define KVX_SFR_ILL_IT26_WIDTH 2
#define KVX_SFR_ILL_IT26_WFXM_MASK _ULL(0x30000000000000)
#define KVX_SFR_ILL_IT26_WFXM_CLEAR _ULL(0x300000)
#define KVX_SFR_ILL_IT26_WFXM_SET _ULL(0x30000000000000)

#define KVX_SFR_ILL_IT27_MASK _ULL(0xc0000000000000) /* Interrupt 27 owner */
#define KVX_SFR_ILL_IT27_SHIFT 54
#define KVX_SFR_ILL_IT27_WIDTH 2
#define KVX_SFR_ILL_IT27_WFXM_MASK _ULL(0xc0000000000000)
#define KVX_SFR_ILL_IT27_WFXM_CLEAR _ULL(0xc00000)
#define KVX_SFR_ILL_IT27_WFXM_SET _ULL(0xc0000000000000)

#define KVX_SFR_ILL_IT28_MASK _ULL(0x300000000000000) /* Interrupt 28 owner */
#define KVX_SFR_ILL_IT28_SHIFT 56
#define KVX_SFR_ILL_IT28_WIDTH 2
#define KVX_SFR_ILL_IT28_WFXM_MASK _ULL(0x300000000000000)
#define KVX_SFR_ILL_IT28_WFXM_CLEAR _ULL(0x3000000)
#define KVX_SFR_ILL_IT28_WFXM_SET _ULL(0x300000000000000)

#define KVX_SFR_ILL_IT29_MASK _ULL(0xc00000000000000) /* Interrupt 29 owner */
#define KVX_SFR_ILL_IT29_SHIFT 58
#define KVX_SFR_ILL_IT29_WIDTH 2
#define KVX_SFR_ILL_IT29_WFXM_MASK _ULL(0xc00000000000000)
#define KVX_SFR_ILL_IT29_WFXM_CLEAR _ULL(0xc000000)
#define KVX_SFR_ILL_IT29_WFXM_SET _ULL(0xc00000000000000)

#define KVX_SFR_ILL_IT30_MASK _ULL(0x3000000000000000) /* Interrupt 30 owner */
#define KVX_SFR_ILL_IT30_SHIFT 60
#define KVX_SFR_ILL_IT30_WIDTH 2
#define KVX_SFR_ILL_IT30_WFXM_MASK _ULL(0x3000000000000000)
#define KVX_SFR_ILL_IT30_WFXM_CLEAR _ULL(0x30000000)
#define KVX_SFR_ILL_IT30_WFXM_SET _ULL(0x3000000000000000)

#define KVX_SFR_ILL_IT31_MASK _ULL(0xc000000000000000) /* Interrupt 31 owner */
#define KVX_SFR_ILL_IT31_SHIFT 62
#define KVX_SFR_ILL_IT31_WIDTH 2
#define KVX_SFR_ILL_IT31_WFXM_MASK _ULL(0xc000000000000000)
#define KVX_SFR_ILL_IT31_WFXM_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ILL_IT31_WFXM_SET _ULL(0xc000000000000000)

#define KVX_SFR_ILR_IT0_MASK _ULL(0x1) /* Interrupt 0 owner */
#define KVX_SFR_ILR_IT0_SHIFT 0
#define KVX_SFR_ILR_IT0_WIDTH 1
#define KVX_SFR_ILR_IT0_WFXL_MASK _ULL(0x1)
#define KVX_SFR_ILR_IT0_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_ILR_IT0_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_ILR_IT1_MASK _ULL(0x2) /* Interrupt 1 owner */
#define KVX_SFR_ILR_IT1_SHIFT 1
#define KVX_SFR_ILR_IT1_WIDTH 1
#define KVX_SFR_ILR_IT1_WFXL_MASK _ULL(0x2)
#define KVX_SFR_ILR_IT1_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_ILR_IT1_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_ILR_IT2_MASK _ULL(0x4) /* Interrupt 2 owner */
#define KVX_SFR_ILR_IT2_SHIFT 2
#define KVX_SFR_ILR_IT2_WIDTH 1
#define KVX_SFR_ILR_IT2_WFXL_MASK _ULL(0x4)
#define KVX_SFR_ILR_IT2_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_ILR_IT2_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_ILR_IT3_MASK _ULL(0x8) /* Interrupt 3 owner */
#define KVX_SFR_ILR_IT3_SHIFT 3
#define KVX_SFR_ILR_IT3_WIDTH 1
#define KVX_SFR_ILR_IT3_WFXL_MASK _ULL(0x8)
#define KVX_SFR_ILR_IT3_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_ILR_IT3_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_ILR_IT4_MASK _ULL(0x10) /* Interrupt 4 owner */
#define KVX_SFR_ILR_IT4_SHIFT 4
#define KVX_SFR_ILR_IT4_WIDTH 1
#define KVX_SFR_ILR_IT4_WFXL_MASK _ULL(0x10)
#define KVX_SFR_ILR_IT4_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_ILR_IT4_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_ILR_IT5_MASK _ULL(0x20) /* Interrupt 5 owner */
#define KVX_SFR_ILR_IT5_SHIFT 5
#define KVX_SFR_ILR_IT5_WIDTH 1
#define KVX_SFR_ILR_IT5_WFXL_MASK _ULL(0x20)
#define KVX_SFR_ILR_IT5_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_ILR_IT5_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_ILR_IT6_MASK _ULL(0x40) /* Interrupt 6 owner */
#define KVX_SFR_ILR_IT6_SHIFT 6
#define KVX_SFR_ILR_IT6_WIDTH 1
#define KVX_SFR_ILR_IT6_WFXL_MASK _ULL(0x40)
#define KVX_SFR_ILR_IT6_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_ILR_IT6_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_ILR_IT7_MASK _ULL(0x80) /* Interrupt 7 owner */
#define KVX_SFR_ILR_IT7_SHIFT 7
#define KVX_SFR_ILR_IT7_WIDTH 1
#define KVX_SFR_ILR_IT7_WFXL_MASK _ULL(0x80)
#define KVX_SFR_ILR_IT7_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_ILR_IT7_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_ILR_IT8_MASK _ULL(0x100) /* Interrupt 8 owner */
#define KVX_SFR_ILR_IT8_SHIFT 8
#define KVX_SFR_ILR_IT8_WIDTH 1
#define KVX_SFR_ILR_IT8_WFXL_MASK _ULL(0x100)
#define KVX_SFR_ILR_IT8_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_ILR_IT8_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_ILR_IT9_MASK _ULL(0x200) /* Interrupt 9 owner */
#define KVX_SFR_ILR_IT9_SHIFT 9
#define KVX_SFR_ILR_IT9_WIDTH 1
#define KVX_SFR_ILR_IT9_WFXL_MASK _ULL(0x200)
#define KVX_SFR_ILR_IT9_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_ILR_IT9_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_ILR_IT10_MASK _ULL(0x400) /* Interrupt 10 owner */
#define KVX_SFR_ILR_IT10_SHIFT 10
#define KVX_SFR_ILR_IT10_WIDTH 1
#define KVX_SFR_ILR_IT10_WFXL_MASK _ULL(0x400)
#define KVX_SFR_ILR_IT10_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_ILR_IT10_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_ILR_IT11_MASK _ULL(0x800) /* Interrupt 11 owner */
#define KVX_SFR_ILR_IT11_SHIFT 11
#define KVX_SFR_ILR_IT11_WIDTH 1
#define KVX_SFR_ILR_IT11_WFXL_MASK _ULL(0x800)
#define KVX_SFR_ILR_IT11_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_ILR_IT11_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_ILR_IT12_MASK _ULL(0x1000) /* Interrupt 12 owner */
#define KVX_SFR_ILR_IT12_SHIFT 12
#define KVX_SFR_ILR_IT12_WIDTH 1
#define KVX_SFR_ILR_IT12_WFXL_MASK _ULL(0x1000)
#define KVX_SFR_ILR_IT12_WFXL_CLEAR _ULL(0x1000)
#define KVX_SFR_ILR_IT12_WFXL_SET _ULL(0x100000000000)

#define KVX_SFR_ILR_IT13_MASK _ULL(0x2000) /* Interrupt 13 owner */
#define KVX_SFR_ILR_IT13_SHIFT 13
#define KVX_SFR_ILR_IT13_WIDTH 1
#define KVX_SFR_ILR_IT13_WFXL_MASK _ULL(0x2000)
#define KVX_SFR_ILR_IT13_WFXL_CLEAR _ULL(0x2000)
#define KVX_SFR_ILR_IT13_WFXL_SET _ULL(0x200000000000)

#define KVX_SFR_ILR_IT14_MASK _ULL(0x4000) /* Interrupt 14 owner */
#define KVX_SFR_ILR_IT14_SHIFT 14
#define KVX_SFR_ILR_IT14_WIDTH 1
#define KVX_SFR_ILR_IT14_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ILR_IT14_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ILR_IT14_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ILR_IT15_MASK _ULL(0x8000) /* Interrupt 15 owner */
#define KVX_SFR_ILR_IT15_SHIFT 15
#define KVX_SFR_ILR_IT15_WIDTH 1
#define KVX_SFR_ILR_IT15_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ILR_IT15_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ILR_IT15_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ILR_IT16_MASK _ULL(0x10000) /* Interrupt 16 owner */
#define KVX_SFR_ILR_IT16_SHIFT 16
#define KVX_SFR_ILR_IT16_WIDTH 1
#define KVX_SFR_ILR_IT16_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_ILR_IT16_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_ILR_IT16_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_ILR_IT17_MASK _ULL(0x20000) /* Interrupt 17 owner */
#define KVX_SFR_ILR_IT17_SHIFT 17
#define KVX_SFR_ILR_IT17_WIDTH 1
#define KVX_SFR_ILR_IT17_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ILR_IT17_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ILR_IT17_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ILR_IT18_MASK _ULL(0x40000) /* Interrupt 18 owner */
#define KVX_SFR_ILR_IT18_SHIFT 18
#define KVX_SFR_ILR_IT18_WIDTH 1
#define KVX_SFR_ILR_IT18_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_ILR_IT18_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_ILR_IT18_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_ILR_IT19_MASK _ULL(0x80000) /* Interrupt 19 owner */
#define KVX_SFR_ILR_IT19_SHIFT 19
#define KVX_SFR_ILR_IT19_WIDTH 1
#define KVX_SFR_ILR_IT19_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_ILR_IT19_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_ILR_IT19_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_ILR_IT20_MASK _ULL(0x100000) /* Interrupt 20 owner */
#define KVX_SFR_ILR_IT20_SHIFT 20
#define KVX_SFR_ILR_IT20_WIDTH 1
#define KVX_SFR_ILR_IT20_WFXL_MASK _ULL(0x100000)
#define KVX_SFR_ILR_IT20_WFXL_CLEAR _ULL(0x100000)
#define KVX_SFR_ILR_IT20_WFXL_SET _ULL(0x10000000000000)

#define KVX_SFR_ILR_IT21_MASK _ULL(0x200000) /* Interrupt 21 owner */
#define KVX_SFR_ILR_IT21_SHIFT 21
#define KVX_SFR_ILR_IT21_WIDTH 1
#define KVX_SFR_ILR_IT21_WFXL_MASK _ULL(0x200000)
#define KVX_SFR_ILR_IT21_WFXL_CLEAR _ULL(0x200000)
#define KVX_SFR_ILR_IT21_WFXL_SET _ULL(0x20000000000000)

#define KVX_SFR_ILR_IT22_MASK _ULL(0x400000) /* Interrupt 22 owner */
#define KVX_SFR_ILR_IT22_SHIFT 22
#define KVX_SFR_ILR_IT22_WIDTH 1
#define KVX_SFR_ILR_IT22_WFXL_MASK _ULL(0x400000)
#define KVX_SFR_ILR_IT22_WFXL_CLEAR _ULL(0x400000)
#define KVX_SFR_ILR_IT22_WFXL_SET _ULL(0x40000000000000)

#define KVX_SFR_ILR_IT23_MASK _ULL(0x800000) /* Interrupt 23 owner */
#define KVX_SFR_ILR_IT23_SHIFT 23
#define KVX_SFR_ILR_IT23_WIDTH 1
#define KVX_SFR_ILR_IT23_WFXL_MASK _ULL(0x800000)
#define KVX_SFR_ILR_IT23_WFXL_CLEAR _ULL(0x800000)
#define KVX_SFR_ILR_IT23_WFXL_SET _ULL(0x80000000000000)

#define KVX_SFR_ILR_IT24_MASK _ULL(0x1000000) /* Interrupt 24 owner */
#define KVX_SFR_ILR_IT24_SHIFT 24
#define KVX_SFR_ILR_IT24_WIDTH 1
#define KVX_SFR_ILR_IT24_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_ILR_IT24_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_ILR_IT24_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_ILR_IT25_MASK _ULL(0x2000000) /* Interrupt 25 owner */
#define KVX_SFR_ILR_IT25_SHIFT 25
#define KVX_SFR_ILR_IT25_WIDTH 1
#define KVX_SFR_ILR_IT25_WFXL_MASK _ULL(0x2000000)
#define KVX_SFR_ILR_IT25_WFXL_CLEAR _ULL(0x2000000)
#define KVX_SFR_ILR_IT25_WFXL_SET _ULL(0x200000000000000)

#define KVX_SFR_ILR_IT26_MASK _ULL(0x4000000) /* Interrupt 26 owner */
#define KVX_SFR_ILR_IT26_SHIFT 26
#define KVX_SFR_ILR_IT26_WIDTH 1
#define KVX_SFR_ILR_IT26_WFXL_MASK _ULL(0x4000000)
#define KVX_SFR_ILR_IT26_WFXL_CLEAR _ULL(0x4000000)
#define KVX_SFR_ILR_IT26_WFXL_SET _ULL(0x400000000000000)

#define KVX_SFR_ILR_IT27_MASK _ULL(0x8000000) /* Interrupt 27 owner */
#define KVX_SFR_ILR_IT27_SHIFT 27
#define KVX_SFR_ILR_IT27_WIDTH 1
#define KVX_SFR_ILR_IT27_WFXL_MASK _ULL(0x8000000)
#define KVX_SFR_ILR_IT27_WFXL_CLEAR _ULL(0x8000000)
#define KVX_SFR_ILR_IT27_WFXL_SET _ULL(0x800000000000000)

#define KVX_SFR_ILR_IT28_MASK _ULL(0x10000000) /* Interrupt 28 owner */
#define KVX_SFR_ILR_IT28_SHIFT 28
#define KVX_SFR_ILR_IT28_WIDTH 1
#define KVX_SFR_ILR_IT28_WFXL_MASK _ULL(0x10000000)
#define KVX_SFR_ILR_IT28_WFXL_CLEAR _ULL(0x10000000)
#define KVX_SFR_ILR_IT28_WFXL_SET _ULL(0x1000000000000000)

#define KVX_SFR_ILR_IT29_MASK _ULL(0x20000000) /* Interrupt 29 owner */
#define KVX_SFR_ILR_IT29_SHIFT 29
#define KVX_SFR_ILR_IT29_WIDTH 1
#define KVX_SFR_ILR_IT29_WFXL_MASK _ULL(0x20000000)
#define KVX_SFR_ILR_IT29_WFXL_CLEAR _ULL(0x20000000)
#define KVX_SFR_ILR_IT29_WFXL_SET _ULL(0x2000000000000000)

#define KVX_SFR_ILR_IT30_MASK _ULL(0x40000000) /* Interrupt 30 owner */
#define KVX_SFR_ILR_IT30_SHIFT 30
#define KVX_SFR_ILR_IT30_WIDTH 1
#define KVX_SFR_ILR_IT30_WFXL_MASK _ULL(0x40000000)
#define KVX_SFR_ILR_IT30_WFXL_CLEAR _ULL(0x40000000)
#define KVX_SFR_ILR_IT30_WFXL_SET _ULL(0x4000000000000000)

#define KVX_SFR_ILR_IT31_MASK _ULL(0x80000000) /* Interrupt 31 owner */
#define KVX_SFR_ILR_IT31_SHIFT 31
#define KVX_SFR_ILR_IT31_WIDTH 1
#define KVX_SFR_ILR_IT31_WFXL_MASK _ULL(0x80000000)
#define KVX_SFR_ILR_IT31_WFXL_CLEAR _ULL(0x80000000)
#define KVX_SFR_ILR_IT31_WFXL_SET _ULL(0x8000000000000000)

#define KVX_SFR_ITOW_IT0_MASK _ULL(0x3) /* Interrupt 0 owner */
#define KVX_SFR_ITOW_IT0_SHIFT 0
#define KVX_SFR_ITOW_IT0_WIDTH 2
#define KVX_SFR_ITOW_IT0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_ITOW_IT0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_ITOW_IT0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_ITOW_IT1_MASK _ULL(0xc) /* Interrupt 1 owner */
#define KVX_SFR_ITOW_IT1_SHIFT 2
#define KVX_SFR_ITOW_IT1_WIDTH 2
#define KVX_SFR_ITOW_IT1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_ITOW_IT1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_ITOW_IT1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_ITOW_IT2_MASK _ULL(0x30) /* Interrupt 2 owner */
#define KVX_SFR_ITOW_IT2_SHIFT 4
#define KVX_SFR_ITOW_IT2_WIDTH 2
#define KVX_SFR_ITOW_IT2_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ITOW_IT2_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ITOW_IT2_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ITOW_IT3_MASK _ULL(0xc0) /* Interrupt 3 owner */
#define KVX_SFR_ITOW_IT3_SHIFT 6
#define KVX_SFR_ITOW_IT3_WIDTH 2
#define KVX_SFR_ITOW_IT3_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ITOW_IT3_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ITOW_IT3_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ITOW_IT4_MASK _ULL(0x300) /* Interrupt 4 owner */
#define KVX_SFR_ITOW_IT4_SHIFT 8
#define KVX_SFR_ITOW_IT4_WIDTH 2
#define KVX_SFR_ITOW_IT4_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ITOW_IT4_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ITOW_IT4_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ITOW_IT5_MASK _ULL(0xc00) /* Interrupt 5 owner */
#define KVX_SFR_ITOW_IT5_SHIFT 10
#define KVX_SFR_ITOW_IT5_WIDTH 2
#define KVX_SFR_ITOW_IT5_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ITOW_IT5_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ITOW_IT5_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ITOW_IT6_MASK _ULL(0x3000) /* Interrupt 6 owner */
#define KVX_SFR_ITOW_IT6_SHIFT 12
#define KVX_SFR_ITOW_IT6_WIDTH 2
#define KVX_SFR_ITOW_IT6_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ITOW_IT6_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ITOW_IT6_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ITOW_IT7_MASK _ULL(0xc000) /* Interrupt 7 owner */
#define KVX_SFR_ITOW_IT7_SHIFT 14
#define KVX_SFR_ITOW_IT7_WIDTH 2
#define KVX_SFR_ITOW_IT7_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_ITOW_IT7_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_ITOW_IT7_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_ITOW_IT8_MASK _ULL(0x30000) /* Interrupt 8 owner */
#define KVX_SFR_ITOW_IT8_SHIFT 16
#define KVX_SFR_ITOW_IT8_WIDTH 2
#define KVX_SFR_ITOW_IT8_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_ITOW_IT8_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_ITOW_IT8_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_ITOW_IT9_MASK _ULL(0xc0000) /* Interrupt 9 owner */
#define KVX_SFR_ITOW_IT9_SHIFT 18
#define KVX_SFR_ITOW_IT9_WIDTH 2
#define KVX_SFR_ITOW_IT9_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_ITOW_IT9_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_ITOW_IT9_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_ITOW_IT10_MASK _ULL(0x300000) /* Interrupt 10 owner */
#define KVX_SFR_ITOW_IT10_SHIFT 20
#define KVX_SFR_ITOW_IT10_WIDTH 2
#define KVX_SFR_ITOW_IT10_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_ITOW_IT10_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_ITOW_IT10_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_ITOW_IT11_MASK _ULL(0xc00000) /* Interrupt 11 owner */
#define KVX_SFR_ITOW_IT11_SHIFT 22
#define KVX_SFR_ITOW_IT11_WIDTH 2
#define KVX_SFR_ITOW_IT11_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_ITOW_IT11_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_ITOW_IT11_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_ITOW_IT12_MASK _ULL(0x3000000) /* Interrupt 12 owner */
#define KVX_SFR_ITOW_IT12_SHIFT 24
#define KVX_SFR_ITOW_IT12_WIDTH 2
#define KVX_SFR_ITOW_IT12_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_ITOW_IT12_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_ITOW_IT12_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_ITOW_IT13_MASK _ULL(0xc000000) /* Interrupt 13 owner */
#define KVX_SFR_ITOW_IT13_SHIFT 26
#define KVX_SFR_ITOW_IT13_WIDTH 2
#define KVX_SFR_ITOW_IT13_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_ITOW_IT13_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_ITOW_IT13_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_ITOW_IT14_MASK _ULL(0x30000000) /* Interrupt 14 owner */
#define KVX_SFR_ITOW_IT14_SHIFT 28
#define KVX_SFR_ITOW_IT14_WIDTH 2
#define KVX_SFR_ITOW_IT14_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_ITOW_IT14_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_ITOW_IT14_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_ITOW_IT15_MASK _ULL(0xc0000000) /* Interrupt 15 owner */
#define KVX_SFR_ITOW_IT15_SHIFT 30
#define KVX_SFR_ITOW_IT15_WIDTH 2
#define KVX_SFR_ITOW_IT15_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_ITOW_IT15_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ITOW_IT15_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_ITOW_IT16_MASK _ULL(0x300000000) /* Interrupt 16 owner */
#define KVX_SFR_ITOW_IT16_SHIFT 32
#define KVX_SFR_ITOW_IT16_WIDTH 2
#define KVX_SFR_ITOW_IT16_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_ITOW_IT16_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_ITOW_IT16_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_ITOW_IT17_MASK _ULL(0xc00000000) /* Interrupt 17 owner */
#define KVX_SFR_ITOW_IT17_SHIFT 34
#define KVX_SFR_ITOW_IT17_WIDTH 2
#define KVX_SFR_ITOW_IT17_WFXM_MASK _ULL(0xc00000000)
#define KVX_SFR_ITOW_IT17_WFXM_CLEAR _ULL(0xc)
#define KVX_SFR_ITOW_IT17_WFXM_SET _ULL(0xc00000000)

#define KVX_SFR_ITOW_IT18_MASK _ULL(0x3000000000) /* Interrupt 18 owner */
#define KVX_SFR_ITOW_IT18_SHIFT 36
#define KVX_SFR_ITOW_IT18_WIDTH 2
#define KVX_SFR_ITOW_IT18_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_ITOW_IT18_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_ITOW_IT18_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_ITOW_IT19_MASK _ULL(0xc000000000) /* Interrupt 19 owner */
#define KVX_SFR_ITOW_IT19_SHIFT 38
#define KVX_SFR_ITOW_IT19_WIDTH 2
#define KVX_SFR_ITOW_IT19_WFXM_MASK _ULL(0xc000000000)
#define KVX_SFR_ITOW_IT19_WFXM_CLEAR _ULL(0xc0)
#define KVX_SFR_ITOW_IT19_WFXM_SET _ULL(0xc000000000)

#define KVX_SFR_ITOW_IT20_MASK _ULL(0x30000000000) /* Interrupt 20 owner */
#define KVX_SFR_ITOW_IT20_SHIFT 40
#define KVX_SFR_ITOW_IT20_WIDTH 2
#define KVX_SFR_ITOW_IT20_WFXM_MASK _ULL(0x30000000000)
#define KVX_SFR_ITOW_IT20_WFXM_CLEAR _ULL(0x300)
#define KVX_SFR_ITOW_IT20_WFXM_SET _ULL(0x30000000000)

#define KVX_SFR_ITOW_IT21_MASK _ULL(0xc0000000000) /* Interrupt 21 owner */
#define KVX_SFR_ITOW_IT21_SHIFT 42
#define KVX_SFR_ITOW_IT21_WIDTH 2
#define KVX_SFR_ITOW_IT21_WFXM_MASK _ULL(0xc0000000000)
#define KVX_SFR_ITOW_IT21_WFXM_CLEAR _ULL(0xc00)
#define KVX_SFR_ITOW_IT21_WFXM_SET _ULL(0xc0000000000)

#define KVX_SFR_ITOW_IT22_MASK _ULL(0x300000000000) /* Interrupt 22 owner */
#define KVX_SFR_ITOW_IT22_SHIFT 44
#define KVX_SFR_ITOW_IT22_WIDTH 2
#define KVX_SFR_ITOW_IT22_WFXM_MASK _ULL(0x300000000000)
#define KVX_SFR_ITOW_IT22_WFXM_CLEAR _ULL(0x3000)
#define KVX_SFR_ITOW_IT22_WFXM_SET _ULL(0x300000000000)

#define KVX_SFR_ITOW_IT23_MASK _ULL(0xc00000000000) /* Interrupt 23 owner */
#define KVX_SFR_ITOW_IT23_SHIFT 46
#define KVX_SFR_ITOW_IT23_WIDTH 2
#define KVX_SFR_ITOW_IT23_WFXM_MASK _ULL(0xc00000000000)
#define KVX_SFR_ITOW_IT23_WFXM_CLEAR _ULL(0xc000)
#define KVX_SFR_ITOW_IT23_WFXM_SET _ULL(0xc00000000000)

#define KVX_SFR_ITOW_IT24_MASK _ULL(0x3000000000000) /* Interrupt 24 owner */
#define KVX_SFR_ITOW_IT24_SHIFT 48
#define KVX_SFR_ITOW_IT24_WIDTH 2
#define KVX_SFR_ITOW_IT24_WFXM_MASK _ULL(0x3000000000000)
#define KVX_SFR_ITOW_IT24_WFXM_CLEAR _ULL(0x30000)
#define KVX_SFR_ITOW_IT24_WFXM_SET _ULL(0x3000000000000)

#define KVX_SFR_ITOW_IT25_MASK _ULL(0xc000000000000) /* Interrupt 25 owner */
#define KVX_SFR_ITOW_IT25_SHIFT 50
#define KVX_SFR_ITOW_IT25_WIDTH 2
#define KVX_SFR_ITOW_IT25_WFXM_MASK _ULL(0xc000000000000)
#define KVX_SFR_ITOW_IT25_WFXM_CLEAR _ULL(0xc0000)
#define KVX_SFR_ITOW_IT25_WFXM_SET _ULL(0xc000000000000)

#define KVX_SFR_ITOW_IT26_MASK _ULL(0x30000000000000) /* Interrupt 26 owner */
#define KVX_SFR_ITOW_IT26_SHIFT 52
#define KVX_SFR_ITOW_IT26_WIDTH 2
#define KVX_SFR_ITOW_IT26_WFXM_MASK _ULL(0x30000000000000)
#define KVX_SFR_ITOW_IT26_WFXM_CLEAR _ULL(0x300000)
#define KVX_SFR_ITOW_IT26_WFXM_SET _ULL(0x30000000000000)

#define KVX_SFR_ITOW_IT27_MASK _ULL(0xc0000000000000) /* Interrupt 27 owner */
#define KVX_SFR_ITOW_IT27_SHIFT 54
#define KVX_SFR_ITOW_IT27_WIDTH 2
#define KVX_SFR_ITOW_IT27_WFXM_MASK _ULL(0xc0000000000000)
#define KVX_SFR_ITOW_IT27_WFXM_CLEAR _ULL(0xc00000)
#define KVX_SFR_ITOW_IT27_WFXM_SET _ULL(0xc0000000000000)

#define KVX_SFR_ITOW_IT28_MASK _ULL(0x300000000000000) /* Interrupt 28 owner */
#define KVX_SFR_ITOW_IT28_SHIFT 56
#define KVX_SFR_ITOW_IT28_WIDTH 2
#define KVX_SFR_ITOW_IT28_WFXM_MASK _ULL(0x300000000000000)
#define KVX_SFR_ITOW_IT28_WFXM_CLEAR _ULL(0x3000000)
#define KVX_SFR_ITOW_IT28_WFXM_SET _ULL(0x300000000000000)

#define KVX_SFR_ITOW_IT29_MASK _ULL(0xc00000000000000) /* Interrupt 29 owner */
#define KVX_SFR_ITOW_IT29_SHIFT 58
#define KVX_SFR_ITOW_IT29_WIDTH 2
#define KVX_SFR_ITOW_IT29_WFXM_MASK _ULL(0xc00000000000000)
#define KVX_SFR_ITOW_IT29_WFXM_CLEAR _ULL(0xc000000)
#define KVX_SFR_ITOW_IT29_WFXM_SET _ULL(0xc00000000000000)

#define KVX_SFR_ITOW_IT30_MASK _ULL(0x3000000000000000) /* Interrupt 30 owner */
#define KVX_SFR_ITOW_IT30_SHIFT 60
#define KVX_SFR_ITOW_IT30_WIDTH 2
#define KVX_SFR_ITOW_IT30_WFXM_MASK _ULL(0x3000000000000000)
#define KVX_SFR_ITOW_IT30_WFXM_CLEAR _ULL(0x30000000)
#define KVX_SFR_ITOW_IT30_WFXM_SET _ULL(0x3000000000000000)

#define KVX_SFR_ITOW_IT31_MASK _ULL(0xc000000000000000) /* Interrupt 31 owner */
#define KVX_SFR_ITOW_IT31_SHIFT 62
#define KVX_SFR_ITOW_IT31_WIDTH 2
#define KVX_SFR_ITOW_IT31_WFXM_MASK _ULL(0xc000000000000000)
#define KVX_SFR_ITOW_IT31_WFXM_CLEAR _ULL(0xc0000000)
#define KVX_SFR_ITOW_IT31_WFXM_SET _ULL(0xc000000000000000)

#define KVX_SFR_DO_B0_MASK _ULL(0x3) /* Breakpoint 0 owner. */
#define KVX_SFR_DO_B0_SHIFT 0
#define KVX_SFR_DO_B0_WIDTH 2
#define KVX_SFR_DO_B0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_DO_B0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_DO_B0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_DO_B1_MASK _ULL(0xc) /* Breakpoint 1 owner. */
#define KVX_SFR_DO_B1_SHIFT 2
#define KVX_SFR_DO_B1_WIDTH 2
#define KVX_SFR_DO_B1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_DO_B1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_DO_B1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_DO_W0_MASK _ULL(0x30) /* Watchpoint 0 owner. */
#define KVX_SFR_DO_W0_SHIFT 4
#define KVX_SFR_DO_W0_WIDTH 2
#define KVX_SFR_DO_W0_WFXL_MASK _ULL(0x30)
#define KVX_SFR_DO_W0_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_DO_W0_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_DO_W1_MASK _ULL(0xc0) /* Watchpoint 1 owner. */
#define KVX_SFR_DO_W1_SHIFT 6
#define KVX_SFR_DO_W1_WIDTH 2
#define KVX_SFR_DO_W1_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_DO_W1_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_DO_W1_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_DBA0_DBA0_MASK _ULL(0xffffffffffffffff) /* Debug Breakpoint Address 0 */
#define KVX_SFR_DBA0_DBA0_SHIFT 0
#define KVX_SFR_DBA0_DBA0_WIDTH 64
#define KVX_SFR_DBA0_DBA0_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_DBA0_DBA0_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DBA0_DBA0_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_DBA0_DBA0_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_DBA0_DBA0_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DBA0_DBA0_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_DBA1_DBA1_MASK _ULL(0xffffffffffffffff) /* Debug Breakpoint Address 1 */
#define KVX_SFR_DBA1_DBA1_SHIFT 0
#define KVX_SFR_DBA1_DBA1_WIDTH 64
#define KVX_SFR_DBA1_DBA1_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_DBA1_DBA1_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DBA1_DBA1_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_DBA1_DBA1_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_DBA1_DBA1_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DBA1_DBA1_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_DWA0_DWA0_MASK _ULL(0xffffffffffffffff) /* Debug Breakpoint Address 0 */
#define KVX_SFR_DWA0_DWA0_SHIFT 0
#define KVX_SFR_DWA0_DWA0_WIDTH 64
#define KVX_SFR_DWA0_DWA0_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_DWA0_DWA0_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DWA0_DWA0_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_DWA0_DWA0_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_DWA0_DWA0_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DWA0_DWA0_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_DWA1_DWA1_MASK _ULL(0xffffffffffffffff) /* Debug Breakpoint Address 1 */
#define KVX_SFR_DWA1_DWA1_SHIFT 0
#define KVX_SFR_DWA1_DWA1_WIDTH 64
#define KVX_SFR_DWA1_DWA1_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_DWA1_DWA1_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DWA1_DWA1_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_DWA1_DWA1_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_DWA1_DWA1_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_DWA1_DWA1_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_DOW_B0_MASK _ULL(0x3) /* Breakpoint 0 owner. */
#define KVX_SFR_DOW_B0_SHIFT 0
#define KVX_SFR_DOW_B0_WIDTH 2
#define KVX_SFR_DOW_B0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_DOW_B0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_DOW_B0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_DOW_B1_MASK _ULL(0xc) /* Breakpoint 1 owner. */
#define KVX_SFR_DOW_B1_SHIFT 2
#define KVX_SFR_DOW_B1_WIDTH 2
#define KVX_SFR_DOW_B1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_DOW_B1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_DOW_B1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_DOW_W0_MASK _ULL(0x30) /* Watchpoint 0 owner. */
#define KVX_SFR_DOW_W0_SHIFT 4
#define KVX_SFR_DOW_W0_WIDTH 2
#define KVX_SFR_DOW_W0_WFXL_MASK _ULL(0x30)
#define KVX_SFR_DOW_W0_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_DOW_W0_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_DOW_W1_MASK _ULL(0xc0) /* Watchpoint 1 owner. */
#define KVX_SFR_DOW_W1_SHIFT 6
#define KVX_SFR_DOW_W1_WIDTH 2
#define KVX_SFR_DOW_W1_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_DOW_W1_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_DOW_W1_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_MO_MMI_MASK _ULL(0x3) /* Memory Management Instructions owner. */
#define KVX_SFR_MO_MMI_SHIFT 0
#define KVX_SFR_MO_MMI_WIDTH 2
#define KVX_SFR_MO_MMI_WFXL_MASK _ULL(0x3)
#define KVX_SFR_MO_MMI_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_MO_MMI_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_MO_RFE_MASK _ULL(0xc) /* RFE instruction owner. */
#define KVX_SFR_MO_RFE_SHIFT 2
#define KVX_SFR_MO_RFE_WIDTH 2
#define KVX_SFR_MO_RFE_WFXL_MASK _ULL(0xc)
#define KVX_SFR_MO_RFE_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_MO_RFE_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_MO_STOP_MASK _ULL(0x30) /* STOP instruction owner. */
#define KVX_SFR_MO_STOP_SHIFT 4
#define KVX_SFR_MO_STOP_WIDTH 2
#define KVX_SFR_MO_STOP_WFXL_MASK _ULL(0x30)
#define KVX_SFR_MO_STOP_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_MO_STOP_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_MO_SYNC_MASK _ULL(0xc0) /* SYNCGROUP instruction owner. */
#define KVX_SFR_MO_SYNC_SHIFT 6
#define KVX_SFR_MO_SYNC_WIDTH 2
#define KVX_SFR_MO_SYNC_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_MO_SYNC_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_MO_SYNC_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_MO_PCR_MASK _ULL(0x300) /* PCR register owner. */
#define KVX_SFR_MO_PCR_SHIFT 8
#define KVX_SFR_MO_PCR_WIDTH 2
#define KVX_SFR_MO_PCR_WFXL_MASK _ULL(0x300)
#define KVX_SFR_MO_PCR_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_MO_PCR_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_MO_MSG_MASK _ULL(0xc00) /* MMU SFR GROUP registers owner. */
#define KVX_SFR_MO_MSG_SHIFT 10
#define KVX_SFR_MO_MSG_WIDTH 2
#define KVX_SFR_MO_MSG_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_MO_MSG_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_MO_MSG_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_MO_MEN_MASK _ULL(0x3000) /* Miscellaneous External Notifications register owner. */
#define KVX_SFR_MO_MEN_SHIFT 12
#define KVX_SFR_MO_MEN_WIDTH 2
#define KVX_SFR_MO_MEN_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_MO_MEN_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_MO_MEN_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_MO_MES_MASK _ULL(0xc000) /* Memory Error Status register owner. */
#define KVX_SFR_MO_MES_SHIFT 14
#define KVX_SFR_MO_MES_WIDTH 2
#define KVX_SFR_MO_MES_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_MO_MES_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_MO_MES_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_MO_CSIT_MASK _ULL(0x30000) /* Compute Status Artithmetic Interrupt register owner. */
#define KVX_SFR_MO_CSIT_SHIFT 16
#define KVX_SFR_MO_CSIT_WIDTH 2
#define KVX_SFR_MO_CSIT_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_MO_CSIT_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_MO_CSIT_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_MO_T0_MASK _ULL(0xc0000) /* Timer 0 register group owner */
#define KVX_SFR_MO_T0_SHIFT 18
#define KVX_SFR_MO_T0_WIDTH 2
#define KVX_SFR_MO_T0_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_MO_T0_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_MO_T0_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_MO_T1_MASK _ULL(0x300000) /* Timer 1 register group owner */
#define KVX_SFR_MO_T1_SHIFT 20
#define KVX_SFR_MO_T1_WIDTH 2
#define KVX_SFR_MO_T1_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_MO_T1_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_MO_T1_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_MO_WD_MASK _ULL(0xc00000) /* Watch Dog register group owner. */
#define KVX_SFR_MO_WD_SHIFT 22
#define KVX_SFR_MO_WD_WIDTH 2
#define KVX_SFR_MO_WD_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_MO_WD_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_MO_WD_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_MO_PM0_MASK _ULL(0x3000000) /* Performance Monitor 0 register owner. */
#define KVX_SFR_MO_PM0_SHIFT 24
#define KVX_SFR_MO_PM0_WIDTH 2
#define KVX_SFR_MO_PM0_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_MO_PM0_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_MO_PM0_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_MO_PM1_MASK _ULL(0xc000000) /* Performance Monitor 1 register owner. */
#define KVX_SFR_MO_PM1_SHIFT 26
#define KVX_SFR_MO_PM1_WIDTH 2
#define KVX_SFR_MO_PM1_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_MO_PM1_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_MO_PM1_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_MO_PM2_MASK _ULL(0x30000000) /* Performance Monitor 2 register owner. */
#define KVX_SFR_MO_PM2_SHIFT 28
#define KVX_SFR_MO_PM2_WIDTH 2
#define KVX_SFR_MO_PM2_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_MO_PM2_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_MO_PM2_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_MO_PM3_MASK _ULL(0xc0000000) /* Performance Monitor 3 register owner. */
#define KVX_SFR_MO_PM3_SHIFT 30
#define KVX_SFR_MO_PM3_WIDTH 2
#define KVX_SFR_MO_PM3_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_MO_PM3_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_MO_PM3_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_MO_PMIT_MASK _ULL(0x300000000) /* Performance Monitor Interrupt register group owner. */
#define KVX_SFR_MO_PMIT_SHIFT 32
#define KVX_SFR_MO_PMIT_WIDTH 2
#define KVX_SFR_MO_PMIT_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_MO_PMIT_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_MO_PMIT_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_MOW_MMI_MASK _ULL(0x3) /* Memory Management Instructions owner. */
#define KVX_SFR_MOW_MMI_SHIFT 0
#define KVX_SFR_MOW_MMI_WIDTH 2
#define KVX_SFR_MOW_MMI_WFXL_MASK _ULL(0x3)
#define KVX_SFR_MOW_MMI_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_MOW_MMI_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_MOW_RFE_MASK _ULL(0xc) /* RFE instruction owner. */
#define KVX_SFR_MOW_RFE_SHIFT 2
#define KVX_SFR_MOW_RFE_WIDTH 2
#define KVX_SFR_MOW_RFE_WFXL_MASK _ULL(0xc)
#define KVX_SFR_MOW_RFE_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_MOW_RFE_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_MOW_STOP_MASK _ULL(0x30) /* STOP instruction owner. */
#define KVX_SFR_MOW_STOP_SHIFT 4
#define KVX_SFR_MOW_STOP_WIDTH 2
#define KVX_SFR_MOW_STOP_WFXL_MASK _ULL(0x30)
#define KVX_SFR_MOW_STOP_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_MOW_STOP_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_MOW_SYNC_MASK _ULL(0xc0) /* SYNCGROUP instruction owner. */
#define KVX_SFR_MOW_SYNC_SHIFT 6
#define KVX_SFR_MOW_SYNC_WIDTH 2
#define KVX_SFR_MOW_SYNC_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_MOW_SYNC_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_MOW_SYNC_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_MOW_PCR_MASK _ULL(0x300) /* PCR register owner. */
#define KVX_SFR_MOW_PCR_SHIFT 8
#define KVX_SFR_MOW_PCR_WIDTH 2
#define KVX_SFR_MOW_PCR_WFXL_MASK _ULL(0x300)
#define KVX_SFR_MOW_PCR_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_MOW_PCR_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_MOW_MSG_MASK _ULL(0xc00) /* MMU SFR GROUP registers owner. */
#define KVX_SFR_MOW_MSG_SHIFT 10
#define KVX_SFR_MOW_MSG_WIDTH 2
#define KVX_SFR_MOW_MSG_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_MOW_MSG_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_MOW_MSG_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_MOW_MEN_MASK _ULL(0x3000) /* Miscellaneous External Notifications register owner. */
#define KVX_SFR_MOW_MEN_SHIFT 12
#define KVX_SFR_MOW_MEN_WIDTH 2
#define KVX_SFR_MOW_MEN_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_MOW_MEN_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_MOW_MEN_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_MOW_MES_MASK _ULL(0xc000) /* Memory Error Status register owner. */
#define KVX_SFR_MOW_MES_SHIFT 14
#define KVX_SFR_MOW_MES_WIDTH 2
#define KVX_SFR_MOW_MES_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_MOW_MES_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_MOW_MES_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_MOW_CSIT_MASK _ULL(0x30000) /* Compute Status Artithmetic Interrupt register owner. */
#define KVX_SFR_MOW_CSIT_SHIFT 16
#define KVX_SFR_MOW_CSIT_WIDTH 2
#define KVX_SFR_MOW_CSIT_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_MOW_CSIT_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_MOW_CSIT_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_MOW_T0_MASK _ULL(0xc0000) /* Timer 0 register group owner */
#define KVX_SFR_MOW_T0_SHIFT 18
#define KVX_SFR_MOW_T0_WIDTH 2
#define KVX_SFR_MOW_T0_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_MOW_T0_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_MOW_T0_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_MOW_T1_MASK _ULL(0x300000) /* Timer 1 register group owner */
#define KVX_SFR_MOW_T1_SHIFT 20
#define KVX_SFR_MOW_T1_WIDTH 2
#define KVX_SFR_MOW_T1_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_MOW_T1_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_MOW_T1_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_MOW_WD_MASK _ULL(0xc00000) /* Watch Dog register group owner. */
#define KVX_SFR_MOW_WD_SHIFT 22
#define KVX_SFR_MOW_WD_WIDTH 2
#define KVX_SFR_MOW_WD_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_MOW_WD_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_MOW_WD_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_MOW_PM0_MASK _ULL(0x3000000) /* Performance Monitor 0 register owner. */
#define KVX_SFR_MOW_PM0_SHIFT 24
#define KVX_SFR_MOW_PM0_WIDTH 2
#define KVX_SFR_MOW_PM0_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_MOW_PM0_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_MOW_PM0_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_MOW_PM1_MASK _ULL(0xc000000) /* Performance Monitor 1 register owner. */
#define KVX_SFR_MOW_PM1_SHIFT 26
#define KVX_SFR_MOW_PM1_WIDTH 2
#define KVX_SFR_MOW_PM1_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_MOW_PM1_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_MOW_PM1_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_MOW_PM2_MASK _ULL(0x30000000) /* Performance Monitor 2 register owner. */
#define KVX_SFR_MOW_PM2_SHIFT 28
#define KVX_SFR_MOW_PM2_WIDTH 2
#define KVX_SFR_MOW_PM2_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_MOW_PM2_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_MOW_PM2_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_MOW_PM3_MASK _ULL(0xc0000000) /* Performance Monitor 3 register owner. */
#define KVX_SFR_MOW_PM3_SHIFT 30
#define KVX_SFR_MOW_PM3_WIDTH 2
#define KVX_SFR_MOW_PM3_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_MOW_PM3_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_MOW_PM3_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_MOW_PMIT_MASK _ULL(0x300000000) /* Performance Monitor Interrupt register group owner. */
#define KVX_SFR_MOW_PMIT_SHIFT 32
#define KVX_SFR_MOW_PMIT_WIDTH 2
#define KVX_SFR_MOW_PMIT_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_MOW_PMIT_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_MOW_PMIT_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_PS_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_PS_PL_SHIFT 0
#define KVX_SFR_PS_PL_WIDTH 2
#define KVX_SFR_PS_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_PS_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_PS_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_PS_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_PS_ET_SHIFT 2
#define KVX_SFR_PS_ET_WIDTH 1
#define KVX_SFR_PS_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_PS_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_PS_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_PS_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_PS_HTD_SHIFT 3
#define KVX_SFR_PS_HTD_WIDTH 1
#define KVX_SFR_PS_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_PS_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_PS_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_PS_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_PS_IE_SHIFT 4
#define KVX_SFR_PS_IE_WIDTH 1
#define KVX_SFR_PS_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_PS_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_PS_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_PS_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_PS_HLE_SHIFT 5
#define KVX_SFR_PS_HLE_WIDTH 1
#define KVX_SFR_PS_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_PS_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_PS_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_PS_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_PS_SRE_SHIFT 6
#define KVX_SFR_PS_SRE_WIDTH 1
#define KVX_SFR_PS_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_PS_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_PS_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_PS_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_PS_DAUS_SHIFT 7
#define KVX_SFR_PS_DAUS_WIDTH 1
#define KVX_SFR_PS_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_PS_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_PS_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_PS_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_PS_ICE_SHIFT 8
#define KVX_SFR_PS_ICE_WIDTH 1
#define KVX_SFR_PS_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_PS_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_PS_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_PS_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_PS_USE_SHIFT 9
#define KVX_SFR_PS_USE_WIDTH 1
#define KVX_SFR_PS_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_PS_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_PS_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_PS_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_PS_DCE_SHIFT 10
#define KVX_SFR_PS_DCE_WIDTH 1
#define KVX_SFR_PS_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_PS_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_PS_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_PS_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_PS_MME_SHIFT 11
#define KVX_SFR_PS_MME_WIDTH 1
#define KVX_SFR_PS_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_PS_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_PS_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_PS_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_PS_IL_SHIFT 12
#define KVX_SFR_PS_IL_WIDTH 2
#define KVX_SFR_PS_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_PS_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_PS_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_PS_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_PS_VS_SHIFT 14
#define KVX_SFR_PS_VS_WIDTH 2
#define KVX_SFR_PS_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_PS_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_PS_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_PS_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_PS_V64_SHIFT 16
#define KVX_SFR_PS_V64_WIDTH 1
#define KVX_SFR_PS_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_PS_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_PS_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_PS_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_PS_L2E_SHIFT 17
#define KVX_SFR_PS_L2E_WIDTH 1
#define KVX_SFR_PS_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_PS_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_PS_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_PS_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_PS_SME_SHIFT 18
#define KVX_SFR_PS_SME_WIDTH 1
#define KVX_SFR_PS_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_PS_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_PS_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_PS_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_PS_SMR_SHIFT 19
#define KVX_SFR_PS_SMR_WIDTH 1
#define KVX_SFR_PS_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_PS_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_PS_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_PS_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_PS_PMJ_SHIFT 20
#define KVX_SFR_PS_PMJ_WIDTH 4
#define KVX_SFR_PS_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_PS_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_PS_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_PS_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_PS_MMUP_SHIFT 24
#define KVX_SFR_PS_MMUP_WIDTH 1
#define KVX_SFR_PS_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_PS_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_PS_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_SPS_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_SPS_PL_SHIFT 0
#define KVX_SFR_SPS_PL_WIDTH 2
#define KVX_SFR_SPS_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SPS_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SPS_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SPS_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_SPS_ET_SHIFT 2
#define KVX_SFR_SPS_ET_WIDTH 1
#define KVX_SFR_SPS_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_SPS_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_SPS_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_SPS_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_SPS_HTD_SHIFT 3
#define KVX_SFR_SPS_HTD_WIDTH 1
#define KVX_SFR_SPS_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_SPS_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_SPS_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_SPS_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_SPS_IE_SHIFT 4
#define KVX_SFR_SPS_IE_WIDTH 1
#define KVX_SFR_SPS_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_SPS_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_SPS_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_SPS_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_SPS_HLE_SHIFT 5
#define KVX_SFR_SPS_HLE_WIDTH 1
#define KVX_SFR_SPS_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_SPS_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_SPS_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_SPS_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_SPS_SRE_SHIFT 6
#define KVX_SFR_SPS_SRE_WIDTH 1
#define KVX_SFR_SPS_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_SPS_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_SPS_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_SPS_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_SPS_DAUS_SHIFT 7
#define KVX_SFR_SPS_DAUS_WIDTH 1
#define KVX_SFR_SPS_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_SPS_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_SPS_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_SPS_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_SPS_ICE_SHIFT 8
#define KVX_SFR_SPS_ICE_WIDTH 1
#define KVX_SFR_SPS_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_SPS_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_SPS_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_SPS_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_SPS_USE_SHIFT 9
#define KVX_SFR_SPS_USE_WIDTH 1
#define KVX_SFR_SPS_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_SPS_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_SPS_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_SPS_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_SPS_DCE_SHIFT 10
#define KVX_SFR_SPS_DCE_WIDTH 1
#define KVX_SFR_SPS_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_SPS_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_SPS_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_SPS_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_SPS_MME_SHIFT 11
#define KVX_SFR_SPS_MME_WIDTH 1
#define KVX_SFR_SPS_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_SPS_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_SPS_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_SPS_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_SPS_IL_SHIFT 12
#define KVX_SFR_SPS_IL_WIDTH 2
#define KVX_SFR_SPS_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_SPS_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_SPS_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_SPS_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_SPS_VS_SHIFT 14
#define KVX_SFR_SPS_VS_WIDTH 2
#define KVX_SFR_SPS_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_SPS_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_SPS_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_SPS_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_SPS_V64_SHIFT 16
#define KVX_SFR_SPS_V64_WIDTH 1
#define KVX_SFR_SPS_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_SPS_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_SPS_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_SPS_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_SPS_L2E_SHIFT 17
#define KVX_SFR_SPS_L2E_WIDTH 1
#define KVX_SFR_SPS_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_SPS_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_SPS_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_SPS_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_SPS_SME_SHIFT 18
#define KVX_SFR_SPS_SME_WIDTH 1
#define KVX_SFR_SPS_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_SPS_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_SPS_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_SPS_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_SPS_SMR_SHIFT 19
#define KVX_SFR_SPS_SMR_WIDTH 1
#define KVX_SFR_SPS_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_SPS_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_SPS_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_SPS_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_SPS_PMJ_SHIFT 20
#define KVX_SFR_SPS_PMJ_WIDTH 4
#define KVX_SFR_SPS_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_SPS_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_SPS_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_SPS_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_SPS_MMUP_SHIFT 24
#define KVX_SFR_SPS_MMUP_WIDTH 1
#define KVX_SFR_SPS_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_SPS_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_SPS_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_SPS_PL0_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_SPS_PL0_PL_SHIFT 0
#define KVX_SFR_SPS_PL0_PL_WIDTH 2
#define KVX_SFR_SPS_PL0_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SPS_PL0_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SPS_PL0_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SPS_PL0_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_SPS_PL0_ET_SHIFT 2
#define KVX_SFR_SPS_PL0_ET_WIDTH 1
#define KVX_SFR_SPS_PL0_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_SPS_PL0_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_SPS_PL0_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_SPS_PL0_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_SPS_PL0_HTD_SHIFT 3
#define KVX_SFR_SPS_PL0_HTD_WIDTH 1
#define KVX_SFR_SPS_PL0_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_SPS_PL0_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_SPS_PL0_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_SPS_PL0_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_SPS_PL0_IE_SHIFT 4
#define KVX_SFR_SPS_PL0_IE_WIDTH 1
#define KVX_SFR_SPS_PL0_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_SPS_PL0_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_SPS_PL0_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_SPS_PL0_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_SPS_PL0_HLE_SHIFT 5
#define KVX_SFR_SPS_PL0_HLE_WIDTH 1
#define KVX_SFR_SPS_PL0_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_SPS_PL0_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_SPS_PL0_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_SPS_PL0_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_SPS_PL0_SRE_SHIFT 6
#define KVX_SFR_SPS_PL0_SRE_WIDTH 1
#define KVX_SFR_SPS_PL0_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_SPS_PL0_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_SPS_PL0_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_SPS_PL0_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_SPS_PL0_DAUS_SHIFT 7
#define KVX_SFR_SPS_PL0_DAUS_WIDTH 1
#define KVX_SFR_SPS_PL0_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_SPS_PL0_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_SPS_PL0_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_SPS_PL0_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_SPS_PL0_ICE_SHIFT 8
#define KVX_SFR_SPS_PL0_ICE_WIDTH 1
#define KVX_SFR_SPS_PL0_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_SPS_PL0_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_SPS_PL0_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_SPS_PL0_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_SPS_PL0_USE_SHIFT 9
#define KVX_SFR_SPS_PL0_USE_WIDTH 1
#define KVX_SFR_SPS_PL0_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_SPS_PL0_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_SPS_PL0_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_SPS_PL0_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_SPS_PL0_DCE_SHIFT 10
#define KVX_SFR_SPS_PL0_DCE_WIDTH 1
#define KVX_SFR_SPS_PL0_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_SPS_PL0_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_SPS_PL0_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_SPS_PL0_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_SPS_PL0_MME_SHIFT 11
#define KVX_SFR_SPS_PL0_MME_WIDTH 1
#define KVX_SFR_SPS_PL0_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_SPS_PL0_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_SPS_PL0_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_SPS_PL0_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_SPS_PL0_IL_SHIFT 12
#define KVX_SFR_SPS_PL0_IL_WIDTH 2
#define KVX_SFR_SPS_PL0_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_SPS_PL0_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_SPS_PL0_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_SPS_PL0_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_SPS_PL0_VS_SHIFT 14
#define KVX_SFR_SPS_PL0_VS_WIDTH 2
#define KVX_SFR_SPS_PL0_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_SPS_PL0_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_SPS_PL0_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_SPS_PL0_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_SPS_PL0_V64_SHIFT 16
#define KVX_SFR_SPS_PL0_V64_WIDTH 1
#define KVX_SFR_SPS_PL0_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_SPS_PL0_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_SPS_PL0_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_SPS_PL0_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_SPS_PL0_L2E_SHIFT 17
#define KVX_SFR_SPS_PL0_L2E_WIDTH 1
#define KVX_SFR_SPS_PL0_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_SPS_PL0_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_SPS_PL0_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_SPS_PL0_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_SPS_PL0_SME_SHIFT 18
#define KVX_SFR_SPS_PL0_SME_WIDTH 1
#define KVX_SFR_SPS_PL0_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_SPS_PL0_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_SPS_PL0_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_SPS_PL0_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_SPS_PL0_SMR_SHIFT 19
#define KVX_SFR_SPS_PL0_SMR_WIDTH 1
#define KVX_SFR_SPS_PL0_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_SPS_PL0_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_SPS_PL0_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_SPS_PL0_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_SPS_PL0_PMJ_SHIFT 20
#define KVX_SFR_SPS_PL0_PMJ_WIDTH 4
#define KVX_SFR_SPS_PL0_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_SPS_PL0_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_SPS_PL0_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_SPS_PL0_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_SPS_PL0_MMUP_SHIFT 24
#define KVX_SFR_SPS_PL0_MMUP_WIDTH 1
#define KVX_SFR_SPS_PL0_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_SPS_PL0_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_SPS_PL0_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_SPS_PL1_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_SPS_PL1_PL_SHIFT 0
#define KVX_SFR_SPS_PL1_PL_WIDTH 2
#define KVX_SFR_SPS_PL1_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SPS_PL1_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SPS_PL1_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SPS_PL1_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_SPS_PL1_ET_SHIFT 2
#define KVX_SFR_SPS_PL1_ET_WIDTH 1
#define KVX_SFR_SPS_PL1_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_SPS_PL1_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_SPS_PL1_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_SPS_PL1_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_SPS_PL1_HTD_SHIFT 3
#define KVX_SFR_SPS_PL1_HTD_WIDTH 1
#define KVX_SFR_SPS_PL1_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_SPS_PL1_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_SPS_PL1_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_SPS_PL1_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_SPS_PL1_IE_SHIFT 4
#define KVX_SFR_SPS_PL1_IE_WIDTH 1
#define KVX_SFR_SPS_PL1_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_SPS_PL1_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_SPS_PL1_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_SPS_PL1_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_SPS_PL1_HLE_SHIFT 5
#define KVX_SFR_SPS_PL1_HLE_WIDTH 1
#define KVX_SFR_SPS_PL1_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_SPS_PL1_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_SPS_PL1_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_SPS_PL1_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_SPS_PL1_SRE_SHIFT 6
#define KVX_SFR_SPS_PL1_SRE_WIDTH 1
#define KVX_SFR_SPS_PL1_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_SPS_PL1_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_SPS_PL1_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_SPS_PL1_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_SPS_PL1_DAUS_SHIFT 7
#define KVX_SFR_SPS_PL1_DAUS_WIDTH 1
#define KVX_SFR_SPS_PL1_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_SPS_PL1_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_SPS_PL1_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_SPS_PL1_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_SPS_PL1_ICE_SHIFT 8
#define KVX_SFR_SPS_PL1_ICE_WIDTH 1
#define KVX_SFR_SPS_PL1_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_SPS_PL1_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_SPS_PL1_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_SPS_PL1_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_SPS_PL1_USE_SHIFT 9
#define KVX_SFR_SPS_PL1_USE_WIDTH 1
#define KVX_SFR_SPS_PL1_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_SPS_PL1_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_SPS_PL1_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_SPS_PL1_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_SPS_PL1_DCE_SHIFT 10
#define KVX_SFR_SPS_PL1_DCE_WIDTH 1
#define KVX_SFR_SPS_PL1_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_SPS_PL1_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_SPS_PL1_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_SPS_PL1_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_SPS_PL1_MME_SHIFT 11
#define KVX_SFR_SPS_PL1_MME_WIDTH 1
#define KVX_SFR_SPS_PL1_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_SPS_PL1_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_SPS_PL1_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_SPS_PL1_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_SPS_PL1_IL_SHIFT 12
#define KVX_SFR_SPS_PL1_IL_WIDTH 2
#define KVX_SFR_SPS_PL1_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_SPS_PL1_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_SPS_PL1_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_SPS_PL1_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_SPS_PL1_VS_SHIFT 14
#define KVX_SFR_SPS_PL1_VS_WIDTH 2
#define KVX_SFR_SPS_PL1_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_SPS_PL1_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_SPS_PL1_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_SPS_PL1_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_SPS_PL1_V64_SHIFT 16
#define KVX_SFR_SPS_PL1_V64_WIDTH 1
#define KVX_SFR_SPS_PL1_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_SPS_PL1_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_SPS_PL1_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_SPS_PL1_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_SPS_PL1_L2E_SHIFT 17
#define KVX_SFR_SPS_PL1_L2E_WIDTH 1
#define KVX_SFR_SPS_PL1_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_SPS_PL1_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_SPS_PL1_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_SPS_PL1_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_SPS_PL1_SME_SHIFT 18
#define KVX_SFR_SPS_PL1_SME_WIDTH 1
#define KVX_SFR_SPS_PL1_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_SPS_PL1_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_SPS_PL1_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_SPS_PL1_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_SPS_PL1_SMR_SHIFT 19
#define KVX_SFR_SPS_PL1_SMR_WIDTH 1
#define KVX_SFR_SPS_PL1_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_SPS_PL1_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_SPS_PL1_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_SPS_PL1_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_SPS_PL1_PMJ_SHIFT 20
#define KVX_SFR_SPS_PL1_PMJ_WIDTH 4
#define KVX_SFR_SPS_PL1_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_SPS_PL1_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_SPS_PL1_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_SPS_PL1_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_SPS_PL1_MMUP_SHIFT 24
#define KVX_SFR_SPS_PL1_MMUP_WIDTH 1
#define KVX_SFR_SPS_PL1_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_SPS_PL1_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_SPS_PL1_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_SPS_PL2_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_SPS_PL2_PL_SHIFT 0
#define KVX_SFR_SPS_PL2_PL_WIDTH 2
#define KVX_SFR_SPS_PL2_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SPS_PL2_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SPS_PL2_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SPS_PL2_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_SPS_PL2_ET_SHIFT 2
#define KVX_SFR_SPS_PL2_ET_WIDTH 1
#define KVX_SFR_SPS_PL2_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_SPS_PL2_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_SPS_PL2_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_SPS_PL2_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_SPS_PL2_HTD_SHIFT 3
#define KVX_SFR_SPS_PL2_HTD_WIDTH 1
#define KVX_SFR_SPS_PL2_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_SPS_PL2_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_SPS_PL2_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_SPS_PL2_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_SPS_PL2_IE_SHIFT 4
#define KVX_SFR_SPS_PL2_IE_WIDTH 1
#define KVX_SFR_SPS_PL2_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_SPS_PL2_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_SPS_PL2_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_SPS_PL2_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_SPS_PL2_HLE_SHIFT 5
#define KVX_SFR_SPS_PL2_HLE_WIDTH 1
#define KVX_SFR_SPS_PL2_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_SPS_PL2_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_SPS_PL2_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_SPS_PL2_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_SPS_PL2_SRE_SHIFT 6
#define KVX_SFR_SPS_PL2_SRE_WIDTH 1
#define KVX_SFR_SPS_PL2_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_SPS_PL2_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_SPS_PL2_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_SPS_PL2_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_SPS_PL2_DAUS_SHIFT 7
#define KVX_SFR_SPS_PL2_DAUS_WIDTH 1
#define KVX_SFR_SPS_PL2_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_SPS_PL2_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_SPS_PL2_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_SPS_PL2_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_SPS_PL2_ICE_SHIFT 8
#define KVX_SFR_SPS_PL2_ICE_WIDTH 1
#define KVX_SFR_SPS_PL2_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_SPS_PL2_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_SPS_PL2_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_SPS_PL2_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_SPS_PL2_USE_SHIFT 9
#define KVX_SFR_SPS_PL2_USE_WIDTH 1
#define KVX_SFR_SPS_PL2_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_SPS_PL2_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_SPS_PL2_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_SPS_PL2_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_SPS_PL2_DCE_SHIFT 10
#define KVX_SFR_SPS_PL2_DCE_WIDTH 1
#define KVX_SFR_SPS_PL2_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_SPS_PL2_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_SPS_PL2_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_SPS_PL2_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_SPS_PL2_MME_SHIFT 11
#define KVX_SFR_SPS_PL2_MME_WIDTH 1
#define KVX_SFR_SPS_PL2_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_SPS_PL2_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_SPS_PL2_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_SPS_PL2_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_SPS_PL2_IL_SHIFT 12
#define KVX_SFR_SPS_PL2_IL_WIDTH 2
#define KVX_SFR_SPS_PL2_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_SPS_PL2_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_SPS_PL2_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_SPS_PL2_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_SPS_PL2_VS_SHIFT 14
#define KVX_SFR_SPS_PL2_VS_WIDTH 2
#define KVX_SFR_SPS_PL2_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_SPS_PL2_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_SPS_PL2_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_SPS_PL2_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_SPS_PL2_V64_SHIFT 16
#define KVX_SFR_SPS_PL2_V64_WIDTH 1
#define KVX_SFR_SPS_PL2_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_SPS_PL2_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_SPS_PL2_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_SPS_PL2_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_SPS_PL2_L2E_SHIFT 17
#define KVX_SFR_SPS_PL2_L2E_WIDTH 1
#define KVX_SFR_SPS_PL2_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_SPS_PL2_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_SPS_PL2_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_SPS_PL2_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_SPS_PL2_SME_SHIFT 18
#define KVX_SFR_SPS_PL2_SME_WIDTH 1
#define KVX_SFR_SPS_PL2_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_SPS_PL2_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_SPS_PL2_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_SPS_PL2_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_SPS_PL2_SMR_SHIFT 19
#define KVX_SFR_SPS_PL2_SMR_WIDTH 1
#define KVX_SFR_SPS_PL2_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_SPS_PL2_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_SPS_PL2_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_SPS_PL2_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_SPS_PL2_PMJ_SHIFT 20
#define KVX_SFR_SPS_PL2_PMJ_WIDTH 4
#define KVX_SFR_SPS_PL2_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_SPS_PL2_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_SPS_PL2_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_SPS_PL2_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_SPS_PL2_MMUP_SHIFT 24
#define KVX_SFR_SPS_PL2_MMUP_WIDTH 1
#define KVX_SFR_SPS_PL2_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_SPS_PL2_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_SPS_PL2_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_SPS_PL3_PL_MASK _ULL(0x3) /* Current Privilege Level */
#define KVX_SFR_SPS_PL3_PL_SHIFT 0
#define KVX_SFR_SPS_PL3_PL_WIDTH 2
#define KVX_SFR_SPS_PL3_PL_WFXL_MASK _ULL(0x3)
#define KVX_SFR_SPS_PL3_PL_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_SPS_PL3_PL_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_SPS_PL3_ET_MASK _ULL(0x4) /* Exception Taken */
#define KVX_SFR_SPS_PL3_ET_SHIFT 2
#define KVX_SFR_SPS_PL3_ET_WIDTH 1
#define KVX_SFR_SPS_PL3_ET_WFXL_MASK _ULL(0x4)
#define KVX_SFR_SPS_PL3_ET_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_SPS_PL3_ET_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_SPS_PL3_HTD_MASK _ULL(0x8) /* Hardware Trap Disable */
#define KVX_SFR_SPS_PL3_HTD_SHIFT 3
#define KVX_SFR_SPS_PL3_HTD_WIDTH 1
#define KVX_SFR_SPS_PL3_HTD_WFXL_MASK _ULL(0x8)
#define KVX_SFR_SPS_PL3_HTD_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_SPS_PL3_HTD_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_SPS_PL3_IE_MASK _ULL(0x10) /* Interrupt Enable */
#define KVX_SFR_SPS_PL3_IE_SHIFT 4
#define KVX_SFR_SPS_PL3_IE_WIDTH 1
#define KVX_SFR_SPS_PL3_IE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_SPS_PL3_IE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_SPS_PL3_IE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_SPS_PL3_HLE_MASK _ULL(0x20) /* Hardware Loop Enable */
#define KVX_SFR_SPS_PL3_HLE_SHIFT 5
#define KVX_SFR_SPS_PL3_HLE_WIDTH 1
#define KVX_SFR_SPS_PL3_HLE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_SPS_PL3_HLE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_SPS_PL3_HLE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_SPS_PL3_SRE_MASK _ULL(0x40) /* Software REserved */
#define KVX_SFR_SPS_PL3_SRE_SHIFT 6
#define KVX_SFR_SPS_PL3_SRE_WIDTH 1
#define KVX_SFR_SPS_PL3_SRE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_SPS_PL3_SRE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_SPS_PL3_SRE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_SPS_PL3_DAUS_MASK _ULL(0x80) /* Data Accesses Use SPS settings */
#define KVX_SFR_SPS_PL3_DAUS_SHIFT 7
#define KVX_SFR_SPS_PL3_DAUS_WIDTH 1
#define KVX_SFR_SPS_PL3_DAUS_WFXL_MASK _ULL(0x80)
#define KVX_SFR_SPS_PL3_DAUS_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_SPS_PL3_DAUS_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_SPS_PL3_ICE_MASK _ULL(0x100) /* Instruction Cache Enable */
#define KVX_SFR_SPS_PL3_ICE_SHIFT 8
#define KVX_SFR_SPS_PL3_ICE_WIDTH 1
#define KVX_SFR_SPS_PL3_ICE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_SPS_PL3_ICE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_SPS_PL3_ICE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_SPS_PL3_USE_MASK _ULL(0x200) /* Uncached Streaming Enable */
#define KVX_SFR_SPS_PL3_USE_SHIFT 9
#define KVX_SFR_SPS_PL3_USE_WIDTH 1
#define KVX_SFR_SPS_PL3_USE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_SPS_PL3_USE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_SPS_PL3_USE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_SPS_PL3_DCE_MASK _ULL(0x400) /* Data Cache Enable */
#define KVX_SFR_SPS_PL3_DCE_SHIFT 10
#define KVX_SFR_SPS_PL3_DCE_WIDTH 1
#define KVX_SFR_SPS_PL3_DCE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_SPS_PL3_DCE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_SPS_PL3_DCE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_SPS_PL3_MME_MASK _ULL(0x800) /* Memory Management Enable */
#define KVX_SFR_SPS_PL3_MME_SHIFT 11
#define KVX_SFR_SPS_PL3_MME_WIDTH 1
#define KVX_SFR_SPS_PL3_MME_WFXL_MASK _ULL(0x800)
#define KVX_SFR_SPS_PL3_MME_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_SPS_PL3_MME_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_SPS_PL3_IL_MASK _ULL(0x3000) /* Interrupt Level */
#define KVX_SFR_SPS_PL3_IL_SHIFT 12
#define KVX_SFR_SPS_PL3_IL_WIDTH 2
#define KVX_SFR_SPS_PL3_IL_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_SPS_PL3_IL_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_SPS_PL3_IL_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_SPS_PL3_VS_MASK _ULL(0xc000) /* Virtual Space */
#define KVX_SFR_SPS_PL3_VS_SHIFT 14
#define KVX_SFR_SPS_PL3_VS_WIDTH 2
#define KVX_SFR_SPS_PL3_VS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_SPS_PL3_VS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_SPS_PL3_VS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_SPS_PL3_V64_MASK _ULL(0x10000) /* Virtual 64 bits mode. */
#define KVX_SFR_SPS_PL3_V64_SHIFT 16
#define KVX_SFR_SPS_PL3_V64_WIDTH 1
#define KVX_SFR_SPS_PL3_V64_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_SPS_PL3_V64_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_SPS_PL3_V64_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_SPS_PL3_L2E_MASK _ULL(0x20000) /* L2 cache Enable. */
#define KVX_SFR_SPS_PL3_L2E_SHIFT 17
#define KVX_SFR_SPS_PL3_L2E_WIDTH 1
#define KVX_SFR_SPS_PL3_L2E_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_SPS_PL3_L2E_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_SPS_PL3_L2E_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_SPS_PL3_SME_MASK _ULL(0x40000) /* Step Mode Enabled */
#define KVX_SFR_SPS_PL3_SME_SHIFT 18
#define KVX_SFR_SPS_PL3_SME_WIDTH 1
#define KVX_SFR_SPS_PL3_SME_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_SPS_PL3_SME_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_SPS_PL3_SME_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_SPS_PL3_SMR_MASK _ULL(0x80000) /* Step Mode Ready */
#define KVX_SFR_SPS_PL3_SMR_SHIFT 19
#define KVX_SFR_SPS_PL3_SMR_WIDTH 1
#define KVX_SFR_SPS_PL3_SMR_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_SPS_PL3_SMR_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_SPS_PL3_SMR_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_SPS_PL3_PMJ_MASK _ULL(0xf00000) /* Page Mask in JTLB. */
#define KVX_SFR_SPS_PL3_PMJ_SHIFT 20
#define KVX_SFR_SPS_PL3_PMJ_WIDTH 4
#define KVX_SFR_SPS_PL3_PMJ_WFXL_MASK _ULL(0xf00000)
#define KVX_SFR_SPS_PL3_PMJ_WFXL_CLEAR _ULL(0xf00000)
#define KVX_SFR_SPS_PL3_PMJ_WFXL_SET _ULL(0xf0000000000000)

#define KVX_SFR_SPS_PL3_MMUP_MASK _ULL(0x1000000) /* Privileged on MMU. */
#define KVX_SFR_SPS_PL3_MMUP_SHIFT 24
#define KVX_SFR_SPS_PL3_MMUP_WIDTH 1
#define KVX_SFR_SPS_PL3_MMUP_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_SPS_PL3_MMUP_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_SPS_PL3_MMUP_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_PSO_PL0_MASK _ULL(0x3) /* Current Privilege Level bit 0 owner */
#define KVX_SFR_PSO_PL0_SHIFT 0
#define KVX_SFR_PSO_PL0_WIDTH 2
#define KVX_SFR_PSO_PL0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_PSO_PL0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_PSO_PL0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_PSO_PL1_MASK _ULL(0xc) /* Current Privilege Level bit 1 owner */
#define KVX_SFR_PSO_PL1_SHIFT 2
#define KVX_SFR_PSO_PL1_WIDTH 2
#define KVX_SFR_PSO_PL1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_PSO_PL1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_PSO_PL1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_PSO_ET_MASK _ULL(0x30) /* Exception Taken owner */
#define KVX_SFR_PSO_ET_SHIFT 4
#define KVX_SFR_PSO_ET_WIDTH 2
#define KVX_SFR_PSO_ET_WFXL_MASK _ULL(0x30)
#define KVX_SFR_PSO_ET_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_PSO_ET_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_PSO_HTD_MASK _ULL(0xc0) /* Hardware Trap Disable owner */
#define KVX_SFR_PSO_HTD_SHIFT 6
#define KVX_SFR_PSO_HTD_WIDTH 2
#define KVX_SFR_PSO_HTD_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_PSO_HTD_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_PSO_HTD_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_PSO_IE_MASK _ULL(0x300) /* Interrupt Enable owner */
#define KVX_SFR_PSO_IE_SHIFT 8
#define KVX_SFR_PSO_IE_WIDTH 2
#define KVX_SFR_PSO_IE_WFXL_MASK _ULL(0x300)
#define KVX_SFR_PSO_IE_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_PSO_IE_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_PSO_HLE_MASK _ULL(0xc00) /* Hardware Loop Enable owner */
#define KVX_SFR_PSO_HLE_SHIFT 10
#define KVX_SFR_PSO_HLE_WIDTH 2
#define KVX_SFR_PSO_HLE_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_PSO_HLE_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_PSO_HLE_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_PSO_SRE_MASK _ULL(0x3000) /* Software REserved owner */
#define KVX_SFR_PSO_SRE_SHIFT 12
#define KVX_SFR_PSO_SRE_WIDTH 2
#define KVX_SFR_PSO_SRE_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_PSO_SRE_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_PSO_SRE_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_PSO_DAUS_MASK _ULL(0xc000) /* Data Accesses Use SPS settings owner */
#define KVX_SFR_PSO_DAUS_SHIFT 14
#define KVX_SFR_PSO_DAUS_WIDTH 2
#define KVX_SFR_PSO_DAUS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_PSO_DAUS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_PSO_DAUS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_PSO_ICE_MASK _ULL(0x30000) /* Instruction Cache Enable owner */
#define KVX_SFR_PSO_ICE_SHIFT 16
#define KVX_SFR_PSO_ICE_WIDTH 2
#define KVX_SFR_PSO_ICE_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_PSO_ICE_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_PSO_ICE_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_PSO_USE_MASK _ULL(0xc0000) /* Uncached Streaming Enable owner */
#define KVX_SFR_PSO_USE_SHIFT 18
#define KVX_SFR_PSO_USE_WIDTH 2
#define KVX_SFR_PSO_USE_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_PSO_USE_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_PSO_USE_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_PSO_DCE_MASK _ULL(0x300000) /* Data Cache Enable owner */
#define KVX_SFR_PSO_DCE_SHIFT 20
#define KVX_SFR_PSO_DCE_WIDTH 2
#define KVX_SFR_PSO_DCE_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_PSO_DCE_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_PSO_DCE_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_PSO_MME_MASK _ULL(0xc00000) /* Memory Management Enable owner */
#define KVX_SFR_PSO_MME_SHIFT 22
#define KVX_SFR_PSO_MME_WIDTH 2
#define KVX_SFR_PSO_MME_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_PSO_MME_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_PSO_MME_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_PSO_IL0_MASK _ULL(0x3000000) /* Interrupt Level bit 0 owner */
#define KVX_SFR_PSO_IL0_SHIFT 24
#define KVX_SFR_PSO_IL0_WIDTH 2
#define KVX_SFR_PSO_IL0_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_PSO_IL0_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_PSO_IL0_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_PSO_IL1_MASK _ULL(0xc000000) /* Interrupt Level bit 1 owner */
#define KVX_SFR_PSO_IL1_SHIFT 26
#define KVX_SFR_PSO_IL1_WIDTH 2
#define KVX_SFR_PSO_IL1_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_PSO_IL1_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_PSO_IL1_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_PSO_VS0_MASK _ULL(0x30000000) /* Virtual Space bit 0 owner */
#define KVX_SFR_PSO_VS0_SHIFT 28
#define KVX_SFR_PSO_VS0_WIDTH 2
#define KVX_SFR_PSO_VS0_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_PSO_VS0_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_PSO_VS0_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_PSO_VS1_MASK _ULL(0xc0000000) /* Virtual Space bit 1 owner */
#define KVX_SFR_PSO_VS1_SHIFT 30
#define KVX_SFR_PSO_VS1_WIDTH 2
#define KVX_SFR_PSO_VS1_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_PSO_VS1_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_PSO_VS1_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_PSO_V64_MASK _ULL(0x300000000) /* Virtual 64 bits mode owner */
#define KVX_SFR_PSO_V64_SHIFT 32
#define KVX_SFR_PSO_V64_WIDTH 2
#define KVX_SFR_PSO_V64_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_PSO_V64_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_PSO_V64_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_PSO_L2E_MASK _ULL(0xc00000000) /* L2 cache Enable owner */
#define KVX_SFR_PSO_L2E_SHIFT 34
#define KVX_SFR_PSO_L2E_WIDTH 2
#define KVX_SFR_PSO_L2E_WFXM_MASK _ULL(0xc00000000)
#define KVX_SFR_PSO_L2E_WFXM_CLEAR _ULL(0xc)
#define KVX_SFR_PSO_L2E_WFXM_SET _ULL(0xc00000000)

#define KVX_SFR_PSO_SME_MASK _ULL(0x3000000000) /* Step Mode Enabled owner */
#define KVX_SFR_PSO_SME_SHIFT 36
#define KVX_SFR_PSO_SME_WIDTH 2
#define KVX_SFR_PSO_SME_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_PSO_SME_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_PSO_SME_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_PSO_SMR_MASK _ULL(0xc000000000) /* Step Mode Ready owner */
#define KVX_SFR_PSO_SMR_SHIFT 38
#define KVX_SFR_PSO_SMR_WIDTH 2
#define KVX_SFR_PSO_SMR_WFXM_MASK _ULL(0xc000000000)
#define KVX_SFR_PSO_SMR_WFXM_CLEAR _ULL(0xc0)
#define KVX_SFR_PSO_SMR_WFXM_SET _ULL(0xc000000000)

#define KVX_SFR_PSO_PMJ0_MASK _ULL(0x30000000000) /* Page Mask in JTLB bit 0 owner */
#define KVX_SFR_PSO_PMJ0_SHIFT 40
#define KVX_SFR_PSO_PMJ0_WIDTH 2
#define KVX_SFR_PSO_PMJ0_WFXM_MASK _ULL(0x30000000000)
#define KVX_SFR_PSO_PMJ0_WFXM_CLEAR _ULL(0x300)
#define KVX_SFR_PSO_PMJ0_WFXM_SET _ULL(0x30000000000)

#define KVX_SFR_PSO_PMJ1_MASK _ULL(0xc0000000000) /* Page Mask in JTLB bit 1 owner */
#define KVX_SFR_PSO_PMJ1_SHIFT 42
#define KVX_SFR_PSO_PMJ1_WIDTH 2
#define KVX_SFR_PSO_PMJ1_WFXM_MASK _ULL(0xc0000000000)
#define KVX_SFR_PSO_PMJ1_WFXM_CLEAR _ULL(0xc00)
#define KVX_SFR_PSO_PMJ1_WFXM_SET _ULL(0xc0000000000)

#define KVX_SFR_PSO_PMJ2_MASK _ULL(0x300000000000) /* Page Mask in JTLB bit 2 owner */
#define KVX_SFR_PSO_PMJ2_SHIFT 44
#define KVX_SFR_PSO_PMJ2_WIDTH 2
#define KVX_SFR_PSO_PMJ2_WFXM_MASK _ULL(0x300000000000)
#define KVX_SFR_PSO_PMJ2_WFXM_CLEAR _ULL(0x3000)
#define KVX_SFR_PSO_PMJ2_WFXM_SET _ULL(0x300000000000)

#define KVX_SFR_PSO_PMJ3_MASK _ULL(0xc00000000000) /* Page Mask in JTLB bit 3 owner */
#define KVX_SFR_PSO_PMJ3_SHIFT 46
#define KVX_SFR_PSO_PMJ3_WIDTH 2
#define KVX_SFR_PSO_PMJ3_WFXM_MASK _ULL(0xc00000000000)
#define KVX_SFR_PSO_PMJ3_WFXM_CLEAR _ULL(0xc000)
#define KVX_SFR_PSO_PMJ3_WFXM_SET _ULL(0xc00000000000)

#define KVX_SFR_PSO_MMUP_MASK _ULL(0x3000000000000) /* Privileged on MMU owner. */
#define KVX_SFR_PSO_MMUP_SHIFT 48
#define KVX_SFR_PSO_MMUP_WIDTH 2
#define KVX_SFR_PSO_MMUP_WFXM_MASK _ULL(0x3000000000000)
#define KVX_SFR_PSO_MMUP_WFXM_CLEAR _ULL(0x30000)
#define KVX_SFR_PSO_MMUP_WFXM_SET _ULL(0x3000000000000)

#define KVX_SFR_PSOW_PL0_MASK _ULL(0x3) /* Current Privilege Level bit 0 owner */
#define KVX_SFR_PSOW_PL0_SHIFT 0
#define KVX_SFR_PSOW_PL0_WIDTH 2
#define KVX_SFR_PSOW_PL0_WFXL_MASK _ULL(0x3)
#define KVX_SFR_PSOW_PL0_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_PSOW_PL0_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_PSOW_PL1_MASK _ULL(0xc) /* Current Privilege Level bit 1 owner */
#define KVX_SFR_PSOW_PL1_SHIFT 2
#define KVX_SFR_PSOW_PL1_WIDTH 2
#define KVX_SFR_PSOW_PL1_WFXL_MASK _ULL(0xc)
#define KVX_SFR_PSOW_PL1_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_PSOW_PL1_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_PSOW_ET_MASK _ULL(0x30) /* Exception Taken owner */
#define KVX_SFR_PSOW_ET_SHIFT 4
#define KVX_SFR_PSOW_ET_WIDTH 2
#define KVX_SFR_PSOW_ET_WFXL_MASK _ULL(0x30)
#define KVX_SFR_PSOW_ET_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_PSOW_ET_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_PSOW_HTD_MASK _ULL(0xc0) /* Hardware Trap Disable owner */
#define KVX_SFR_PSOW_HTD_SHIFT 6
#define KVX_SFR_PSOW_HTD_WIDTH 2
#define KVX_SFR_PSOW_HTD_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_PSOW_HTD_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_PSOW_HTD_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_PSOW_IE_MASK _ULL(0x300) /* Interrupt Enable owner */
#define KVX_SFR_PSOW_IE_SHIFT 8
#define KVX_SFR_PSOW_IE_WIDTH 2
#define KVX_SFR_PSOW_IE_WFXL_MASK _ULL(0x300)
#define KVX_SFR_PSOW_IE_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_PSOW_IE_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_PSOW_HLE_MASK _ULL(0xc00) /* Hardware Loop Enable owner */
#define KVX_SFR_PSOW_HLE_SHIFT 10
#define KVX_SFR_PSOW_HLE_WIDTH 2
#define KVX_SFR_PSOW_HLE_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_PSOW_HLE_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_PSOW_HLE_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_PSOW_SRE_MASK _ULL(0x3000) /* Software REserved owner */
#define KVX_SFR_PSOW_SRE_SHIFT 12
#define KVX_SFR_PSOW_SRE_WIDTH 2
#define KVX_SFR_PSOW_SRE_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_PSOW_SRE_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_PSOW_SRE_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_PSOW_DAUS_MASK _ULL(0xc000) /* Data Accesses Use SPS settings owner */
#define KVX_SFR_PSOW_DAUS_SHIFT 14
#define KVX_SFR_PSOW_DAUS_WIDTH 2
#define KVX_SFR_PSOW_DAUS_WFXL_MASK _ULL(0xc000)
#define KVX_SFR_PSOW_DAUS_WFXL_CLEAR _ULL(0xc000)
#define KVX_SFR_PSOW_DAUS_WFXL_SET _ULL(0xc00000000000)

#define KVX_SFR_PSOW_ICE_MASK _ULL(0x30000) /* Instruction Cache Enable owner */
#define KVX_SFR_PSOW_ICE_SHIFT 16
#define KVX_SFR_PSOW_ICE_WIDTH 2
#define KVX_SFR_PSOW_ICE_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_PSOW_ICE_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_PSOW_ICE_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_PSOW_USE_MASK _ULL(0xc0000) /* Uncached Streaming Enable owner */
#define KVX_SFR_PSOW_USE_SHIFT 18
#define KVX_SFR_PSOW_USE_WIDTH 2
#define KVX_SFR_PSOW_USE_WFXL_MASK _ULL(0xc0000)
#define KVX_SFR_PSOW_USE_WFXL_CLEAR _ULL(0xc0000)
#define KVX_SFR_PSOW_USE_WFXL_SET _ULL(0xc000000000000)

#define KVX_SFR_PSOW_DCE_MASK _ULL(0x300000) /* Data Cache Enable owner */
#define KVX_SFR_PSOW_DCE_SHIFT 20
#define KVX_SFR_PSOW_DCE_WIDTH 2
#define KVX_SFR_PSOW_DCE_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_PSOW_DCE_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_PSOW_DCE_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_PSOW_MME_MASK _ULL(0xc00000) /* Memory Management Enable owner */
#define KVX_SFR_PSOW_MME_SHIFT 22
#define KVX_SFR_PSOW_MME_WIDTH 2
#define KVX_SFR_PSOW_MME_WFXL_MASK _ULL(0xc00000)
#define KVX_SFR_PSOW_MME_WFXL_CLEAR _ULL(0xc00000)
#define KVX_SFR_PSOW_MME_WFXL_SET _ULL(0xc0000000000000)

#define KVX_SFR_PSOW_IL0_MASK _ULL(0x3000000) /* Interrupt Level bit 0 owner */
#define KVX_SFR_PSOW_IL0_SHIFT 24
#define KVX_SFR_PSOW_IL0_WIDTH 2
#define KVX_SFR_PSOW_IL0_WFXL_MASK _ULL(0x3000000)
#define KVX_SFR_PSOW_IL0_WFXL_CLEAR _ULL(0x3000000)
#define KVX_SFR_PSOW_IL0_WFXL_SET _ULL(0x300000000000000)

#define KVX_SFR_PSOW_IL1_MASK _ULL(0xc000000) /* Interrupt Level bit 1 owner */
#define KVX_SFR_PSOW_IL1_SHIFT 26
#define KVX_SFR_PSOW_IL1_WIDTH 2
#define KVX_SFR_PSOW_IL1_WFXL_MASK _ULL(0xc000000)
#define KVX_SFR_PSOW_IL1_WFXL_CLEAR _ULL(0xc000000)
#define KVX_SFR_PSOW_IL1_WFXL_SET _ULL(0xc00000000000000)

#define KVX_SFR_PSOW_VS0_MASK _ULL(0x30000000) /* Virtual Space bit 0 owner */
#define KVX_SFR_PSOW_VS0_SHIFT 28
#define KVX_SFR_PSOW_VS0_WIDTH 2
#define KVX_SFR_PSOW_VS0_WFXL_MASK _ULL(0x30000000)
#define KVX_SFR_PSOW_VS0_WFXL_CLEAR _ULL(0x30000000)
#define KVX_SFR_PSOW_VS0_WFXL_SET _ULL(0x3000000000000000)

#define KVX_SFR_PSOW_VS1_MASK _ULL(0xc0000000) /* Virtual Space bit 1 owner */
#define KVX_SFR_PSOW_VS1_SHIFT 30
#define KVX_SFR_PSOW_VS1_WIDTH 2
#define KVX_SFR_PSOW_VS1_WFXL_MASK _ULL(0xc0000000)
#define KVX_SFR_PSOW_VS1_WFXL_CLEAR _ULL(0xc0000000)
#define KVX_SFR_PSOW_VS1_WFXL_SET _ULL(0xc000000000000000)

#define KVX_SFR_PSOW_V64_MASK _ULL(0x300000000) /* Virtual 64 bits mode owner */
#define KVX_SFR_PSOW_V64_SHIFT 32
#define KVX_SFR_PSOW_V64_WIDTH 2
#define KVX_SFR_PSOW_V64_WFXM_MASK _ULL(0x300000000)
#define KVX_SFR_PSOW_V64_WFXM_CLEAR _ULL(0x3)
#define KVX_SFR_PSOW_V64_WFXM_SET _ULL(0x300000000)

#define KVX_SFR_PSOW_L2E_MASK _ULL(0xc00000000) /* L2 cache Enable owner */
#define KVX_SFR_PSOW_L2E_SHIFT 34
#define KVX_SFR_PSOW_L2E_WIDTH 2
#define KVX_SFR_PSOW_L2E_WFXM_MASK _ULL(0xc00000000)
#define KVX_SFR_PSOW_L2E_WFXM_CLEAR _ULL(0xc)
#define KVX_SFR_PSOW_L2E_WFXM_SET _ULL(0xc00000000)

#define KVX_SFR_PSOW_SME_MASK _ULL(0x3000000000) /* Step Mode Enabled owner */
#define KVX_SFR_PSOW_SME_SHIFT 36
#define KVX_SFR_PSOW_SME_WIDTH 2
#define KVX_SFR_PSOW_SME_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_PSOW_SME_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_PSOW_SME_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_PSOW_SMR_MASK _ULL(0xc000000000) /* Step Mode Ready owner */
#define KVX_SFR_PSOW_SMR_SHIFT 38
#define KVX_SFR_PSOW_SMR_WIDTH 2
#define KVX_SFR_PSOW_SMR_WFXM_MASK _ULL(0xc000000000)
#define KVX_SFR_PSOW_SMR_WFXM_CLEAR _ULL(0xc0)
#define KVX_SFR_PSOW_SMR_WFXM_SET _ULL(0xc000000000)

#define KVX_SFR_PSOW_PMJ0_MASK _ULL(0x30000000000) /* Page Mask in JTLB bit 0 owner */
#define KVX_SFR_PSOW_PMJ0_SHIFT 40
#define KVX_SFR_PSOW_PMJ0_WIDTH 2
#define KVX_SFR_PSOW_PMJ0_WFXM_MASK _ULL(0x30000000000)
#define KVX_SFR_PSOW_PMJ0_WFXM_CLEAR _ULL(0x300)
#define KVX_SFR_PSOW_PMJ0_WFXM_SET _ULL(0x30000000000)

#define KVX_SFR_PSOW_PMJ1_MASK _ULL(0xc0000000000) /* Page Mask in JTLB bit 1 owner */
#define KVX_SFR_PSOW_PMJ1_SHIFT 42
#define KVX_SFR_PSOW_PMJ1_WIDTH 2
#define KVX_SFR_PSOW_PMJ1_WFXM_MASK _ULL(0xc0000000000)
#define KVX_SFR_PSOW_PMJ1_WFXM_CLEAR _ULL(0xc00)
#define KVX_SFR_PSOW_PMJ1_WFXM_SET _ULL(0xc0000000000)

#define KVX_SFR_PSOW_PMJ2_MASK _ULL(0x300000000000) /* Page Mask in JTLB bit 2 owner */
#define KVX_SFR_PSOW_PMJ2_SHIFT 44
#define KVX_SFR_PSOW_PMJ2_WIDTH 2
#define KVX_SFR_PSOW_PMJ2_WFXM_MASK _ULL(0x300000000000)
#define KVX_SFR_PSOW_PMJ2_WFXM_CLEAR _ULL(0x3000)
#define KVX_SFR_PSOW_PMJ2_WFXM_SET _ULL(0x300000000000)

#define KVX_SFR_PSOW_PMJ3_MASK _ULL(0xc00000000000) /* Page Mask in JTLB bit 3 owner */
#define KVX_SFR_PSOW_PMJ3_SHIFT 46
#define KVX_SFR_PSOW_PMJ3_WIDTH 2
#define KVX_SFR_PSOW_PMJ3_WFXM_MASK _ULL(0xc00000000000)
#define KVX_SFR_PSOW_PMJ3_WFXM_CLEAR _ULL(0xc000)
#define KVX_SFR_PSOW_PMJ3_WFXM_SET _ULL(0xc00000000000)

#define KVX_SFR_PSOW_MMUP_MASK _ULL(0x3000000000000) /* Privileged on MMU owner. */
#define KVX_SFR_PSOW_MMUP_SHIFT 48
#define KVX_SFR_PSOW_MMUP_WIDTH 2
#define KVX_SFR_PSOW_MMUP_WFXM_MASK _ULL(0x3000000000000)
#define KVX_SFR_PSOW_MMUP_WFXM_CLEAR _ULL(0x30000)
#define KVX_SFR_PSOW_MMUP_WFXM_SET _ULL(0x3000000000000)

#define KVX_SFR_CS_IC_MASK _ULL(0x1) /* Integer Carry */
#define KVX_SFR_CS_IC_SHIFT 0
#define KVX_SFR_CS_IC_WIDTH 1
#define KVX_SFR_CS_IC_WFXL_MASK _ULL(0x1)
#define KVX_SFR_CS_IC_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_CS_IC_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_CS_IO_MASK _ULL(0x2) /* IEEE 754 Invalid Operation */
#define KVX_SFR_CS_IO_SHIFT 1
#define KVX_SFR_CS_IO_WIDTH 1
#define KVX_SFR_CS_IO_WFXL_MASK _ULL(0x2)
#define KVX_SFR_CS_IO_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_CS_IO_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_CS_DZ_MASK _ULL(0x4) /* IEEE 754 Divide by Zero */
#define KVX_SFR_CS_DZ_SHIFT 2
#define KVX_SFR_CS_DZ_WIDTH 1
#define KVX_SFR_CS_DZ_WFXL_MASK _ULL(0x4)
#define KVX_SFR_CS_DZ_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_CS_DZ_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_CS_OV_MASK _ULL(0x8) /* IEEE 754 Overflow */
#define KVX_SFR_CS_OV_SHIFT 3
#define KVX_SFR_CS_OV_WIDTH 1
#define KVX_SFR_CS_OV_WFXL_MASK _ULL(0x8)
#define KVX_SFR_CS_OV_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_CS_OV_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_CS_UN_MASK _ULL(0x10) /* IEEE 754 Underflow */
#define KVX_SFR_CS_UN_SHIFT 4
#define KVX_SFR_CS_UN_WIDTH 1
#define KVX_SFR_CS_UN_WFXL_MASK _ULL(0x10)
#define KVX_SFR_CS_UN_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_CS_UN_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_CS_IN_MASK _ULL(0x20) /* IEEE 754 Inexact */
#define KVX_SFR_CS_IN_SHIFT 5
#define KVX_SFR_CS_IN_WIDTH 1
#define KVX_SFR_CS_IN_WFXL_MASK _ULL(0x20)
#define KVX_SFR_CS_IN_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_CS_IN_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_CS_XIO_MASK _ULL(0x200) /* Extension IEEE 754 Invalid Operation */
#define KVX_SFR_CS_XIO_SHIFT 9
#define KVX_SFR_CS_XIO_WIDTH 1
#define KVX_SFR_CS_XIO_WFXL_MASK _ULL(0x200)
#define KVX_SFR_CS_XIO_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_CS_XIO_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_CS_XDZ_MASK _ULL(0x400) /* Extension IEEE 754 Divide by Zero */
#define KVX_SFR_CS_XDZ_SHIFT 10
#define KVX_SFR_CS_XDZ_WIDTH 1
#define KVX_SFR_CS_XDZ_WFXL_MASK _ULL(0x400)
#define KVX_SFR_CS_XDZ_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_CS_XDZ_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_CS_XOV_MASK _ULL(0x800) /* Extension IEEE 754 Overflow */
#define KVX_SFR_CS_XOV_SHIFT 11
#define KVX_SFR_CS_XOV_WIDTH 1
#define KVX_SFR_CS_XOV_WFXL_MASK _ULL(0x800)
#define KVX_SFR_CS_XOV_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_CS_XOV_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_CS_XUN_MASK _ULL(0x1000) /* Extension IEEE 754 Underflow */
#define KVX_SFR_CS_XUN_SHIFT 12
#define KVX_SFR_CS_XUN_WIDTH 1
#define KVX_SFR_CS_XUN_WFXL_MASK _ULL(0x1000)
#define KVX_SFR_CS_XUN_WFXL_CLEAR _ULL(0x1000)
#define KVX_SFR_CS_XUN_WFXL_SET _ULL(0x100000000000)

#define KVX_SFR_CS_XIN_MASK _ULL(0x2000) /* Extension IEEE 754 Inexact */
#define KVX_SFR_CS_XIN_SHIFT 13
#define KVX_SFR_CS_XIN_WIDTH 1
#define KVX_SFR_CS_XIN_WFXL_MASK _ULL(0x2000)
#define KVX_SFR_CS_XIN_WFXL_CLEAR _ULL(0x2000)
#define KVX_SFR_CS_XIN_WFXL_SET _ULL(0x200000000000)

#define KVX_SFR_CS_RM_MASK _ULL(0x30000) /* IEEE 754 Rounding Mode */
#define KVX_SFR_CS_RM_SHIFT 16
#define KVX_SFR_CS_RM_WIDTH 2
#define KVX_SFR_CS_RM_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_CS_RM_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_CS_RM_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_CS_XRM_MASK _ULL(0x300000) /* Extension IEEE 754 Rounding Mode */
#define KVX_SFR_CS_XRM_SHIFT 20
#define KVX_SFR_CS_XRM_WIDTH 2
#define KVX_SFR_CS_XRM_WFXL_MASK _ULL(0x300000)
#define KVX_SFR_CS_XRM_WFXL_CLEAR _ULL(0x300000)
#define KVX_SFR_CS_XRM_WFXL_SET _ULL(0x30000000000000)

#define KVX_SFR_CS_XMF_MASK _ULL(0x1000000) /* eXtension ModiFied */
#define KVX_SFR_CS_XMF_SHIFT 24
#define KVX_SFR_CS_XMF_WIDTH 1
#define KVX_SFR_CS_XMF_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_CS_XMF_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_CS_XMF_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_CS_CC_MASK _ULL(0xffff00000000) /* Carry Counter */
#define KVX_SFR_CS_CC_SHIFT 32
#define KVX_SFR_CS_CC_WIDTH 16
#define KVX_SFR_CS_CC_WFXM_MASK _ULL(0xffff00000000)
#define KVX_SFR_CS_CC_WFXM_CLEAR _ULL(0xffff)
#define KVX_SFR_CS_CC_WFXM_SET _ULL(0xffff00000000)

#define KVX_SFR_CS_XDROP_MASK _ULL(0x3f000000000000) /* Extension Conversion Drop Bits */
#define KVX_SFR_CS_XDROP_SHIFT 48
#define KVX_SFR_CS_XDROP_WIDTH 6
#define KVX_SFR_CS_XDROP_WFXM_MASK _ULL(0x3f000000000000)
#define KVX_SFR_CS_XDROP_WFXM_CLEAR _ULL(0x3f0000)
#define KVX_SFR_CS_XDROP_WFXM_SET _ULL(0x3f000000000000)

#define KVX_SFR_CS_XPOW2_MASK _ULL(0xfc0000000000000) /* Extension FScale Power of Two */
#define KVX_SFR_CS_XPOW2_SHIFT 54
#define KVX_SFR_CS_XPOW2_WIDTH 6
#define KVX_SFR_CS_XPOW2_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_CS_XPOW2_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_CS_XPOW2_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_AESPC_AESPC_MASK _ULL(0xffffffffffffffff) /* Arithmetic Exception Saved PC */
#define KVX_SFR_AESPC_AESPC_SHIFT 0
#define KVX_SFR_AESPC_AESPC_WIDTH 64
#define KVX_SFR_AESPC_AESPC_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_AESPC_AESPC_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_AESPC_AESPC_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_AESPC_AESPC_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_AESPC_AESPC_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_AESPC_AESPC_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_CSIT_ICIE_MASK _ULL(0x1) /* Integer Carry Interrupt Enable */
#define KVX_SFR_CSIT_ICIE_SHIFT 0
#define KVX_SFR_CSIT_ICIE_WIDTH 1
#define KVX_SFR_CSIT_ICIE_WFXL_MASK _ULL(0x1)
#define KVX_SFR_CSIT_ICIE_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_CSIT_ICIE_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_CSIT_IOIE_MASK _ULL(0x2) /* IEEE 754 Invalid Operation Interrupt Enable */
#define KVX_SFR_CSIT_IOIE_SHIFT 1
#define KVX_SFR_CSIT_IOIE_WIDTH 1
#define KVX_SFR_CSIT_IOIE_WFXL_MASK _ULL(0x2)
#define KVX_SFR_CSIT_IOIE_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_CSIT_IOIE_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_CSIT_DZIE_MASK _ULL(0x4) /* IEEE 754 Divide by Zero Interrupt Enable */
#define KVX_SFR_CSIT_DZIE_SHIFT 2
#define KVX_SFR_CSIT_DZIE_WIDTH 1
#define KVX_SFR_CSIT_DZIE_WFXL_MASK _ULL(0x4)
#define KVX_SFR_CSIT_DZIE_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_CSIT_DZIE_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_CSIT_OVIE_MASK _ULL(0x8) /* IEEE 754 Overflow Interrupt Enable */
#define KVX_SFR_CSIT_OVIE_SHIFT 3
#define KVX_SFR_CSIT_OVIE_WIDTH 1
#define KVX_SFR_CSIT_OVIE_WFXL_MASK _ULL(0x8)
#define KVX_SFR_CSIT_OVIE_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_CSIT_OVIE_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_CSIT_UNIE_MASK _ULL(0x10) /* IEEE 754 Underflow Interrupt Enable */
#define KVX_SFR_CSIT_UNIE_SHIFT 4
#define KVX_SFR_CSIT_UNIE_WIDTH 1
#define KVX_SFR_CSIT_UNIE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_CSIT_UNIE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_CSIT_UNIE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_CSIT_INIE_MASK _ULL(0x20) /* IEEE 754 Inexact Interrupt Enable */
#define KVX_SFR_CSIT_INIE_SHIFT 5
#define KVX_SFR_CSIT_INIE_WIDTH 1
#define KVX_SFR_CSIT_INIE_WFXL_MASK _ULL(0x20)
#define KVX_SFR_CSIT_INIE_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_CSIT_INIE_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_CSIT_XIOIE_MASK _ULL(0x200) /* Extension IEEE 754 Invalid Operation Interrupt Enable */
#define KVX_SFR_CSIT_XIOIE_SHIFT 9
#define KVX_SFR_CSIT_XIOIE_WIDTH 1
#define KVX_SFR_CSIT_XIOIE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_CSIT_XIOIE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_CSIT_XIOIE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_CSIT_XDZIE_MASK _ULL(0x400) /* Extension IEEE 754 Divide by Zero Interrupt Enable */
#define KVX_SFR_CSIT_XDZIE_SHIFT 10
#define KVX_SFR_CSIT_XDZIE_WIDTH 1
#define KVX_SFR_CSIT_XDZIE_WFXL_MASK _ULL(0x400)
#define KVX_SFR_CSIT_XDZIE_WFXL_CLEAR _ULL(0x400)
#define KVX_SFR_CSIT_XDZIE_WFXL_SET _ULL(0x40000000000)

#define KVX_SFR_CSIT_XOVIE_MASK _ULL(0x800) /* Extension IEEE 754 Overflow Interrupt Enable */
#define KVX_SFR_CSIT_XOVIE_SHIFT 11
#define KVX_SFR_CSIT_XOVIE_WIDTH 1
#define KVX_SFR_CSIT_XOVIE_WFXL_MASK _ULL(0x800)
#define KVX_SFR_CSIT_XOVIE_WFXL_CLEAR _ULL(0x800)
#define KVX_SFR_CSIT_XOVIE_WFXL_SET _ULL(0x80000000000)

#define KVX_SFR_CSIT_XUNIE_MASK _ULL(0x1000) /* Extension IEEE 754 Underflow Interrupt Enable */
#define KVX_SFR_CSIT_XUNIE_SHIFT 12
#define KVX_SFR_CSIT_XUNIE_WIDTH 1
#define KVX_SFR_CSIT_XUNIE_WFXL_MASK _ULL(0x1000)
#define KVX_SFR_CSIT_XUNIE_WFXL_CLEAR _ULL(0x1000)
#define KVX_SFR_CSIT_XUNIE_WFXL_SET _ULL(0x100000000000)

#define KVX_SFR_CSIT_XINIE_MASK _ULL(0x2000) /* Extension IEEE 754 Inexact Interrupt Enable */
#define KVX_SFR_CSIT_XINIE_SHIFT 13
#define KVX_SFR_CSIT_XINIE_WIDTH 1
#define KVX_SFR_CSIT_XINIE_WFXL_MASK _ULL(0x2000)
#define KVX_SFR_CSIT_XINIE_WFXL_CLEAR _ULL(0x2000)
#define KVX_SFR_CSIT_XINIE_WFXL_SET _ULL(0x200000000000)

#define KVX_SFR_CSIT_AEIR_MASK _ULL(0x10000) /* Arithmetic Exception Interrupt Raised */
#define KVX_SFR_CSIT_AEIR_SHIFT 16
#define KVX_SFR_CSIT_AEIR_WIDTH 1
#define KVX_SFR_CSIT_AEIR_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_CSIT_AEIR_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_CSIT_AEIR_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_CSIT_AEC_MASK _ULL(0xe0000) /* Arithmetic Exception Code */
#define KVX_SFR_CSIT_AEC_SHIFT 17
#define KVX_SFR_CSIT_AEC_WIDTH 3
#define KVX_SFR_CSIT_AEC_WFXL_MASK _ULL(0xe0000)
#define KVX_SFR_CSIT_AEC_WFXL_CLEAR _ULL(0xe0000)
#define KVX_SFR_CSIT_AEC_WFXL_SET _ULL(0xe000000000000)

#define KVX_SFR_CSIT_SPCV_MASK _ULL(0x100000) /* SPC Valid */
#define KVX_SFR_CSIT_SPCV_SHIFT 20
#define KVX_SFR_CSIT_SPCV_WIDTH 1
#define KVX_SFR_CSIT_SPCV_WFXL_MASK _ULL(0x100000)
#define KVX_SFR_CSIT_SPCV_WFXL_CLEAR _ULL(0x100000)
#define KVX_SFR_CSIT_SPCV_WFXL_SET _ULL(0x10000000000000)

#define KVX_SFR_ES_EC_MASK _ULL(0xf) /* Exception Class */
#define KVX_SFR_ES_EC_SHIFT 0
#define KVX_SFR_ES_EC_WIDTH 4
#define KVX_SFR_ES_EC_WFXL_MASK _ULL(0xf)
#define KVX_SFR_ES_EC_WFXL_CLEAR _ULL(0xf)
#define KVX_SFR_ES_EC_WFXL_SET _ULL(0xf00000000)

#define KVX_SFR_ES_ED_MASK _ULL(0xfffffffffffffff0) /* Exception Details */
#define KVX_SFR_ES_ED_SHIFT 4
#define KVX_SFR_ES_ED_WIDTH 60
#define KVX_SFR_ES_ED_WFXL_MASK _ULL(0xfffffff0)
#define KVX_SFR_ES_ED_WFXL_CLEAR _ULL(0xfffffff0)
#define KVX_SFR_ES_ED_WFXL_SET _ULL(0xfffffff000000000)
#define KVX_SFR_ES_ED_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_ES_ED_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_ES_ED_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_ES_OAPL_MASK _ULL(0x30) /* Origin Absolute PL */
#define KVX_SFR_ES_OAPL_SHIFT 4
#define KVX_SFR_ES_OAPL_WIDTH 2
#define KVX_SFR_ES_OAPL_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ES_OAPL_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ES_OAPL_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ES_ORPL_MASK _ULL(0xc0) /* Origin Relative PL */
#define KVX_SFR_ES_ORPL_SHIFT 6
#define KVX_SFR_ES_ORPL_WIDTH 2
#define KVX_SFR_ES_ORPL_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ES_ORPL_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ES_ORPL_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ES_PTAPL_MASK _ULL(0x300) /* Primary Target Absolute PL */
#define KVX_SFR_ES_PTAPL_SHIFT 8
#define KVX_SFR_ES_PTAPL_WIDTH 2
#define KVX_SFR_ES_PTAPL_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ES_PTAPL_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ES_PTAPL_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ES_PTRPL_MASK _ULL(0xc00) /* Primary Target Relative PL */
#define KVX_SFR_ES_PTRPL_SHIFT 10
#define KVX_SFR_ES_PTRPL_WIDTH 2
#define KVX_SFR_ES_PTRPL_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ES_PTRPL_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ES_PTRPL_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ES_ITN_MASK _ULL(0x1f000) /* InTerrupt Number */
#define KVX_SFR_ES_ITN_SHIFT 12
#define KVX_SFR_ES_ITN_WIDTH 5
#define KVX_SFR_ES_ITN_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_ITN_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_ITN_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_ITL_MASK _ULL(0x60000) /* InTerrupt Level */
#define KVX_SFR_ES_ITL_SHIFT 17
#define KVX_SFR_ES_ITL_WIDTH 2
#define KVX_SFR_ES_ITL_WFXL_MASK _ULL(0x60000)
#define KVX_SFR_ES_ITL_WFXL_CLEAR _ULL(0x60000)
#define KVX_SFR_ES_ITL_WFXL_SET _ULL(0x6000000000000)

#define KVX_SFR_ES_ITI_MASK _ULL(0x1ff80000) /* InTerrupt Info */
#define KVX_SFR_ES_ITI_SHIFT 19
#define KVX_SFR_ES_ITI_WIDTH 10
#define KVX_SFR_ES_ITI_WFXL_MASK _ULL(0x1ff80000)
#define KVX_SFR_ES_ITI_WFXL_CLEAR _ULL(0x1ff80000)
#define KVX_SFR_ES_ITI_WFXL_SET _ULL(0x1ff8000000000000)

#define KVX_SFR_ES_SN_MASK _ULL(0xfff000) /* Syscall Number */
#define KVX_SFR_ES_SN_SHIFT 12
#define KVX_SFR_ES_SN_WIDTH 12
#define KVX_SFR_ES_SN_WFXL_MASK _ULL(0xfff000)
#define KVX_SFR_ES_SN_WFXL_CLEAR _ULL(0xfff000)
#define KVX_SFR_ES_SN_WFXL_SET _ULL(0xfff00000000000)

#define KVX_SFR_ES_HTC_MASK _ULL(0x1f000) /* Hardware Trap Cause */
#define KVX_SFR_ES_HTC_SHIFT 12
#define KVX_SFR_ES_HTC_WIDTH 5
#define KVX_SFR_ES_HTC_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_HTC_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_HTC_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_SFRT_MASK _ULL(0x20000) /* SFR Trap */
#define KVX_SFR_ES_SFRT_SHIFT 17
#define KVX_SFR_ES_SFRT_WIDTH 1
#define KVX_SFR_ES_SFRT_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ES_SFRT_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ES_SFRT_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ES_SFRI_MASK _ULL(0x1c0000) /* SFR Instruction */
#define KVX_SFR_ES_SFRI_SHIFT 18
#define KVX_SFR_ES_SFRI_WIDTH 3
#define KVX_SFR_ES_SFRI_WFXL_MASK _ULL(0x1c0000)
#define KVX_SFR_ES_SFRI_WFXL_CLEAR _ULL(0x1c0000)
#define KVX_SFR_ES_SFRI_WFXL_SET _ULL(0x1c000000000000)

#define KVX_SFR_ES_GPRP_MASK _ULL(0x7e00000) /* GPR Pointer */
#define KVX_SFR_ES_GPRP_SHIFT 21
#define KVX_SFR_ES_GPRP_WIDTH 6
#define KVX_SFR_ES_GPRP_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_ES_GPRP_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_ES_GPRP_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_ES_SFRP_MASK _ULL(0xff8000000) /* SFR Pointer */
#define KVX_SFR_ES_SFRP_SHIFT 27
#define KVX_SFR_ES_SFRP_WIDTH 9
#define KVX_SFR_ES_SFRP_WFXL_MASK _ULL(0xf8000000)
#define KVX_SFR_ES_SFRP_WFXL_CLEAR _ULL(0xf8000000)
#define KVX_SFR_ES_SFRP_WFXL_SET _ULL(0xf800000000000000)
#define KVX_SFR_ES_SFRP_WFXM_MASK _ULL(0xf00000000)
#define KVX_SFR_ES_SFRP_WFXM_CLEAR _ULL(0xf)
#define KVX_SFR_ES_SFRP_WFXM_SET _ULL(0xf00000000)

#define KVX_SFR_ES_DHT_MASK _ULL(0x1000000000) /* Disabled Hardware Trap */
#define KVX_SFR_ES_DHT_SHIFT 36
#define KVX_SFR_ES_DHT_WIDTH 1
#define KVX_SFR_ES_DHT_WFXM_MASK _ULL(0x1000000000)
#define KVX_SFR_ES_DHT_WFXM_CLEAR _ULL(0x10)
#define KVX_SFR_ES_DHT_WFXM_SET _ULL(0x1000000000)

#define KVX_SFR_ES_RWX_MASK _ULL(0x38000000000) /* Read Write Execute */
#define KVX_SFR_ES_RWX_SHIFT 39
#define KVX_SFR_ES_RWX_WIDTH 3
#define KVX_SFR_ES_RWX_WFXM_MASK _ULL(0x38000000000)
#define KVX_SFR_ES_RWX_WFXM_CLEAR _ULL(0x380)
#define KVX_SFR_ES_RWX_WFXM_SET _ULL(0x38000000000)

#define KVX_SFR_ES_NTA_MASK _ULL(0x40000000000) /* Non-Trapping Access */
#define KVX_SFR_ES_NTA_SHIFT 42
#define KVX_SFR_ES_NTA_WIDTH 1
#define KVX_SFR_ES_NTA_WFXM_MASK _ULL(0x40000000000)
#define KVX_SFR_ES_NTA_WFXM_CLEAR _ULL(0x400)
#define KVX_SFR_ES_NTA_WFXM_SET _ULL(0x40000000000)

#define KVX_SFR_ES_UCA_MASK _ULL(0x80000000000) /* Un-Cached Access */
#define KVX_SFR_ES_UCA_SHIFT 43
#define KVX_SFR_ES_UCA_WIDTH 1
#define KVX_SFR_ES_UCA_WFXM_MASK _ULL(0x80000000000)
#define KVX_SFR_ES_UCA_WFXM_CLEAR _ULL(0x800)
#define KVX_SFR_ES_UCA_WFXM_SET _ULL(0x80000000000)

#define KVX_SFR_ES_AS_MASK _ULL(0x3f00000000000) /* Access Size */
#define KVX_SFR_ES_AS_SHIFT 44
#define KVX_SFR_ES_AS_WIDTH 6
#define KVX_SFR_ES_AS_WFXM_MASK _ULL(0x3f00000000000)
#define KVX_SFR_ES_AS_WFXM_CLEAR _ULL(0x3f000)
#define KVX_SFR_ES_AS_WFXM_SET _ULL(0x3f00000000000)

#define KVX_SFR_ES_BS_MASK _ULL(0x3c000000000000) /* Bundle Size */
#define KVX_SFR_ES_BS_SHIFT 50
#define KVX_SFR_ES_BS_WIDTH 4
#define KVX_SFR_ES_BS_WFXM_MASK _ULL(0x3c000000000000)
#define KVX_SFR_ES_BS_WFXM_CLEAR _ULL(0x3c0000)
#define KVX_SFR_ES_BS_WFXM_SET _ULL(0x3c000000000000)

#define KVX_SFR_ES_DRI_MASK _ULL(0xfc0000000000000) /* Data Register Index */
#define KVX_SFR_ES_DRI_SHIFT 54
#define KVX_SFR_ES_DRI_WIDTH 6
#define KVX_SFR_ES_DRI_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_ES_DRI_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_ES_DRI_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_ES_PIC_MASK _ULL(0xf000000000000000) /* Privileged Instruction Code */
#define KVX_SFR_ES_PIC_SHIFT 60
#define KVX_SFR_ES_PIC_WIDTH 4
#define KVX_SFR_ES_PIC_WFXM_MASK _ULL(0xf000000000000000)
#define KVX_SFR_ES_PIC_WFXM_CLEAR _ULL(0xf0000000)
#define KVX_SFR_ES_PIC_WFXM_SET _ULL(0xf000000000000000)

#define KVX_SFR_ES_DC_MASK _ULL(0x3000) /* Debug Cause */
#define KVX_SFR_ES_DC_SHIFT 12
#define KVX_SFR_ES_DC_WIDTH 2
#define KVX_SFR_ES_DC_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ES_DC_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ES_DC_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ES_BN_MASK _ULL(0x4000) /* Breakpoint Number */
#define KVX_SFR_ES_BN_SHIFT 14
#define KVX_SFR_ES_BN_WIDTH 1
#define KVX_SFR_ES_BN_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ES_BN_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ES_BN_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ES_WN_MASK _ULL(0x8000) /* Watchpoint Number */
#define KVX_SFR_ES_WN_SHIFT 15
#define KVX_SFR_ES_WN_WIDTH 1
#define KVX_SFR_ES_WN_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ES_WN_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ES_WN_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ES_PL0_EC_MASK _ULL(0xf) /* Exception Class */
#define KVX_SFR_ES_PL0_EC_SHIFT 0
#define KVX_SFR_ES_PL0_EC_WIDTH 4
#define KVX_SFR_ES_PL0_EC_WFXL_MASK _ULL(0xf)
#define KVX_SFR_ES_PL0_EC_WFXL_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL0_EC_WFXL_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL0_ED_MASK _ULL(0xfffffffffffffff0) /* Exception Details */
#define KVX_SFR_ES_PL0_ED_SHIFT 4
#define KVX_SFR_ES_PL0_ED_WIDTH 60
#define KVX_SFR_ES_PL0_ED_WFXL_MASK _ULL(0xfffffff0)
#define KVX_SFR_ES_PL0_ED_WFXL_CLEAR _ULL(0xfffffff0)
#define KVX_SFR_ES_PL0_ED_WFXL_SET _ULL(0xfffffff000000000)
#define KVX_SFR_ES_PL0_ED_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_ES_PL0_ED_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_ES_PL0_ED_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_ES_PL0_OAPL_MASK _ULL(0x30) /* Origin Absolute PL */
#define KVX_SFR_ES_PL0_OAPL_SHIFT 4
#define KVX_SFR_ES_PL0_OAPL_WIDTH 2
#define KVX_SFR_ES_PL0_OAPL_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ES_PL0_OAPL_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ES_PL0_OAPL_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ES_PL0_ORPL_MASK _ULL(0xc0) /* Origin Relative PL */
#define KVX_SFR_ES_PL0_ORPL_SHIFT 6
#define KVX_SFR_ES_PL0_ORPL_WIDTH 2
#define KVX_SFR_ES_PL0_ORPL_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ES_PL0_ORPL_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ES_PL0_ORPL_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ES_PL0_PTAPL_MASK _ULL(0x300) /* Target Absolute PL */
#define KVX_SFR_ES_PL0_PTAPL_SHIFT 8
#define KVX_SFR_ES_PL0_PTAPL_WIDTH 2
#define KVX_SFR_ES_PL0_PTAPL_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ES_PL0_PTAPL_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ES_PL0_PTAPL_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ES_PL0_PTRPL_MASK _ULL(0xc00) /* Target Relative PL */
#define KVX_SFR_ES_PL0_PTRPL_SHIFT 10
#define KVX_SFR_ES_PL0_PTRPL_WIDTH 2
#define KVX_SFR_ES_PL0_PTRPL_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ES_PL0_PTRPL_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ES_PL0_PTRPL_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ES_PL0_ITN_MASK _ULL(0x1f000) /* InTerrupt Number */
#define KVX_SFR_ES_PL0_ITN_SHIFT 12
#define KVX_SFR_ES_PL0_ITN_WIDTH 5
#define KVX_SFR_ES_PL0_ITN_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL0_ITN_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL0_ITN_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL0_ITL_MASK _ULL(0x60000) /* InTerrupt Level */
#define KVX_SFR_ES_PL0_ITL_SHIFT 17
#define KVX_SFR_ES_PL0_ITL_WIDTH 2
#define KVX_SFR_ES_PL0_ITL_WFXL_MASK _ULL(0x60000)
#define KVX_SFR_ES_PL0_ITL_WFXL_CLEAR _ULL(0x60000)
#define KVX_SFR_ES_PL0_ITL_WFXL_SET _ULL(0x6000000000000)

#define KVX_SFR_ES_PL0_ITI_MASK _ULL(0x1ff80000) /* InTerrupt Info */
#define KVX_SFR_ES_PL0_ITI_SHIFT 19
#define KVX_SFR_ES_PL0_ITI_WIDTH 10
#define KVX_SFR_ES_PL0_ITI_WFXL_MASK _ULL(0x1ff80000)
#define KVX_SFR_ES_PL0_ITI_WFXL_CLEAR _ULL(0x1ff80000)
#define KVX_SFR_ES_PL0_ITI_WFXL_SET _ULL(0x1ff8000000000000)

#define KVX_SFR_ES_PL0_SN_MASK _ULL(0xfff000) /* Syscall Number */
#define KVX_SFR_ES_PL0_SN_SHIFT 12
#define KVX_SFR_ES_PL0_SN_WIDTH 12
#define KVX_SFR_ES_PL0_SN_WFXL_MASK _ULL(0xfff000)
#define KVX_SFR_ES_PL0_SN_WFXL_CLEAR _ULL(0xfff000)
#define KVX_SFR_ES_PL0_SN_WFXL_SET _ULL(0xfff00000000000)

#define KVX_SFR_ES_PL0_HTC_MASK _ULL(0x1f000) /* Hardware Trap Cause */
#define KVX_SFR_ES_PL0_HTC_SHIFT 12
#define KVX_SFR_ES_PL0_HTC_WIDTH 5
#define KVX_SFR_ES_PL0_HTC_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL0_HTC_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL0_HTC_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL0_SFRT_MASK _ULL(0x20000) /* SFR Trap */
#define KVX_SFR_ES_PL0_SFRT_SHIFT 17
#define KVX_SFR_ES_PL0_SFRT_WIDTH 1
#define KVX_SFR_ES_PL0_SFRT_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ES_PL0_SFRT_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ES_PL0_SFRT_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ES_PL0_SFRI_MASK _ULL(0x1c0000) /* SFR Instruction */
#define KVX_SFR_ES_PL0_SFRI_SHIFT 18
#define KVX_SFR_ES_PL0_SFRI_WIDTH 3
#define KVX_SFR_ES_PL0_SFRI_WFXL_MASK _ULL(0x1c0000)
#define KVX_SFR_ES_PL0_SFRI_WFXL_CLEAR _ULL(0x1c0000)
#define KVX_SFR_ES_PL0_SFRI_WFXL_SET _ULL(0x1c000000000000)

#define KVX_SFR_ES_PL0_GPRP_MASK _ULL(0x7e00000) /* GPR Pointer */
#define KVX_SFR_ES_PL0_GPRP_SHIFT 21
#define KVX_SFR_ES_PL0_GPRP_WIDTH 6
#define KVX_SFR_ES_PL0_GPRP_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_ES_PL0_GPRP_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_ES_PL0_GPRP_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_ES_PL0_SFRP_MASK _ULL(0xff8000000) /* SFR Pointer */
#define KVX_SFR_ES_PL0_SFRP_SHIFT 27
#define KVX_SFR_ES_PL0_SFRP_WIDTH 9
#define KVX_SFR_ES_PL0_SFRP_WFXL_MASK _ULL(0xf8000000)
#define KVX_SFR_ES_PL0_SFRP_WFXL_CLEAR _ULL(0xf8000000)
#define KVX_SFR_ES_PL0_SFRP_WFXL_SET _ULL(0xf800000000000000)
#define KVX_SFR_ES_PL0_SFRP_WFXM_MASK _ULL(0xf00000000)
#define KVX_SFR_ES_PL0_SFRP_WFXM_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL0_SFRP_WFXM_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL0_DHT_MASK _ULL(0x1000000000) /* Disabled Hardware Trap */
#define KVX_SFR_ES_PL0_DHT_SHIFT 36
#define KVX_SFR_ES_PL0_DHT_WIDTH 1
#define KVX_SFR_ES_PL0_DHT_WFXM_MASK _ULL(0x1000000000)
#define KVX_SFR_ES_PL0_DHT_WFXM_CLEAR _ULL(0x10)
#define KVX_SFR_ES_PL0_DHT_WFXM_SET _ULL(0x1000000000)

#define KVX_SFR_ES_PL0_RWX_MASK _ULL(0x38000000000) /* Read Write Execute */
#define KVX_SFR_ES_PL0_RWX_SHIFT 39
#define KVX_SFR_ES_PL0_RWX_WIDTH 3
#define KVX_SFR_ES_PL0_RWX_WFXM_MASK _ULL(0x38000000000)
#define KVX_SFR_ES_PL0_RWX_WFXM_CLEAR _ULL(0x380)
#define KVX_SFR_ES_PL0_RWX_WFXM_SET _ULL(0x38000000000)

#define KVX_SFR_ES_PL0_NTA_MASK _ULL(0x40000000000) /* Non-Trapping Access */
#define KVX_SFR_ES_PL0_NTA_SHIFT 42
#define KVX_SFR_ES_PL0_NTA_WIDTH 1
#define KVX_SFR_ES_PL0_NTA_WFXM_MASK _ULL(0x40000000000)
#define KVX_SFR_ES_PL0_NTA_WFXM_CLEAR _ULL(0x400)
#define KVX_SFR_ES_PL0_NTA_WFXM_SET _ULL(0x40000000000)

#define KVX_SFR_ES_PL0_UCA_MASK _ULL(0x80000000000) /* Un-Cached Access */
#define KVX_SFR_ES_PL0_UCA_SHIFT 43
#define KVX_SFR_ES_PL0_UCA_WIDTH 1
#define KVX_SFR_ES_PL0_UCA_WFXM_MASK _ULL(0x80000000000)
#define KVX_SFR_ES_PL0_UCA_WFXM_CLEAR _ULL(0x800)
#define KVX_SFR_ES_PL0_UCA_WFXM_SET _ULL(0x80000000000)

#define KVX_SFR_ES_PL0_AS_MASK _ULL(0x3f00000000000) /* Access Size */
#define KVX_SFR_ES_PL0_AS_SHIFT 44
#define KVX_SFR_ES_PL0_AS_WIDTH 6
#define KVX_SFR_ES_PL0_AS_WFXM_MASK _ULL(0x3f00000000000)
#define KVX_SFR_ES_PL0_AS_WFXM_CLEAR _ULL(0x3f000)
#define KVX_SFR_ES_PL0_AS_WFXM_SET _ULL(0x3f00000000000)

#define KVX_SFR_ES_PL0_BS_MASK _ULL(0x3c000000000000) /* Bundle Size */
#define KVX_SFR_ES_PL0_BS_SHIFT 50
#define KVX_SFR_ES_PL0_BS_WIDTH 4
#define KVX_SFR_ES_PL0_BS_WFXM_MASK _ULL(0x3c000000000000)
#define KVX_SFR_ES_PL0_BS_WFXM_CLEAR _ULL(0x3c0000)
#define KVX_SFR_ES_PL0_BS_WFXM_SET _ULL(0x3c000000000000)

#define KVX_SFR_ES_PL0_DRI_MASK _ULL(0xfc0000000000000) /* Data Register Index */
#define KVX_SFR_ES_PL0_DRI_SHIFT 54
#define KVX_SFR_ES_PL0_DRI_WIDTH 6
#define KVX_SFR_ES_PL0_DRI_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_ES_PL0_DRI_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_ES_PL0_DRI_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_ES_PL0_PIC_MASK _ULL(0xf000000000000000) /* Privileged Instruction Code */
#define KVX_SFR_ES_PL0_PIC_SHIFT 60
#define KVX_SFR_ES_PL0_PIC_WIDTH 4
#define KVX_SFR_ES_PL0_PIC_WFXM_MASK _ULL(0xf000000000000000)
#define KVX_SFR_ES_PL0_PIC_WFXM_CLEAR _ULL(0xf0000000)
#define KVX_SFR_ES_PL0_PIC_WFXM_SET _ULL(0xf000000000000000)

#define KVX_SFR_ES_PL0_DC_MASK _ULL(0x3000) /* Debug Cause */
#define KVX_SFR_ES_PL0_DC_SHIFT 12
#define KVX_SFR_ES_PL0_DC_WIDTH 2
#define KVX_SFR_ES_PL0_DC_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ES_PL0_DC_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ES_PL0_DC_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ES_PL0_BN_MASK _ULL(0x4000) /* Breakpoint Number */
#define KVX_SFR_ES_PL0_BN_SHIFT 14
#define KVX_SFR_ES_PL0_BN_WIDTH 1
#define KVX_SFR_ES_PL0_BN_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ES_PL0_BN_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ES_PL0_BN_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ES_PL0_WN_MASK _ULL(0x8000) /* Watchpoint Number */
#define KVX_SFR_ES_PL0_WN_SHIFT 15
#define KVX_SFR_ES_PL0_WN_WIDTH 1
#define KVX_SFR_ES_PL0_WN_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ES_PL0_WN_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ES_PL0_WN_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ES_PL1_EC_MASK _ULL(0xf) /* Exception Class */
#define KVX_SFR_ES_PL1_EC_SHIFT 0
#define KVX_SFR_ES_PL1_EC_WIDTH 4
#define KVX_SFR_ES_PL1_EC_WFXL_MASK _ULL(0xf)
#define KVX_SFR_ES_PL1_EC_WFXL_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL1_EC_WFXL_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL1_ED_MASK _ULL(0xfffffffffffffff0) /* Exception Details */
#define KVX_SFR_ES_PL1_ED_SHIFT 4
#define KVX_SFR_ES_PL1_ED_WIDTH 60
#define KVX_SFR_ES_PL1_ED_WFXL_MASK _ULL(0xfffffff0)
#define KVX_SFR_ES_PL1_ED_WFXL_CLEAR _ULL(0xfffffff0)
#define KVX_SFR_ES_PL1_ED_WFXL_SET _ULL(0xfffffff000000000)
#define KVX_SFR_ES_PL1_ED_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_ES_PL1_ED_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_ES_PL1_ED_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_ES_PL1_OAPL_MASK _ULL(0x30) /* Origin Absolute PL */
#define KVX_SFR_ES_PL1_OAPL_SHIFT 4
#define KVX_SFR_ES_PL1_OAPL_WIDTH 2
#define KVX_SFR_ES_PL1_OAPL_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ES_PL1_OAPL_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ES_PL1_OAPL_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ES_PL1_ORPL_MASK _ULL(0xc0) /* Origin Relative PL */
#define KVX_SFR_ES_PL1_ORPL_SHIFT 6
#define KVX_SFR_ES_PL1_ORPL_WIDTH 2
#define KVX_SFR_ES_PL1_ORPL_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ES_PL1_ORPL_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ES_PL1_ORPL_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ES_PL1_PTAPL_MASK _ULL(0x300) /* Target Absolute PL */
#define KVX_SFR_ES_PL1_PTAPL_SHIFT 8
#define KVX_SFR_ES_PL1_PTAPL_WIDTH 2
#define KVX_SFR_ES_PL1_PTAPL_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ES_PL1_PTAPL_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ES_PL1_PTAPL_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ES_PL1_PTRPL_MASK _ULL(0xc00) /* Target Relative PL */
#define KVX_SFR_ES_PL1_PTRPL_SHIFT 10
#define KVX_SFR_ES_PL1_PTRPL_WIDTH 2
#define KVX_SFR_ES_PL1_PTRPL_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ES_PL1_PTRPL_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ES_PL1_PTRPL_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ES_PL1_ITN_MASK _ULL(0x1f000) /* InTerrupt Number */
#define KVX_SFR_ES_PL1_ITN_SHIFT 12
#define KVX_SFR_ES_PL1_ITN_WIDTH 5
#define KVX_SFR_ES_PL1_ITN_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL1_ITN_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL1_ITN_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL1_ITL_MASK _ULL(0x60000) /* InTerrupt Level */
#define KVX_SFR_ES_PL1_ITL_SHIFT 17
#define KVX_SFR_ES_PL1_ITL_WIDTH 2
#define KVX_SFR_ES_PL1_ITL_WFXL_MASK _ULL(0x60000)
#define KVX_SFR_ES_PL1_ITL_WFXL_CLEAR _ULL(0x60000)
#define KVX_SFR_ES_PL1_ITL_WFXL_SET _ULL(0x6000000000000)

#define KVX_SFR_ES_PL1_ITI_MASK _ULL(0x1ff80000) /* InTerrupt Info */
#define KVX_SFR_ES_PL1_ITI_SHIFT 19
#define KVX_SFR_ES_PL1_ITI_WIDTH 10
#define KVX_SFR_ES_PL1_ITI_WFXL_MASK _ULL(0x1ff80000)
#define KVX_SFR_ES_PL1_ITI_WFXL_CLEAR _ULL(0x1ff80000)
#define KVX_SFR_ES_PL1_ITI_WFXL_SET _ULL(0x1ff8000000000000)

#define KVX_SFR_ES_PL1_SN_MASK _ULL(0xfff000) /* Syscall Number */
#define KVX_SFR_ES_PL1_SN_SHIFT 12
#define KVX_SFR_ES_PL1_SN_WIDTH 12
#define KVX_SFR_ES_PL1_SN_WFXL_MASK _ULL(0xfff000)
#define KVX_SFR_ES_PL1_SN_WFXL_CLEAR _ULL(0xfff000)
#define KVX_SFR_ES_PL1_SN_WFXL_SET _ULL(0xfff00000000000)

#define KVX_SFR_ES_PL1_HTC_MASK _ULL(0x1f000) /* Hardware Trap Cause */
#define KVX_SFR_ES_PL1_HTC_SHIFT 12
#define KVX_SFR_ES_PL1_HTC_WIDTH 5
#define KVX_SFR_ES_PL1_HTC_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL1_HTC_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL1_HTC_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL1_SFRT_MASK _ULL(0x20000) /* SFR Trap */
#define KVX_SFR_ES_PL1_SFRT_SHIFT 17
#define KVX_SFR_ES_PL1_SFRT_WIDTH 1
#define KVX_SFR_ES_PL1_SFRT_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ES_PL1_SFRT_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ES_PL1_SFRT_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ES_PL1_SFRI_MASK _ULL(0x1c0000) /* SFR Instruction */
#define KVX_SFR_ES_PL1_SFRI_SHIFT 18
#define KVX_SFR_ES_PL1_SFRI_WIDTH 3
#define KVX_SFR_ES_PL1_SFRI_WFXL_MASK _ULL(0x1c0000)
#define KVX_SFR_ES_PL1_SFRI_WFXL_CLEAR _ULL(0x1c0000)
#define KVX_SFR_ES_PL1_SFRI_WFXL_SET _ULL(0x1c000000000000)

#define KVX_SFR_ES_PL1_GPRP_MASK _ULL(0x7e00000) /* GPR Pointer */
#define KVX_SFR_ES_PL1_GPRP_SHIFT 21
#define KVX_SFR_ES_PL1_GPRP_WIDTH 6
#define KVX_SFR_ES_PL1_GPRP_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_ES_PL1_GPRP_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_ES_PL1_GPRP_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_ES_PL1_SFRP_MASK _ULL(0xff8000000) /* SFR Pointer */
#define KVX_SFR_ES_PL1_SFRP_SHIFT 27
#define KVX_SFR_ES_PL1_SFRP_WIDTH 9
#define KVX_SFR_ES_PL1_SFRP_WFXL_MASK _ULL(0xf8000000)
#define KVX_SFR_ES_PL1_SFRP_WFXL_CLEAR _ULL(0xf8000000)
#define KVX_SFR_ES_PL1_SFRP_WFXL_SET _ULL(0xf800000000000000)
#define KVX_SFR_ES_PL1_SFRP_WFXM_MASK _ULL(0xf00000000)
#define KVX_SFR_ES_PL1_SFRP_WFXM_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL1_SFRP_WFXM_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL1_DHT_MASK _ULL(0x1000000000) /* Disabled Hardware Trap */
#define KVX_SFR_ES_PL1_DHT_SHIFT 36
#define KVX_SFR_ES_PL1_DHT_WIDTH 1
#define KVX_SFR_ES_PL1_DHT_WFXM_MASK _ULL(0x1000000000)
#define KVX_SFR_ES_PL1_DHT_WFXM_CLEAR _ULL(0x10)
#define KVX_SFR_ES_PL1_DHT_WFXM_SET _ULL(0x1000000000)

#define KVX_SFR_ES_PL1_RWX_MASK _ULL(0x38000000000) /* Read Write Execute */
#define KVX_SFR_ES_PL1_RWX_SHIFT 39
#define KVX_SFR_ES_PL1_RWX_WIDTH 3
#define KVX_SFR_ES_PL1_RWX_WFXM_MASK _ULL(0x38000000000)
#define KVX_SFR_ES_PL1_RWX_WFXM_CLEAR _ULL(0x380)
#define KVX_SFR_ES_PL1_RWX_WFXM_SET _ULL(0x38000000000)

#define KVX_SFR_ES_PL1_NTA_MASK _ULL(0x40000000000) /* Non-Trapping Access */
#define KVX_SFR_ES_PL1_NTA_SHIFT 42
#define KVX_SFR_ES_PL1_NTA_WIDTH 1
#define KVX_SFR_ES_PL1_NTA_WFXM_MASK _ULL(0x40000000000)
#define KVX_SFR_ES_PL1_NTA_WFXM_CLEAR _ULL(0x400)
#define KVX_SFR_ES_PL1_NTA_WFXM_SET _ULL(0x40000000000)

#define KVX_SFR_ES_PL1_UCA_MASK _ULL(0x80000000000) /* Un-Cached Access */
#define KVX_SFR_ES_PL1_UCA_SHIFT 43
#define KVX_SFR_ES_PL1_UCA_WIDTH 1
#define KVX_SFR_ES_PL1_UCA_WFXM_MASK _ULL(0x80000000000)
#define KVX_SFR_ES_PL1_UCA_WFXM_CLEAR _ULL(0x800)
#define KVX_SFR_ES_PL1_UCA_WFXM_SET _ULL(0x80000000000)

#define KVX_SFR_ES_PL1_AS_MASK _ULL(0x3f00000000000) /* Access Size */
#define KVX_SFR_ES_PL1_AS_SHIFT 44
#define KVX_SFR_ES_PL1_AS_WIDTH 6
#define KVX_SFR_ES_PL1_AS_WFXM_MASK _ULL(0x3f00000000000)
#define KVX_SFR_ES_PL1_AS_WFXM_CLEAR _ULL(0x3f000)
#define KVX_SFR_ES_PL1_AS_WFXM_SET _ULL(0x3f00000000000)

#define KVX_SFR_ES_PL1_BS_MASK _ULL(0x3c000000000000) /* Bundle Size */
#define KVX_SFR_ES_PL1_BS_SHIFT 50
#define KVX_SFR_ES_PL1_BS_WIDTH 4
#define KVX_SFR_ES_PL1_BS_WFXM_MASK _ULL(0x3c000000000000)
#define KVX_SFR_ES_PL1_BS_WFXM_CLEAR _ULL(0x3c0000)
#define KVX_SFR_ES_PL1_BS_WFXM_SET _ULL(0x3c000000000000)

#define KVX_SFR_ES_PL1_DRI_MASK _ULL(0xfc0000000000000) /* Data Register Index */
#define KVX_SFR_ES_PL1_DRI_SHIFT 54
#define KVX_SFR_ES_PL1_DRI_WIDTH 6
#define KVX_SFR_ES_PL1_DRI_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_ES_PL1_DRI_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_ES_PL1_DRI_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_ES_PL1_PIC_MASK _ULL(0xf000000000000000) /* Privileged Instruction Code */
#define KVX_SFR_ES_PL1_PIC_SHIFT 60
#define KVX_SFR_ES_PL1_PIC_WIDTH 4
#define KVX_SFR_ES_PL1_PIC_WFXM_MASK _ULL(0xf000000000000000)
#define KVX_SFR_ES_PL1_PIC_WFXM_CLEAR _ULL(0xf0000000)
#define KVX_SFR_ES_PL1_PIC_WFXM_SET _ULL(0xf000000000000000)

#define KVX_SFR_ES_PL1_DC_MASK _ULL(0x3000) /* Debug Cause */
#define KVX_SFR_ES_PL1_DC_SHIFT 12
#define KVX_SFR_ES_PL1_DC_WIDTH 2
#define KVX_SFR_ES_PL1_DC_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ES_PL1_DC_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ES_PL1_DC_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ES_PL1_BN_MASK _ULL(0x4000) /* Breakpoint Number */
#define KVX_SFR_ES_PL1_BN_SHIFT 14
#define KVX_SFR_ES_PL1_BN_WIDTH 1
#define KVX_SFR_ES_PL1_BN_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ES_PL1_BN_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ES_PL1_BN_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ES_PL1_WN_MASK _ULL(0x8000) /* Watchpoint Number */
#define KVX_SFR_ES_PL1_WN_SHIFT 15
#define KVX_SFR_ES_PL1_WN_WIDTH 1
#define KVX_SFR_ES_PL1_WN_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ES_PL1_WN_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ES_PL1_WN_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ES_PL2_EC_MASK _ULL(0xf) /* Exception Class */
#define KVX_SFR_ES_PL2_EC_SHIFT 0
#define KVX_SFR_ES_PL2_EC_WIDTH 4
#define KVX_SFR_ES_PL2_EC_WFXL_MASK _ULL(0xf)
#define KVX_SFR_ES_PL2_EC_WFXL_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL2_EC_WFXL_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL2_ED_MASK _ULL(0xfffffffffffffff0) /* Exception Details */
#define KVX_SFR_ES_PL2_ED_SHIFT 4
#define KVX_SFR_ES_PL2_ED_WIDTH 60
#define KVX_SFR_ES_PL2_ED_WFXL_MASK _ULL(0xfffffff0)
#define KVX_SFR_ES_PL2_ED_WFXL_CLEAR _ULL(0xfffffff0)
#define KVX_SFR_ES_PL2_ED_WFXL_SET _ULL(0xfffffff000000000)
#define KVX_SFR_ES_PL2_ED_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_ES_PL2_ED_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_ES_PL2_ED_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_ES_PL2_OAPL_MASK _ULL(0x30) /* Origin Absolute PL */
#define KVX_SFR_ES_PL2_OAPL_SHIFT 4
#define KVX_SFR_ES_PL2_OAPL_WIDTH 2
#define KVX_SFR_ES_PL2_OAPL_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ES_PL2_OAPL_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ES_PL2_OAPL_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ES_PL2_ORPL_MASK _ULL(0xc0) /* Origin Relative PL */
#define KVX_SFR_ES_PL2_ORPL_SHIFT 6
#define KVX_SFR_ES_PL2_ORPL_WIDTH 2
#define KVX_SFR_ES_PL2_ORPL_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ES_PL2_ORPL_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ES_PL2_ORPL_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ES_PL2_PTAPL_MASK _ULL(0x300) /* Target Absolute PL */
#define KVX_SFR_ES_PL2_PTAPL_SHIFT 8
#define KVX_SFR_ES_PL2_PTAPL_WIDTH 2
#define KVX_SFR_ES_PL2_PTAPL_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ES_PL2_PTAPL_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ES_PL2_PTAPL_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ES_PL2_PTRPL_MASK _ULL(0xc00) /* Target Relative PL */
#define KVX_SFR_ES_PL2_PTRPL_SHIFT 10
#define KVX_SFR_ES_PL2_PTRPL_WIDTH 2
#define KVX_SFR_ES_PL2_PTRPL_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ES_PL2_PTRPL_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ES_PL2_PTRPL_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ES_PL2_ITN_MASK _ULL(0x1f000) /* InTerrupt Number */
#define KVX_SFR_ES_PL2_ITN_SHIFT 12
#define KVX_SFR_ES_PL2_ITN_WIDTH 5
#define KVX_SFR_ES_PL2_ITN_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL2_ITN_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL2_ITN_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL2_ITL_MASK _ULL(0x60000) /* InTerrupt Level */
#define KVX_SFR_ES_PL2_ITL_SHIFT 17
#define KVX_SFR_ES_PL2_ITL_WIDTH 2
#define KVX_SFR_ES_PL2_ITL_WFXL_MASK _ULL(0x60000)
#define KVX_SFR_ES_PL2_ITL_WFXL_CLEAR _ULL(0x60000)
#define KVX_SFR_ES_PL2_ITL_WFXL_SET _ULL(0x6000000000000)

#define KVX_SFR_ES_PL2_ITI_MASK _ULL(0x1ff80000) /* InTerrupt Info */
#define KVX_SFR_ES_PL2_ITI_SHIFT 19
#define KVX_SFR_ES_PL2_ITI_WIDTH 10
#define KVX_SFR_ES_PL2_ITI_WFXL_MASK _ULL(0x1ff80000)
#define KVX_SFR_ES_PL2_ITI_WFXL_CLEAR _ULL(0x1ff80000)
#define KVX_SFR_ES_PL2_ITI_WFXL_SET _ULL(0x1ff8000000000000)

#define KVX_SFR_ES_PL2_SN_MASK _ULL(0xfff000) /* Syscall Number */
#define KVX_SFR_ES_PL2_SN_SHIFT 12
#define KVX_SFR_ES_PL2_SN_WIDTH 12
#define KVX_SFR_ES_PL2_SN_WFXL_MASK _ULL(0xfff000)
#define KVX_SFR_ES_PL2_SN_WFXL_CLEAR _ULL(0xfff000)
#define KVX_SFR_ES_PL2_SN_WFXL_SET _ULL(0xfff00000000000)

#define KVX_SFR_ES_PL2_HTC_MASK _ULL(0x1f000) /* Hardware Trap Cause */
#define KVX_SFR_ES_PL2_HTC_SHIFT 12
#define KVX_SFR_ES_PL2_HTC_WIDTH 5
#define KVX_SFR_ES_PL2_HTC_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL2_HTC_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL2_HTC_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL2_SFRT_MASK _ULL(0x20000) /* SFR Trap */
#define KVX_SFR_ES_PL2_SFRT_SHIFT 17
#define KVX_SFR_ES_PL2_SFRT_WIDTH 1
#define KVX_SFR_ES_PL2_SFRT_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ES_PL2_SFRT_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ES_PL2_SFRT_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ES_PL2_SFRI_MASK _ULL(0x1c0000) /* SFR Instruction */
#define KVX_SFR_ES_PL2_SFRI_SHIFT 18
#define KVX_SFR_ES_PL2_SFRI_WIDTH 3
#define KVX_SFR_ES_PL2_SFRI_WFXL_MASK _ULL(0x1c0000)
#define KVX_SFR_ES_PL2_SFRI_WFXL_CLEAR _ULL(0x1c0000)
#define KVX_SFR_ES_PL2_SFRI_WFXL_SET _ULL(0x1c000000000000)

#define KVX_SFR_ES_PL2_GPRP_MASK _ULL(0x7e00000) /* GPR Pointer */
#define KVX_SFR_ES_PL2_GPRP_SHIFT 21
#define KVX_SFR_ES_PL2_GPRP_WIDTH 6
#define KVX_SFR_ES_PL2_GPRP_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_ES_PL2_GPRP_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_ES_PL2_GPRP_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_ES_PL2_SFRP_MASK _ULL(0xff8000000) /* SFR Pointer */
#define KVX_SFR_ES_PL2_SFRP_SHIFT 27
#define KVX_SFR_ES_PL2_SFRP_WIDTH 9
#define KVX_SFR_ES_PL2_SFRP_WFXL_MASK _ULL(0xf8000000)
#define KVX_SFR_ES_PL2_SFRP_WFXL_CLEAR _ULL(0xf8000000)
#define KVX_SFR_ES_PL2_SFRP_WFXL_SET _ULL(0xf800000000000000)
#define KVX_SFR_ES_PL2_SFRP_WFXM_MASK _ULL(0xf00000000)
#define KVX_SFR_ES_PL2_SFRP_WFXM_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL2_SFRP_WFXM_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL2_DHT_MASK _ULL(0x1000000000) /* Disabled Hardware Trap */
#define KVX_SFR_ES_PL2_DHT_SHIFT 36
#define KVX_SFR_ES_PL2_DHT_WIDTH 1
#define KVX_SFR_ES_PL2_DHT_WFXM_MASK _ULL(0x1000000000)
#define KVX_SFR_ES_PL2_DHT_WFXM_CLEAR _ULL(0x10)
#define KVX_SFR_ES_PL2_DHT_WFXM_SET _ULL(0x1000000000)

#define KVX_SFR_ES_PL2_RWX_MASK _ULL(0x38000000000) /* Read Write Execute */
#define KVX_SFR_ES_PL2_RWX_SHIFT 39
#define KVX_SFR_ES_PL2_RWX_WIDTH 3
#define KVX_SFR_ES_PL2_RWX_WFXM_MASK _ULL(0x38000000000)
#define KVX_SFR_ES_PL2_RWX_WFXM_CLEAR _ULL(0x380)
#define KVX_SFR_ES_PL2_RWX_WFXM_SET _ULL(0x38000000000)

#define KVX_SFR_ES_PL2_NTA_MASK _ULL(0x40000000000) /* Non-Trapping Access */
#define KVX_SFR_ES_PL2_NTA_SHIFT 42
#define KVX_SFR_ES_PL2_NTA_WIDTH 1
#define KVX_SFR_ES_PL2_NTA_WFXM_MASK _ULL(0x40000000000)
#define KVX_SFR_ES_PL2_NTA_WFXM_CLEAR _ULL(0x400)
#define KVX_SFR_ES_PL2_NTA_WFXM_SET _ULL(0x40000000000)

#define KVX_SFR_ES_PL2_UCA_MASK _ULL(0x80000000000) /* Un-Cached Access */
#define KVX_SFR_ES_PL2_UCA_SHIFT 43
#define KVX_SFR_ES_PL2_UCA_WIDTH 1
#define KVX_SFR_ES_PL2_UCA_WFXM_MASK _ULL(0x80000000000)
#define KVX_SFR_ES_PL2_UCA_WFXM_CLEAR _ULL(0x800)
#define KVX_SFR_ES_PL2_UCA_WFXM_SET _ULL(0x80000000000)

#define KVX_SFR_ES_PL2_AS_MASK _ULL(0x3f00000000000) /* Access Size */
#define KVX_SFR_ES_PL2_AS_SHIFT 44
#define KVX_SFR_ES_PL2_AS_WIDTH 6
#define KVX_SFR_ES_PL2_AS_WFXM_MASK _ULL(0x3f00000000000)
#define KVX_SFR_ES_PL2_AS_WFXM_CLEAR _ULL(0x3f000)
#define KVX_SFR_ES_PL2_AS_WFXM_SET _ULL(0x3f00000000000)

#define KVX_SFR_ES_PL2_BS_MASK _ULL(0x3c000000000000) /* Bundle Size */
#define KVX_SFR_ES_PL2_BS_SHIFT 50
#define KVX_SFR_ES_PL2_BS_WIDTH 4
#define KVX_SFR_ES_PL2_BS_WFXM_MASK _ULL(0x3c000000000000)
#define KVX_SFR_ES_PL2_BS_WFXM_CLEAR _ULL(0x3c0000)
#define KVX_SFR_ES_PL2_BS_WFXM_SET _ULL(0x3c000000000000)

#define KVX_SFR_ES_PL2_DRI_MASK _ULL(0xfc0000000000000) /* Data Register Index */
#define KVX_SFR_ES_PL2_DRI_SHIFT 54
#define KVX_SFR_ES_PL2_DRI_WIDTH 6
#define KVX_SFR_ES_PL2_DRI_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_ES_PL2_DRI_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_ES_PL2_DRI_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_ES_PL2_PIC_MASK _ULL(0xf000000000000000) /* Privileged Instruction Code */
#define KVX_SFR_ES_PL2_PIC_SHIFT 60
#define KVX_SFR_ES_PL2_PIC_WIDTH 4
#define KVX_SFR_ES_PL2_PIC_WFXM_MASK _ULL(0xf000000000000000)
#define KVX_SFR_ES_PL2_PIC_WFXM_CLEAR _ULL(0xf0000000)
#define KVX_SFR_ES_PL2_PIC_WFXM_SET _ULL(0xf000000000000000)

#define KVX_SFR_ES_PL2_DC_MASK _ULL(0x3000) /* Debug Cause */
#define KVX_SFR_ES_PL2_DC_SHIFT 12
#define KVX_SFR_ES_PL2_DC_WIDTH 2
#define KVX_SFR_ES_PL2_DC_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ES_PL2_DC_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ES_PL2_DC_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ES_PL2_BN_MASK _ULL(0x4000) /* Breakpoint Number */
#define KVX_SFR_ES_PL2_BN_SHIFT 14
#define KVX_SFR_ES_PL2_BN_WIDTH 1
#define KVX_SFR_ES_PL2_BN_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ES_PL2_BN_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ES_PL2_BN_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ES_PL2_WN_MASK _ULL(0x8000) /* Watchpoint Number */
#define KVX_SFR_ES_PL2_WN_SHIFT 15
#define KVX_SFR_ES_PL2_WN_WIDTH 1
#define KVX_SFR_ES_PL2_WN_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ES_PL2_WN_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ES_PL2_WN_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_ES_PL3_EC_MASK _ULL(0xf) /* Exception Class */
#define KVX_SFR_ES_PL3_EC_SHIFT 0
#define KVX_SFR_ES_PL3_EC_WIDTH 4
#define KVX_SFR_ES_PL3_EC_WFXL_MASK _ULL(0xf)
#define KVX_SFR_ES_PL3_EC_WFXL_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL3_EC_WFXL_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL3_ED_MASK _ULL(0xfffffffffffffff0) /* Exception Details */
#define KVX_SFR_ES_PL3_ED_SHIFT 4
#define KVX_SFR_ES_PL3_ED_WIDTH 60
#define KVX_SFR_ES_PL3_ED_WFXL_MASK _ULL(0xfffffff0)
#define KVX_SFR_ES_PL3_ED_WFXL_CLEAR _ULL(0xfffffff0)
#define KVX_SFR_ES_PL3_ED_WFXL_SET _ULL(0xfffffff000000000)
#define KVX_SFR_ES_PL3_ED_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_ES_PL3_ED_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_ES_PL3_ED_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_ES_PL3_OAPL_MASK _ULL(0x30) /* Origin Absolute PL */
#define KVX_SFR_ES_PL3_OAPL_SHIFT 4
#define KVX_SFR_ES_PL3_OAPL_WIDTH 2
#define KVX_SFR_ES_PL3_OAPL_WFXL_MASK _ULL(0x30)
#define KVX_SFR_ES_PL3_OAPL_WFXL_CLEAR _ULL(0x30)
#define KVX_SFR_ES_PL3_OAPL_WFXL_SET _ULL(0x3000000000)

#define KVX_SFR_ES_PL3_ORPL_MASK _ULL(0xc0) /* Origin Relative PL */
#define KVX_SFR_ES_PL3_ORPL_SHIFT 6
#define KVX_SFR_ES_PL3_ORPL_WIDTH 2
#define KVX_SFR_ES_PL3_ORPL_WFXL_MASK _ULL(0xc0)
#define KVX_SFR_ES_PL3_ORPL_WFXL_CLEAR _ULL(0xc0)
#define KVX_SFR_ES_PL3_ORPL_WFXL_SET _ULL(0xc000000000)

#define KVX_SFR_ES_PL3_PTAPL_MASK _ULL(0x300) /* Target Absolute PL */
#define KVX_SFR_ES_PL3_PTAPL_SHIFT 8
#define KVX_SFR_ES_PL3_PTAPL_WIDTH 2
#define KVX_SFR_ES_PL3_PTAPL_WFXL_MASK _ULL(0x300)
#define KVX_SFR_ES_PL3_PTAPL_WFXL_CLEAR _ULL(0x300)
#define KVX_SFR_ES_PL3_PTAPL_WFXL_SET _ULL(0x30000000000)

#define KVX_SFR_ES_PL3_PTRPL_MASK _ULL(0xc00) /* Target Relative PL */
#define KVX_SFR_ES_PL3_PTRPL_SHIFT 10
#define KVX_SFR_ES_PL3_PTRPL_WIDTH 2
#define KVX_SFR_ES_PL3_PTRPL_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_ES_PL3_PTRPL_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_ES_PL3_PTRPL_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_ES_PL3_ITN_MASK _ULL(0x1f000) /* InTerrupt Number */
#define KVX_SFR_ES_PL3_ITN_SHIFT 12
#define KVX_SFR_ES_PL3_ITN_WIDTH 5
#define KVX_SFR_ES_PL3_ITN_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL3_ITN_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL3_ITN_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL3_ITL_MASK _ULL(0x60000) /* InTerrupt Level */
#define KVX_SFR_ES_PL3_ITL_SHIFT 17
#define KVX_SFR_ES_PL3_ITL_WIDTH 2
#define KVX_SFR_ES_PL3_ITL_WFXL_MASK _ULL(0x60000)
#define KVX_SFR_ES_PL3_ITL_WFXL_CLEAR _ULL(0x60000)
#define KVX_SFR_ES_PL3_ITL_WFXL_SET _ULL(0x6000000000000)

#define KVX_SFR_ES_PL3_ITI_MASK _ULL(0x1ff80000) /* InTerrupt Info */
#define KVX_SFR_ES_PL3_ITI_SHIFT 19
#define KVX_SFR_ES_PL3_ITI_WIDTH 10
#define KVX_SFR_ES_PL3_ITI_WFXL_MASK _ULL(0x1ff80000)
#define KVX_SFR_ES_PL3_ITI_WFXL_CLEAR _ULL(0x1ff80000)
#define KVX_SFR_ES_PL3_ITI_WFXL_SET _ULL(0x1ff8000000000000)

#define KVX_SFR_ES_PL3_SN_MASK _ULL(0xfff000) /* Syscall Number */
#define KVX_SFR_ES_PL3_SN_SHIFT 12
#define KVX_SFR_ES_PL3_SN_WIDTH 12
#define KVX_SFR_ES_PL3_SN_WFXL_MASK _ULL(0xfff000)
#define KVX_SFR_ES_PL3_SN_WFXL_CLEAR _ULL(0xfff000)
#define KVX_SFR_ES_PL3_SN_WFXL_SET _ULL(0xfff00000000000)

#define KVX_SFR_ES_PL3_HTC_MASK _ULL(0x1f000) /* Hardware Trap Cause */
#define KVX_SFR_ES_PL3_HTC_SHIFT 12
#define KVX_SFR_ES_PL3_HTC_WIDTH 5
#define KVX_SFR_ES_PL3_HTC_WFXL_MASK _ULL(0x1f000)
#define KVX_SFR_ES_PL3_HTC_WFXL_CLEAR _ULL(0x1f000)
#define KVX_SFR_ES_PL3_HTC_WFXL_SET _ULL(0x1f00000000000)

#define KVX_SFR_ES_PL3_SFRT_MASK _ULL(0x20000) /* SFR Trap */
#define KVX_SFR_ES_PL3_SFRT_SHIFT 17
#define KVX_SFR_ES_PL3_SFRT_WIDTH 1
#define KVX_SFR_ES_PL3_SFRT_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_ES_PL3_SFRT_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_ES_PL3_SFRT_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_ES_PL3_SFRI_MASK _ULL(0x1c0000) /* SFR Instruction */
#define KVX_SFR_ES_PL3_SFRI_SHIFT 18
#define KVX_SFR_ES_PL3_SFRI_WIDTH 3
#define KVX_SFR_ES_PL3_SFRI_WFXL_MASK _ULL(0x1c0000)
#define KVX_SFR_ES_PL3_SFRI_WFXL_CLEAR _ULL(0x1c0000)
#define KVX_SFR_ES_PL3_SFRI_WFXL_SET _ULL(0x1c000000000000)

#define KVX_SFR_ES_PL3_GPRP_MASK _ULL(0x7e00000) /* GPR Pointer */
#define KVX_SFR_ES_PL3_GPRP_SHIFT 21
#define KVX_SFR_ES_PL3_GPRP_WIDTH 6
#define KVX_SFR_ES_PL3_GPRP_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_ES_PL3_GPRP_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_ES_PL3_GPRP_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_ES_PL3_SFRP_MASK _ULL(0xff8000000) /* SFR Pointer */
#define KVX_SFR_ES_PL3_SFRP_SHIFT 27
#define KVX_SFR_ES_PL3_SFRP_WIDTH 9
#define KVX_SFR_ES_PL3_SFRP_WFXL_MASK _ULL(0xf8000000)
#define KVX_SFR_ES_PL3_SFRP_WFXL_CLEAR _ULL(0xf8000000)
#define KVX_SFR_ES_PL3_SFRP_WFXL_SET _ULL(0xf800000000000000)
#define KVX_SFR_ES_PL3_SFRP_WFXM_MASK _ULL(0xf00000000)
#define KVX_SFR_ES_PL3_SFRP_WFXM_CLEAR _ULL(0xf)
#define KVX_SFR_ES_PL3_SFRP_WFXM_SET _ULL(0xf00000000)

#define KVX_SFR_ES_PL3_DHT_MASK _ULL(0x1000000000) /* Disabled Hardware Trap */
#define KVX_SFR_ES_PL3_DHT_SHIFT 36
#define KVX_SFR_ES_PL3_DHT_WIDTH 1
#define KVX_SFR_ES_PL3_DHT_WFXM_MASK _ULL(0x1000000000)
#define KVX_SFR_ES_PL3_DHT_WFXM_CLEAR _ULL(0x10)
#define KVX_SFR_ES_PL3_DHT_WFXM_SET _ULL(0x1000000000)

#define KVX_SFR_ES_PL3_RWX_MASK _ULL(0x38000000000) /* Read Write Execute */
#define KVX_SFR_ES_PL3_RWX_SHIFT 39
#define KVX_SFR_ES_PL3_RWX_WIDTH 3
#define KVX_SFR_ES_PL3_RWX_WFXM_MASK _ULL(0x38000000000)
#define KVX_SFR_ES_PL3_RWX_WFXM_CLEAR _ULL(0x380)
#define KVX_SFR_ES_PL3_RWX_WFXM_SET _ULL(0x38000000000)

#define KVX_SFR_ES_PL3_NTA_MASK _ULL(0x40000000000) /* Non-Trapping Access */
#define KVX_SFR_ES_PL3_NTA_SHIFT 42
#define KVX_SFR_ES_PL3_NTA_WIDTH 1
#define KVX_SFR_ES_PL3_NTA_WFXM_MASK _ULL(0x40000000000)
#define KVX_SFR_ES_PL3_NTA_WFXM_CLEAR _ULL(0x400)
#define KVX_SFR_ES_PL3_NTA_WFXM_SET _ULL(0x40000000000)

#define KVX_SFR_ES_PL3_UCA_MASK _ULL(0x80000000000) /* Un-Cached Access */
#define KVX_SFR_ES_PL3_UCA_SHIFT 43
#define KVX_SFR_ES_PL3_UCA_WIDTH 1
#define KVX_SFR_ES_PL3_UCA_WFXM_MASK _ULL(0x80000000000)
#define KVX_SFR_ES_PL3_UCA_WFXM_CLEAR _ULL(0x800)
#define KVX_SFR_ES_PL3_UCA_WFXM_SET _ULL(0x80000000000)

#define KVX_SFR_ES_PL3_AS_MASK _ULL(0x3f00000000000) /* Access Size */
#define KVX_SFR_ES_PL3_AS_SHIFT 44
#define KVX_SFR_ES_PL3_AS_WIDTH 6
#define KVX_SFR_ES_PL3_AS_WFXM_MASK _ULL(0x3f00000000000)
#define KVX_SFR_ES_PL3_AS_WFXM_CLEAR _ULL(0x3f000)
#define KVX_SFR_ES_PL3_AS_WFXM_SET _ULL(0x3f00000000000)

#define KVX_SFR_ES_PL3_BS_MASK _ULL(0x3c000000000000) /* Bundle Size */
#define KVX_SFR_ES_PL3_BS_SHIFT 50
#define KVX_SFR_ES_PL3_BS_WIDTH 4
#define KVX_SFR_ES_PL3_BS_WFXM_MASK _ULL(0x3c000000000000)
#define KVX_SFR_ES_PL3_BS_WFXM_CLEAR _ULL(0x3c0000)
#define KVX_SFR_ES_PL3_BS_WFXM_SET _ULL(0x3c000000000000)

#define KVX_SFR_ES_PL3_DRI_MASK _ULL(0xfc0000000000000) /* Data Register Index */
#define KVX_SFR_ES_PL3_DRI_SHIFT 54
#define KVX_SFR_ES_PL3_DRI_WIDTH 6
#define KVX_SFR_ES_PL3_DRI_WFXM_MASK _ULL(0xfc0000000000000)
#define KVX_SFR_ES_PL3_DRI_WFXM_CLEAR _ULL(0xfc00000)
#define KVX_SFR_ES_PL3_DRI_WFXM_SET _ULL(0xfc0000000000000)

#define KVX_SFR_ES_PL3_PIC_MASK _ULL(0xf000000000000000) /* Privileged Instruction Code */
#define KVX_SFR_ES_PL3_PIC_SHIFT 60
#define KVX_SFR_ES_PL3_PIC_WIDTH 4
#define KVX_SFR_ES_PL3_PIC_WFXM_MASK _ULL(0xf000000000000000)
#define KVX_SFR_ES_PL3_PIC_WFXM_CLEAR _ULL(0xf0000000)
#define KVX_SFR_ES_PL3_PIC_WFXM_SET _ULL(0xf000000000000000)

#define KVX_SFR_ES_PL3_DC_MASK _ULL(0x3000) /* Debug Cause */
#define KVX_SFR_ES_PL3_DC_SHIFT 12
#define KVX_SFR_ES_PL3_DC_WIDTH 2
#define KVX_SFR_ES_PL3_DC_WFXL_MASK _ULL(0x3000)
#define KVX_SFR_ES_PL3_DC_WFXL_CLEAR _ULL(0x3000)
#define KVX_SFR_ES_PL3_DC_WFXL_SET _ULL(0x300000000000)

#define KVX_SFR_ES_PL3_BN_MASK _ULL(0x4000) /* Breakpoint Number */
#define KVX_SFR_ES_PL3_BN_SHIFT 14
#define KVX_SFR_ES_PL3_BN_WIDTH 1
#define KVX_SFR_ES_PL3_BN_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_ES_PL3_BN_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_ES_PL3_BN_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_ES_PL3_WN_MASK _ULL(0x8000) /* Watchpoint Number */
#define KVX_SFR_ES_PL3_WN_SHIFT 15
#define KVX_SFR_ES_PL3_WN_WIDTH 1
#define KVX_SFR_ES_PL3_WN_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_ES_PL3_WN_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_ES_PL3_WN_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_TCR_T0CE_MASK _ULL(0x10000) /* Timer 0 Count Enable */
#define KVX_SFR_TCR_T0CE_SHIFT 16
#define KVX_SFR_TCR_T0CE_WIDTH 1
#define KVX_SFR_TCR_T0CE_WFXL_MASK _ULL(0x10000)
#define KVX_SFR_TCR_T0CE_WFXL_CLEAR _ULL(0x10000)
#define KVX_SFR_TCR_T0CE_WFXL_SET _ULL(0x1000000000000)

#define KVX_SFR_TCR_T1CE_MASK _ULL(0x20000) /* Timer 1 Count Enable */
#define KVX_SFR_TCR_T1CE_SHIFT 17
#define KVX_SFR_TCR_T1CE_WIDTH 1
#define KVX_SFR_TCR_T1CE_WFXL_MASK _ULL(0x20000)
#define KVX_SFR_TCR_T1CE_WFXL_CLEAR _ULL(0x20000)
#define KVX_SFR_TCR_T1CE_WFXL_SET _ULL(0x2000000000000)

#define KVX_SFR_TCR_T0IE_MASK _ULL(0x40000) /* Timer 0 Interrupt Enable */
#define KVX_SFR_TCR_T0IE_SHIFT 18
#define KVX_SFR_TCR_T0IE_WIDTH 1
#define KVX_SFR_TCR_T0IE_WFXL_MASK _ULL(0x40000)
#define KVX_SFR_TCR_T0IE_WFXL_CLEAR _ULL(0x40000)
#define KVX_SFR_TCR_T0IE_WFXL_SET _ULL(0x4000000000000)

#define KVX_SFR_TCR_T1IE_MASK _ULL(0x80000) /* Timer 1 Interrupt Enable */
#define KVX_SFR_TCR_T1IE_SHIFT 19
#define KVX_SFR_TCR_T1IE_WIDTH 1
#define KVX_SFR_TCR_T1IE_WFXL_MASK _ULL(0x80000)
#define KVX_SFR_TCR_T1IE_WFXL_CLEAR _ULL(0x80000)
#define KVX_SFR_TCR_T1IE_WFXL_SET _ULL(0x8000000000000)

#define KVX_SFR_TCR_T0ST_MASK _ULL(0x100000) /* Timer 0 Status */
#define KVX_SFR_TCR_T0ST_SHIFT 20
#define KVX_SFR_TCR_T0ST_WIDTH 1
#define KVX_SFR_TCR_T0ST_WFXL_MASK _ULL(0x100000)
#define KVX_SFR_TCR_T0ST_WFXL_CLEAR _ULL(0x100000)
#define KVX_SFR_TCR_T0ST_WFXL_SET _ULL(0x10000000000000)

#define KVX_SFR_TCR_T1ST_MASK _ULL(0x200000) /* Timer 1 Status */
#define KVX_SFR_TCR_T1ST_SHIFT 21
#define KVX_SFR_TCR_T1ST_WIDTH 1
#define KVX_SFR_TCR_T1ST_WFXL_MASK _ULL(0x200000)
#define KVX_SFR_TCR_T1ST_WFXL_CLEAR _ULL(0x200000)
#define KVX_SFR_TCR_T1ST_WFXL_SET _ULL(0x20000000000000)

#define KVX_SFR_TCR_T0SI_MASK _ULL(0x400000) /* Stop Timer 0 in Idle */
#define KVX_SFR_TCR_T0SI_SHIFT 22
#define KVX_SFR_TCR_T0SI_WIDTH 1
#define KVX_SFR_TCR_T0SI_WFXL_MASK _ULL(0x400000)
#define KVX_SFR_TCR_T0SI_WFXL_CLEAR _ULL(0x400000)
#define KVX_SFR_TCR_T0SI_WFXL_SET _ULL(0x40000000000000)

#define KVX_SFR_TCR_T1SI_MASK _ULL(0x800000) /* Stop Timer 1 in Idle */
#define KVX_SFR_TCR_T1SI_SHIFT 23
#define KVX_SFR_TCR_T1SI_WIDTH 1
#define KVX_SFR_TCR_T1SI_WFXL_MASK _ULL(0x800000)
#define KVX_SFR_TCR_T1SI_WFXL_CLEAR _ULL(0x800000)
#define KVX_SFR_TCR_T1SI_WFXL_SET _ULL(0x80000000000000)

#define KVX_SFR_TCR_WCE_MASK _ULL(0x1000000) /* Watchdog Counting Enable */
#define KVX_SFR_TCR_WCE_SHIFT 24
#define KVX_SFR_TCR_WCE_WIDTH 1
#define KVX_SFR_TCR_WCE_WFXL_MASK _ULL(0x1000000)
#define KVX_SFR_TCR_WCE_WFXL_CLEAR _ULL(0x1000000)
#define KVX_SFR_TCR_WCE_WFXL_SET _ULL(0x100000000000000)

#define KVX_SFR_TCR_WIE_MASK _ULL(0x2000000) /* Watchdog Interrupt Enable */
#define KVX_SFR_TCR_WIE_SHIFT 25
#define KVX_SFR_TCR_WIE_WIDTH 1
#define KVX_SFR_TCR_WIE_WFXL_MASK _ULL(0x2000000)
#define KVX_SFR_TCR_WIE_WFXL_CLEAR _ULL(0x2000000)
#define KVX_SFR_TCR_WIE_WFXL_SET _ULL(0x200000000000000)

#define KVX_SFR_TCR_WUI_MASK _ULL(0x4000000) /* Watchdog Underflow Inform */
#define KVX_SFR_TCR_WUI_SHIFT 26
#define KVX_SFR_TCR_WUI_WIDTH 1
#define KVX_SFR_TCR_WUI_WFXL_MASK _ULL(0x4000000)
#define KVX_SFR_TCR_WUI_WFXL_CLEAR _ULL(0x4000000)
#define KVX_SFR_TCR_WUI_WFXL_SET _ULL(0x400000000000000)

#define KVX_SFR_TCR_WUS_MASK _ULL(0x8000000) /* Watchdog Underflow Status */
#define KVX_SFR_TCR_WUS_SHIFT 27
#define KVX_SFR_TCR_WUS_WIDTH 1
#define KVX_SFR_TCR_WUS_WFXL_MASK _ULL(0x8000000)
#define KVX_SFR_TCR_WUS_WFXL_CLEAR _ULL(0x8000000)
#define KVX_SFR_TCR_WUS_WFXL_SET _ULL(0x800000000000000)

#define KVX_SFR_TCR_WSI_MASK _ULL(0x10000000) /* Watchdog Stop in Idle */
#define KVX_SFR_TCR_WSI_SHIFT 28
#define KVX_SFR_TCR_WSI_WIDTH 1
#define KVX_SFR_TCR_WSI_WFXL_MASK _ULL(0x10000000)
#define KVX_SFR_TCR_WSI_WFXL_CLEAR _ULL(0x10000000)
#define KVX_SFR_TCR_WSI_WFXL_SET _ULL(0x1000000000000000)

#define KVX_SFR_PM0_PM0_MASK _ULL(0xffffffffffffffff) /* Performance Monitor 0 */
#define KVX_SFR_PM0_PM0_SHIFT 0
#define KVX_SFR_PM0_PM0_WIDTH 64
#define KVX_SFR_PM0_PM0_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_PM0_PM0_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM0_PM0_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_PM0_PM0_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_PM0_PM0_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM0_PM0_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_PM1_PM1_MASK _ULL(0xffffffffffffffff) /* Performance Monitor 1 */
#define KVX_SFR_PM1_PM1_SHIFT 0
#define KVX_SFR_PM1_PM1_WIDTH 64
#define KVX_SFR_PM1_PM1_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_PM1_PM1_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM1_PM1_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_PM1_PM1_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_PM1_PM1_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM1_PM1_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_PM2_PM2_MASK _ULL(0xffffffffffffffff) /* Performance Monitor 2 */
#define KVX_SFR_PM2_PM2_SHIFT 0
#define KVX_SFR_PM2_PM2_WIDTH 64
#define KVX_SFR_PM2_PM2_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_PM2_PM2_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM2_PM2_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_PM2_PM2_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_PM2_PM2_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM2_PM2_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_PM3_PM3_MASK _ULL(0xffffffffffffffff) /* Performance Monitor 3 */
#define KVX_SFR_PM3_PM3_SHIFT 0
#define KVX_SFR_PM3_PM3_WIDTH 64
#define KVX_SFR_PM3_PM3_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_PM3_PM3_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM3_PM3_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_PM3_PM3_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_PM3_PM3_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PM3_PM3_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_PMSA_PMSA_MASK _ULL(0xffffffffffffffff) /* Performance Monitor Saved Address */
#define KVX_SFR_PMSA_PMSA_SHIFT 0
#define KVX_SFR_PMSA_PMSA_WIDTH 64
#define KVX_SFR_PMSA_PMSA_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_PMSA_PMSA_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PMSA_PMSA_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_PMSA_PMSA_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_PMSA_PMSA_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_PMSA_PMSA_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_T0V_T0V_MASK _ULL(0xffffffffffffffff) /* Timer 0 value */
#define KVX_SFR_T0V_T0V_SHIFT 0
#define KVX_SFR_T0V_T0V_WIDTH 64
#define KVX_SFR_T0V_T0V_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_T0V_T0V_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T0V_T0V_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_T0V_T0V_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_T0V_T0V_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T0V_T0V_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_T1V_T1V_MASK _ULL(0xffffffffffffffff) /* Timer 1 value */
#define KVX_SFR_T1V_T1V_SHIFT 0
#define KVX_SFR_T1V_T1V_WIDTH 64
#define KVX_SFR_T1V_T1V_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_T1V_T1V_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T1V_T1V_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_T1V_T1V_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_T1V_T1V_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T1V_T1V_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_T0R_T0R_MASK _ULL(0xffffffffffffffff) /* Timer 0 reload value */
#define KVX_SFR_T0R_T0R_SHIFT 0
#define KVX_SFR_T0R_T0R_WIDTH 64
#define KVX_SFR_T0R_T0R_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_T0R_T0R_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T0R_T0R_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_T0R_T0R_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_T0R_T0R_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T0R_T0R_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_T1R_T1R_MASK _ULL(0xffffffffffffffff) /* Timer 1 reload value */
#define KVX_SFR_T1R_T1R_SHIFT 0
#define KVX_SFR_T1R_T1R_WIDTH 64
#define KVX_SFR_T1R_T1R_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_T1R_T1R_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T1R_T1R_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_T1R_T1R_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_T1R_T1R_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_T1R_T1R_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_WDV_WDV_MASK _ULL(0xffffffffffffffff) /* Watchdog Value */
#define KVX_SFR_WDV_WDV_SHIFT 0
#define KVX_SFR_WDV_WDV_WIDTH 64
#define KVX_SFR_WDV_WDV_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_WDV_WDV_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_WDV_WDV_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_WDV_WDV_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_WDV_WDV_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_WDV_WDV_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_WDR_WDR_MASK _ULL(0xffffffffffffffff) /* Watchdog Reload Value */
#define KVX_SFR_WDR_WDR_SHIFT 0
#define KVX_SFR_WDR_WDR_WIDTH 64
#define KVX_SFR_WDR_WDR_WFXL_MASK _ULL(0xffffffff)
#define KVX_SFR_WDR_WDR_WFXL_CLEAR _ULL(0xffffffff)
#define KVX_SFR_WDR_WDR_WFXL_SET _ULL(0xffffffff00000000)
#define KVX_SFR_WDR_WDR_WFXM_MASK _ULL(0xffffffff00000000)
#define KVX_SFR_WDR_WDR_WFXM_CLEAR _ULL(0xffffffff)
#define KVX_SFR_WDR_WDR_WFXM_SET _ULL(0xffffffff00000000)

#define KVX_SFR_PMC_PM0C_MASK _ULL(0x3f) /* PM0 Configuration */
#define KVX_SFR_PMC_PM0C_SHIFT 0
#define KVX_SFR_PMC_PM0C_WIDTH 6
#define KVX_SFR_PMC_PM0C_WFXL_MASK _ULL(0x3f)
#define KVX_SFR_PMC_PM0C_WFXL_CLEAR _ULL(0x3f)
#define KVX_SFR_PMC_PM0C_WFXL_SET _ULL(0x3f00000000)

#define KVX_SFR_PMC_PM1C_MASK _ULL(0x1f80) /* PM1 Configuration */
#define KVX_SFR_PMC_PM1C_SHIFT 7
#define KVX_SFR_PMC_PM1C_WIDTH 6
#define KVX_SFR_PMC_PM1C_WFXL_MASK _ULL(0x1f80)
#define KVX_SFR_PMC_PM1C_WFXL_CLEAR _ULL(0x1f80)
#define KVX_SFR_PMC_PM1C_WFXL_SET _ULL(0x1f8000000000)

#define KVX_SFR_PMC_PM2C_MASK _ULL(0xfc000) /* PM2 Configuration */
#define KVX_SFR_PMC_PM2C_SHIFT 14
#define KVX_SFR_PMC_PM2C_WIDTH 6
#define KVX_SFR_PMC_PM2C_WFXL_MASK _ULL(0xfc000)
#define KVX_SFR_PMC_PM2C_WFXL_CLEAR _ULL(0xfc000)
#define KVX_SFR_PMC_PM2C_WFXL_SET _ULL(0xfc00000000000)

#define KVX_SFR_PMC_PM3C_MASK _ULL(0x7e00000) /* PM3 Configuration */
#define KVX_SFR_PMC_PM3C_SHIFT 21
#define KVX_SFR_PMC_PM3C_WIDTH 6
#define KVX_SFR_PMC_PM3C_WFXL_MASK _ULL(0x7e00000)
#define KVX_SFR_PMC_PM3C_WFXL_CLEAR _ULL(0x7e00000)
#define KVX_SFR_PMC_PM3C_WFXL_SET _ULL(0x7e0000000000000)

#define KVX_SFR_PMC_SAV_MASK _ULL(0x40000000) /* Saved Address Valid */
#define KVX_SFR_PMC_SAV_SHIFT 30
#define KVX_SFR_PMC_SAV_WIDTH 1
#define KVX_SFR_PMC_SAV_WFXL_MASK _ULL(0x40000000)
#define KVX_SFR_PMC_SAV_WFXL_CLEAR _ULL(0x40000000)
#define KVX_SFR_PMC_SAV_WFXL_SET _ULL(0x4000000000000000)

#define KVX_SFR_PMC_PM0IE_MASK _ULL(0x100000000) /* PM0 Interrupt Enable */
#define KVX_SFR_PMC_PM0IE_SHIFT 32
#define KVX_SFR_PMC_PM0IE_WIDTH 1
#define KVX_SFR_PMC_PM0IE_WFXM_MASK _ULL(0x100000000)
#define KVX_SFR_PMC_PM0IE_WFXM_CLEAR _ULL(0x1)
#define KVX_SFR_PMC_PM0IE_WFXM_SET _ULL(0x100000000)

#define KVX_SFR_PMC_PM1IE_MASK _ULL(0x200000000) /* PM1 Interrupt Enable */
#define KVX_SFR_PMC_PM1IE_SHIFT 33
#define KVX_SFR_PMC_PM1IE_WIDTH 1
#define KVX_SFR_PMC_PM1IE_WFXM_MASK _ULL(0x200000000)
#define KVX_SFR_PMC_PM1IE_WFXM_CLEAR _ULL(0x2)
#define KVX_SFR_PMC_PM1IE_WFXM_SET _ULL(0x200000000)

#define KVX_SFR_PMC_PM2IE_MASK _ULL(0x400000000) /* PM2 Interrupt Enable */
#define KVX_SFR_PMC_PM2IE_SHIFT 34
#define KVX_SFR_PMC_PM2IE_WIDTH 1
#define KVX_SFR_PMC_PM2IE_WFXM_MASK _ULL(0x400000000)
#define KVX_SFR_PMC_PM2IE_WFXM_CLEAR _ULL(0x4)
#define KVX_SFR_PMC_PM2IE_WFXM_SET _ULL(0x400000000)

#define KVX_SFR_PMC_PM3IE_MASK _ULL(0x800000000) /* PM3 Interrupt Enable */
#define KVX_SFR_PMC_PM3IE_SHIFT 35
#define KVX_SFR_PMC_PM3IE_WIDTH 1
#define KVX_SFR_PMC_PM3IE_WFXM_MASK _ULL(0x800000000)
#define KVX_SFR_PMC_PM3IE_WFXM_CLEAR _ULL(0x8)
#define KVX_SFR_PMC_PM3IE_WFXM_SET _ULL(0x800000000)

#define KVX_SFR_PMC_SAT_MASK _ULL(0x3000000000) /* Saved Address Type */
#define KVX_SFR_PMC_SAT_SHIFT 36
#define KVX_SFR_PMC_SAT_WIDTH 2
#define KVX_SFR_PMC_SAT_WFXM_MASK _ULL(0x3000000000)
#define KVX_SFR_PMC_SAT_WFXM_CLEAR _ULL(0x30)
#define KVX_SFR_PMC_SAT_WFXM_SET _ULL(0x3000000000)

#define KVX_SFR_PCR_PID_MASK _ULL(0xff) /* Processing Identifier in cluster */
#define KVX_SFR_PCR_PID_SHIFT 0
#define KVX_SFR_PCR_PID_WIDTH 8
#define KVX_SFR_PCR_PID_WFXL_MASK _ULL(0xff)
#define KVX_SFR_PCR_PID_WFXL_CLEAR _ULL(0xff)
#define KVX_SFR_PCR_PID_WFXL_SET _ULL(0xff00000000)

#define KVX_SFR_PCR_CID_MASK _ULL(0xff00) /* Cluster Identifier in system */
#define KVX_SFR_PCR_CID_SHIFT 8
#define KVX_SFR_PCR_CID_WIDTH 8
#define KVX_SFR_PCR_CID_WFXL_MASK _ULL(0xff00)
#define KVX_SFR_PCR_CID_WFXL_CLEAR _ULL(0xff00)
#define KVX_SFR_PCR_CID_WFXL_SET _ULL(0xff0000000000)

#define KVX_SFR_PCR_MID_MASK _ULL(0xff0000) /* MPPA Identifier */
#define KVX_SFR_PCR_MID_SHIFT 16
#define KVX_SFR_PCR_MID_WIDTH 8
#define KVX_SFR_PCR_MID_WFXL_MASK _ULL(0xff0000)
#define KVX_SFR_PCR_MID_WFXL_CLEAR _ULL(0xff0000)
#define KVX_SFR_PCR_MID_WFXL_SET _ULL(0xff000000000000)

#define KVX_SFR_PCR_CAR_MASK _ULL(0xf000000) /* Core Architecture Revision ID */
#define KVX_SFR_PCR_CAR_SHIFT 24
#define KVX_SFR_PCR_CAR_WIDTH 4
#define KVX_SFR_PCR_CAR_WFXL_MASK _ULL(0xf000000)
#define KVX_SFR_PCR_CAR_WFXL_CLEAR _ULL(0xf000000)
#define KVX_SFR_PCR_CAR_WFXL_SET _ULL(0xf00000000000000)

#define KVX_SFR_PCR_CMA_MASK _ULL(0xf0000000) /* Core Micro-Architecture Revision ID */
#define KVX_SFR_PCR_CMA_SHIFT 28
#define KVX_SFR_PCR_CMA_WIDTH 4
#define KVX_SFR_PCR_CMA_WFXL_MASK _ULL(0xf0000000)
#define KVX_SFR_PCR_CMA_WFXL_CLEAR _ULL(0xf0000000)
#define KVX_SFR_PCR_CMA_WFXL_SET _ULL(0xf000000000000000)

#define KVX_SFR_PCR_SV_MASK _ULL(0xff00000000) /* System-On-Chip Version */
#define KVX_SFR_PCR_SV_SHIFT 32
#define KVX_SFR_PCR_SV_WIDTH 8
#define KVX_SFR_PCR_SV_WFXM_MASK _ULL(0xff00000000)
#define KVX_SFR_PCR_SV_WFXM_CLEAR _ULL(0xff)
#define KVX_SFR_PCR_SV_WFXM_SET _ULL(0xff00000000)

#define KVX_SFR_PCR_ST_MASK _ULL(0xf0000000000) /* System-On-Chip Type */
#define KVX_SFR_PCR_ST_SHIFT 40
#define KVX_SFR_PCR_ST_WIDTH 4
#define KVX_SFR_PCR_ST_WFXM_MASK _ULL(0xf0000000000)
#define KVX_SFR_PCR_ST_WFXM_CLEAR _ULL(0xf00)
#define KVX_SFR_PCR_ST_WFXM_SET _ULL(0xf0000000000)

#define KVX_SFR_PCR_BM_MASK _ULL(0xff00000000000) /* Boot Mode */
#define KVX_SFR_PCR_BM_SHIFT 44
#define KVX_SFR_PCR_BM_WIDTH 8
#define KVX_SFR_PCR_BM_WFXM_MASK _ULL(0xff00000000000)
#define KVX_SFR_PCR_BM_WFXM_CLEAR _ULL(0xff000)
#define KVX_SFR_PCR_BM_WFXM_SET _ULL(0xff00000000000)

#define KVX_SFR_PCR_COE_MASK _ULL(0x10000000000000) /* COprocessor Enable */
#define KVX_SFR_PCR_COE_SHIFT 52
#define KVX_SFR_PCR_COE_WIDTH 1
#define KVX_SFR_PCR_COE_WFXM_MASK _ULL(0x10000000000000)
#define KVX_SFR_PCR_COE_WFXM_CLEAR _ULL(0x100000)
#define KVX_SFR_PCR_COE_WFXM_SET _ULL(0x10000000000000)

#define KVX_SFR_PCR_L1CE_MASK _ULL(0x20000000000000) /* L1 cache Coherency Enable */
#define KVX_SFR_PCR_L1CE_SHIFT 53
#define KVX_SFR_PCR_L1CE_WIDTH 1
#define KVX_SFR_PCR_L1CE_WFXM_MASK _ULL(0x20000000000000)
#define KVX_SFR_PCR_L1CE_WFXM_CLEAR _ULL(0x200000)
#define KVX_SFR_PCR_L1CE_WFXM_SET _ULL(0x20000000000000)

#define KVX_SFR_PCR_DSEM_MASK _ULL(0x40000000000000) /* Data Simple Ecc exception Mode */
#define KVX_SFR_PCR_DSEM_SHIFT 54
#define KVX_SFR_PCR_DSEM_WIDTH 1
#define KVX_SFR_PCR_DSEM_WFXM_MASK _ULL(0x40000000000000)
#define KVX_SFR_PCR_DSEM_WFXM_CLEAR _ULL(0x400000)
#define KVX_SFR_PCR_DSEM_WFXM_SET _ULL(0x40000000000000)

#define KVX_SFR_MMC_ASN_MASK _ULL(0x1ff) /* Address Space Number */
#define KVX_SFR_MMC_ASN_SHIFT 0
#define KVX_SFR_MMC_ASN_WIDTH 9
#define KVX_SFR_MMC_ASN_WFXL_MASK _ULL(0x1ff)
#define KVX_SFR_MMC_ASN_WFXL_CLEAR _ULL(0x1ff)
#define KVX_SFR_MMC_ASN_WFXL_SET _ULL(0x1ff00000000)

#define KVX_SFR_MMC_S_MASK _ULL(0x200) /* Speculative */
#define KVX_SFR_MMC_S_SHIFT 9
#define KVX_SFR_MMC_S_WIDTH 1
#define KVX_SFR_MMC_S_WFXL_MASK _ULL(0x200)
#define KVX_SFR_MMC_S_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_MMC_S_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_MMC_SNE_MASK _ULL(0x4000) /* Speculative NOMAPPING Enable */
#define KVX_SFR_MMC_SNE_SHIFT 14
#define KVX_SFR_MMC_SNE_WIDTH 1
#define KVX_SFR_MMC_SNE_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_MMC_SNE_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_MMC_SNE_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_MMC_SPE_MASK _ULL(0x8000) /* Speculative PROTECTION Enable */
#define KVX_SFR_MMC_SPE_SHIFT 15
#define KVX_SFR_MMC_SPE_WIDTH 1
#define KVX_SFR_MMC_SPE_WFXL_MASK _ULL(0x8000)
#define KVX_SFR_MMC_SPE_WFXL_CLEAR _ULL(0x8000)
#define KVX_SFR_MMC_SPE_WFXL_SET _ULL(0x800000000000)

#define KVX_SFR_MMC_PTC_MASK _ULL(0x30000) /* Protection Trap Cause */
#define KVX_SFR_MMC_PTC_SHIFT 16
#define KVX_SFR_MMC_PTC_WIDTH 2
#define KVX_SFR_MMC_PTC_WFXL_MASK _ULL(0x30000)
#define KVX_SFR_MMC_PTC_WFXL_CLEAR _ULL(0x30000)
#define KVX_SFR_MMC_PTC_WFXL_SET _ULL(0x3000000000000)

#define KVX_SFR_MMC_SW_MASK _ULL(0x3c0000) /* Select Way */
#define KVX_SFR_MMC_SW_SHIFT 18
#define KVX_SFR_MMC_SW_WIDTH 4
#define KVX_SFR_MMC_SW_WFXL_MASK _ULL(0x3c0000)
#define KVX_SFR_MMC_SW_WFXL_CLEAR _ULL(0x3c0000)
#define KVX_SFR_MMC_SW_WFXL_SET _ULL(0x3c000000000000)

#define KVX_SFR_MMC_SS_MASK _ULL(0xfc00000) /* Select Set */
#define KVX_SFR_MMC_SS_SHIFT 22
#define KVX_SFR_MMC_SS_WIDTH 6
#define KVX_SFR_MMC_SS_WFXL_MASK _ULL(0xfc00000)
#define KVX_SFR_MMC_SS_WFXL_CLEAR _ULL(0xfc00000)
#define KVX_SFR_MMC_SS_WFXL_SET _ULL(0xfc0000000000000)

#define KVX_SFR_MMC_SB_MASK _ULL(0x10000000) /* Select Buffer */
#define KVX_SFR_MMC_SB_SHIFT 28
#define KVX_SFR_MMC_SB_WIDTH 1
#define KVX_SFR_MMC_SB_WFXL_MASK _ULL(0x10000000)
#define KVX_SFR_MMC_SB_WFXL_CLEAR _ULL(0x10000000)
#define KVX_SFR_MMC_SB_WFXL_SET _ULL(0x1000000000000000)

#define KVX_SFR_MMC_PAR_MASK _ULL(0x40000000) /* PARity error flag */
#define KVX_SFR_MMC_PAR_SHIFT 30
#define KVX_SFR_MMC_PAR_WIDTH 1
#define KVX_SFR_MMC_PAR_WFXL_MASK _ULL(0x40000000)
#define KVX_SFR_MMC_PAR_WFXL_CLEAR _ULL(0x40000000)
#define KVX_SFR_MMC_PAR_WFXL_SET _ULL(0x4000000000000000)

#define KVX_SFR_MMC_E_MASK _ULL(0x80000000) /* Error Flag */
#define KVX_SFR_MMC_E_SHIFT 31
#define KVX_SFR_MMC_E_WIDTH 1
#define KVX_SFR_MMC_E_WFXL_MASK _ULL(0x80000000)
#define KVX_SFR_MMC_E_WFXL_CLEAR _ULL(0x80000000)
#define KVX_SFR_MMC_E_WFXL_SET _ULL(0x8000000000000000)

#define KVX_SFR_TEL_ES_MASK _ULL(0x3) /* Entry Status */
#define KVX_SFR_TEL_ES_SHIFT 0
#define KVX_SFR_TEL_ES_WIDTH 2
#define KVX_SFR_TEL_ES_WFXL_MASK _ULL(0x3)
#define KVX_SFR_TEL_ES_WFXL_CLEAR _ULL(0x3)
#define KVX_SFR_TEL_ES_WFXL_SET _ULL(0x300000000)

#define KVX_SFR_TEL_CP_MASK _ULL(0xc) /* Cache Policy */
#define KVX_SFR_TEL_CP_SHIFT 2
#define KVX_SFR_TEL_CP_WIDTH 2
#define KVX_SFR_TEL_CP_WFXL_MASK _ULL(0xc)
#define KVX_SFR_TEL_CP_WFXL_CLEAR _ULL(0xc)
#define KVX_SFR_TEL_CP_WFXL_SET _ULL(0xc00000000)

#define KVX_SFR_TEL_PA_MASK _ULL(0xf0) /* Protection Attributes */
#define KVX_SFR_TEL_PA_SHIFT 4
#define KVX_SFR_TEL_PA_WIDTH 4
#define KVX_SFR_TEL_PA_WFXL_MASK _ULL(0xf0)
#define KVX_SFR_TEL_PA_WFXL_CLEAR _ULL(0xf0)
#define KVX_SFR_TEL_PA_WFXL_SET _ULL(0xf000000000)

#define KVX_SFR_TEL_PS_MASK _ULL(0xc00) /* Page Size */
#define KVX_SFR_TEL_PS_SHIFT 10
#define KVX_SFR_TEL_PS_WIDTH 2
#define KVX_SFR_TEL_PS_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_TEL_PS_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_TEL_PS_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_TEL_FN_MASK _ULL(0xfffffff000) /* Frame Number */
#define KVX_SFR_TEL_FN_SHIFT 12
#define KVX_SFR_TEL_FN_WIDTH 28
#define KVX_SFR_TEL_FN_WFXL_MASK _ULL(0xfffff000)
#define KVX_SFR_TEL_FN_WFXL_CLEAR _ULL(0xfffff000)
#define KVX_SFR_TEL_FN_WFXL_SET _ULL(0xfffff00000000000)
#define KVX_SFR_TEL_FN_WFXM_MASK _ULL(0xff00000000)
#define KVX_SFR_TEL_FN_WFXM_CLEAR _ULL(0xff)
#define KVX_SFR_TEL_FN_WFXM_SET _ULL(0xff00000000)

#define KVX_SFR_TEH_ASN_MASK _ULL(0x1ff) /* Adress Space Number */
#define KVX_SFR_TEH_ASN_SHIFT 0
#define KVX_SFR_TEH_ASN_WIDTH 9
#define KVX_SFR_TEH_ASN_WFXL_MASK _ULL(0x1ff)
#define KVX_SFR_TEH_ASN_WFXL_CLEAR _ULL(0x1ff)
#define KVX_SFR_TEH_ASN_WFXL_SET _ULL(0x1ff00000000)

#define KVX_SFR_TEH_G_MASK _ULL(0x200) /* Global page indicator */
#define KVX_SFR_TEH_G_SHIFT 9
#define KVX_SFR_TEH_G_WIDTH 1
#define KVX_SFR_TEH_G_WFXL_MASK _ULL(0x200)
#define KVX_SFR_TEH_G_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_TEH_G_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_TEH_VS_MASK _ULL(0xc00) /* Virtual Space */
#define KVX_SFR_TEH_VS_SHIFT 10
#define KVX_SFR_TEH_VS_WIDTH 2
#define KVX_SFR_TEH_VS_WFXL_MASK _ULL(0xc00)
#define KVX_SFR_TEH_VS_WFXL_CLEAR _ULL(0xc00)
#define KVX_SFR_TEH_VS_WFXL_SET _ULL(0xc0000000000)

#define KVX_SFR_TEH_PN_MASK _ULL(0x1fffffff000) /* Page Number */
#define KVX_SFR_TEH_PN_SHIFT 12
#define KVX_SFR_TEH_PN_WIDTH 29
#define KVX_SFR_TEH_PN_WFXL_MASK _ULL(0xfffff000)
#define KVX_SFR_TEH_PN_WFXL_CLEAR _ULL(0xfffff000)
#define KVX_SFR_TEH_PN_WFXL_SET _ULL(0xfffff00000000000)
#define KVX_SFR_TEH_PN_WFXM_MASK _ULL(0x1ff00000000)
#define KVX_SFR_TEH_PN_WFXM_CLEAR _ULL(0x1ff)
#define KVX_SFR_TEH_PN_WFXM_SET _ULL(0x1ff00000000)

#define KVX_SFR_DC_BE0_MASK _ULL(0x1) /* Breakpoint 0 Enable */
#define KVX_SFR_DC_BE0_SHIFT 0
#define KVX_SFR_DC_BE0_WIDTH 1
#define KVX_SFR_DC_BE0_WFXL_MASK _ULL(0x1)
#define KVX_SFR_DC_BE0_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_DC_BE0_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_DC_BR0_MASK _ULL(0x7e) /* Breakpoint 0 Range */
#define KVX_SFR_DC_BR0_SHIFT 1
#define KVX_SFR_DC_BR0_WIDTH 6
#define KVX_SFR_DC_BR0_WFXL_MASK _ULL(0x7e)
#define KVX_SFR_DC_BR0_WFXL_CLEAR _ULL(0x7e)
#define KVX_SFR_DC_BR0_WFXL_SET _ULL(0x7e00000000)

#define KVX_SFR_DC_BE1_MASK _ULL(0x80) /* Breakpoint 1 Enable */
#define KVX_SFR_DC_BE1_SHIFT 7
#define KVX_SFR_DC_BE1_WIDTH 1
#define KVX_SFR_DC_BE1_WFXL_MASK _ULL(0x80)
#define KVX_SFR_DC_BE1_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_DC_BE1_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_DC_BR1_MASK _ULL(0x3f00) /* Breakpoint 1 Range */
#define KVX_SFR_DC_BR1_SHIFT 8
#define KVX_SFR_DC_BR1_WIDTH 6
#define KVX_SFR_DC_BR1_WFXL_MASK _ULL(0x3f00)
#define KVX_SFR_DC_BR1_WFXL_CLEAR _ULL(0x3f00)
#define KVX_SFR_DC_BR1_WFXL_SET _ULL(0x3f0000000000)

#define KVX_SFR_DC_WE0_MASK _ULL(0x4000) /* Watchpoint 0 Enable */
#define KVX_SFR_DC_WE0_SHIFT 14
#define KVX_SFR_DC_WE0_WIDTH 1
#define KVX_SFR_DC_WE0_WFXL_MASK _ULL(0x4000)
#define KVX_SFR_DC_WE0_WFXL_CLEAR _ULL(0x4000)
#define KVX_SFR_DC_WE0_WFXL_SET _ULL(0x400000000000)

#define KVX_SFR_DC_WR0_MASK _ULL(0x1f8000) /* Watchpoint 0 Range */
#define KVX_SFR_DC_WR0_SHIFT 15
#define KVX_SFR_DC_WR0_WIDTH 6
#define KVX_SFR_DC_WR0_WFXL_MASK _ULL(0x1f8000)
#define KVX_SFR_DC_WR0_WFXL_CLEAR _ULL(0x1f8000)
#define KVX_SFR_DC_WR0_WFXL_SET _ULL(0x1f800000000000)

#define KVX_SFR_DC_WE1_MASK _ULL(0x200000) /* Watchpoint 1 Enable */
#define KVX_SFR_DC_WE1_SHIFT 21
#define KVX_SFR_DC_WE1_WIDTH 1
#define KVX_SFR_DC_WE1_WFXL_MASK _ULL(0x200000)
#define KVX_SFR_DC_WE1_WFXL_CLEAR _ULL(0x200000)
#define KVX_SFR_DC_WE1_WFXL_SET _ULL(0x20000000000000)

#define KVX_SFR_DC_WR1_MASK _ULL(0xfc00000) /* Watchpoint 1 Range */
#define KVX_SFR_DC_WR1_SHIFT 22
#define KVX_SFR_DC_WR1_WIDTH 6
#define KVX_SFR_DC_WR1_WFXL_MASK _ULL(0xfc00000)
#define KVX_SFR_DC_WR1_WFXL_CLEAR _ULL(0xfc00000)
#define KVX_SFR_DC_WR1_WFXL_SET _ULL(0xfc0000000000000)

#define KVX_SFR_MES_PSE_MASK _ULL(0x1) /* Program Simple Ecc */
#define KVX_SFR_MES_PSE_SHIFT 0
#define KVX_SFR_MES_PSE_WIDTH 1
#define KVX_SFR_MES_PSE_WFXL_MASK _ULL(0x1)
#define KVX_SFR_MES_PSE_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_MES_PSE_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_MES_PILSY_MASK _ULL(0x2) /* Program cache Invalidated Line following pSYs error. */
#define KVX_SFR_MES_PILSY_SHIFT 1
#define KVX_SFR_MES_PILSY_WIDTH 1
#define KVX_SFR_MES_PILSY_WFXL_MASK _ULL(0x2)
#define KVX_SFR_MES_PILSY_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_MES_PILSY_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_MES_PILDE_MASK _ULL(0x4) /* Program cache Invalidated Line following pDEcc error. */
#define KVX_SFR_MES_PILDE_SHIFT 2
#define KVX_SFR_MES_PILDE_WIDTH 1
#define KVX_SFR_MES_PILDE_WFXL_MASK _ULL(0x4)
#define KVX_SFR_MES_PILDE_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_MES_PILDE_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_MES_PILPA_MASK _ULL(0x8) /* Program cache Invalidated Line following pPArity error. */
#define KVX_SFR_MES_PILPA_SHIFT 3
#define KVX_SFR_MES_PILPA_WIDTH 1
#define KVX_SFR_MES_PILPA_WFXL_MASK _ULL(0x8)
#define KVX_SFR_MES_PILPA_WFXL_CLEAR _ULL(0x8)
#define KVX_SFR_MES_PILPA_WFXL_SET _ULL(0x800000000)

#define KVX_SFR_MES_DSE_MASK _ULL(0x10) /* Data Simple Ecc */
#define KVX_SFR_MES_DSE_SHIFT 4
#define KVX_SFR_MES_DSE_WIDTH 1
#define KVX_SFR_MES_DSE_WFXL_MASK _ULL(0x10)
#define KVX_SFR_MES_DSE_WFXL_CLEAR _ULL(0x10)
#define KVX_SFR_MES_DSE_WFXL_SET _ULL(0x1000000000)

#define KVX_SFR_MES_DILSY_MASK _ULL(0x20) /* Data cache Invalidated Line following dSYs error. */
#define KVX_SFR_MES_DILSY_SHIFT 5
#define KVX_SFR_MES_DILSY_WIDTH 1
#define KVX_SFR_MES_DILSY_WFXL_MASK _ULL(0x20)
#define KVX_SFR_MES_DILSY_WFXL_CLEAR _ULL(0x20)
#define KVX_SFR_MES_DILSY_WFXL_SET _ULL(0x2000000000)

#define KVX_SFR_MES_DILDE_MASK _ULL(0x40) /* Data cache Invalidated Line following dDEcc error. */
#define KVX_SFR_MES_DILDE_SHIFT 6
#define KVX_SFR_MES_DILDE_WIDTH 1
#define KVX_SFR_MES_DILDE_WFXL_MASK _ULL(0x40)
#define KVX_SFR_MES_DILDE_WFXL_CLEAR _ULL(0x40)
#define KVX_SFR_MES_DILDE_WFXL_SET _ULL(0x4000000000)

#define KVX_SFR_MES_DILPA_MASK _ULL(0x80) /* Data cache Invalidated Line following dPArity error. */
#define KVX_SFR_MES_DILPA_SHIFT 7
#define KVX_SFR_MES_DILPA_WIDTH 1
#define KVX_SFR_MES_DILPA_WFXL_MASK _ULL(0x80)
#define KVX_SFR_MES_DILPA_WFXL_CLEAR _ULL(0x80)
#define KVX_SFR_MES_DILPA_WFXL_SET _ULL(0x8000000000)

#define KVX_SFR_MES_DDEE_MASK _ULL(0x100) /* Data DEcc Error. */
#define KVX_SFR_MES_DDEE_SHIFT 8
#define KVX_SFR_MES_DDEE_WIDTH 1
#define KVX_SFR_MES_DDEE_WFXL_MASK _ULL(0x100)
#define KVX_SFR_MES_DDEE_WFXL_CLEAR _ULL(0x100)
#define KVX_SFR_MES_DDEE_WFXL_SET _ULL(0x10000000000)

#define KVX_SFR_MES_DSYE_MASK _ULL(0x200) /* Data dSYs Error. */
#define KVX_SFR_MES_DSYE_SHIFT 9
#define KVX_SFR_MES_DSYE_WIDTH 1
#define KVX_SFR_MES_DSYE_WFXL_MASK _ULL(0x200)
#define KVX_SFR_MES_DSYE_WFXL_CLEAR _ULL(0x200)
#define KVX_SFR_MES_DSYE_WFXL_SET _ULL(0x20000000000)

#define KVX_SFR_WS_WU0_MASK _ULL(0x1) /* Wake-Up 0 */
#define KVX_SFR_WS_WU0_SHIFT 0
#define KVX_SFR_WS_WU0_WIDTH 1
#define KVX_SFR_WS_WU0_WFXL_MASK _ULL(0x1)
#define KVX_SFR_WS_WU0_WFXL_CLEAR _ULL(0x1)
#define KVX_SFR_WS_WU0_WFXL_SET _ULL(0x100000000)

#define KVX_SFR_WS_WU1_MASK _ULL(0x2) /* Wake-Up 1 */
#define KVX_SFR_WS_WU1_SHIFT 1
#define KVX_SFR_WS_WU1_WIDTH 1
#define KVX_SFR_WS_WU1_WFXL_MASK _ULL(0x2)
#define KVX_SFR_WS_WU1_WFXL_CLEAR _ULL(0x2)
#define KVX_SFR_WS_WU1_WFXL_SET _ULL(0x200000000)

#define KVX_SFR_WS_WU2_MASK _ULL(0x4) /* Wake-Up 2 */
#define KVX_SFR_WS_WU2_SHIFT 2
#define KVX_SFR_WS_WU2_WIDTH 1
#define KVX_SFR_WS_WU2_WFXL_MASK _ULL(0x4)
#define KVX_SFR_WS_WU2_WFXL_CLEAR _ULL(0x4)
#define KVX_SFR_WS_WU2_WFXL_SET _ULL(0x400000000)

#define KVX_SFR_IPE_FE_MASK _ULL(0xffff) /* Forward Events */
#define KVX_SFR_IPE_FE_SHIFT 0
#define KVX_SFR_IPE_FE_WIDTH 16
#define KVX_SFR_IPE_FE_WFXL_MASK _ULL(0xffff)
#define KVX_SFR_IPE_FE_WFXL_CLEAR _ULL(0xffff)
#define KVX_SFR_IPE_FE_WFXL_SET _ULL(0xffff00000000)

#define KVX_SFR_IPE_BE_MASK _ULL(0xffff0000) /* Backward Events */
#define KVX_SFR_IPE_BE_SHIFT 16
#define KVX_SFR_IPE_BE_WIDTH 16
#define KVX_SFR_IPE_BE_WFXL_MASK _ULL(0xffff0000)
#define KVX_SFR_IPE_BE_WFXL_CLEAR _ULL(0xffff0000)
#define KVX_SFR_IPE_BE_WFXL_SET _ULL(0xffff000000000000)

#define KVX_SFR_IPE_FM_MASK _ULL(0xffff00000000) /* Forward Mode */
#define KVX_SFR_IPE_FM_SHIFT 32
#define KVX_SFR_IPE_FM_WIDTH 16
#define KVX_SFR_IPE_FM_WFXM_MASK _ULL(0xffff00000000)
#define KVX_SFR_IPE_FM_WFXM_CLEAR _ULL(0xffff)
#define KVX_SFR_IPE_FM_WFXM_SET _ULL(0xffff00000000)

#define KVX_SFR_IPE_BM_MASK _ULL(0xffff000000000000) /* Backward Modes */
#define KVX_SFR_IPE_BM_SHIFT 48
#define KVX_SFR_IPE_BM_WIDTH 16
#define KVX_SFR_IPE_BM_WFXM_MASK _ULL(0xffff000000000000)
#define KVX_SFR_IPE_BM_WFXM_CLEAR _ULL(0xffff0000)
#define KVX_SFR_IPE_BM_WFXM_SET _ULL(0xffff000000000000)

#endif/*_ASM_KVX_SFR_DEFS_H */

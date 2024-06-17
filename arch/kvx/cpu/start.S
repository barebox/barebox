/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <linux/linkage.h>
#include <asm/privilege.h>
#include <asm/sys_arch.h>

#define PS_VAL_WFXL(__field, __val) \
	SFR_SET_VAL_WFXL(PS, __field, __val)

#define PS_WFXL_START_VALUE	PS_VAL_WFXL(HLE, 1) | \
				PS_VAL_WFXL(USE, 1) | \
				PS_VAL_WFXL(DCE, 1) | \
				PS_VAL_WFXL(ICE, 1) | \
				PS_VAL_WFXL(V64, 1) | \
				PS_VAL_WFXL(ET, 0)

#define PCR_VAL_WFXM(__field, __val) \
	SFR_SET_VAL_WFXM(PCR, __field, __val)

#define PCR_WFXM_START_VALUE   PCR_VAL_WFXM(L1CE, 1)

/* Enable STOP in WS */
#define WS_ENABLE_WU2		(KVX_SFR_WS_WU2_MASK)

#define WS_WFXL_VALUE		(WS_ENABLE_WU2)

/*
 * This is our entry point. When entering from bootloader,
 * the following registers are set:
 * $r0 is a magic "KALARGV1" (FSBL_PARAM_MAGIC) indicating parameters are passed
 * $r1 Device tree pointer
 *
 * WARNING WARNING WARNING
 * ! DO NOT CLOBBER THEM !
 * WARNING WARNING WARNING
 *
 * Try to use register above $r20 to ease parameter adding in future
 */
.section .startup, "ax"
ENTRY(kvx_start)
	/* (Re)initialize performance counter */
	make $r20 = 0x00000000
	;;
	set $pmc = $r20
	;;
	call asm_init_pl
	;;
	/* Setup default processor status */
	make $r25 = PS_WFXL_START_VALUE
	/* Make sure there is no outstanding
	 * load(s)/store(s) before dcache enable
	 */
	fence
	;;
	wfxl $ps, $r25
	;;
	/* Make sure icache starts clean */
	i1inval
	;;
	barrier
	;;
	make $r25 = PCR_WFXM_START_VALUE
	/* Make sure dcache starts clean */
	d1inval
	;;
	wfxm $pcr, $r25
	;;
	/* Clear BSS */
	make $r22 = __bss_stop
	make $r21 = __bss_start
	;;
	sbfd $r22 = $r21, $r22
	make $r24 = 0
	;;
	/* Divide by 16 for hardware loop */
	srld $r22 = $r22, 4
	make $r25 = 0
	;;
	/* Clear bss with hardware loop */
        loopdo $r22, clear_bss_done
		;;
		sq 0[$r21] = $r24r25
		addd $r21 = $r21, 16
		;;
	clear_bss_done:
	/* Setup stack */
	make $sp = __stack_start
	;;
	call kvx_lowlevel_setup
	;;
	call kvx_start_barebox
	;;
	goto kvx_proc_power_off
	;;
ENDPROC(kvx_start)

#define request_ownership(__pl) ;\
	make $r21 = SYO_WFXL_VALUE_##__pl ;\
	;; ;\
	wfxl $syow, $r21 ;\
	;; ;\
	make $r21 = HTO_WFXL_VALUE_##__pl ;\
	;; ;\
	wfxl $htow, $r21 ;\
	;; ;\
	make $r21 = MO_WFXL_VALUE_##__pl ;\
	make $r22 = MO_WFXM_VALUE_##__pl ;\
	;; ;\
	wfxl $mow, $r21 ;\
	;; ;\
	wfxm $mow, $r22 ;\
	;; ;\
	make $r21 = ITO_WFXL_VALUE_##__pl ;\
	make $r22 = ITO_WFXM_VALUE_##__pl ;\
	;; ;\
	wfxl $itow, $r21 ;\
	;; ;\
	wfxm $itow, $r22 ;\
	;; ;\
	make $r21 = PSO_WFXL_VALUE_##__pl ;\
	make $r22 = PSO_WFXM_VALUE_##__pl ;\
	;; ;\
	wfxl $psow, $r21 ;\
	;; ;\
	wfxm $psow, $r22 ;\
	;; ;\
	make $r21 = DO_WFXL_VALUE_##__pl ;\
	;; ;\
	wfxl $dow, $r21 ;\
	;;

/**
 * Initialize privilege level
 */
ENTRY(asm_init_pl)
	get $r21 = $ps
	;;
	/* Extract privilege level from $ps to check if we need to
	 * lower our privilege level (we might already be in PL1)
	 */
	extfz $r20 = $r21, KVX_SFR_END(PS_PL), KVX_SFR_START(PS_PL)
	;;
	/* If our privilege level is 0, then we need to lower in execution level
	 * to ring 1 in order to let the debug routines be inserted at runtime
	 * by the JTAG. In both case, we will request the resources we need for
	 * barebox to run.
	 */
	cb.deqz $r20? delegate_pl
	;;
	/*
	 * When someone is already above us, request the resources we need to
	 * run barebox . No need to request double exception or ECC traps for
	 * instance. When doing so, the more privileged level will trap for
	 * permission and delegate us the required resources.
	 */
	request_ownership(PL_CUR)
	;;
	ret
	;;
delegate_pl:
	request_ownership(PL_CUR_PLUS_1)
	;;
	/* Copy our $ps into $sps for 1:1 restoration */
	get $r22 = $ps
	;;
	/* We will return to $ra after rfe */
	get $r21 = $ra
	/* Set privilege level to +1 in $sps (relative level from the
	 * current one)
	 */
	addd $r22 = $r22, PL_CUR_PLUS_1
	;;
	set $spc = $r21
	;;
	set $sps = $r22
	;;
	/* When using rfe, $spc and $sps will be restored in $ps and $pc,
	 * We will then return to the caller ($ra) in current PL + 1
	 */
	rfe
	;;
ENDPROC(asm_init_pl)

ENTRY(kvx_proc_power_off):
	d1inval
	make $r1 = WS_WFXL_VALUE
	;;
        /* Enable STOP */
	wfxl $ws, $r1
	;;
1:	stop
	;;
	goto 1b
	;;
ENDPROC(kvx_proc_power_off)

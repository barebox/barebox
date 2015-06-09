@r_shadow@
type T;
@@

- static T curr_shadow_reg = 0;

@r_out2_delay@
type T;
@@

- T scc_mgr_apply_group_dq_out2_delay(...)
- {
- ...
- }

@r_oct_out2@
type T;
@@
- T scc_mgr_apply_group_dqs_io_and_oct_out2(...)
- {
- ...
- }

@r_oct_out2_gradual@
type T;
@@

- T scc_mgr_set_group_dqs_io_and_oct_out2_gradual(...)
- {
- ...
- }

@r_eye_diag@
type T;
@@

- T rw_mgr_mem_calibrate_eye_diag_aid(...)
- {
- ...
- }

@r_full_test@
type T;
@@

- T rw_mgr_mem_calibrate_full_test(...)
- {
- ...
- }

@r_user_init_cal_req@
@@

- static void user_init_cal_req (...)
- {
- ...
- }

@r_tcl@
@@

- TCLRPT_SET(...);

@r_bfm@
@@

- BFM_STAGE(...);

@r_trace@
@@

- TRACE_FUNC(...);

@r_assert@
@@

- ALTERA_ASSERT(...);

@r_if@
@@

- if (...) {}

@r_if_else@
@@

- if (...) {
- } else {
- }

@r_for@
@@

- for (...;...;...) {}

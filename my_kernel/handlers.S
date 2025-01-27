.section ".text.handlers"

/*
--------------------
 Current EL, SP_EL0
--------------------
 */
.global sync_current_el_sp_el0_handler
.global irq_current_el_sp_el0_handler
.global fiq_current_el_sp_el0_handler
.global serr_current_el_sp_el0_handler

sync_current_el_sp_el0_handler:
    bl _syscallHandler

irq_current_el_sp_el0_handler:
    bl _syscallHandler

fiq_current_el_sp_el0_handler:
    bl _syscallHandler

serr_current_el_sp_el0_handler:
    bl _syscallHandler

/*
 --------------------
 Current
 --------------------
 */
.global sync_current_el_sp_elx_handler
.global irq_current_el_sp_elx_handler
.global fiq_current_el_sp_elx_handler
.global serr_current_el_sp_elx_handler

sync_current_el_sp_elx_handler:
    bl _syscallHandler

irq_current_el_sp_elx_handler:
    bl _syscallHandler

fiq_current_el_sp_elx_handler:
    bl _syscallHandler

serr_current_el_sp_elx_handler:
    bl _syscallHandler

/*
 --------------------
 Lower EL, AArch64
 --------------------
 */
.global sync_lower_el_aarch64_handler
.global irq_lower_el_aarch64_handler
.global fiq_lower_el_aarch64_handler
.global serr_lower_el_aarch64_handler

sync_lower_el_aarch64_handler:
    bl _syscallHandler
irq_lower_el_aarch64_handler:
    bl _syscallHandler
fiq_lower_el_aarch64_handler:
    bl _syscallHandler
serr_lower_el_aarch64_handler:
    bl _syscallHandler

/*
 --------------------
 Lower
 ---------------------
 */
.global sync_lower_el_aarch32_handler
.global irq_lower_el_aarch32_handler
.global fiq_lower_el_aarch32_handler
.global serr_lower_el_aarch32_handler

sync_lower_el_aarch32_handler:
    bl _syscallHandler
irq_lower_el_aarch32_handler:
    bl _syscallHandler
fiq_lower_el_aarch32_handler:
    bl _syscallHandler
serr_lower_el_aarch32_handler:
    bl _syscallHandler
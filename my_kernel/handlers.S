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
    bl _dummyHandler

irq_current_el_sp_el0_handler:
    bl _dummyHandler

fiq_current_el_sp_el0_handler:
    bl _dummyHandler

serr_current_el_sp_el0_handler:
    bl _dummyHandler

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
    bl _dummyHandler

irq_current_el_sp_elx_handler:
    bl _dummyHandler

fiq_current_el_sp_elx_handler:
    bl _dummyHandler

serr_current_el_sp_elx_handler:
    bl _dummyHandler

/*
 --------------------
 Lower EL, AArch64
 --------------------
 */
.global sync_lower_el_aarch64_handler
.global irq_lower_el_aarch64_handler
.global fiq_lower_el_aarch64_handler
.global serr_lower_el_aarch64_handler


// THIS IS THE MAIN HANDLER FOR A1
sync_lower_el_aarch64_handler:
    // SVC <N> -> N is stored in ESR_EL1[15:0]
    mrs x0, esr_el1
    and x0, x0, #0xFFFF

    bl _syscallHandler


irq_lower_el_aarch64_handler:
    bl _dummyHandler
fiq_lower_el_aarch64_handler:
    bl _dummyHandler
serr_lower_el_aarch64_handler:
    bl _dummyHandler

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
    bl _dummyHandler
irq_lower_el_aarch32_handler:
    bl _dummyHandler
fiq_lower_el_aarch32_handler:
    bl _dummyHandler
serr_lower_el_aarch32_handler:
    bl _dummyHandler
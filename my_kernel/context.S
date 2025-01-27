.macro SAVE_FULL_STATE stack_ptr
    stp x29, x30, [\stack_ptr, #-16]!   // Save FP and LR
    stp x27, x28, [\stack_ptr, #-16]!   // Save X27 and X28
    stp x25, x26, [\stack_ptr, #-16]!   // Save X25 and X26
    stp x23, x24, [\stack_ptr, #-16]!   // Save X23 and X24
    stp x21, x22, [\stack_ptr, #-16]!   // Save X21 and X22
    stp x19, x20, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x17, x18, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x15, x16, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x13, x14, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x11, x12, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x9, x10, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x7, x8, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x5, x6, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x3, x4, [\stack_ptr, #-16]!   // Save X19 and X20
    stp x1, x2, [\stack_ptr, #-16]!   // Save X19 and X20
    str x0, [\stack_ptr, #-8]!         // Save X0
.endm

.macro RESTORE_FULL_STATE stack_ptr
    ldr x0, [\stack_ptr], #8          // Restore X0
    ldp x1, x2, [\stack_ptr], #16     // Restore X1 and X2
    ldp x3, x4, [\stack_ptr], #16     // Restore X3 and X4
    ldp x5, x6, [\stack_ptr], #16     // Restore X5 and X6
    ldp x7, x8, [\stack_ptr], #16     // Restore X7 and X8
    ldp x9, x10, [\stack_ptr], #16    // Restore X9 and X10
    ldp x11, x12, [\stack_ptr], #16   // Restore X11 and X12
    ldp x13, x14, [\stack_ptr], #16   // Restore X13 and X14
    ldp x15, x16, [\stack_ptr], #16   // Restore X15 and X16
    ldp x17, x18, [\stack_ptr], #16   // Restore X17 and X18
    ldp x19, x20, [\stack_ptr], #16    // Restore X19 and X20
    ldp x21, x22, [\stack_ptr], #16    // Restore X21 and X22
    ldp x23, x24, [\stack_ptr], #16    // Restore X23 and X24
    ldp x25, x26, [\stack_ptr], #16    // Restore X25 and X26
    ldp x27, x28, [\stack_ptr], #16    // Restore X27 and X28
    ldp x29, x30, [\stack_ptr], #16    // Restore FP and LR
.endm

.macro SAVE_PROCESSOR_STATE stack_ptr
    mrs x16, spsr_el1                // Save SPSR_EL1 (processor state)
    str x16, [\stack_ptr, #-8]!      // Push onto stack
    mrs x16, elr_el1                 // Save ELR_EL1 (return address)
    str x16, [\stack_ptr, #-8]!      // Push onto stack
.endm


.macro RESTORE_PROCESSOR_STATE stack_ptr
    ldr x16, [\stack_ptr], #8        // Pop ELR_EL1 (return address)
    msr elr_el1, x16                 // Restore ELR_EL1
    ldr x16, [\stack_ptr], #8        // Pop SPSR_EL1 (processor state)
    msr spsr_el1, x16                // Restore SPSR_EL1

    msr sp_el0, \stack_ptr                  // Restore SP_EL0
.endm

.section ".text.context"
.global _context_switch
_context_switch:
    // Argument: x0 = task's SP

    // SAVE KERNEL CONTEXT
// SAVE_PROCESSOR_STATE sp
// SAVE_FULL_STATE sp

    // WE ARE CURRENTLT IN EL1
    RESTORE_FULL_STATE x0
    RESTORE_PROCESSOR_STATE x0

    
    eret                   // Return from exception


.section ".text.debug"
.global _get_el_debug
_get_el_debug:
    mrs x0, CurrentEL
    and x0, x0, #0xC
    lsr x0, x0, #2
    ret
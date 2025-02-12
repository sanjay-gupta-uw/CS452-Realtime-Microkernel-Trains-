.equ IRQ, 11  // define IRQ value

.macro transition_to_kernel isIRQ
    // x0 and x1 contain arguments to pass to kernel
    ldr x16, [sp], #16 // pop the task context pointer

    str x0, [x16, #0]  // save x0 arg
    add x0, x16, #8 // make x0 point to x1 location
    
    
    // store all the registers
    stp x1, x2, [x0], #16
    stp x3, x4, [x0], #16
    stp x5, x6, [x0], #16
    stp x7, x8, [x0], #16
    stp x9, x10, [x0], #16
    stp x11, x12, [x0], #16
    stp x13, x14, [x0], #16
    stp x15, x16, [x0], #16
    stp x17, x18, [x0], #16
    stp x19, x20, [x0], #16
    stp x21, x22, [x0], #16
    stp x23, x24, [x0], #16
    stp x25, x26, [x0], #16
    stp x27, x28, [x0], #16
    stp x29, x30, [x0], #16

    // CONTEXT:
    /*
    struct context_t
    {
        int64_t registers[31]; // x0 to x30
        uint64_t spsr;         // saved program status reg
        uint64_t stack_pointer;
        uint64_t exception_lr;
    };
     */
    
    // now save special registers
    mrs x1, spsr_el1
    str x1, [x0], #8
    mrs x1, sp_el0
    str x1, [x0], #8
    mrs x1, elr_el1
    str x1, [x0], #8

    // return to kernel
    ldr x0, [sp], #16
    add x0, x0, #8
    ldp x1, x2, [x0], #16 
    ldp x3, x4, [x0], #16 
    ldp x5, x6, [x0], #16 
    ldp x7, x8, [x0], #16 
    ldp x9, x10, [x0], #16
    ldp x11, x12, [x0], #16
    ldp x13, x14, [x0], #16
    ldp x15, x16, [x0], #16
    ldp x17, x18, [x0], #16
    ldp x19, x20, [x0], #16
    ldp x21, x22, [x0], #16
    ldp x23, x24, [x0], #16
    ldp x25, x26, [x0], #16
    ldp x27, x28, [x0], #16
    ldp x29, x30, [x0], #16

    .if \isIRQ
        mov x0, IRQ
    .else 
        mrs x0, esr_el1 // return ESR_EL1 to 0
    .endif
    
    ret // return to kernel
.endm


.global kernel_to_task_asm
.balign 16
kernel_to_task_asm:
    // X0 contains pointer to kernel context: 0x7fec8
    // X1 contains pointer to task context: 0x84f28

    // sp: 0x7fe70
    str x0, [sp, #-16]! // Store the kernel context pointer on the stack (sp -= 16, [sp] = x0)
    str x1, [sp, #-16]! // Store the task context pointer on the stack (sp -= 16, [sp] = x1)
    // sp: 0x7fe50
    
    // store registers in kernel_context
    add x0, x0, #8 // X0: 0x7fed0
    
    stp x1, x2, [x0], #16 // X0: 0x7fee0
    stp x3, x4, [x0], #16 // X0: 0x7fef0
    stp x5, x6, [x0], #16 // X0: 0x7ff00
    stp x7, x8, [x0], #16 // X0: 0x7ff10
    stp x9, x10, [x0], #16 // X0: 0x7ff20
    stp x11, x12, [x0], #16 // X0: 0x7ff30
    stp x13, x14, [x0], #16 // X0: 0x7ff40
    stp x15, x16, [x0], #16 // X0: 0x7ff50
    stp x17, x18, [x0], #16 // X0: 0x7ff60
    stp x19, x20, [x0], #16 // X0: 0x7ff70
    stp x21, x22, [x0], #16 // X0: 0x7ff80
    stp x23, x24, [x0], #16 // X0: 0x7ff90
    stp x25, x26, [x0], #16 // X0: 0x7ffa0
    stp x27, x28, [x0], #16 // X0: 0x7ffb0
    stp x29, x30, [x0], #16 // X0: 0x7ffc0
    // X0: 7ffC0

    // CONTEXT:
    /*
    struct context_t
    {
        int64_t registers[31]; // x0 to x30
        uint64_t spsr;         // saved program status reg
        uint64_t stack_pointer;
        uint64_t exception_lr;
    };
     */
    // 31 * 8 = 248 since each register is 8 bytes
    add x1, x1, #248 // now x1 points to the spsr
    ldr x0, [x1, #0]
    msr SPSR_EL1, x0
    ldr x0, [x1, #8]
    msr SP_EL0, x0
    ldr x0, [x1, #16]
    msr ELR_EL1, x0


    // now we restore the task context
    ldr x0, [sp] // (pointer to task context)

    add x0, x0, #8
    ldp x1, x2, [x0], #16
    ldp x3, x4, [x0], #16
    ldp x5, x6, [x0], #16
    ldp x7, x8, [x0], #16
    ldp x9, x10, [x0], #16
    ldp x11, x12, [x0], #16
    ldp x13, x14, [x0], #16
    ldp x15, x16, [x0], #16
    ldp x17, x18, [x0], #16
    ldp x19, x20, [x0], #16
    ldp x21, x22, [x0], #16
    ldp x23, x24, [x0], #16
    ldp x25, x26, [x0], #16
    ldp x27, x28, [x0], #16
    ldp x29, x30, [x0], #16
    

    // lastly restore x0
    ldr x0, [sp] // get the pointer
    ldr x0, [x0] // dereference it (load the value from the address)

    eret // return from exception: EL1->EL0
    




.global task_to_kernel_asm
.balign 16
task_to_kernel_asm:
    transition_to_kernel 0

.global irq_to_kernel_asm
.balign 16
irq_to_kernel_asm:
    transition_to_kernel 1

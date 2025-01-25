/*
exception return addr stored in ELR (Exception Link Register) 
-> X30 is link register (LR)
SPSR_ELx contains the saved PSTATE immediately before the exception was taken
ESR updated by synchronous exceptions or SError interrupts
FAR_ELx contains MMU faults
->
PE:
Automatically preserves the state of the current exception level in the ELR_ELx and SPSR_ELx registers

-> if in EL0 and exception is taken, the processor saves the current EL0 state in the ELR_EL1 and SPSR_EL1 registers

@ The values of the VBAR registers are undefined after reset, so they must be configured before interrupts are enabled.
@ https://developer.arm.com/documentation/102412/0103/Handling-exceptions/Taking-an-exception
@ table 4 for VBAR addrs
@ figure 4 also helpful

@ eret: Exception Return

SP: PSTATE.SP = 0 or 1

@ XZR/WZR: Zero Register

@ SP refers to the stack pointer register in current exception level


@ post index processing is useful for popping off stack

@ RESEARCH:
@ The TBZ and TBNZ instructions work in a similar way to the CBZ and CBNZ instructions, but test the single bit specified by <imm> rather than the entire register value.


@ In C++, X0 is used to pass the implicit this pointer that points to the called function.: https://developer.arm.com/documentation/102374/0102/Procedure-Call-Standard

 */
.section ".text.context"
.global _context_switch
_context_switch:
   
   ret


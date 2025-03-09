// ***************************************
// SCTLR_EL1, System Control Register (EL1)
// Architecture Reference Manual Section D17.2.118
// ***************************************
#define SCTLR_RESERVED   (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_WFE_WFI    (1 << 18) | (1 << 16)

// ***************************************
// CPACR_EL1, Architectural Feature Access Control Register (EL1)
// Architecture Reference Manual Section D17.2.30
// CPTR_EL2, Architectural Feature Trap Register (EL2)
// Architecture Reference Manual Section D17.2.31
// ***************************************
#define FPEN_TRAP (3 << 20)
#define  ZEN_TRAP (3 << 16)

// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2)
// Architecture Reference Manual Section D17.2.48
// ***************************************
#define HCR_RW (1 << 31)

// ***************************************
// SPSR_EL2, Saved Program Status Register (EL2)
// Architecture Reference Manual Section C5.2.19
// ***************************************

#define SPSR_MASK_ALL (11 << 6) // mask D,I,F
#define SPSR_EL1       (5 << 0) // EL1 with SP_EL1
#define GICD_ENABLE (3) // forward group0 and group1 interrupts to the CPU interface

// ensure the linker puts this at the start of the image
.section ".text.boot"
.global _start
_start:
	// if this is not Core 0, exit to wfe-loop
	mrs  x0, mpidr_el1
	and  x0, x0, #3
	cbnz x0, exit

	// are we already in EL1?
	mrs	x1, CurrentEL
	and	x1, x1, #8 // XXXX AND 1000 (check if bit is set)
	cbz	x1, el1_entry

	// disable EL1 trap on FP/SIMD/SVE instructions
	ldr x2, =(FPEN_TRAP | ZEN_TRAP)
	msr cptr_el2, x2

	// set execution state for EL1 to AArch64
	ldr x3, =HCR_RW
	msr hcr_el2, x3

	// switch to EL1 by fake exception return
	ldr x4, =(SPSR_MASK_ALL | SPSR_EL1)
	msr spsr_el2, x4

	adr x5, el1_entry
	msr elr_el2, x5

	eret // -> el1_entry

.global el1_entry
el1_entry:
	// set up vector table
	ldr x0, =vectors
	msr vbar_el1, x0
	// configure processor
	ldr x0, =(SCTLR_RESERVED | SCTLR_WFE_WFI)
	msr sctlr_el1, x0
	
	// disable EL0 trap on FP/SIMD/SVE instructions
	ldr x1, =(FPEN_TRAP | ZEN_TRAP)
	msr cpacr_el1, x1

	// initialize SP (initial stack below text here)
	ldr x2, =_start
	mov sp, x2
	
	// Set SPSel to 1
	mov x3, 1
	msr spsel, x3

	bl kmain

.global _reboot
_reboot:
	// use watchdog to reboot
	mov x2, 36
	mov x0, 28
	movk	x2, 0xfe10, lsl 16
	movk	x0, 0xfe10, lsl 16
	mov w3, 1
	mov w1, 32
	movk	w3, 0x5a00, lsl 16
	str w3, [x2]
	movk	w1, 0x5a00, lsl 16
	str w1, [x0]
	// shouldn't be reached

exit:
	// nop until event
	wfe 
	b exit




.section ".text.vectors"
.balign 0x800 // 2KB alignment
vectors:
    // ---------------------
    // Current EL, SP_EL0
    // ---------------------
    eret  // VBAR_ELx + 0x000
    .balign 0x80
    b _dummyHandler1  // VBAR_ELx + 0x080
    .balign 0x80
    eret  // VBAR_ELx + 0x100
    .balign 0x80
    eret // VBAR_ELx + 0x180
    .balign 0x80

    // ---------------------
    // Current EL, SP_ELx
    // ---------------------
    eret  // VBAR_ELx + 0x200
    .balign 0x80
    mov x0, #2
    b _dummyHandler2  // VBAR_ELx + 0x280
    .balign 0x80
    mov x0, #3
    eret   // VBAR_ELx + 0x300
    .balign 0x80
    mov x0, #4
    eret  // VBAR_ELx + 0x380
    .balign 0x80

    // ---------------------
    // Lower EL, AArch64
    // ---------------------
    b task_to_kernel_asm  // VBAR_ELx + 0x400
    .balign 0x80
    b irq_to_kernel_asm  // VBAR_ELx + 0x480
    // b _dummyHandler3
    .balign 0x80
    eret   // VBAR_ELx + 0x500
    .balign 0x80
    eret  // VBAR_ELx + 0x580
    .balign 0x80

    // ---------------------
    // Lower EL, AArch32
    // ---------------------
    eret  // VBAR_ELx + 0x600
    .balign 0x80
    b _dummyHandler4
    .balign 0x80
    eret   // VBAR_ELx + 0x700
    .balign 0x80
    eret  // VBAR_ELx + 0x780
    .balign 0x80
 

 .global fetch_sp
fetch_sp:
    mov x0, sp
    ret
    
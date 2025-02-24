#ifndef _interrupts_h
#define _interrupts_h

/*
file:///Users/sanjaygupta/Downloads/IHI0048B_b_gic_architecture_specification%20(2).pdf
(p24 - 2.2 The Distributor)
Distributor:
- Globally enable the forwarding of interrupts to CPU interfaces
- Enable/Disable each interrupt
- Set priority of each interrupt
- Set the target CPU for each interrupt
- Set the interrupt type (level or edge triggered)
- Set the interrupt group (group 0 or group 1)
- Forwarding SGIs to processor

- visibility of interrupt state
- set/clear pending state


// SGI uses sender processor id, SGI number (0-15), and target id
// in multu processor, sender/target are 0-7
// in single processor, sender/target are 0 (this is what we are using)

// 1020 - 1023 are reserved for special purposes (3-43 for more info)

// Convention:
// use 0-7 for non-secure interrupts
// use 8-15 for secure interrupts

*/

/*
Interfaces:
- SIGNAL INTERRUPTS to processor is implementation defined, but traditional
mecahnism is to assert nIRQ/nFIQ
   - Then processor aknowledges the interrupt (p26) -> returns id of pending interrupt
   - Might return spurious interrupt (special value)


- GICD_ISENABLER0!!


- Identify supported interrupts (3.1.2)

- "Before interrupts can be used, the GIC must be configured to assign interrupt priorities and enable the desired interrupts"
   - https://www.realdigital.org/doc/87be521204ed2c4447d567b666961ce8#configuring-the-arm-processor-to-receive-interrupts


 11 = 0b1011
 11 << 6 -> 0b 0010 1100 0000
 9 << 6 -> 0b 0010 0100 0000


   @ #define SPSR_MASK_ALL (11 << 6) // mask E,I,F
   ldr x4, =(SPSR_MASK_ALL | SPSR_EL1)
   msr spsr_el2, x4
   adr x5, el1_entry
   msr elr_el2, x5


*/

void enable_irq();
void disable_irq();

void InitGIC();

#endif // _interrupts_h

//  shared_constant.h for the syscall numbers

.global CREATE
.balign 16
CREATE:
    svc #0
    ret

.global MYTID
.balign 16
MYTID:
    svc #1
    ret

.global MYPARENTTID
.balign 16
MYPARENTTID:
    svc #2
    ret

.global YIELD
.balign 16
YIELD:
    svc #3
    ret

.global EXIT
.balign 16
EXIT:
    svc #4
    ret


.global SEND
.balign 16
SEND:
    svc #5
    ret


.global RECEIVE
.balign 16
RECEIVE:
    svc #6
    ret


.global REPLY
.balign 16
REPLY:
    svc #7
    ret


.global ENABLE_ICACHE
.balign 16
ENABLE_ICACHE:
    svc #8
    ret

.global ENABLE_DCACHE
.balign 16
ENABLE_DCACHE:
    svc #9
    ret

.global ENABLE_BCACHE
.balign 16
ENABLE_BCACHE:
    svc #10
    ret

//.global GENERATE_SGI
//.balign 16
//GENERATE_SGI:
    //movz x0, #0x0F00
    //movk x0, #0xFF84, lsl 16  // Load GICD_SGIR address into x0
    //mov w1, #1                // Example SGI value
    //str w1, [x0]              // Store SGI command to GICD_SGIR
    //dsb sy
    //isb
    //ret


.global AWAITEVENT
.balign 16
AWAITEVENT:
    svc #11
    ret


// irq

.global GETIDLEPERCENT
.balign 16
GETIDLEPERCENT:
    svc #13
    ret

.global ABORT
.balign 16
ABORT:
    svc #14
    ret

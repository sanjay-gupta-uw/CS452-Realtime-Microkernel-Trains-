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

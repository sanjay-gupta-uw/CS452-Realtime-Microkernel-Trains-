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

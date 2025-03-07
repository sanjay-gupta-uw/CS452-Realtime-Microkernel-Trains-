.section ".text.util"
// get current Exception Level and store it in x0
.global _get_el_
_get_el_:
    mrs x0, CurrentEL
    lsr x0, x0, #2 // need to shift the value to get the EL
    ret


#ifndef _kassert_h_
#define _kassert_h_
#include "../kern/kabort.h" // kernel abort
#include "../include/syscall.h"

extern "C" int _get_el_();

// assert macro
#define kassert(condition)                          \
    if (!(condition))                               \
    {                                               \
        int EL = _get_el_();                        \
        kabort(#condition, __LINE__, __FILE__, EL); \
    }

#endif // _kassert_h_
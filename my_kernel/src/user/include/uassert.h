#ifndef _uassert_h_
#define _uassert_h_
#include "../../include/syscall.h"

#define uassert(condition)                                                                      \
    if (!(condition))                                                                           \
    {                                                                                           \
        ABORT((const char *)(#condition), sizeof(condition), __LINE__, (const char *)__FILE__); \
    }

#endif // _uassert_h_

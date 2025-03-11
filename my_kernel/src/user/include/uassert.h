#ifndef _uassert_h_
#define _uassert_h_
#include "../../include/syscall.h"
// #include "io.h"

#define uassert(condition)                                                                      \
    if (!(condition))                                                                           \
    {                                                                                           \
        ABORT((const char *)(#condition), sizeof(condition), __LINE__, (const char *)__FILE__); \
    }

#endif // _uassert_h_

// IO_NS::PrintTerminal("ASSERTION FAILED: %s at line %d in file %s\r\n", #condition, __LINE__, __FILE__);
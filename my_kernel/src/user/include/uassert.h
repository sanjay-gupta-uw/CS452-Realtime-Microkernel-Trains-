#ifndef _uassert_h_
#define _uassert_h_
#include "../../include/syscall.h"
#include "io.h"
#include "../../rpi.h"

#define uassert(condition)                                                                                                                      \
    if (!(condition))                                                                                                                           \
    {                                                                                                                                           \
        uart_printf(CONSOLE, RESTORE_CURSOR "\r\n UASSERT FAILED: {%s} at line %d in file %s\r\n" SAVE_CURSOR, #condition, __LINE__, __FILE__); \
        ABORT((const char *)(#condition), sizeof(condition), __LINE__, (const char *)__FILE__);                                                 \
    }

#endif // _uassert_h_

// // IO_NS::PrintTerminal("ASSERTION FAILED: %s at line %d in file %s\r\n", #condition, __LINE__, __FILE__);
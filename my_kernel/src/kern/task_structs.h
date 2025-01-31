#ifndef _task_structs_h_
#define _task_structs_h_

#include <stdint.h>

#define NUM_REGISTERS 31

struct Context
{
   uint64_t x[NUM_REGISTERS]; // general purpose registers
   uint64_t spsr;
   uint64_t sp;
   uint64_t elr;
};

typedef enum
{
   ACTIVE,
   READY,
   EXITED,
   SEND_BLOCKED,
   RECEIVE_BLOCKED,
   REPLY_BLOCKED,
   EVENT_BLOCKED,
} RunState;

#endif // _task_structs_h_
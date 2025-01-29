#ifndef _task_h_
#define _task_h_

#include "memory.h"
#include "../shared_constants.h"

#define MAX_TASKS MBLOCK_COUNT

extern "C" int _get_el_debug();

#define NUM_REGISTERS 31
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

struct Context
{
   uint64_t x[NUM_REGISTERS]; // general purpose registers
   uint64_t spsr;
   uint64_t sp;
   uint64_t elr;
};

class Kernel;

class TaskDescriptor
{
public:
   TaskDescriptor();
   ~TaskDescriptor();
   // use wrapper Create to call this
   int CreateTask(int priority, MemoryBlock *block, void (*function)());
   bool isReady();
   int getPriority();
   int getTid();
   TaskDescriptor *getParent();
   void setState(RunState state);
   RunState getState();
   void Print();

private:
   volatile Context context;
   int tid;
   int priority;
   TaskDescriptor *parent;
   RunState state;
   int block_index;

   int ret_val;

   friend class Kernel;
};

#endif // _td_h_
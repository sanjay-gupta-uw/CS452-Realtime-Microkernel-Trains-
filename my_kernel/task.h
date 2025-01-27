#ifndef _task_h_
#define _task_h_

#define MAX_TASKS 64

#include "memory.h"
#include "shared_constants.h"

extern "C" int _get_el_debug();

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
   MemoryBlock *getBlock();

   uintptr_t sp;
   uintptr_t spsr;
   int ret_val;

private:
   int tid;
   int priority;
   TaskDescriptor *parent;
   RunState state;
   MemoryBlock *block;
};

#endif // _td_h_
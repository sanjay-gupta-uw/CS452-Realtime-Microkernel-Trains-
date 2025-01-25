#ifndef _task_h_
#define _task_h_

#define MAX_TASKS 64

#include "memory.h"

// priority levels
typedef enum
{
   HIGH,
   MEDIUM,
   LOW,
   IDLE,
} Priority;

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

private:
   int tid;
   int priority;
   TaskDescriptor *parent;
   RunState state;
   uintptr_t sp;
};

void ContextSwitch();

#endif // _td_h_
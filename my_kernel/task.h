#ifndef _task_h_
#define _task_h_

#define MAX_TASKS 64

#include "memory.h"
extern "C" int _get_el_debug();
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
   int getPriority();

   uintptr_t sp;
   uintptr_t spsr;
   int ret_val;

   int tid;

private:
   int priority;
   TaskDescriptor *parent;
   RunState state;
};

void ContextSwitch(TaskDescriptor *td);
void Task1();
void Task2();

#endif // _td_h_
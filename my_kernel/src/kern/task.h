#ifndef _task_h_
#define _task_h_

#include "task_structs.h" // importing Context and Inbox
#include "memory.h"
#include "../shared_constants.h"
#include "../containers/ringbuffer.h"

#define MAX_TASKS MBLOCK_COUNT

class Kernel;

class TaskDescriptor
{
public:
   TaskDescriptor();
   ~TaskDescriptor();
   // use wrapper Create to call this
   int InitTask(int priority, MemoryBlock *block, void (*function)());
   bool isReady();
   int getPriority();
   int getTid();
   TaskDescriptor *getParent();
   void setState(RunState state);
   RunState getState();
   void Print();
   void SetRetval(int ret_val);

private:
   volatile Context context;
   int tid;
   int priority;
   TaskDescriptor *parent;
   RunState state;
   int block_index;
   RingBuffer<int> inbox;

   friend class Kernel;
};

#endif // _td_h_
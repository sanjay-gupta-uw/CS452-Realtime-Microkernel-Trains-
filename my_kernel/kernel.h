#ifndef _kernel_h_
#define _kernel_h_

#include "ringbuffer.h"
#include "task.h"
#include "memory.h"
#include "pqueue.h"

class Kernel
{
public:
   Kernel();
   ~Kernel();
   int Create(int priority, void (*function)()); // use free_tid to get a tid, create a new task, and push it to ready_queue
   int MyTid();
   int MyParentTid();
   void Yield();
   void Exit();      // relesase the task's tid and memory block, and push the task to free_tid
   void Scheduler(); // sets the next task
   void ContextSwitch();

private:
   void Dispatcher();

   TaskDescriptor *active_task;
   TaskDescriptor *next_task;

   TaskDescriptor task_table[MAX_TASKS];
   RingBuffer<int> free_tid;
   PQueue<TaskDescriptor *> ready_queue;
   MemoryManager mem_manager;
};

#endif
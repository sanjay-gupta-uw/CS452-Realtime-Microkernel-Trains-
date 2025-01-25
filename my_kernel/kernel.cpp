#include "kernel.h"

Kernel::Kernel()
{
   // initialize free_tid
   for (int i = 0; i < MAX_TASKS; i++)
   {
      free_tid.Push(&i);
   }

   // initilize default task
}

Kernel::~Kernel()
{
}

// returns -1 if invalid priority, -2 if no free tid, tid if success
int Kernel::Create(int priority, void (*function)())
{
   // check if priority is valid
   if (priority < HIGH || priority > IDLE)
   {
      return -2;
   }

   // get a free tid
   int tid;
   if (free_tid.Pop(&tid) == -1)
   {
      return -1;
   }

   // create a new task
   TaskDescriptor *td = &task_table[tid];
   MemoryBlock *stackBlock = mem_manager.Allocate();
   if (!stackBlock)
   {
      free_tid.Push(&tid); // Return TID to the free pool
      return -2;           // No memory available
   }
   td->CreateTask(priority, stackBlock, function);

   if (td->isReady())
   {
      ready_queue.Push(&td, priority);
   }

   return tid;
}

// returns the task id of the calling task.
// not implemented yet
int Kernel::MyTid()
{
   return -1;
}

// returns the task id of the task that created the calling task. This will be problematic only if the parent task has exited or been destroyed, in which case the return value is implementation-dependent.
// not implemented yet
int Kernel::MyParentTid()
{
   return -1;
}

// causes a task to pause executing. The task is moved to the end of its priority queue, and will resume executing when next scheduled.
// not implemented yet
void Kernel::Yield()
{
}

// causes a task to cease execution permanently. It is removed from all priority queues, send queues, receive queues and event queues. Resources owned by the task, primarily its memory and task descriptor, may be reclaimed.
// not implemented yet
void Kernel::Exit()
{
}
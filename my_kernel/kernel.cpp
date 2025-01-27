#include "kernel.h"
#include "rpi.h"

Kernel::Kernel()
{
   // initialize free_tid
   for (int i = 0; i < MAX_TASKS; i++)
   {
      free_tid.Push(i);
   }

   active_task = nullptr; // initilize active task
   next_task = nullptr;

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
      free_tid.Push(tid); // Return TID to the free pool
      return -2;          // No memory available
   }
   td->CreateTask(priority, stackBlock, function);

   if (td->isReady())
   {
      uart_printf(CONSOLE, "Task %d is ready and has priority %d\n", tid, priority);
      ready_queue.Push(&td, priority);
   }

   return tid;
}

// returns the task id of the calling task.
// not implemented yet
int Kernel::MyTid()
{
   if (active_task == nullptr)
   {
      return -1;
   }
   return active_task->getTid();
}

// returns the task id of the task that created the calling task. This will be problematic only if the parent task has exited or been destroyed, in which case the return value is implementation-dependent.
// not implemented yet
int Kernel::MyParentTid()
{
   if (active_task == nullptr)
   {
      return -1;
   }
   return (active_task->getParent())->getTid();
}

// causes a task to pause executing. The task is moved to the end of its priority queue, and will resume executing when next scheduled.
// not implemented yet
void Kernel::Yield()
{
   uart_printf(CONSOLE, "Yielding task %d\n", active_task->getTid());
   TaskDescriptor *td = active_task;
   td->setState(READY);
   ready_queue.Push(&td, td->getPriority());
   Scheduler();
}

// causes a task to cease execution permanently. It is removed from all priority queues, send queues, receive queues and event queues. Resources owned by the task, primarily its memory and task descriptor, may be reclaimed.
// not implemented yet
void Kernel::Exit()
{
   TaskDescriptor *td = active_task;
   td->setState(EXITED);
   // release the task's tid and memory block
   free_tid.Push(td->getTid());
   mem_manager.Free(td->getBlock());
   // MAKE SURE EXIT DOEs NOT ERET
}

void Kernel::Scheduler()
{
   // get the highest priority task
   TaskDescriptor *td;

   if (ready_queue.Pop(&td) == -1)
   {
      uart_printf(CONSOLE, "No tasks to run\n");
      return;
   }
   next_task = td;
   uart_printf(CONSOLE, "Next task is %d\n", next_task->getTid());
   Dispatcher();
}

void Kernel::Dispatcher()
{
   if (active_task != nullptr && active_task->getState() == READY)
   {
      // save task
      // SaveContext();

      ready_queue.Push(&active_task, active_task->getPriority());
   }

   // set the active task to the highest priority task
   active_task = next_task;
   uart_printf(CONSOLE, "SWITCHING TO %d\n", active_task->getTid());
   // ContextSwitch();
}

extern "C" void _context_switch(int sp, int spsr);

// KERNEL DOESNT GET SAVED, ONLY SAVE USER TASKS
void Kernel::ContextSwitch()
{
   active_task->setState(ACTIVE);
   // swap active with next
   asm volatile(
       "mov x1, %1\n"       // Load the next task's stack pointer into x0
       "mov x0, %0\n"       // Load the next task's SPSR into x1
       "bl _context_switch" // Call the assembly context switch routine
       :
       : "r"(active_task->sp), "r"(active_task->spsr));
}

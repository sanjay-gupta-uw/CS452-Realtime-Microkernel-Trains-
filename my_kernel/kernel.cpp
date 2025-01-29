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

   // initilize default task
}

Kernel::~Kernel()
{
}

inline void Kernel::SetRetval(int ret_val)
{
   active_task->context.x[0] = ret_val;
}

inline void Kernel::RepushActiveTask()
{
   if (active_task != nullptr)
   {
      ready_queue.Push(active_task->tid, active_task->priority);
   }
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
      return -3;          // No memory available
   }
   td->CreateTask(priority, stackBlock, function);
   td->parent = active_task;

   if (td->isReady())
   {
      ready_queue.Push(tid, priority);
   }
   RepushActiveTask();
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

   RepushActiveTask();
   return active_task->tid;
}

// returns the task id of the task that created the calling task. This will be problematic only if the parent task has exited or been destroyed, in which case the return value is implementation-dependent.
// not implemented yet
int Kernel::MyParentTid()
{
   if (active_task == nullptr)
   {
      return -1;
   }
   RepushActiveTask();
   return (active_task->parent->tid);
}

// causes a task to pause executing. The task is moved to the end of its priority queue, and will resume executing when next scheduled.
// not implemented yet
void Kernel::Yield()
{
   RepushActiveTask();
}

// causes a task to cease execution permanently. It is removed from all priority queues, send queues, receive queues and event queues. Resources owned by the task, primarily its memory and task descriptor, may be reclaimed.
// not implemented yet
void Kernel::Exit()
{
   TaskDescriptor *td = active_task;
   td->setState(EXITED);
   // release the task's tid and memory block
   free_tid.Push(td->tid);
   mem_manager.Free(td->block_index);
   // MAKE SURE EXIT DOEs NOT ERET
}

TaskDescriptor *
Kernel::Scheduler()
{
   // get the highest priority task
   int tid;

   if (ready_queue.Pop(&tid) == -1)
   {
      // uart_printf(CONSOLE, "No tasks to run\n");
      return nullptr;
   }

   return &task_table[tid];
}

extern "C" int kernel_to_task_asm(volatile Context *kernel, volatile Context *scheduled_task);

int Kernel::DispatchTask(volatile Context *kernel, TaskDescriptor *scheduled_task)
{
   // print kernel context location SUPER HELPFUL
   // uart_printf(CONSOLE, "DISPATCH KC: 0x%x, SC: 0x%x\n", kernel, &scheduled_task->context);

   scheduled_task->setState(ACTIVE);
   active_task = scheduled_task;

   // call the task
   int esr_el1 = kernel_to_task_asm(kernel, &scheduled_task->context);
   return esr_el1;
}

void Kernel::Handler(int N)
{
   switch (N)
   {
   case SVC_CREATE:
      // uart_printf(CONSOLE, "Creating Task\r\n");
      if (active_task != nullptr)
      {
         int PRIORITY = active_task->context.x[0];
         // uart_printf(CONSOLE, "F{0x%x}\n", active_task->context.x[1]); // this prints the correct value
         int ret_val = Create(PRIORITY, (void (*)())active_task->context.x[1]);
         // uart_printf(CONSOLE, "TID: {%d}\n", ret_val);
         SetRetval(ret_val);
      }
      else
      {
         // Shouldnt happen
         uart_printf(CONSOLE, "PANIC No active task\r\n");
      }
      break;

   case SVC_MYTID:
      // uart_printf(CONSOLE, "Getting Task ID\r\n");
      SetRetval(MyTid());
      break;

   case SVC_MYPARENTID:
   {
      // uart_printf(CONSOLE, "Getting Parent Task ID\r\n");
      int parent_id = MyParentTid();
      SetRetval(parent_id);
      // uart_printf(CONSOLE, "Parent ID: %d\r\n", parent_id);
      break;
   }

   case SVC_YIELD:
      // uart_printf(CONSOLE, "Yielding Task\n");
      Yield();
      break;

   case SVC_EXIT:
      // uart_printf(CONSOLE, "Exiting Task\n");
      Exit();
      break;

   default:
      break;
   }
}

extern "C" void printASM(int x0)
{
   uart_printf(CONSOLE, "X0: 0x%x \n", x0);
}
#include "kernel.h"
#include "../rpi.h"
// #if !defined(MMU)
#include <cstddef>

// define our own memset to avoid SIMD instructions emitted from the compiler
extern "C" void *memset(void *s, int c, size_t n)
{
   for (char *it = (char *)s; n > 0; --n)
      *it++ = c;
   return s;
}

// define our own memcpy to avoid SIMD instructions emitted from the compiler
extern "C" void *memcpy(void *dest, const void *src, size_t n)
{
   char *sit = (char *)src;
   char *cdest = (char *)dest;
   for (size_t i = 0; i < n; ++i)
   {
      *cdest++ = *sit++;
   }
   return dest;
}
// #endif

Kernel::Kernel()
{
   uart_printf(CONSOLE, "Kernel Constructor1\r\n");
   // initialize free_tid
   for (int i = 0; i < MAX_TASKS; i++)
   {
      free_tid.Push(i);
   }

   active_task = nullptr; // initilize active task

   // initilize default task
}

Kernel::Kernel(void (*function)()) : Kernel()
{
   // initilize default task with medium priority
   int tid = Create(MEDIUM, function);
   uart_printf(CONSOLE, "FUNCTION: 0x%x\r\n", function);
   uart_printf(CONSOLE, "BootStrap Task ID: %d\r\n", tid);
}

Kernel::~Kernel()
{
}

inline void Kernel::RepushActiveTask()
{
   if (active_task != nullptr && (active_task->state == READY || active_task->state == ACTIVE))
   {
      ready_queue.Push(active_task->tid, active_task->priority);
   }
   else
   {
      uart_printf(CONSOLE, "PANIC: ACTIVE TASK NOT READY\r\n");
      if (active_task != nullptr)
      {
         active_task->Print();
      }
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
      uart_printf(CONSOLE, "No free TID available\r\n");
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
   td->InitTask(priority, stackBlock, function);
   td->parent = active_task;

   if (td->isReady())
   {
      // uart_printf(CONSOLE, "Task %d is ready\r\n", tid);
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

void Kernel::Send()
{
   // WHAT ABOUT RECIEVERS THAT HAVE EXITED?
   int receiver_tid = active_task->context.x[0];

   TaskDescriptor *sender_task = active_task;
   TaskDescriptor *receiver_task = &task_table[receiver_tid];

   receiver_task->inbox.Push(sender_task->tid); // transfer sender's tid to receiver's inbox

   if (receiver_task->state != RECEIVE_BLOCKED)
   {
      sender_task->setState(SEND_BLOCKED); // SENDER BLOCKS
      return;
   }
   else
   {
      // CREATE FUNCTION FOR THIS
      receiver_task->setState(READY); // RECEIVER UNBLOCKS
      ready_queue.Push(receiver_tid, receiver_task->getPriority());

      sender_task->setState(REPLY_BLOCKED);                  // SENDER BLOCKS
      int destLen = CopyMessage(sender_task, receiver_task); // KERNEL COPIES DATA
      if (destLen == -1)
      {
         uart_printf(CONSOLE, "PANIC: COPY MESSAGE FAILED\r\n");
      }
   }
}

TaskDescriptor *
Kernel::Scheduler()
{
   // get the highest priority task
   int tid;

   if (ready_queue.Pop(&tid) == -1)
   {
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
   uart_printf(CONSOLE, "HANDLER: %d\r\n", N);
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
         active_task->SetRetval(ret_val);
      }
      else
      {
         // Shouldnt happen
         uart_printf(CONSOLE, "PANIC No active task\r\n");
      }
      break;

   case SVC_MYTID:
      // uart_printf(CONSOLE, "Getting Task ID\r\n");
      active_task->SetRetval(MyTid());
      break;

   case SVC_MYPARENTID:
   {
      // uart_printf(CONSOLE, "Getting Parent Task ID\r\n");
      int parent_id = MyParentTid();
      active_task->SetRetval(parent_id);
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

   case SVC_SEND:
      uart_printf(CONSOLE, "Sending Message Triggered\r\n");
      Send();
      break;

   case SVC_RECEIVE:
   {
      uart_printf(CONSOLE, "Receiving Message Triggered\r\n");

      int sender_tid;
      if (active_task->inbox.Pop(&sender_tid) == -1)
      {
         active_task->setState(RECEIVE_BLOCKED); // RECEIVER BLOCKS
         break;
      }
      // ensure sender is in send block
      // LOOKUP SENDER TASK
      TaskDescriptor *sender_task = &task_table[sender_tid];
      if (sender_task->state != SEND_BLOCKED) // SANITY
      {
         uart_printf(CONSOLE, "PANIC: POTENTIAL CAUSE ~ SENDER EXIT ALREADY.\r\n");
         break;
      }
      sender_task->setState(REPLY_BLOCKED); // SENDER STILL BLOCKED
      CopyMessage(sender_task, active_task);
      RepushActiveTask();

      // KERNEL COPIES DATA
      // SetRetval(-1);
      break;
   }

   case SVC_REPLY:
   {
      uart_printf(CONSOLE, "Replying Message Triggered\r\n");
      // int tid = active_task->context.x[0];
      // const char *reply = (const char *)active_task->context.x[1];
      // int replylen = active_task->context.x[2];
      // int ret_val = Reply(tid, reply, replylen);
      // SetRetval(-1);
      break;
   }

   default:
      break;
   }
}

int Kernel::CopyMessage(TaskDescriptor *sender_td, TaskDescriptor *receiver_td)
{
   // copy message from sender to receiver
   int requested_tid = static_cast<int>(sender_td->context.x[0]);

   if (receiver_td->tid != requested_tid)
   {
      uart_printf(CONSOLE, "PANIC: SENDER TID MISMATCH\r\n");
      return -1; // Define enum for this to use by usertasks
   }
   uart_printf(CONSOLE, "COPYING MESSAGE\r\n");
   const char *src = reinterpret_cast<const char *>(sender_td->context.x[1]);
   int srclen = static_cast<int>(sender_td->context.x[2]);

   uart_printf(CONSOLE, "SRC: %s, SRCLEN: %d\r\n", src, srclen);

   int *receiver_tid = reinterpret_cast<int *>(receiver_td->context.x[0]);
   char *dest = reinterpret_cast<char *>(receiver_td->context.x[1]);
   int destlen = static_cast<int>(receiver_td->context.x[2]);

   destlen = (destlen < srclen) ? destlen : srclen;

   *receiver_tid = sender_td->tid;
   // dest, src, len
   memcpy(dest, src, destlen);

   uart_printf(CONSOLE, "RECEIVER TID RET: %d, MSG: %s, MSGLEN: %d\r\n", *receiver_tid, dest, destlen);

   receiver_td->SetRetval(srclen);
   sender_td->SetRetval(destlen);

   return destlen;
}

extern "C" void printASM(int x0)
{
   uart_printf(CONSOLE, "X0: 0x%x \n", x0);
}
#include "kernel.h"
#include "../rpi.h"
/***************************************************/
// #if !defined(MMU)
#include <cstddef>
#include "interrupts.h"

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
/***************************************************/

Kernel::Kernel()
{
   uart_printf(CONSOLE, "Kernel Constructor1\r\n");
   // initialize free_tid
   for (int i = 0; i < MAX_TASKS; i++)
   {
      free_tid.Push(i);
   }

   active_task = nullptr; // initilize active task
   uart_printf(CONSOLE, "TIDs: ");
   for (int i = 0; i < MAX_TASKS; i++)
   {
      task_table[i].tid = i;
      // uart_printf(CONSOLE, " %d ", task_table[i].tid);
   }

   tasks_awaiting_event = 0;

   // initilize default task
}

Kernel::Kernel(void (*function)()) : Kernel()
{
   // initilize default task with medium priority
   int tid = Create(MEDIUM, function);
   uart_printf(CONSOLE, "FUNCTION: 0x%x\r\n", function);
   uart_printf(CONSOLE, "BootStrap Task ID: %d\r\n", tid);

   // initialize INTERRUPTS
   InitGIC();
   uart_printf(CONSOLE, "Finished Initializing GIC\r\n");
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
   // uart_printf(CONSOLE, "TID FROM TABLE: %d\r\n", td->tid);
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
   // uart_printf(CONSOLE, "Task %d created with priority %d\r\n", tid, priority);
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

   if (receiver_task->state != RECEIVE_BLOCKED)
   {
      receiver_task->inbox.Push(sender_task->tid); // transfer sender's tid to receiver's inbox
      sender_task->setState(SEND_BLOCKED);         // SENDER BLOCKS
      return;
   }
   else
   {
      // CREATE FUNCTION FOR THIS
      receiver_task->setState(READY); // RECEIVER UNBLOCKS
      receiver_task->inbox.Pop(&receiver_tid);
      ready_queue.Push(receiver_tid, receiver_task->getPriority());

      sender_task->setState(REPLY_BLOCKED); // SENDER BLOCKS
      // uart_printf(CONSOLE, "SENDER BLOCKED\r\n");                   // SENDER BLOCKS
      int destLen = CopyMessage(sender_task, receiver_task, false); // KERNEL COPIES DATA
      if (destLen == -1)
      {
         uart_printf(CONSOLE, "PANIC: COPY MESSAGE FAILED\r\n");
      }
   }
}

void Kernel::Receive()
{
   int sender_tid;
   if (active_task->inbox.Pop(&sender_tid) == -1)
   {
      active_task->setState(RECEIVE_BLOCKED); // RECEIVER BLOCKS
      uart_printf(CONSOLE, "RECEIVER BLOCKED\r\n");
      return;
   }
   // ensure sender is in send block
   // LOOKUP SENDER TASK
   TaskDescriptor *sender_task = &task_table[sender_tid];
   // uart_printf(CONSOLE, "ACTIVE TASK: %d, SENDER TASK: %d\r\n", active_task->tid, sender_task->tid);
   if (sender_task->state != SEND_BLOCKED) // SANITY
   {
      uart_printf(CONSOLE, "PANIC: POTENTIAL CAUSE ~ SENDER EXIT ALREADY.\r\n");
      return;
   }
   sender_task->setState(REPLY_BLOCKED); // SENDER STILL BLOCKED
   int destLen = CopyMessage(sender_task, active_task, false);
   if (destLen == -1)
   {
      uart_printf(CONSOLE, "PANIC: COPY MESSAGE FAILED\r\n");
   }
   RepushActiveTask();

   // KERNEL COPIES DATA
   // SetRetval(-1);
}

void Kernel::Reply()
{
   int sender_tid = active_task->context.x[0];
   TaskDescriptor *sender_task = &task_table[sender_tid];
   if (sender_task->state != REPLY_BLOCKED) // check sender is waiting
   {
      uart_printf(CONSOLE, "PANIC: SENDER NOT WAITING FOR REPLY\r\n");
      return;
   }
   int destLen = CopyMessage(active_task, sender_task, true); // KERNEL COPIES DATA
   if (destLen == -1)
   {
      uart_printf(CONSOLE, "PANIC: COPY MESSAGE FAILED\r\n");
   }
   sender_task->setState(READY); // SENDER UNBLOCKS
   ready_queue.Push(sender_tid, sender_task->getPriority());
   RepushActiveTask();
}

void Kernel::AwaitEvent(int eventType)
{
   enable_irq(); // need to disable this interrupt after handling
   uart_printf(CONSOLE, "AWAITING EVENT: %d\r\n", eventType);
   // enable this interrupt
   // active_task->Print();
   active_task->setState(EVENT_BLOCKED);
   tasks_awaiting_event++;
   // RepushActiveTask();
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
   // scheduled_task->Print();
   active_task = scheduled_task;

   // call the task
   // uart_printf(CONSOLE, "DISPATCHING TASK: %d\r\n", scheduled_task->tid);
   int esr_el1 = kernel_to_task_asm(kernel, &scheduled_task->context);
   return esr_el1;
}

void Kernel::Handler(int N)
{
   // uart_printf(CONSOLE, "HANDLER: {%d}, ACTIVE TASK: {%d} ", N, active_task->tid);
   switch (N)
   {
   case SVC_CREATE:
      // uart_printf(CONSOLE, "(CREATE)\r\n");
      if (active_task != nullptr)
      {
         int PRIORITY = active_task->context.x[0];
         // uart_printf(CONSOLE, "PRIORITY: {%d}\r\n", PRIORITY);
         // uart_printf(CONSOLE, "F{0x%x}\n", active_task->context.x[1]); // this prints the correct value
         int ret_val = Create(PRIORITY, (void (*)())active_task->context.x[1]);
         // uart_printf(CONSOLE, "CREATED TASK WITH TID: {%d}\r\n", ret_val);
         // VERIFY IN TABLE:
         TaskDescriptor *TD = &(task_table[ret_val]);
         // TD->Print();
         if (ret_val != TD->tid)
         {
            uart_printf(CONSOLE, "PANIC: TID MISMATCH\r\n");
         }
         active_task->SetRetval(ret_val);
      }
      else
      {
         // Shouldnt happen
         uart_printf(CONSOLE, "PANIC No active task\r\n");
      }
      break;

   case SVC_MYTID:
      // uart_printf(CONSOLE, "(MYTID)\r\n");
      active_task->SetRetval(MyTid());
      break;

   case SVC_MYPARENTID:
   {
      // uart_printf(CONSOLE, "(MYPARENTID)\r\n");
      int parent_id = MyParentTid();
      active_task->SetRetval(parent_id);
      // uart_printf(CONSOLE, "Parent ID: %d\r\n", parent_id);
      break;
   }

   case SVC_YIELD:
      // uart_printf(CONSOLE, "(YIELD)\r\n");
      Yield();
      break;

   case SVC_EXIT:
      // uart_printf(CONSOLE, "(EXIT)\r\n");
      Exit();
      break;

   case SVC_SEND:
      // uart_printf(CONSOLE, "(SEND)\r\n");
      Send();
      break;

   case SVC_RECEIVE:
   {
      // uart_printf(CONSOLE, "(RECEIVE)\r\n");
      Receive();
      break;
   }

   case SVC_REPLY:
   {
      // uart_printf(CONSOLE, "(REPLY) (TID %d)\r\n", active_task->context.x[0]);
      Reply();
      break;
   }

   case SVC_ICACHE:
      // uart_printf(CONSOLE, "(ICACHE)\r\n");
      enable_icache();
      break;

   case SVC_DCACHE:
      // uart_printf(CONSOLE, "(DCACHE)\r\n");
      enable_dcache();
      break;

   case SVC_BCACHE:
      // uart_printf(CONSOLE, "(BCACHE)\r\n");
      enable_bcache();
      break;

   case SVC_AWAITEVENT:
      uart_printf(CONSOLE, "(AWAITEVENT) Triggered\r\n");
      AwaitEvent(active_task->context.x[0]);
      break;

   case IRQ:
      uart_printf(CONSOLE, "(IRQ) Triggered \r\n");
      break;

   default:
      break;
   }
}

int Kernel::CopyMessage(TaskDescriptor *sender_td, TaskDescriptor *receiver_td, bool is_reply)
{
   // NOTE THAT THE ONLY DIFFERENCE FOR REPLY IS THAT WE USE x[3], x[4] instead of x[1], x[2] for sender

   // copy message from sender to receiver
   int requested_tid = static_cast<int>(sender_td->context.x[0]);

   if (receiver_td->tid != requested_tid)
   {
      uart_printf(CONSOLE, "PANIC: SENDER TID MISMATCH\r\n");
      return -1; // Define enum for this to use by usertasks
   }

   const char *src = reinterpret_cast<const char *>(sender_td->context.x[1]);
   int srclen = static_cast<int>(sender_td->context.x[2]);

   char *dest = reinterpret_cast<char *>(receiver_td->context.x[is_reply ? 3 : 1]);
   int destlen = static_cast<int>(receiver_td->context.x[is_reply ? 4 : 2]);

   destlen = (destlen < srclen) ? destlen : srclen;
   memcpy(dest, src, destlen);

   if (!is_reply)
   {
      // set tid of sender in receiver's context
      int *tid_loc = reinterpret_cast<int *>(receiver_td->context.x[0]);
      *tid_loc = sender_td->tid;
   }

   receiver_td->SetRetval(srclen);
   sender_td->SetRetval(destlen);
   return destlen;
}

// *********************************************
// Perf test runs in this order, and starts with no cache
// so when we enable icache first, it is from disabled state -> enabled state
// void Kernel::enable_icache()
// {
//    uart_printf(CONSOLE, "KERNEL Enabling I-Cache\r\n");
//    asm volatile("msr sctlr_el1, %x0\n\t" ::"r"(1 << 12));
//    uart_printf(CONSOLE, "KERNEL I-Cache Enabled, repushing active task\r\n");
//    RepushActiveTask();
// }
static void invalidate_icache()
{
   asm volatile("dsb sy\n\t"
                "ic ialluis\n\t"
                "dsb sy\n\t"
                "isb\n\t");
}

extern "C" void InvalidateANDCleanDCache();

void Kernel::enable_icache()
{
   invalidate_icache();
   // uart_printf(CONSOLE, "KERNEL Enabling I-Cache\r\n");
   asm volatile(
       "mrs x0, sctlr_el1\n\t"      // Read SCTLR_EL1 into x0
       "orr x0, x0, #(1 << 12)\n\t" // Set bit 12 to enable I-Cache
       "msr sctlr_el1, x0\n\t"      // Write back to SCTLR_EL1
       "isb"                        // Instruction Synchronization Barrier
       : : : "x0");
   // uart_printf(CONSOLE, "KERNEL I-Cache Enabled, repushing active task\r\n");
   RepushActiveTask();
}

void Kernel::enable_dcache()
{
   InvalidateANDCleanDCache();

   asm volatile(
       "mrs x0, sctlr_el1\n\t"     // Read SCTLR_EL1 into x0
       "orr x0, x0, #(1 << 2)\n\t" // Set bit 2 to enable D-Cache
       "msr sctlr_el1, x0\n\t"     // Write back to SCTLR_EL1
       "isb"                       // Instruction Synchronization Barrier
       :
       :
       : "x0");
   RepushActiveTask();
}

void Kernel::enable_bcache()
{
   invalidate_icache();
   InvalidateANDCleanDCache();

   // enabled -> enabled (clean cache)
   asm volatile(
       "mrs x0, sctlr_el1\n\t"      // Read SCTLR_EL1 into x0
       "orr x0, x0, #(1 << 2)\n\t"  // Set bit 2 (D-Cache enable)
       "orr x0, x0, #(1 << 12)\n\t" // Set bit 12 (I-Cache enable)
       "msr sctlr_el1, x0\n\t"      // Write back to SCTLR_EL1
       "isb"                        // Instruction Synchronization Barrier
       :
       :
       : "x0");

   RepushActiveTask();
}

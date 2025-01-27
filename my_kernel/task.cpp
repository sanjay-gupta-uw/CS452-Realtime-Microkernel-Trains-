#include "task.h"

#include "rpi.h"
#include "util.h"
#include "clock.h"

#define DEBUG 65
extern Clock clock;

static int tid_counter = 0;

// ********** TaskDescriptor **********
TaskDescriptor::TaskDescriptor()
{
   tid = tid_counter++ % MAX_TASKS;
   parent = nullptr;
   priority = IDLE;
   state = EXITED;
}

TaskDescriptor::~TaskDescriptor()
{
}

// use kernel.Create to call this
int TaskDescriptor::CreateTask(int priority, MemoryBlock *block, void (*function)())
{
   this->priority = priority;
   this->state = READY;
   // set parent to active task

   // Allocate a stack for the task
   this->sp = block->addr; // Align the stack pointer to 16 bytes
   this->spsr = 0x00;      // set spsr EL0

   uart_printf(CONSOLE, "Task %d stack pointer: %x\n", this->tid, this->sp);

   // Initialize the stack with the context
   uint64_t *sp = (uint64_t *)this->sp;

   *(--sp) = this->spsr;          // Push SPSR
   *(--sp) = (uintptr_t)function; // Push program counter (PC) (entry point)

   for (int i = 30; i >= 0; --i)
   {
      *(--sp) = 0; // dummy values for x0-x12
   }

   this->sp = (uintptr_t)sp; // Update the stack pointer

   return 0;
}

bool TaskDescriptor::isReady()
{
   return state == READY;
}

int TaskDescriptor::getPriority()
{
   return priority;
}

// ********** End TaskDescriptor **********

extern "C" size_t _context_switch(int sp, int spsr);
// should take two TDs as arguments
void ContextSwitch(TaskDescriptor *td)
{
   // uart_printf(CONSOLE, "Context Switching to Task %d\n", td->tid);
   // int res = _context_switch(td->sp, td->spsr);
   // uart_printf(CONSOLE, "Context Switching Result: 0x%x\n", res);

   asm volatile(
       "mov x1, %1\n"       // Load the next task's stack pointer into x0
       "mov x0, %0\n"       // Load the next task's SPSR into x1
       "bl _context_switch" // Call the assembly context switch routine
       :
       : "r"(td->sp), "r"(td->spsr));
}

// ********** USER TASKS **********
void Task1()
{
   uart_puts(CONSOLE, "INSIDE Task 1\n");

   int EL;
   // Request the kernel to determine the exception level
   asm volatile(
       "svc #1\n"     // System call with ID 1
       "mov %0, x0\n" // Get the return value from x0
       : "=r"(EL)     // Output to C variable EL
       :              // No inputs
       : "x0"         // Clobbers x0
   );

   // Print the exception level
   uart_printf(CONSOLE, "EL: %d\n", EL);
   uart_puts(CONSOLE, "INSIDE Task 1\n");

   // Continue with task logic
   for (;;)
   {
   }
}

void Task2()
{
   uart_puts(CONSOLE, "INSIDE Task 2");
}
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
   this->sp = block->addr;

   // Initialize the stack with the context
   uint32_t *sp = (uint32_t *)this->sp;

   *(--sp) = (uintptr_t)function; // Push the initial program counter (entry point)
   *(--sp) = 0x10;                // SPSR -- Saved Program Status Register

   for (int i = 0; i < 13; ++i)
   {
      *(--sp) = 0; // Push placeholder values for general-purpose registers (R0-R12)
   }

   this->sp = (uintptr_t)sp; // Update the stack pointer

   return 0;
}

bool TaskDescriptor::isReady()
{
   return state == READY;
}

// ********** End TaskDescriptor **********

// should take two TDs as arguments
void ContextSwitch()
{
   move_cursor(CONSOLE, DEBUG, 1);
   clear_to_end_line(CONSOLE);
   uart_putc(CONSOLE, 'C');

   // asm volatile("mov r0, %[currentSP]" ::[currentSP] "r"(currentTask->stackPointer));
   // asm volatile("mov r1, %[nextSP]" ::[nextSP] "r"(nextTask->stackPointer));
   // asm volatile("bl _context_switch");
}

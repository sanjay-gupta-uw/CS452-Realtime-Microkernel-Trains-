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
   this->block = block;
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

int TaskDescriptor::getTid()
{
   return tid;
}

TaskDescriptor *TaskDescriptor::getParent()
{
   return parent;
}

void TaskDescriptor::setState(RunState state)
{
   this->state = state;
}

RunState TaskDescriptor::getState()
{
   return state;
}

MemoryBlock *TaskDescriptor::getBlock()
{
   return block;
}

// ********** End TaskDescriptor **********

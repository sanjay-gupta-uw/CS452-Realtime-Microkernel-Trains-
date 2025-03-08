#include "task.h"

#include "../rpi.h"
#include "../util.h"
#include "../clock.h"

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
int TaskDescriptor::InitTask(int priority, MemoryBlock *block, void (*function)())
{

    this->priority = priority;
    this->state = READY;
    this->block_index = block->index; // Store the memory block for this task

    // Initialize the context structure
    this->context.sp = block->addr;          // top of the stack
    this->context.elr = (uintptr_t)function; // Entry point for the task
    this->context.spsr = 0x00;               // Initial SPSR value for EL0 execution

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        this->context.x[i] = i;
    }

    // Print();

    // Prepare the stack to match the expected restore layout

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

void TaskDescriptor::SetRetval(int ret_val)
{
    this->context.x[0] = ret_val;
}

// DISABLED
void TaskDescriptor::Print()
{
    // (CONSOLE, "tid {%d} :: Priority {%d} :: F {0x%x} :: SP {0x%x} :: &context {0x%x} :: STATE {%d}\r\n", tid, priority, context.elr, context.sp, &context, state);
}

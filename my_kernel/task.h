#ifndef _task_h_
#define _task_h_

typedef enum
{
   ACTIVE,
   READY,
   EXITED,
   SEND_BLOCKED,
   RECEIVE_BLOCKED,
   REPLY_BLOCKED,
   EVENT_BLOCKED,
} RunState;

class TaskDescriptor
{
private:
   int tid;                    // a pointer to the TD of the task that created it, its parent,
   TaskDescriptor *parent;     // a pointer to the TD of the next task in the task's ready queue,
   int priority;               // the task's priority,
   RunState current_run_state; // the task's current run state
                               // a pointer to the TD of the next task on the task's send queue,
                               // the task's current stack pointer.
                               // the task's return value, and
                               // the task's SPSR.

public:
};

class Task
{
private:
   TaskDescriptor *td;
   int *send_queue;
   int *ready_queue;
};

#endif // _td_h_
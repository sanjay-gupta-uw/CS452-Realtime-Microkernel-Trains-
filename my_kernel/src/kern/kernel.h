#ifndef _kernel_h_
#define _kernel_h_

#include "../shared_constants.h"

#include "../containers/ringbuffer.h"
#include "../containers/queue.h"
#include "../containers/pqueue.h"

#include "task.h"
#include "memory.h"
#include "constants.h"

class Kernel
{
public:
    Kernel();
    Kernel(void (*function)());
    ~Kernel();
    TaskDescriptor *Scheduler();
    int DispatchTask(volatile Context *kernel, TaskDescriptor *scheduled_task);
    void Handler(int N, uint32_t idleTime);
    bool areTasksWaiting();
    void printStats(uint32_t idle, uint32_t total);

private:
    MemoryManager mem_manager;
    TaskDescriptor *active_task;

    TaskDescriptor task_table[MAX_TASKS];
    RingBuffer<int> free_tid;
    PQueue<int> ready_queue; // ready queue is a priority queue of task ids
    // PQueue<TaskDescriptor *> ready_queue;

    // Array of Queues for each event type
    Queue<TaskDescriptor *, MAX_TASKS> event_queues[InterruptEvents::NUM_EVENTS];

    int Create(int priority, void (*function)()); // use free_tid to get a tid, create a new task, and push it to ready_queue
    int MyTid();
    int MyParentTid();
    void Yield();
    void Exit(); // relesase the task's tid and memory block, and push the task to free_tid
    void Send();
    void Receive();
    void Reply();

    int CopyMessage(TaskDescriptor *sender_td, TaskDescriptor *receiver_td, bool is_reply);
    void inline RepushActiveTask();
    void enable_icache();
    void enable_dcache();
    void enable_bcache();
    void AwaitEvent(int eventType);

    void IRQ_Handler();

    int tasks_awaiting_event;
};

#endif
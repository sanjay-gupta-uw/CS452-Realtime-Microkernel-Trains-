#include "../register.h"
#include "constants.h"
#include "kernel.h"
#include <cstddef>
#include "interrupts.h"
#include "../clock.h"
#include "kabort.h"
#include "kassert.h"
#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

extern Clock clock;

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
    disable_irq();
    // initialize free_tid
    for (int i = 1; i <= MAX_TASKS; i++)
    {
        free_tid.Push(i);
    }

    active_task = nullptr; // initilize active task
    for (int i = 0; i < MAX_TASKS; i++)
    {
        task_table[i].tid = i;
    }

    tasks_awaiting_event = 0;
}

Kernel::Kernel(void (*function)()) : Kernel()
{
    // initilize default task with idle priority
    int tid = Create(PRIORITY::IDLE, function);
    // initialize INTERRUPTS
    InitGIC();
    // disable_irq(); // disable interrupts
    // disable irqs to uart
    // set first 11 bits to 1
    UART_IMSC_DISABLE(CONSOLE, 0x7FF);
    UART_IMSC_DISABLE(MARKLIN, 0x7FF);

    // disable fifo
    UART_REG(CONSOLE, UART_LCRH) &= ~(1 << 4);
    UART_REG(MARKLIN, UART_LCRH) &= ~(1 << 4);

    // set trigger level to 1/8
    // since using RTM, trigger level doesnt matter really...?
    // int mask = 0x38;
    // UART_REG(CONSOLE, UART_IFLS) &= ~mask;
    // 0011 1000
    //  0x38
}

Kernel::~Kernel()
{
}

inline void Kernel::RepushActiveTask()
{
    if (active_task != nullptr && (active_task->state == READY || active_task->state == ACTIVE))
    {
        active_task->setState(READY);
        ready_queue.Push(active_task->tid, active_task->priority);
    }
}

int Kernel::CopyMessage(TaskDescriptor *sender_td, TaskDescriptor *receiver_td, bool is_reply)
{
    // NOTE THAT THE ONLY DIFFERENCE FOR REPLY IS THAT WE USE x[3], x[4] instead of x[1], x[2] for sender

    // copy message from sender to receiver
    int requested_tid = static_cast<int>(sender_td->context.x[0]);
    kassert(receiver_td->tid == requested_tid && "PANIC: WRONG RECEIVER ATTEMPTED TO ACCESS MESSAGE");

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

// returns -1 if invalid priority, -2 if no free tid, tid if success
int Kernel::Create(int priority, void (*function)())
{
    // check if priority is valid
    if (priority < PRIORITY::CORE_NOTIFIER || priority > PRIORITY::IDLE)
    {
        return -2;
    }

    // get a free tid
    int tid;
    kassert(free_tid.Pop(&tid) != -1 && "PANIC: MAX TASKS RUNNING -- NO FREE TID AVAILABLE");

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

        sender_task->setState(REPLY_BLOCKED);                         // SENDER BLOCKS
        int destLen = CopyMessage(sender_task, receiver_task, false); // KERNEL COPIES DATA
        kassert(destLen != -1 && "PANIC: COPY MESSAGE FAILED");
    }
}

void Kernel::Receive()
{
    int sender_tid;
    if (active_task->inbox.Pop(&sender_tid) == -1)
    {
        active_task->setState(RECEIVE_BLOCKED); // RECEIVER BLOCKS
        return;
    }
    // ensure sender is in send block
    // LOOKUP SENDER TASK
    TaskDescriptor *sender_task = &task_table[sender_tid];
    if (sender_task->state != SEND_BLOCKED) // SANITY
    {
        // POTENTIAL CAUSE ~ SENDER EXIT ALREADY.\r\n"
        return;
    }
    sender_task->setState(REPLY_BLOCKED); // SENDER STILL BLOCKED
    int destLen = CopyMessage(sender_task, active_task, false);
    kassert(destLen != -1 && "PANIC: COPY MESSAGE FAILED");
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
        // SENDER NOT WAITING FOR REPLY
        return;
    }
    int destLen = CopyMessage(active_task, sender_task, true); // KERNEL COPIES DATA
    kassert(destLen != -1 && "PANIC: COPY MESSAGE FAILED");
    sender_task->setState(READY); // SENDER UNBLOCKS
    ready_queue.Push(sender_tid, sender_task->getPriority());
    RepushActiveTask();
}

void Kernel::AwaitEvent(int eventType)
{

    switch (eventType)
    {
    case TIMER_TICK:
    {
        // #if IRQ_ENABLED == 0
        //         // kassert(false && "PANIC: IRQ NOT ENABLED");
        //         active_task->SetRetval(-1);   // set return value to -1
        //         active_task->setState(READY); // set task state to ready
        //         RepushActiveTask();
        //         return;
        // #endif

        clock.ReArmTimer(TEN_MS); // Rearm timer for next interval
        active_task->SetRetval(0);
        active_task->setState(EVENT_BLOCKED);
        event_queues[TIMER_TICK].Push(active_task);
    }
    break;
    case UART_RX_TIMEOUT: // console interrupt
    {
        UART_IMSC_ENABLE(CONSOLE, (RTM_INTERRUPT_MASK)); // enable receive timeout interrupt
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_RX_TIMEOUT].Push(active_task);
    }
    break;
    case UART_TX:
    {
        // ensure TX is low
        int tx_status = TX_STATUS(CONSOLE);
        // uart_printf(CONSOLE, RESTORE_CURSOR "KERNEL AWAIT: TX STATUS: %d\r\n" SAVE_CURSOR, tx_status);
        // kassert(false && "PANIC: UART_TX ENABLED");
        // ENABLE LEVEL INTERRUPT using IMSC register
        UART_IMSC_ENABLE(CONSOLE, (TX_INTERRUPT_MASK));
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_TX].Push(active_task);
    }
    break;
    case UART_MARKLIN_RX:
    {
        UART_IMSC_ENABLE(MARKLIN, (RX_INTERRUPT_MASK));
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_MARKLIN_RX].Push(active_task);
    }
    break;
    case UART_MARKLIN_TX:
    {
        UART_IMSC_ENABLE(MARKLIN, (TX_INTERRUPT_MASK));
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_MARKLIN_TX].Push(active_task);
    }
    break;
    case UART_MARKLIN_CTS_HIGH:
    {
        // ensure CTS is not high
        // int cts_status = CTS_STATUS(MARKLIN);
        // kassert(cts_status == 0 && "WAITING FOR CTS HIGH, BUT IS ALREADY HIGH");
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_MARKLIN_CTS_HIGH].Push(active_task);
    }
    break;
    case UART_MARKLIN_CTS_LOW:
    {
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_MARKLIN_CTS_LOW].Push(active_task);
    }
    break;
    case UART_CTS:
    {
        active_task->SetRetval(-1);
        active_task->setState(EVENT_BLOCKED);
        event_queues[UART_CTS].Push(active_task);
    }
    break;
    default:
        active_task->SetRetval(-1);   // set return value to -1
        active_task->setState(READY); // set task state to ready
        RepushActiveTask();
        break;
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
    scheduled_task->setState(ACTIVE);
    // scheduled_task->Print();
    active_task = scheduled_task;

    // call the task
    // enable_irq();
    int esr_el1 = kernel_to_task_asm(kernel, &scheduled_task->context);
    // disable_irq();
    return esr_el1;
}

void Kernel::Handler(int N, uint32_t idleTime)
{
    kassert(active_task != nullptr && "PANIC: NO ACTIVE TASK");
    switch (N)
    {
    case SVC_CREATE:
    {
        int PRIORITY = active_task->context.x[0];
        int ret_val = Create(PRIORITY, (void (*)())active_task->context.x[1]);
        kassert(ret_val >= 0 && "PANIC: ERROR CREATING TASK");
        TaskDescriptor *TD = &(task_table[ret_val]);
        kassert(TD->tid == ret_val && "PANIC: TID MISMATCH");
        active_task->SetRetval(ret_val);
        break;
    }

    case SVC_MYTID:
        active_task->SetRetval(MyTid());
        break;

    case SVC_MYPARENTID:
    {
        int parent_id = MyParentTid();
        active_task->SetRetval(parent_id);
        break;
    }

    case SVC_YIELD:
        Yield();
        break;

    case SVC_EXIT:
        Exit();
        break;

    case SVC_SEND:
        Send();
        break;

    case SVC_RECEIVE:
        Receive();
        break;

    case SVC_REPLY:
        Reply();
        break;

    case SVC_ICACHE:
        // enable_icache();
        break;

    case SVC_DCACHE:
        // enable_dcache();
        break;

    case SVC_BCACHE:
        // enable_bcache();
        break;

    case SVC_AWAITEVENT:
        AwaitEvent(active_task->context.x[0]);
        break;

    case IRQ:
        IRQ_Handler();
        break;

    case SVC_GETIDLE:
        active_task->SetRetval(idleTime);
        RepushActiveTask();
        break;

    case SVC_ABORT:
    {
        const char *condition = reinterpret_cast<const char *>(active_task->context.x[0]); // extract condition from active task
        int line = static_cast<int>(active_task->context.x[2]);
        const char *file = reinterpret_cast<const char *>(active_task->context.x[3]);
        kabort(condition, line, file, 1);

        break;
    }

    default:
        break;
    }
}

void Kernel::IRQ_Handler()
{
    uint32_t irq_id = D_REG(GICC_BASE, GICC_IAR); // Read the interrupt ID from the GICC_IAR register
    irq_id &= 0x3FF;                              // Mask to extract last 10 bits (interrupt ID)

    switch (irq_id)
    {
    case TIMER_C1: // timERTICK?
    {
        clock.DisarmTimer();

        TaskDescriptor *task;
        while (event_queues[TIMER_TICK].Pop(&task) != -1)
        {
            task->setState(READY);
            ready_queue.Push(task->tid, task->priority);
        }
        break;
    }
    case UART_IRQ:
    {
        TaskDescriptor *task;
        // read UART_MIS register to find out which interrupt is triggered
        uint32_t uart_mis = UART_REG(CONSOLE, UART_MIS);

        if (uart_mis & TX_INTERRUPT_MASK)
        {
            UART_IMSC_DISABLE(CONSOLE, TX_INTERRUPT_MASK);
            UART_CLEAR_INTERRUPT(CONSOLE, TX_INTERRUPT_MASK);

            int tx_status = TX_STATUS(CONSOLE);
            if (tx_status == 1)
            {
                // kassert(false && "PANIC: UART_TX INTERRUPT HIGH");
            }
            else if (tx_status == 0)
            {
                kassert(false && "PANIC: UART_TX INTERRUPT LOW"); // halt the system
            }

            while (event_queues[UART_TX].Pop(&task) != -1)
            {
                // uart_printf(CONSOLE, RESTORE_CURSOR "UART_TX INTERRUPT TRIGGERED -- waking up task {%d}\r\n" SAVE_CURSOR, task->tid);
                // kassert(false && "WAKING UP TX TASK");
                task->setState(READY);
                task->SetRetval(0);
                ready_queue.Push(task->tid, task->priority);

                // INSPECT READY QUEUE
            }
        }
        if (uart_mis & RTM_INTERRUPT_MASK)
        {
            UART_IMSC_DISABLE(CONSOLE, RTM_INTERRUPT_MASK);
            UART_CLEAR_INTERRUPT(CONSOLE, RTM_INTERRUPT_MASK);

            // kassert(false && "RECEIVE TIMEOUT INTERRUPT TRIGGERED");

            while (event_queues[UART_RX_TIMEOUT].Pop(&task) != -1)
            {
                task->setState(READY);
                task->SetRetval(0);
                ready_queue.Push(task->tid, task->priority);
            }
        }

        uart_mis = UART_REG(MARKLIN, UART_MIS);
        if (uart_mis & RX_INTERRUPT_MASK)
        {
            UART_IMSC_DISABLE(MARKLIN, RX_INTERRUPT_MASK);
            UART_CLEAR_INTERRUPT(MARKLIN, RX_INTERRUPT_MASK);

            while (event_queues[UART_MARKLIN_RX].Pop(&task) != -1)
            {
                task->setState(READY);
                task->SetRetval(0);
                ready_queue.Push(task->tid, task->priority);
            }
        }
        if (uart_mis & TX_INTERRUPT_MASK)
        {
            UART_IMSC_DISABLE(MARKLIN, TX_INTERRUPT_MASK);
            UART_CLEAR_INTERRUPT(MARKLIN, TX_INTERRUPT_MASK);

            while (event_queues[UART_MARKLIN_TX].Pop(&task) != -1)
            {
                task->setState(READY);
                task->SetRetval(0);
                ready_queue.Push(task->tid, task->priority);
            }
        }
        if (uart_mis & CTS_INTERRUPT_MASK)
        {
            // UART_IMSC_DISABLE(MARKLIN, CTS_INTERRUPT_MASK);
            UART_CLEAR_INTERRUPT(MARKLIN, CTS_INTERRUPT_MASK);
            int cts_status = CTS_STATUS(MARKLIN);
            int idx = (cts_status == 1) ? UART_MARKLIN_CTS_HIGH : UART_MARKLIN_CTS_LOW;
            // kassert(idx == UART_MARKLIN_CTS_HIGH && "PANIC: LOW CTS INTERRUPT TRIGGERED");
            // kassert(idx == UART_MARKLIN_CTS_LOW && "PANIC: HIGH CTS INTERRUPT TRIGGERED");
            // kassert(!event_queues[idx].IsEmpty() && "PANIC: NO TASK WAITING FOR CTS INTERRUPT");
            while (event_queues[idx].Pop(&task) != -1)
            {
                task->setState(READY);
                task->SetRetval(0);
                ready_queue.Push(task->tid, task->priority);
            }
        }
        break;
    }
    case SPURIOUS_INTERRUPT:
    default:
        break;
    }
    // uart_printf(CONSOLE, RESTORE_CURSOR "IRQ HANDLER: %d\r\n" SAVE_CURSOR, irq_id);
    // kassert(false && "PANIC: INTERRUPT TRIGGERED");
    RepushActiveTask();

    D_REG(GICC_BASE, GICC_EOIR) = irq_id; // must write the interrupt ID to GICC_EOIR to signal end of interrupt
}

bool Kernel::areTasksWaiting()
{
    for (int i = 0; i < InterruptEvents::NUM_EVENTS; i++)
    {
        if (!event_queues[i].IsEmpty())
        {
            return true;
        }
    }
    return false;
}
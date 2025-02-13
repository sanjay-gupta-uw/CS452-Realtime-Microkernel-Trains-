#include "clock_server.h"
#include "clock_notifier.h"
#include "../kern/syscall.h"
#include "../kern/kernel.h"
#include "../kern/task.h"
#include "name_server.h"
#include "../containers/pqueue.h"
#include "../shared_constants.h"
#include "../rpi.h"

#define TENTH_OF_SECOND_MICRO_SECONDS 100000

struct ClockRequest {
    ClockRequestType type;
    int ticks;
};

struct ClockResponse {
    int time;
};

struct ClockTask {
    int tid;
    int trigger_time;
};

class Server {
public:
    Server() {
        REGISTERAS("ClockServer");
        // Start the Clock Notifier
        int notifierTid = CREATE(HIGH, clockNotifier);
        if (notifierTid < 0) {
            uart_printf(CONSOLE, "Error starting Clock Notifier\n");
        }
        mainLoop();
    }

private:
    PQueue<ClockTask> waitingTasks; 

    void mainLoop() {
        int sender_tid;
        ClockRequest req;
        while (true) {
            RECEIVE(&sender_tid, (char*)&req, sizeof(req));
            processRequest(sender_tid, req);
        }
    }

    void processRequest(int sender_tid, const ClockRequest& req) {
        switch (req.type) {
            case ClockRequestType::TIME:
                replyTime(sender_tid);
                break;
            case ClockRequestType::DELAY:
                handleDelay(sender_tid, req.ticks);
                break;
            case ClockRequestType::DELAY_UNTIL:
                handleDelayUntil(sender_tid, req.ticks);
                break;
            case ClockRequestType::TICK:
                updateWaitingTasks();
                break;   
        }
    }

    void replyTime(int sender_tid) {
        int current_time = getTimeInTicks();
        ClockResponse resp { current_time };
        REPLY(sender_tid, (char*)&resp, sizeof(resp));
    }

    void handleDelay(int sender_tid, int ticks) {
        if (ticks < 0) {
            int error = -2; // Negative delay error
            REPLY(sender_tid, (char*)&error, sizeof(error));
            return;
        }
        int target_time = getTimeInTicks() + ticks;
        TaskDescriptor* task = TaskFromTid(sender_tid);
        if (task) {
            task->setState(BLOCKED);
            waitingTasks.Push({sender_tid, target_time}, target_time);
        }
    }

    void handleDelayUntil(int sender_tid, int ticks) {
        if (ticks < 0) {
            int error = -2; // Negative delay error
            REPLY(sender_tid, (char*)&error, sizeof(error));
            return;
        }
        TaskDescriptor* task = TaskFromTid(sender_tid); // Retrieve task descriptor
        task->state = BLOCKED;
        waitingTasks.Push({sender_tid, ticks}, ticks);
    }

    void updateWaitingTasks() {
        int current_time = getTimeInTicks();
        ClockTask task;
        while (!waitingTasks.isEmpty() && waitingTasks.Peek(&task) == 0 && task.trigger_time <= current_time) {
            waitingTasks.Pop(&task);
            TaskDescriptor* taskDesc = TaskFromTid(task.tid); // Retrieve task descriptor
            if (taskDesc) {
                taskDesc->state = READY;
                readyQueue.push(taskDesc); // Push back to ready queue
            }
            REPLY(task.tid, (char*)&current_time, sizeof(current_time));
        }
    }

    int getTimeInTicks() {
        return SYSTIMER_REG(SYSTIMER_CLO) / TENTH_OF_SECOND_MICRO_SECONDS;
    }

    TaskDescriptor* TaskFromTid(int tid) {
        if (tid >= 0 && tid < MAX_TASKS) {
            TaskDescriptor& task = task_table[tid];
            if (task.state != TaskState::EXITED) { 
                return &task;
            }
        }
        return nullptr;  // Return null if no valid task found or task is exited
    }
};

void ClockServer() {
    Server server;
}
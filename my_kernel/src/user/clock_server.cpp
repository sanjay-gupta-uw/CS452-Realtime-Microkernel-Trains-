#include "clock_server.h"
#include "syscall.h"
#include "name_server.h"
#include "../containers/pqueue.h"
#include "../rpi.h"

#define TENTH_OF_SECOND_MICRO_SECONDS 100000

/*********** SYSTEM TIMER CONTROL ************************ ************/
static char *const SYSTIMER_BASE = (char *)(0xFE003000);
#define SYSTIMER_REG(offset) (*(volatile uint32_t *)(SYSTIMER_BASE + offset))

// System Timer register offsets
static const uint32_t SYSTIMER_CS = 0x00;  // System Timer Control/Status
static const uint32_t SYSTIMER_CLO = 0x04; // System Timer Counter Lower 32 bits
static const uint32_t SYSTIMER_CHI = 0x08; // System Timer Counter Higher 32 bits
// static const uint32_t SYSTIMER_C0 = 0x0C;	 // System Timer Compare 0
// static const uint32_t SYSTIMER_C1 = 0x10;	 // System Timer Compare 1
// static const uint32_t SYSTIMER_C2 = 0x14;	 // System Timer Compare 2
// static const uint32_t SYSTIMER_C3 = 0x18;	 // System Timer Compare 3

// CS register masks
static const uint32_t SYSTIMER_CS_M0 = 0x01; // Match 0
static const uint32_t SYSTIMER_CS_M1 = 0x02; // Match 1
static const uint32_t SYSTIMER_CS_M2 = 0x03; // Match 2
static const uint32_t SYSTIMER_CS_M3 = 0x04; // Match 3

// CLO register masks

/*********** ************************************************ ************/


enum class ClockRequestType { TIME, DELAY, DELAY_UNTIL };

struct ClockRequest {
    ClockRequestType type;
    int ticks;
};

struct ClockResponse {
    int time;
};

class ClockServer {
public:
    ClockServer() {
        REGISTERAS("ClockServer");
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
            updateWaitingTasks();
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
        }
    }

    void replyTime(int sender_tid) {
        int current_time = getTimeInTicks();
        ClockResponse resp { current_time };
        REPLY(sender_tid, (char*)&resp, sizeof(resp));
    }

    void handleDelay(int sender_tid, int ticks) {
        int target_time = getTimeInTicks() + ticks;
        if (ticks < 0) {
            int error = -2; // Negative delay error
            REPLY(sender_tid, (char*)&error, sizeof(error));
            return;
        }
        // Todo: Block the task: 
        // sender_tid->state = BLOCKED;
        waitingTasks.Push({sender_tid, target_time});
    }

    void handleDelayUntil(int sender_tid, int ticks) {
        if (ticks < 0) {
            int error = -2; // Negative delay error
            REPLY(sender_tid, (char*)&error, sizeof(error));
            return;
        }
        // Todo: Block the task: 
        // sender_tid->state = BLOCKED;
        waitingTasks.Push({sender_tid, ticks});
    }

    void updateWaitingTasks() {
        int current_time = getTimeInTicks();
        while (!waitingTasks.isEmpty() && waitingTasks.top().trigger_time <= current_time) {
            auto task = waitingTasks.pop();
            // Todo: Awake the task: 
            // if (taskDesc) {
            //     taskDesc->state = READY;
            //     readyQueue.push(taskDesc);
            // }
            REPLY(task.tid, (char*)&current_time, sizeof(current_time));
        }
    }

    int getTimeInTicks() {
        return SYSTIMER_REG(SYSTIMER_CLO) / TENTH_OF_SECOND_MICRO_SECONDS;
    }

    struct ClockTask {
        int tid;
        int trigger_time;
    };
};

void ClockServer() {
    ClockServer server;
}

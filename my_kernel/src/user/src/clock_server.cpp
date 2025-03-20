#include "../include/clock_server.h"
#include "../include/name_server.h"
#include "../../containers/pqueue.h"
#include "../../shared_constants.h"
#include "../../rpi.h"
#include "../../include/syscall.h"
#include "../include/uassert.h"
#include "../include/io.h"
#include "../../clock.h"
static int CLOCK_SERVER_TID = -1;

class TimeServer
{
public:
    TimeServer()
    {
        CLOCK_SERVER_TID = MYTID();
        REGISTERAS("ClockServer");
        int notifierTid = CREATE(PRIORITY::P1, ClockNotifier);
        uassert(notifierTid > 0 && "Error starting Clock Notifier");

        run();
    }
    ~TimeServer() {}

private:
    void run()
    {
        int sender_tid;
        ClockRequest req;
        while (true)
        {
            RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            processRequest(sender_tid, req);
        }
    }
    void processRequest(int sender_tid, const ClockRequest &req)
    {
        switch (req.type)
        {
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
            Tick(sender_tid);
            break;
        }
    }

    void replyTime(int sender_tid)
    {
        ClockResponse resp{TOTAL_TICKS};
        REPLY(sender_tid, (char *)&resp, sizeof(resp));
    }

    void handleDelay(int sender_tid, int ticks)
    {
        waitingTasks.Push(sender_tid, TOTAL_TICKS + ticks); // target_time is the priority (lower is higher priority)
    }

    void handleDelayUntil(int sender_tid, int ticks)
    {
        waitingTasks.Push(sender_tid, ticks); // ticks is the priority (lower is higher priority)
    }

    void Tick(int sender_tid)
    {
        TOTAL_TICKS++;
        int waiting_tid, trigger_time;
        while (!waitingTasks.isEmpty() && waitingTasks.Peek(&waiting_tid, &trigger_time) == 0 && (uint32_t)trigger_time <= TOTAL_TICKS)
        {
            int ret = 1;
            waitingTasks.Pop(&waiting_tid);
            REPLY(waiting_tid, (char *)&ret, sizeof(ret));
        }
        REPLY(sender_tid, nullptr, 0); // FREE THE CLOCK NOTIFIER
    }

    uint32_t TOTAL_TICKS = 0;
    PQueue<int> waitingTasks;
};

// wrappers for calls to the clock server
int TIME(int tid)
{
    if (tid != CLOCK_SERVER_TID)
    {
        return -1;
    }
    ClockRequest req{ClockRequestType::TIME, 0};
    int current_time;
    SEND(CLOCK_SERVER_TID, (char *)&req, sizeof(req), (char *)&current_time, sizeof(current_time));
    return current_time;
}

int DELAY(int tid, int ticks)
{
    if (tid != CLOCK_SERVER_TID)
    {
        return -1;
    }
    if (ticks < 0)
    {
        return -2; // Negative delay error
    }
    ClockRequest req{ClockRequestType::DELAY, ticks};
    int reply;
    SEND(CLOCK_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
    uassert(reply >= 0 && "Error in delay");
    return TIME(tid); // return current Time()
}

int DELAY_UNTIL(int tid, int ticks)
{
    if (tid != CLOCK_SERVER_TID)
    {
        return -1;
    }
    if (ticks < 0)
    {
        return -2; // Negative delay error
    }
    ClockRequest req{ClockRequestType::DELAY_UNTIL, ticks};
    int reply;
    SEND(CLOCK_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
    uassert(reply == 0 && "Error in delay until");
    return TIME(tid); // return current Time()
}

void ClockServer()
{
    TimeServer timeServer;
}

// THIS SHOULD BE THE ONLY THREAD THAT CALLS THE CLOCK SERVER WITH TICK
void ClockNotifier()
{
    REGISTERAS("ClockNotifier");
    Clock clock;
    int clockServerTid = WHOIS("ClockServer");
    while (true)
    {
        int ret = AWAITEVENT(TIMER_TICK);
        // IO_NS::PrintTerminal("ClockNotifier::Tick\r\n");
        clock.Display();

        if (ret < 0)
        {
            // delay until the next tick
            // clock.Delay(1);
        }

        ClockRequest tickReq = {ClockRequestType::TICK, 0};                  // No ticks needed for a notification
        SEND(clockServerTid, (char *)&tickReq, sizeof(tickReq), nullptr, 0); // Notify the clock server with a TICK
    }
}
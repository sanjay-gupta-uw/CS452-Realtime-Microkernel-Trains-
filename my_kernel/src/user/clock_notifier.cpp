#include "clock_notifier.h"
#include "syscall.h"
#include "clock_server.h"
#include "shared_constants.h"
#include "../rpi.h"

void clockNotifier() {
    int clockServerTid = WhoIs("ClockServer");
    while (true) {
        AwaitEvent(TIMER_TICK);
        ClockRequest tickReq = {ClockRequestType::TICK, 0};  // No ticks needed for a notification
        SEND(clockServerTid, (char*)&tickReq, sizeof(tickReq), nullptr, 0);  // Notify the clock server with a TICK
    }
}

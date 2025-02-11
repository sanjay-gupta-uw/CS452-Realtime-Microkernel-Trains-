#include "clock_notifier.h"
#include "syscall.h"
#include "clock_server.h"
#include "../rpi.h"

void clockNotifier() {
    int clockServerTid = WhoIs("ClockServer");
    while (true) {
        AwaitEvent(TIMER_TICK);
        SEND(clockServerTid, nullptr, 0, nullptr, 0);  // Notify the clock server
    }
}

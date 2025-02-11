#include "clock_client.h"
#include "syscall.h"
#include "name_server.h"
#include "clock_server.h"
#include "../rpi.h"

void ClockClient(int delayInterval, int numDelays) {
    int clockServerTid = WhoIs("ClockServer");
    for (int i = 0; i < numDelays; i++) {
        int result = Delay(clockServerTid, delayInterval);
        if (result >= 0) {
            uart_printf(CONSOLE, "Client %d: Delay %d complete. Current Time: %d ticks.\n", MYTID(), i + 1, result);
        } else {
            uart_printf(CONSOLE, "Error: Client %d encountered an error with code %d.\n", MYTID(), result);
        }
    }
}
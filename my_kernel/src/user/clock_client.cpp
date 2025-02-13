#include "clock_client.h"
#include "syscall.h"
#include "name_server.h"
#include "clock_server.h"
#include "../rpi.h"

void ClockClient(int delayInterval, int numDelays) {
    int clockServerTid = WhoIs("ClockServer");
    ClockRequest req;
    ClockResponse resp;

    for (int i = 0; i < numDelays; i++) {
        req.type = ClockRequestType::DELAY;
        req.ticks = delayInterval;
        SEND(clockServerTid, (char*)&req, sizeof(req), (char*)&resp, sizeof(resp));

        if (resp.time >= 0) {
            uart_printf(CONSOLE, "Client %d: Delay %d complete. Current Time: %d ticks.\n", MYTID(), i + 1, resp.time);
        } else {
            uart_printf(CONSOLE, "Error: Client %d encountered an error with code %d.\n", MYTID(), resp.time);
        }
    }
}

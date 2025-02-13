#include "usertask.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "clock_server.h"
#include "clock_client.h"
#include "clock_notifier.h"
#include "../shared_constants.h"
#include "../rpi.h"

void Task1() {
    // Start the Clock Server
    int clockServerTid = CREATE(HIGH, ClockServer);
    if (clockServerTid < 0) {
        uart_printf(CONSOLE, "Error starting Clock Server\n");
        EXIT();
    }

    // Create Clock Client Tasks with varying priorities and delays
    int delays[] = {10, 23, 33, 71};
    int counts[] = {20, 9, 6, 3};
    int priorities[] = {3, 4, 5, 6}; 

    for (int i = 0; i < 4; i++) {
        int clientTid = CREATE(priorities[i], [i, clockServerTid]() {
            clockClient(clockServerTid, delays[i], counts[i]);
        });

        if (clientTid < 0) {
            uart_printf(CONSOLE, "Error creating Clock Client Task\n");
            continue;
        }
    }

    // Wait for initialization messages from clients
    for (int i = 0; i < 4; i++) {
        int senderTid;
        char msg[64];  
        RECEIVE(&senderTid, msg, sizeof(msg));
        uart_printf(CONSOLE, "Initialization message from client %d: %s\n", senderTid, msg);
    }

    EXIT();
}

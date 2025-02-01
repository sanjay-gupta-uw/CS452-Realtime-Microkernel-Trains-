#include "rps_client.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "../rpi.h"
#include <cstdio>

void RPSClient() {
    int server_tid = WHOIS("RPS_SERVER");
    char msg = 'S';
    char reply;
    SEND(server_tid, &msg, 1, &reply, 1);

    if (reply == 'P') {
        char play = 'R';  // Always play Rock for testing
        SEND(server_tid, &play, 1, &reply, 1);
        uart_printf(CONSOLE, "Result: %c\n", reply);
    }
}

#include "usertask.h"
#include "../rpi.h"
#include "../shared_constants.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "rps_server.h"
#include "rps_client.h"
#include <cstring>
void clientTask1() {
    GameMove moves[] = {ROCK, PAPER, SCISSORS, QUIT};
    RpsClient(moves, 4);
}

void clientTask2() {
    GameMove moves[] = {SCISSORS, ROCK, PAPER, PAPER, QUIT};
    RpsClient(moves, 5);
}

void Task1() {
    uart_printf(CONSOLE, "FirstUserTask: Starting system services.\r\n");

    int ns_tid = CREATE(HIGH, NameServer);
    if (ns_tid < 0) {
        uart_printf(CONSOLE, "Failed to start Name Server. Exiting...\r\n");
        EXIT();
    }
    setNameServerTid(ns_tid);
    uart_printf(CONSOLE, "Name Server started with TID: %d\r\n", ns_tid);

    int rps_server_tid = CREATE(HIGH, RpsServer);
    uart_printf(CONSOLE, "RPS Server started with TID: %d\r\n", rps_server_tid);

    YIELD(); 
    YIELD(); // Allow servers to initialize

    int client_tid1 = CREATE(MEDIUM, clientTask1);
    int client_tid2 = CREATE(MEDIUM, clientTask2);


    for (int i = 0; i < 20; ++i) {
        YIELD();
    }

    uart_printf(CONSOLE, "FirstUserTask: Shutting down system.\r\n");
    EXIT();
}

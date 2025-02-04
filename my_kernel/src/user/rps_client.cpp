#include "rps_client.h"
#include "rps_server.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../rpi.h"
#include <cstring>

void RpsClient(GameMove *moves, int num_moves) {
    int server_tid = WHOIS("RPSServer");
    if (server_tid < 0) {
        uart_printf(CONSOLE, "Failed to find RPS Server\r\n");
        return;
    }
    uart_printf(CONSOLE, "RPS Server found by RPS Client with TID %d\r\n", MYTID());

    // Sign up for the game
    GameRequest req = {SIGNUP, ROCK};  // Initial move does not matter for signup
    char reply[20];
    int result = SEND(server_tid, (char*)&req, sizeof(req), reply, sizeof(reply));
    if (result < 0 || strcmp(reply, "PAIRED") != 0) {
        uart_printf(CONSOLE, "Failed to sign up or bad reply: %s\r\n", reply);
        return;
    }

    // Play the game according to the list of moves
    for (int i = 0; i < num_moves; i++) {
        if (moves[i] == QUIT) {
            req.type = QUIT_GAME;
            SEND(server_tid, (char*)&req, sizeof(req), NULL, 0);
            uart_printf(CONSOLE, "Player TID %d quitting game.\r\n", MYTID());
            break;
        } else {
            req.type = PLAY;
            req.move = static_cast<GameMove>(moves[i]);
            result = SEND(server_tid, (char*)&req, sizeof(req), reply, sizeof(reply));
            if (result < 0) {
                uart_printf(CONSOLE, "Failed to send play move\r\n");
                break;
            }
            uart_printf(CONSOLE, "Game result for TID %d: %s\r\n", MYTID(), reply);
        }
    }
}

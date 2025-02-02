#include "rps_client.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../rpi.h"
#include <cstring>

// Define game moves and request types
enum GameMove {
    ROCK, PAPER, SCISSORS
};

enum RequestType {
    SIGNUP, PLAY, QUIT
};

// Structure to send game requests
struct GameRequest {
    RequestType type;
    GameMove move;
};

void RpsClient() {
    int server_tid = WHOIS("RPSServer");
    if (server_tid < 0) {
        uart_printf(CONSOLE, "Failed to find RPS Server\r\n");
        return;
    }

    // Register with the server
    GameRequest req;
    req.type = SIGNUP;
    char reply[10]; // Ensure this is enough space for any reply
    int result = SEND(server_tid, (char*)&req, sizeof(req), reply, sizeof(reply));
    if (result < 0 || strcmp(reply, "OK") != 0) {
        uart_printf(CONSOLE, "Failed to sign up or bad reply: %s\r\n", reply);
        return;
    }

    // Wait for a match and play the game
    req.type = PLAY;
    req.move = ROCK;  // Example move
    result = SEND(server_tid, (char*)&req, sizeof(req), reply, sizeof(reply));
    if (result < 0) {
        uart_printf(CONSOLE, "Failed to send play move\r\n");
        return;
    }

    // Interpret the game result
    uart_printf(CONSOLE, "Game result: %s\r\n", reply);

    // Send a quit message
    req.type = QUIT;
    SEND(server_tid, (char*)&req, sizeof(req), NULL, 0);
}
